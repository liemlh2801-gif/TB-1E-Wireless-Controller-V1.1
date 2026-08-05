/*

 * Bluetooth motor controller for MAY THU DAY AN TOAN (TB-1E).

 *

 * Supported boards (Arduino IDE):

 *   ESP32 Dev Module      — ESP32-WROOM-32

 *   ESP32S3 / C3 / C6     — BLE-only chips

 *

 * All boards use BLE Nordic UART Service (Chrome web, Android, iOS).

 * Phone/PC sees Bluetooth name TB-1E.

 *

 * GPIO 32 mode: INPUT_PULLDOWN — Manual=open/GND, Auto=3.3V only.

 * GPIO 18 top limit: while active GPIO26 disabled; GPIO 19 bottom limit: while active GPIO27 disabled.
 * GPIO 33 ON-HOLD: active=LOW/GND; after active→inactive, if inactive 3s → DOWN (GPIO26 inactive, GPIO27 GND, GPIO14 inactive).
 * While ON-HOLD active, UP input (GPIO21/app) and UP output (GPIO26) are never disabled.
 * AUTO-DOWN output: GPIO26 HIGH, GPIO27 LOW, GPIO14 HIGH until button or bottom limit.
 * GPIO 21/22/23 panel LÊN/XUỐNG/DỪNG: LOW to GND = pressed (INPUT_PULLUP).
 * Panel inputs are always polled — never disabled.
 * STOP (panel GPIO23 / app) input and output are never disabled.

 */



#include <Arduino.h>



const int IN1 = 26;

const int IN2 = 27;

const int STOP_PIN = 14;

const int BT_CONNECTED_PIN = 25;  // idle HIGH; LOW while phone/PC is connected via BLE

const int MODE_SWITCH_PIN = 32;   // INPUT_PULLDOWN: open/GND=Manual (default), 3.3V=Auto

const int TOP_LIMIT_PIN = 18;     // INPUT_PULLUP: LOW to GND=active, open/HIGH=inactive

const int BOT_LIMIT_PIN = 19;     // INPUT_PULLUP: LOW to GND=active, open/HIGH=inactive

const int AUTO_DOWN_PIN = 4;      // INPUT_PULLDOWN: 3.3V=enabled, open/GND=inactive

const int ON_HOLD_PIN = 33;       // INPUT_PULLUP: LOW=active (to GND); release=open (HIGH) latches AUTO-DOWN

const int PANEL_UP_PIN = 21;      // INPUT_PULLUP: LOW to GND = LÊN pressed

const int PANEL_DOWN_PIN = 22;    // INPUT_PULLUP: LOW to GND = XUỐNG pressed

const int PANEL_STOP_PIN = 23;    // INPUT_PULLUP: LOW to GND = DỪNG pressed

const char BT_DEVICE_NAME[] = "TB-1E";

const long USB_BAUD = 115200;

const unsigned long INPUT_DEBOUNCE_MS = 50;

const unsigned long PANEL_DEBOUNCE_MS = 10;

const unsigned long AUTO_DOWN_FIRST_START_DELAY_MS = 3000;



bool modeSwitchAuto = false;

bool modeSwitchLastReading = false;

unsigned long modeSwitchLastChangeMs = 0;



bool topLimitActive = false;

bool botLimitActive = false;

bool topLimitLastReading = false;

bool botLimitLastReading = false;

unsigned long topLimitLastChangeMs = 0;

unsigned long botLimitLastChangeMs = 0;

enum PanelEdge { PANEL_EDGE_NONE, PANEL_EDGE_RISE, PANEL_EDGE_FALL };

struct PanelButtonState {

  bool stable;

  bool lastRaw;

  unsigned long lastChangeMs;

};

bool autoDownFirstStartPending = true;

unsigned long autoDownFirstStartAt = 0;

bool autoDownOnHoldInactiveArmed = false;

unsigned long autoDownOnHoldInactiveSince = 0;

bool autoDownOnHoldInactiveFired = false;

bool autoDownLatched = false;

PanelButtonState onHoldInput = {false, false, 0};

PanelButtonState panelUp = {false, false, 0};

PanelButtonState panelDown = {false, false, 0};

PanelButtonState panelStop = {false, false, 0};

bool appUpHeld = false;

bool appDownHeld = false;

bool appStopHeld = false;



void logUsb(const char* line);

void setAllHigh();

void writeIn1(bool levelHigh);

void writeIn2(bool levelHigh);

void setBtConnectedOutput(bool connected);

void onAppDisconnected();

void moveUp();

void moveDown();

void stopPressed();

void stopReleased();

void handleCommand(const String& cmd);

bool isAppConnected();

bool isAutoMode();

bool isTopLimitActive();

bool isBotLimitActive();

void sendModeStatus();

void sendLimitStatus();

void sendHoldStatus();

void pollModeSwitch();

void pollLimitSwitches();

void scheduleAutoDownFirstStart();

void pollAutoDownFirstStart();

void resetAutoDownOnHoldInactiveTimer();

void armAutoDownOnHoldInactiveTimer();

void pollAutoDownOnHoldInactive();

void pollOnHold();

void activateAutoDown();

void clearAutoDownLatch();

void maintainAutoDownLatch();

bool isAutoDownEnabled();

bool isStopOutputActive();

bool isPanelStopPressed();

void setupPanelInputs();

bool readPanelPressed(int pin);

void initPanelButtons();

PanelEdge pollPanelEdge(PanelButtonState& btn, int pin);

void pollPanelButtons();

void clearAppButtonState();

bool isPanelActive();

void applyAppPanelOutputs();

void maintainAppPanelButtons();

void enforceLimitOutputs();



#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6



#include <BLEDevice.h>

#include <BLEServer.h>

#include <BLEUtils.h>

#include <BLE2902.h>



#define NUS_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"

#define NUS_RX_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"

#define NUS_TX_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"



BLEServer* bleServer = nullptr;

BLECharacteristic* bleTxChar = nullptr;

bool bleClientConnected = false;

bool blePendingAdvertRestart = false;

unsigned long bleAdvertRestartAt = 0;



void scheduleBleAdvertRestart() {

  blePendingAdvertRestart = true;

  bleAdvertRestartAt = millis() + 500;

}



void pollBleAdvertising() {

  if (blePendingAdvertRestart && millis() >= bleAdvertRestartAt) {

    blePendingAdvertRestart = false;

    BLEDevice::startAdvertising();

    logUsb("BLE advertising restarted");

  }

}



void sendBtReply(const char* msg) {

  if (bleClientConnected && bleTxChar != nullptr) {

    bleTxChar->setValue(msg);

    bleTxChar->notify();

  }

}



class BleServerCallbacks : public BLEServerCallbacks {

  void onConnect(BLEServer* server) override {

    bleClientConnected = true;

    setBtConnectedOutput(true);

    logUsb("BLE client connected");

    sendModeStatus();

    sendLimitStatus();

    sendHoldStatus();

  }



  void onDisconnect(BLEServer* server) override {

    bleClientConnected = false;

    setBtConnectedOutput(false);

    onAppDisconnected();

    logUsb("BLE client disconnected");

    scheduleBleAdvertRestart();

  }

};



class BleRxCallbacks : public BLECharacteristicCallbacks {

  void onWrite(BLECharacteristic* characteristic) override {

    String cmd = String(characteristic->getValue().c_str());

    cmd.trim();

    if (cmd.length() > 0) {

      handleCommand(cmd);

    }

  }

};



bool beginBluetooth() {

  BLEDevice::init(BT_DEVICE_NAME);

  BLEDevice::setMTU(517);



  bleServer = BLEDevice::createServer();

  bleServer->setCallbacks(new BleServerCallbacks());



  BLEService* service = bleServer->createService(NUS_SERVICE_UUID);



  bleTxChar = service->createCharacteristic(

    NUS_TX_UUID,

    BLECharacteristic::PROPERTY_NOTIFY

  );

  bleTxChar->addDescriptor(new BLE2902());



  BLECharacteristic* rxChar = service->createCharacteristic(

    NUS_RX_UUID,

    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR

  );

  rxChar->setCallbacks(new BleRxCallbacks());



  service->start();



  BLEAdvertisementData advertisementData;

  advertisementData.setFlags(0x06);

  advertisementData.setName(BT_DEVICE_NAME);

  advertisementData.setCompleteServices(BLEUUID(NUS_SERVICE_UUID));



  BLEAdvertisementData scanResponseData;

  scanResponseData.setName(BT_DEVICE_NAME);

  scanResponseData.setCompleteServices(BLEUUID(NUS_SERVICE_UUID));



  BLEAdvertising* advertising = BLEDevice::getAdvertising();

  advertising->setAdvertisementData(advertisementData);

  advertising->setScanResponseData(scanResponseData);

  advertising->start();

  return true;

}



bool bluetoothHasCommand() {

  return false;

}



String readBluetoothCommand() {

  return String();

}



void logBtMode() {

  Serial.println("Bluetooth: BLE UART (TB-1E)");

}



bool isAppConnected() {

  return bleClientConnected;

}



#else



#error Unsupported board. Use ESP32 Dev Module (WROOM-32) or ESP32-S3/C3/C6.



#endif



void logUsb(const char* line) {

  Serial.println(line);

}



void setAllHigh() {

  digitalWrite(IN1, HIGH);

  digitalWrite(IN2, HIGH);

  digitalWrite(STOP_PIN, HIGH);

}



void writeIn1(bool levelHigh) {

  if (!levelHigh && topLimitActive) {

    digitalWrite(IN1, HIGH);

    return;

  }

  digitalWrite(IN1, levelHigh ? HIGH : LOW);

}



void writeIn2(bool levelHigh) {

  if (!levelHigh && botLimitActive) {

    digitalWrite(IN2, HIGH);

    return;

  }

  digitalWrite(IN2, levelHigh ? HIGH : LOW);

}



void setBtConnectedOutput(bool connected) {

  digitalWrite(BT_CONNECTED_PIN, connected ? LOW : HIGH);

}



void onAppDisconnected() {

  clearAppButtonState();

  clearAutoDownLatch();

  setAllHigh();

  logUsb("No app connection — GPIO26/27/14 HIGH");

}



void stopPressed() {

  digitalWrite(IN1, HIGH);

  digitalWrite(IN2, HIGH);

  digitalWrite(STOP_PIN, LOW);

}



void stopReleased() {

  digitalWrite(IN1, HIGH);

  digitalWrite(IN2, HIGH);

  digitalWrite(STOP_PIN, HIGH);

}



void moveUp() {

  writeIn1(LOW);

  writeIn2(HIGH);

  digitalWrite(STOP_PIN, HIGH);

}



void moveDown() {

  writeIn1(HIGH);

  writeIn2(LOW);

  digitalWrite(STOP_PIN, HIGH);

}



bool isAutoMode() {

  return modeSwitchAuto;

}



bool readModeSwitchRaw() {

  return digitalRead(MODE_SWITCH_PIN) == HIGH;

}



bool readTopLimitRaw() {

  return digitalRead(TOP_LIMIT_PIN) == LOW;

}



bool readBotLimitRaw() {

  return digitalRead(BOT_LIMIT_PIN) == LOW;

}



bool readOnHoldPressed() {

  return digitalRead(ON_HOLD_PIN) == LOW;

}



bool isOnHoldActive() {

  return readOnHoldPressed();

}



bool isAutoDownEnabled() {

  return digitalRead(AUTO_DOWN_PIN) == HIGH;

}



bool isStopOutputActive() {

  return digitalRead(STOP_PIN) == LOW;

}



bool isPanelStopPressed() {

  return digitalRead(PANEL_STOP_PIN) == LOW;

}



bool isTopLimitActive() {

  return topLimitActive;

}



bool isBotLimitActive() {

  return botLimitActive;

}



bool isMovingUp() {

  return digitalRead(IN1) == LOW &&

         digitalRead(IN2) == HIGH &&

         digitalRead(STOP_PIN) == HIGH;

}



bool isMovingDown() {

  return digitalRead(IN1) == HIGH &&

         digitalRead(IN2) == LOW &&

         digitalRead(STOP_PIN) == HIGH;

}



void sendModeStatus() {

  sendBtReply(isAutoMode() ? "MODE: AUTO" : "MODE: MANUAL");

}



void sendLimitStatus() {

  sendBtReply(topLimitActive ? "LIMIT: TOP" : "LIMIT: TOP OFF");

  sendBtReply(botLimitActive ? "LIMIT: BOT" : "LIMIT: BOT OFF");

}



void sendHoldStatus() {

  sendBtReply(readOnHoldPressed() ? "HOLD: ON" : "HOLD: OFF");

}



void clearAutoDownLatch() {

  if (autoDownLatched) {

    autoDownLatched = false;

    logUsb("AUTO-DOWN latch cleared");

  }

}



void activateAutoDown() {

  if (!isAutoMode()) {

    return;

  }

  if (!isAutoDownEnabled()) {

    logUsb("AUTO-DOWN skipped — GPIO4 not on 3.3V");

    return;

  }

  if (isBotLimitActive()) {

    logUsb("AUTO-DOWN skipped — bottom limit active");

    return;

  }

  autoDownLatched = true;

  moveDown();

  logUsb("AUTO-DOWN latched — GPIO26 HIGH, GPIO27 LOW, GPIO14 HIGH");

}



void maintainAutoDownLatch() {

  if (!autoDownLatched) {

    return;

  }

  if (isOnHoldActive()) {

    return;

  }

  if (isPanelStopPressed() || isStopOutputActive()) {

    return;

  }

  if (botLimitActive) {

    clearAutoDownLatch();

    return;

  }

  moveDown();

}



void scheduleAutoDownFirstStart() {

  if (!isAutoMode() || !isAutoDownEnabled()) {

    autoDownFirstStartPending = false;

    return;

  }

  if (readOnHoldPressed()) {

    autoDownFirstStartPending = false;

    logUsb("AUTO-DOWN first start skipped — ON-HOLD active (GPIO33 LOW) at boot");

    return;

  }

  autoDownFirstStartAt = millis() + AUTO_DOWN_FIRST_START_DELAY_MS;

  logUsb("AUTO-DOWN first start scheduled — DOWN in 3 seconds");

}



void resetAutoDownOnHoldInactiveTimer() {

  if (autoDownOnHoldInactiveArmed && !autoDownOnHoldInactiveFired) {

    logUsb("AUTO-DOWN ON-HOLD inactive count reset");

  }

  autoDownOnHoldInactiveArmed = false;

  autoDownOnHoldInactiveFired = false;

}



void armAutoDownOnHoldInactiveTimer() {

  if (!isAutoMode()) {

    logUsb("AUTO-DOWN ON-HOLD release ignored — not Auto mode (GPIO32)");

    return;

  }

  if (!isAutoDownEnabled()) {

    logUsb("AUTO-DOWN skipped — GPIO4 not on 3.3V");

    return;

  }

  autoDownOnHoldInactiveArmed = true;

  autoDownOnHoldInactiveSince = millis();

  autoDownOnHoldInactiveFired = false;

  logUsb("ON-HOLD active→inactive — counting 3s inactive for DOWN");

}



void pollAutoDownOnHoldInactive() {

  if (!autoDownOnHoldInactiveArmed || autoDownOnHoldInactiveFired) {

    return;

  }

  if (readOnHoldPressed()) {

    resetAutoDownOnHoldInactiveTimer();

    return;

  }

  if (millis() - autoDownOnHoldInactiveSince < AUTO_DOWN_FIRST_START_DELAY_MS) {

    return;

  }

  autoDownOnHoldInactiveFired = true;

  if (!isAutoMode() || !isAutoDownEnabled()) {

    logUsb("AUTO-DOWN cancelled — not Auto or GPIO4 inactive");

    return;

  }

  if (isBotLimitActive()) {

    logUsb("AUTO-DOWN skipped — bottom limit active");

    return;

  }

  clearAutoDownLatch();

  moveDown();

  logUsb("ON-HOLD inactive 3s — DOWN (GPIO26 inactive, GPIO27 GND, GPIO14 inactive)");

}



void pollAutoDownFirstStart() {

  if (!autoDownFirstStartPending) {

    return;

  }

  if (millis() < autoDownFirstStartAt) {

    return;

  }

  autoDownFirstStartPending = false;

  if (!isAutoMode() || !isAutoDownEnabled()) {

    logUsb("AUTO-DOWN first start cancelled — not Auto or GPIO4 inactive");

    return;

  }

  if (readOnHoldPressed()) {

    logUsb("AUTO-DOWN first start cancelled — ON-HOLD active (GPIO33 LOW)");

    return;

  }

  logUsb("AUTO-DOWN first start — 3s elapsed, DOWN once");

  activateAutoDown();

}



void pollOnHold() {

  // ON-HOLD: LOW/GND = active; open/HIGH = inactive
  // pollPanelEdge: RISE = pressed (active), FALL = released (inactive)

  const PanelEdge holdEdge = pollPanelEdge(onHoldInput, ON_HOLD_PIN);



  if (holdEdge == PANEL_EDGE_RISE) {

    resetAutoDownOnHoldInactiveTimer();

    clearAutoDownLatch();

    sendBtReply("HOLD: ON");

    logUsb("ON-HOLD active (GPIO33 to GND) — UP input/output enabled");

  } else if (holdEdge == PANEL_EDGE_FALL) {

    sendBtReply("HOLD: OFF");

    armAutoDownOnHoldInactiveTimer();

  }

}



void onModeChanged() {

  resetAutoDownOnHoldInactiveTimer();

  clearAutoDownLatch();

  setAllHigh();

  initPanelButtons();

  logUsb(isAutoMode()

    ? "MODE: AUTO — panel UP/DOWN latch until DỪNG; DỪNG momentary"

    : "MODE: MANUAL — panel momentary UP/DOWN/DỪNG");

  sendModeStatus();

}



void onTopLimitChanged() {

  if (topLimitActive) {

    digitalWrite(IN1, HIGH);

    sendBtReply("LIMIT: TOP");

    logUsb("LIMIT TOP — GPIO26 set HIGH, then disabled");

  } else {

    sendBtReply("LIMIT: TOP OFF");

    logUsb("Top limit clear — GPIO26 enabled");

    if (readPanelPressed(PANEL_UP_PIN)) {

      moveUp();

      logUsb("GPIO21 still to GND — GPIO26 allowed LOW");

    }

  }

}



void onBotLimitChanged() {

  if (botLimitActive) {

    clearAutoDownLatch();

    digitalWrite(IN2, HIGH);

    sendBtReply("LIMIT: BOT");

    logUsb("LIMIT BOT — GPIO27 set HIGH, then disabled");

  } else {

    sendBtReply("LIMIT: BOT OFF");

    logUsb("Bottom limit clear — GPIO27 enabled");

    if (readPanelPressed(PANEL_DOWN_PIN)) {

      moveDown();

      logUsb("GPIO22 still to GND — GPIO27 allowed LOW");

    }

  }

}



void pollDebouncedInput(

  bool& active,

  bool& lastReading,

  unsigned long& lastChangeMs,

  bool (*readRaw)(),

  void (*onChanged)()

) {

  const bool reading = readRaw();



  if (reading != lastReading) {

    lastChangeMs = millis();

    lastReading = reading;

  }



  if (millis() - lastChangeMs >= INPUT_DEBOUNCE_MS && reading != active) {

    active = reading;

    onChanged();

  }

}



void pollModeSwitch() {

  pollDebouncedInput(

    modeSwitchAuto,

    modeSwitchLastReading,

    modeSwitchLastChangeMs,

    readModeSwitchRaw,

    onModeChanged

  );

}



void pollLimitSwitches() {

  pollDebouncedInput(

    topLimitActive,

    topLimitLastReading,

    topLimitLastChangeMs,

    readTopLimitRaw,

    onTopLimitChanged

  );

  pollDebouncedInput(

    botLimitActive,

    botLimitLastReading,

    botLimitLastChangeMs,

    readBotLimitRaw,

    onBotLimitChanged

  );

}



void setupAutoDownInputs() {

  pinMode(AUTO_DOWN_PIN, INPUT_PULLDOWN);

  pinMode(ON_HOLD_PIN, INPUT_PULLUP);

  initPanelButton(onHoldInput, ON_HOLD_PIN);

}



bool readPanelPressed(int pin) {

  return digitalRead(pin) == LOW;

}



void setupPanelInputs() {

  pinMode(PANEL_UP_PIN, INPUT_PULLUP);

  pinMode(PANEL_DOWN_PIN, INPUT_PULLUP);

  pinMode(PANEL_STOP_PIN, INPUT_PULLUP);

}



void initPanelButton(PanelButtonState& btn, int pin) {

  const bool raw = readPanelPressed(pin);

  btn.lastRaw = raw;

  btn.stable = raw;

  btn.lastChangeMs = millis();

}



void initPanelButtons() {

  initPanelButton(panelUp, PANEL_UP_PIN);

  initPanelButton(panelDown, PANEL_DOWN_PIN);

  initPanelButton(panelStop, PANEL_STOP_PIN);

}



PanelEdge pollPanelEdge(PanelButtonState& btn, int pin) {

  const bool raw = readPanelPressed(pin);



  if (raw != btn.lastRaw) {

    btn.lastChangeMs = millis();

    btn.lastRaw = raw;

  }



  if (millis() - btn.lastChangeMs < PANEL_DEBOUNCE_MS) {

    return PANEL_EDGE_NONE;

  }



  const bool debounced = btn.lastRaw;

  if (debounced == btn.stable) {

    return PANEL_EDGE_NONE;

  }



  const bool wasStable = btn.stable;

  btn.stable = debounced;



  if (debounced && !wasStable) {

    return PANEL_EDGE_RISE;

  }

  if (!debounced && wasStable) {

    return PANEL_EDGE_FALL;

  }

  return PANEL_EDGE_NONE;

}



void clearAppButtonState() {

  appUpHeld = false;

  appDownHeld = false;

  appStopHeld = false;

}



bool isPanelActive() {

  return panelUp.stable || panelDown.stable || panelStop.stable;

}



void applyAppPanelOutputs() {

  if (!isAppConnected() || isPanelActive()) {

    return;

  }



  if (!isAutoMode()) {

    if (appStopHeld) {

      clearAutoDownLatch();

      stopPressed();

    } else if (appUpHeld) {

      clearAutoDownLatch();

      moveUp();

    } else if (appDownHeld) {

      clearAutoDownLatch();

      moveDown();

    } else if (!autoDownLatched) {

      stopReleased();

    }

    return;

  }



  if (appStopHeld) {

    clearAutoDownLatch();

    stopPressed();

  }

}



void maintainAppPanelButtons() {

  applyAppPanelOutputs();

}



void pollPanelButtons() {

  if (!isAutoMode()) {

    pollPanelEdge(panelUp, PANEL_UP_PIN);

    pollPanelEdge(panelDown, PANEL_DOWN_PIN);

    pollPanelEdge(panelStop, PANEL_STOP_PIN);



    if (panelStop.stable) {

      clearAutoDownLatch();

      stopPressed();

    } else if (panelUp.stable) {

      clearAutoDownLatch();

      moveUp();

    } else if (panelDown.stable) {

      clearAutoDownLatch();

      moveDown();

    } else if (!autoDownLatched) {

      stopReleased();

    }

    return;

  }



  const PanelEdge stopEdge = pollPanelEdge(panelStop, PANEL_STOP_PIN);



  if (panelStop.stable) {

    clearAutoDownLatch();

    stopPressed();

    return;

  }



  if (stopEdge == PANEL_EDGE_FALL) {

    clearAutoDownLatch();

    stopReleased();

    return;

  }



  if (pollPanelEdge(panelUp, PANEL_UP_PIN) == PANEL_EDGE_RISE) {

    clearAutoDownLatch();

    moveUp();

  }

  if (pollPanelEdge(panelDown, PANEL_DOWN_PIN) == PANEL_EDGE_RISE) {

    clearAutoDownLatch();

    moveDown();

  }

}



void enforceLimitOutputs() {

  if (topLimitActive && digitalRead(IN1) == LOW) {

    digitalWrite(IN1, HIGH);

  }

  if (botLimitActive && digitalRead(IN2) == LOW) {

    digitalWrite(IN2, HIGH);

  }

}



void handleCommand(const String& cmd) {

  if (cmd == "STOP" || cmd == "RELEASE") {

    if (cmd == "RELEASE") {

      if (isAutoMode()) {

        if (appStopHeld) {

          appStopHeld = false;

          clearAutoDownLatch();

          stopReleased();

          if (isAppConnected()) {

            sendBtReply("STATUS: RELEASED");

            logUsb("TX: STATUS: RELEASED");

            logUsb("MOTOR: STOP released (GPIO26/27/14=HIGH)");

          }

        }

        return;

      }

      clearAppButtonState();

      clearAutoDownLatch();

      stopReleased();

      if (isAppConnected()) {

        sendBtReply("STATUS: RELEASED");

        logUsb("TX: STATUS: RELEASED");

        logUsb("MOTOR: STOP released (GPIO26/27/14=HIGH)");

      }

      return;

    }

    clearAutoDownLatch();

    appStopHeld = true;

    appUpHeld = false;

    appDownHeld = false;

    stopPressed();

    if (isAppConnected()) {

      Serial.print("RX: ");

      Serial.println(cmd);

      sendBtReply("STATUS: STOP");

      logUsb("TX: STATUS: STOP");

      logUsb("MOTOR: STOP (GPIO26=HIGH, GPIO27=HIGH, GPIO14=LOW)");

    }

    return;

  }



  if (!isAppConnected()) {

    setAllHigh();

    return;

  }



  Serial.print("RX: ");

  Serial.println(cmd);



  if (cmd == "UP") {

    clearAutoDownLatch();

    appStopHeld = false;



    if (isAutoMode()) {

      if (isTopLimitActive() || readTopLimitRaw()) {

        logUsb("UP at top limit — GPIO26 disabled, GPIO27 still active");

      }

      moveUp();

      sendBtReply("STATUS: UP");

      logUsb("TX: STATUS: UP");

      logUsb("MOTOR: UP (GPIO26=LOW, GPIO27=HIGH, GPIO14=HIGH)");

      return;

    }



    appUpHeld = true;

    appDownHeld = false;

    applyAppPanelOutputs();

    sendBtReply("STATUS: UP");

    logUsb("TX: STATUS: UP");

    logUsb("MOTOR: UP (GPIO26=LOW, GPIO27=HIGH, GPIO14=HIGH)");

    return;

  }



  if (cmd == "DOWN") {

    clearAutoDownLatch();

    appStopHeld = false;



    if (isAutoMode()) {

      if (isBotLimitActive() || readBotLimitRaw()) {

        logUsb("DOWN at bottom limit — GPIO27 disabled, GPIO26 still active");

      }

      moveDown();

      sendBtReply("STATUS: DOWN");

      logUsb("TX: STATUS: DOWN");

      logUsb("MOTOR: DOWN (GPIO26=HIGH, GPIO27=LOW, GPIO14=HIGH)");

      return;

    }



    appDownHeld = true;

    appUpHeld = false;

    applyAppPanelOutputs();

    sendBtReply("STATUS: DOWN");

    logUsb("TX: STATUS: DOWN");

    logUsb("MOTOR: DOWN (GPIO26=HIGH, GPIO27=LOW, GPIO14=HIGH)");

    return;

  }



  if (cmd.length() > 0) {

    logUsb("RX ignored (unknown command)");

  }

}



void setup() {

  pinMode(IN1, OUTPUT);

  pinMode(IN2, OUTPUT);

  pinMode(STOP_PIN, OUTPUT);

  pinMode(BT_CONNECTED_PIN, OUTPUT);

  pinMode(MODE_SWITCH_PIN, INPUT_PULLDOWN);

  pinMode(TOP_LIMIT_PIN, INPUT_PULLUP);

  pinMode(BOT_LIMIT_PIN, INPUT_PULLUP);

  setupAutoDownInputs();

  setupPanelInputs();

  setAllHigh();

  setBtConnectedOutput(false);



  modeSwitchLastReading = readModeSwitchRaw();

  modeSwitchAuto = modeSwitchLastReading;

  modeSwitchLastChangeMs = millis();



  topLimitLastReading = readTopLimitRaw();

  topLimitActive = topLimitLastReading;

  topLimitLastChangeMs = millis();

  if (topLimitActive) {

    digitalWrite(IN1, HIGH);

  }



  botLimitLastReading = readBotLimitRaw();

  botLimitActive = botLimitLastReading;

  botLimitLastChangeMs = millis();

  if (botLimitActive) {

    digitalWrite(IN2, HIGH);

  }



  initPanelButtons();



  Serial.begin(USB_BAUD);



  if (!beginBluetooth()) {

    logUsb("Bluetooth init failed — rebooting...");

    delay(1000);

    ESP.restart();

  }



  logUsb("ESP32 BT controller ready");

  logBtMode();

  Serial.print("Bluetooth name: ");

  Serial.println(BT_DEVICE_NAME);

  logUsb(isAutoMode() ? "Mode switch: AUTO" : "Mode switch: MANUAL");

  logUsb("GPIO32 mode — open/GND=Manual (default), 3.3V=Auto");

  logUsb("GPIO18 top limit, GPIO19 bottom limit — LOW/GND=active, open/HIGH=inactive");

  logUsb("GPIO4 AUTO-DOWN — 3.3V=enabled; ON-HOLD inactive 3s triggers DOWN; first start latches DOWN");

  logUsb("GPIO21 LÊN, GPIO22 XUỐNG, GPIO23 DỪNG — LOW to GND=pressed");

  if (topLimitActive) logUsb("Top limit active at startup");

  if (botLimitActive) logUsb("Bottom limit active at startup");

  scheduleAutoDownFirstStart();

  logUsb("Waiting for commands...");

}



void loop() {

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6

  pollBleAdvertising();

#endif



  pollModeSwitch();

  pollLimitSwitches();

  pollOnHold();

  pollAutoDownOnHoldInactive();

  pollAutoDownFirstStart();

  maintainAutoDownLatch();

  pollPanelButtons();

  maintainAppPanelButtons();

  enforceLimitOutputs();



  static bool appWasConnected = false;

  const bool appConnected = isAppConnected();



  if (appWasConnected && !appConnected) {

    onAppDisconnected();

  }

  appWasConnected = appConnected;



  if (bluetoothHasCommand()) {

    handleCommand(readBluetoothCommand());

  }

}


