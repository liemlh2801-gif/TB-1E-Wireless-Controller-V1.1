const NUS_SERVICE = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const NUS_RX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
const NUS_TX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
const DEVICE_NAME = "TB-1E";

export function isWebBluetoothSupported() {
  return typeof navigator !== "undefined" && !!navigator.bluetooth;
}

export class TB1EBluetooth {
  constructor(handlers = {}) {
    this.device = null;
    this.server = null;
    this.rxChar = null;
    this.txChar = null;
    this.connected = false;
    this.onStatus = handlers.onStatus || (() => {});
    this.onMessage = handlers.onMessage || (() => {});
    this.onError = handlers.onError || (() => {});
  }

  async connect(options = {}) {
    const browseAll = options.browseAll === true;

    if (!isWebBluetoothSupported()) {
      throw new Error(
        "Trình duyệt không hỗ trợ Web Bluetooth. Dùng Chrome hoặc Edge trên PC/Android.",
      );
    }

    this.onStatus("connecting");

    try {
      this.device = await navigator.bluetooth.requestDevice(
        browseAll
          ? {
              acceptAllDevices: true,
              optionalServices: [NUS_SERVICE],
            }
          : {
              filters: [
                { name: DEVICE_NAME },
                { namePrefix: "TB-" },
                { services: [NUS_SERVICE] },
              ],
              optionalServices: [NUS_SERVICE],
            },
      );
    } catch (error) {
      this.onStatus("disconnected");
      throw error;
    }

    this.device.addEventListener("gattserverdisconnected", () => {
      this.handleDisconnect();
    });

    try {
      this.server = await this.device.gatt.connect();
      const service = await this.server.getPrimaryService(NUS_SERVICE);
      this.rxChar = await service.getCharacteristic(NUS_RX);
      this.txChar = await service.getCharacteristic(NUS_TX);

      await this.txChar.startNotifications();
      this.txChar.addEventListener("characteristicvaluechanged", (event) => {
        const value = event.target.value;
        const text = new TextDecoder().decode(value).trim();
        if (text) {
          this.onMessage(text);
        }
      });

      this.connected = true;
      this.onStatus("connected");
    } catch (error) {
      this.onStatus("disconnected");
      throw error;
    }
  }

  async disconnect(sendRelease = true) {
    if (sendRelease && this.connected) {
      try {
        await this.send("RELEASE");
      } catch (_) {
        // Best effort before closing GATT.
      }
    }

    if (this.device?.gatt?.connected) {
      this.device.gatt.disconnect();
    }

    this.handleDisconnect();
  }

  handleDisconnect() {
    this.connected = false;
    this.server = null;
    this.rxChar = null;
    this.txChar = null;
    this.onStatus("disconnected");
  }

  async send(command) {
    if (!this.connected || !this.rxChar) {
      return;
    }

    const payload = new TextEncoder().encode(`${command}\n`);
    try {
      await this.rxChar.writeValue(payload);
    } catch (_) {
      await this.rxChar.writeValueWithoutResponse(payload);
    }
  }
}
