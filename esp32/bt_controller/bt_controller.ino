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

 * GPIO 32 mode switch: LOW = Manual (momentary), HIGH = Auto (latch UP/DOWN).

 * GPIO 18 top limit, GPIO 19 bottom limit, GPIO 33 ON-HOLD: LOW = active.

 */



#include <Arduino.h>



const int IN1 = 26;

const int IN2 = 27;

const int STOP_PIN = 14;

const int BT_CONNECTED_PIN = 25;  // idle HIGH; LOW while phone/PC is connected via BLE

const int MODE_SWITCH_PIN = 32;   // INPUT_PULLUP: LOW = Manual, HIGH = Auto

const int TOP_LIMIT_PIN = 18;     // INPUT_PULLUP: LOW = top limit reached

const int BOT_LIMIT_PIN = 19;     // INPUT_PULLUP: LOW = bottom limit reached

const int ON_HOLD_PIN = 33;       // INPUT_PULLUP: LOW = ON-HOLD active



const char BT_DEVICE_NAME[] = "TB-1E";

const long USB_BAUD = 115200;

const unsigned long INPUT_DEBOUNCE_MS = 50;



bool modeSwitchAuto = false;

bool modeSwitchLastReading = false;

unsigned long modeSwitchLastChangeMs = 0;



bool topLimitActive = false;

bool botLimitActive = false;

bool topLimitLastReading = false;

bool botLimitLastReading = false;

unsigned long topLimitLastChangeMs = 0;

unsigned long botLimitLastChangeMs = 0;



bool onHoldActive = false;

bool onHoldLastReading = false;

unsigned long onHoldLastChangeMs = 0;



void logUsb(const char* line);

void setAllHigh();

void setBtConnectedOutput(bool connected);

void onAppDisconnected();

void moveUp();

void moveDown();

void stopPressed();

void handleCommand(const String& cmd);

bool isAppConnected();

bool isAutoMode();

bool isTopLimitActive();

bool isBotLimitActive();

bool isOnHoldActive();

void sendModeStatus();

void sendLimitStatus();

void sendHoldStatus();

void pollModeSwitch();

void pollLimitSwitches();

void pollOnHold();

void enforceSafety();



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



void setBtConnectedOutput(bool connected) {

  digitalWrite(BT_CONNECTED_PIN, connected ? LOW : HIGH);

}



void onAppDisconnected() {

  setAllHigh();

  logUsb("No app connection — GPIO26/27/14 HIGH");

}



void stopPressed() {

  digitalWrite(IN1, HIGH);

  digitalWrite(IN2, HIGH);

  digitalWrite(STOP_PIN, LOW);

}



void moveUp() {

  digitalWrite(IN1, LOW);

  digitalWrite(IN2, HIGH);

  digitalWrite(STOP_PIN, HIGH);

}



void moveDown() {

  digitalWrite(IN1, HIGH);

  digitalWrite(IN2, LOW);

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



bool readOnHoldRaw() {

  return digitalRead(ON_HOLD_PIN) == LOW;

}



bool isTopLimitActive() {

  return topLimitActive;

}



bool isBotLimitActive() {

  return botLimitActive;

}



bool isOnHoldActive() {

  return onHoldActive;

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

  sendBtReply(onHoldActive ? "HOLD: ON" : "HOLD: OFF");

}



void onModeChanged() {

  setAllHigh();

  logUsb(isAutoMode()

    ? "MODE: AUTO — UP/DOWN latch until STOP, opposite dir, or limit"

    : "MODE: MANUAL — momentary UP/DOWN/STOP");

  sendModeStatus();

}



void onTopLimitChanged() {

  setAllHigh();

  if (topLimitActive) {

    sendBtReply("LIMIT: TOP");

    logUsb("LIMIT TOP — GPIO14/26/27 HIGH, UP disabled");

  } else {

    sendBtReply("LIMIT: TOP OFF");

    logUsb("Top limit clear — UP enabled");

  }

}



void onBotLimitChanged() {

  setAllHigh();

  if (botLimitActive) {

    sendBtReply("LIMIT: BOT");

    logUsb("LIMIT BOT — GPIO14/26/27 HIGH, DOWN disabled");

  } else {

    sendBtReply("LIMIT: BOT OFF");

    logUsb("Bottom limit clear — DOWN enabled");

  }

}



void onOnHoldChanged() {

  if (onHoldActive) {

    setAllHigh();

    sendBtReply("HOLD: ON");

    logUsb("ON-HOLD active — GPIO14/26/27 HIGH");

  } else {

    sendBtReply("HOLD: OFF");

    logUsb("ON-HOLD released");

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



void pollOnHold() {

  pollDebouncedInput(

    onHoldActive,

    onHoldLastReading,

    onHoldLastChangeMs,

    readOnHoldRaw,

    onOnHoldChanged

  );

}



void enforceSafety() {

  if (topLimitActive && isMovingUp()) {

    setAllHigh();

  }

  if (botLimitActive && isMovingDown()) {

    setAllHigh();

  }

}



bool isStopActive() {
  return digitalRead(STOP_PIN) == LOW;
}

void handleCommand(const String& cmd) {

  if (!isAppConnected()) {

    setAllHigh();

    return;

  }



  Serial.print("RX: ");

  Serial.println(cmd);



  if (cmd == "RELEASE") {

    if (isAutoMode()) {

      if (isStopActive()) {

        setAllHigh();

        sendBtReply("STATUS: RELEASED");

        logUsb("TX: STATUS: RELEASED (stop released)");

      }

      return;

    }

    setAllHigh();

    sendBtReply("STATUS: RELEASED");

    logUsb("TX: STATUS: RELEASED");

    logUsb("MOTOR: all HIGH (GPIO26/27/14=HIGH)");

    return;

  }



  if (cmd == "UP") {

    if (isTopLimitActive()) {

      setAllHigh();

      sendBtReply("ERR: TOP LIMIT");

      logUsb("RX blocked — top limit active");

      return;

    }

    moveUp();

    sendBtReply("STATUS: UP");

    logUsb("TX: STATUS: UP");

    logUsb("MOTOR: UP (GPIO26=LOW, GPIO27=HIGH, GPIO14=HIGH)");

    return;

  }



  if (cmd == "DOWN") {

    if (isBotLimitActive()) {

      setAllHigh();

      sendBtReply("ERR: BOT LIMIT");

      logUsb("RX blocked — bottom limit active");

      return;

    }

    moveDown();

    sendBtReply("STATUS: DOWN");

    logUsb("TX: STATUS: DOWN");

    logUsb("MOTOR: DOWN (GPIO26=HIGH, GPIO27=LOW, GPIO14=HIGH)");

    return;

  }



  if (cmd == "STOP") {

    stopPressed();

    sendBtReply("STATUS: STOP");

    logUsb("TX: STATUS: STOP");

    logUsb("MOTOR: STOP (GPIO26=HIGH, GPIO27=HIGH, GPIO14=LOW)");

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

  pinMode(MODE_SWITCH_PIN, INPUT_PULLUP);

  pinMode(TOP_LIMIT_PIN, INPUT_PULLUP);

  pinMode(BOT_LIMIT_PIN, INPUT_PULLUP);

  pinMode(ON_HOLD_PIN, INPUT_PULLUP);

  setAllHigh();

  setBtConnectedOutput(false);



  modeSwitchLastReading = readModeSwitchRaw();

  modeSwitchAuto = modeSwitchLastReading;

  modeSwitchLastChangeMs = millis();



  topLimitLastReading = readTopLimitRaw();

  topLimitActive = topLimitLastReading;

  topLimitLastChangeMs = millis();



  botLimitLastReading = readBotLimitRaw();

  botLimitActive = botLimitLastReading;

  botLimitLastChangeMs = millis();



  onHoldLastReading = readOnHoldRaw();

  onHoldActive = onHoldLastReading;

  onHoldLastChangeMs = millis();

  if (onHoldActive) {

    setAllHigh();

  }



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

  logUsb("GPIO32 mode — LOW=Manual, HIGH=Auto");

  logUsb("GPIO18 top limit, GPIO19 bottom limit — LOW=at limit");

  logUsb("GPIO33 ON-HOLD — LOW=hold active");

  if (topLimitActive) logUsb("Top limit active at startup");

  if (botLimitActive) logUsb("Bottom limit active at startup");

  if (onHoldActive) logUsb("ON-HOLD active at startup — GPIO14/26/27 HIGH");

  logUsb("Waiting for commands...");

}



void loop() {

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6

  pollBleAdvertising();

#endif



  pollModeSwitch();

  pollLimitSwitches();

  pollOnHold();

  enforceSafety();



  static bool appWasConnected = false;

  const bool appConnected = isAppConnected();



  if (appWasConnected && !appConnected) {

    onAppDisconnected();

  } else if (!appConnected) {

    setAllHigh();

  }

  appWasConnected = appConnected;



  if (bluetoothHasCommand()) {

    handleCommand(readBluetoothCommand());

  }

}


