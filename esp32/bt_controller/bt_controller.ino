/*
 * Bluetooth motor controller for MAY THU DAY AN TOAN (TB-1E).
 *
 * Board A — ESP32-WROOM-32 (Classic Bluetooth):
 *   Tools -> Board -> "ESP32 Dev Module"
 *   Uses BluetoothSerial (same as old HC-05 app protocol).
 *
 * Board B — ESP32-S3 / ESP32-C3 / ESP32-C6 (BLE only):
 *   Tools -> Board -> "ESP32S3 Dev Module" (or your exact S3/C3/C6 board)
 *   Uses BLE Nordic UART Service — app v1.3+ scans and connects automatically.
 *
 * Phone sees Bluetooth name TB-1E. Commands: UP / STOP / DOWN / RELEASE (momentary).
 */

#include <Arduino.h>

const int IN1 = 26;
const int IN2 = 27;
const int STOP_PIN = 14;

const char BT_DEVICE_NAME[] = "TB-1E";
const long USB_BAUD = 115200;

void logUsb(const char* line);
void setAllHigh();
void onAppDisconnected();
void moveUp();
void moveDown();
void stopPressed();
void handleCommand(const String& cmd);
bool isAppConnected();

#if CONFIG_IDF_TARGET_ESP32

#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error For ESP32 Dev Module: install esp32 board package and select "ESP32 Dev Module".
#endif

BluetoothSerial SerialBT;

void sendBtReply(const char* msg) {
  SerialBT.println(msg);
}

bool beginBluetooth() {
  return SerialBT.begin(BT_DEVICE_NAME);
}

bool bluetoothHasCommand() {
  return SerialBT.available() > 0;
}

String readBluetoothCommand() {
  String cmd = SerialBT.readStringUntil('\n');
  cmd.trim();
  return cmd;
}

void logBtMode() {
  Serial.println("Bluetooth: Classic SPP (ESP32-WROOM-32)");
}

bool isAppConnected() {
  return SerialBT.hasClient();
}

#elif CONFIG_IDF_TARGET_ESP32S3 || CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C6

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Nordic UART Service — same UUIDs used by the Android app for ESP32-S3/C3/C6.
#define NUS_SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_RX_UUID "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define NUS_TX_UUID "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer* bleServer = nullptr;
BLECharacteristic* bleTxChar = nullptr;
bool bleClientConnected = false;

void sendBtReply(const char* msg) {
  if (bleClientConnected && bleTxChar != nullptr) {
    bleTxChar->setValue(msg);
    bleTxChar->notify();
  }
}

class BleServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* server) override {
    bleClientConnected = true;
  }

  void onDisconnect(BLEServer* server) override {
    bleClientConnected = false;
    server->getAdvertising()->start();
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

  BLEAdvertising* advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(NUS_SERVICE_UUID);
  advertising->setScanResponse(true);
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
  Serial.println("Bluetooth: BLE UART (ESP32-S3 / C3 / C6)");
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

void handleCommand(const String& cmd) {
  if (!isAppConnected()) {
    setAllHigh();
    return;
  }

  Serial.print("RX: ");
  Serial.println(cmd);

  if (cmd == "UP") {
    moveUp();
    sendBtReply("STATUS: UP");
    logUsb("TX: STATUS: UP");
    logUsb("MOTOR: UP (GPIO26=LOW, GPIO27=HIGH, GPIO14=HIGH)");
  } else if (cmd == "DOWN") {
    moveDown();
    sendBtReply("STATUS: DOWN");
    logUsb("TX: STATUS: DOWN");
    logUsb("MOTOR: DOWN (GPIO26=HIGH, GPIO27=LOW, GPIO14=HIGH)");
  } else if (cmd == "STOP") {
    stopPressed();
    sendBtReply("STATUS: STOP");
    logUsb("TX: STATUS: STOP");
    logUsb("MOTOR: STOP (GPIO26=HIGH, GPIO27=HIGH, GPIO14=LOW)");
  } else if (cmd == "RELEASE") {
    setAllHigh();
    sendBtReply("STATUS: RELEASED");
    logUsb("TX: STATUS: RELEASED");
    logUsb("MOTOR: all HIGH (GPIO26/27/14=HIGH)");
  } else if (cmd.length() > 0) {
    logUsb("RX ignored (unknown command)");
  }
}

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(STOP_PIN, OUTPUT);
  setAllHigh();

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
  logUsb("Open app v1.3+, tap refresh, select TB-1E, connect");
  logUsb("Waiting for UP / STOP / DOWN (hold) / RELEASE...");
}

void loop() {
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
