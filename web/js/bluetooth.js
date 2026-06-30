const NUS_SERVICE = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
const NUS_RX = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
const NUS_TX = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
const DEVICE_NAME = "TB-1E";

const RECONNECT_DELAY_MS = 1500;
const KEEPALIVE_MS = 12000;
const MAX_RECONNECT_ATTEMPTS = 5;

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
    this.wantConnected = false;
    this.reconnecting = false;
    this.reconnectAttempts = 0;
    this.sendQueue = Promise.resolve();
    this.keepAliveTimer = null;
    this.onStatus = handlers.onStatus || (() => {});
    this.onMessage = handlers.onMessage || (() => {});
    this.onError = handlers.onError || (() => {});
    this.onDisconnect = handlers.onDisconnect || (() => {});
    this.onNotify = (event) => {
      const value = event.target.value;
      const text = new TextDecoder().decode(value).trim();
      if (text) {
        this.onMessage(text);
      }
    };
  }

  clearKeepAlive() {
    if (this.keepAliveTimer !== null) {
      clearInterval(this.keepAliveTimer);
      this.keepAliveTimer = null;
    }
  }

  startKeepAlive() {
    this.clearKeepAlive();
    this.keepAliveTimer = setInterval(async () => {
      if (!this.wantConnected || !this.server?.connected) {
        return;
      }
      try {
        await this.server.getPrimaryService(NUS_SERVICE);
      } catch (_) {
        this.scheduleReconnect();
      }
    }, KEEPALIVE_MS);
  }

  scheduleReconnect() {
    if (!this.wantConnected || this.reconnecting || !this.device) {
      return;
    }
    if (this.reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
      this.onError("Mất kết nối TB-1E. Bấm KẾT NỐI lại.");
      this.wantConnected = false;
      return;
    }

    this.reconnectAttempts += 1;
    this.reconnecting = true;
    this.connected = false;
    this.onStatus("connecting");

    setTimeout(async () => {
      try {
        await this.attachGatt();
        this.reconnectAttempts = 0;
      } catch (_) {
        this.reconnecting = false;
        this.scheduleReconnect();
      }
    }, RECONNECT_DELAY_MS);
  }

  async attachGatt() {
    if (!this.device) {
      throw new Error("No Bluetooth device selected");
    }

    if (!this.device.gatt.connected) {
      this.server = await this.device.gatt.connect();
    } else {
      this.server = this.device.gatt;
    }

    const service = await this.server.getPrimaryService(NUS_SERVICE);
    this.rxChar = await service.getCharacteristic(NUS_RX);
    this.txChar = await service.getCharacteristic(NUS_TX);

    await this.txChar.startNotifications();
    this.txChar.removeEventListener("characteristicvaluechanged", this.onNotify);
    this.txChar.addEventListener("characteristicvaluechanged", this.onNotify);

    this.connected = true;
    this.reconnecting = false;
    this.onStatus("connected");
    this.startKeepAlive();
  }

  async connect(options = {}) {
    const browseAll = options.browseAll === true;

    if (!isWebBluetoothSupported()) {
      throw new Error(
        "Trình duyệt không hỗ trợ Web Bluetooth. Dùng Chrome hoặc Edge trên PC/Android.",
      );
    }

    this.wantConnected = true;
    this.reconnectAttempts = 0;
    this.onStatus("connecting");

    try {
      if (!this.device) {
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

        this.device.addEventListener("gattserverdisconnected", () => {
          this.handleDisconnect(true);
        });
      }

      await this.attachGatt();
    } catch (error) {
      this.wantConnected = false;
      this.onStatus("disconnected");
      throw error;
    }
  }

  async disconnect(sendRelease = true) {
    this.wantConnected = false;
    this.reconnectAttempts = MAX_RECONNECT_ATTEMPTS;
    this.clearKeepAlive();

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

    this.handleDisconnect(false);
  }

  handleDisconnect(allowReconnect) {
    this.connected = false;
    this.server = null;
    this.rxChar = null;
    this.txChar = null;
    this.clearKeepAlive();
    this.onStatus("disconnected");
    this.onDisconnect();

    if (allowReconnect && this.wantConnected) {
      this.scheduleReconnect();
    }
  }

  async send(command) {
    if (!this.connected || !this.rxChar) {
      return;
    }

    this.sendQueue = this.sendQueue
      .catch(() => {})
      .then(async () => {
        if (!this.connected || !this.rxChar) {
          return;
        }

        const payload = new TextEncoder().encode(`${command}\n`);
        try {
          await this.rxChar.writeValue(payload);
        } catch (_) {
          await this.rxChar.writeValueWithoutResponse(payload);
        }
      });

    return this.sendQueue;
  }
}
