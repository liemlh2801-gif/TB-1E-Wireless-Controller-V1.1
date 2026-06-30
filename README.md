# ESP32 Bluetooth Controller

Android app + **iOS app** + **web app** + ESP32 firmware for controlling a motor/actuator with **UP**, **STOP**, and **DOWN** commands over Bluetooth.

No HC-05 module required — Bluetooth is built into the ESP32.

## Hardware

### Which ESP32 board?

| Board in Arduino IDE | Chip | Bluetooth | App connection |
|----------------------|------|-----------|----------------|
| **ESP32 Dev Module** | WROOM-32 | Classic BT (SPP) | Pair in Settings, then connect in app |
| **ESP32S3 Dev Module** | S3 | BLE only | Android, **Chrome web**, **iOS app** |
| ESP32-C3 / C6 | C3 / C6 | BLE only | Same as S3 |

**Important:** If you see `BluetoothSerial.h: No such file`, you selected **ESP32-S3** (or C3/C6). That is normal — the sketch now uses **BLE** on those boards automatically. Keep your **ESP32S3 Dev Module** board selected and upload again.

### Wiring — ESP32 to L298N

| ESP32 GPIO | L298N / function |
|------------|------------------|
| **3.3V or 5V** | VCC (logic; module power separate) |
| **GND** | GND |
| **GPIO 26** | IN1 |
| **GPIO 27** | IN2 |
| **GPIO 14** | Stop signal (HIGH = stop) |

| Command | GPIO 26 (IN1) | GPIO 27 (IN2) | GPIO 14 (STOP) |
|---------|---------------|---------------|----------------|
| UP      | HIGH          | LOW           | LOW            |
| DOWN    | LOW           | HIGH          | LOW            |
| STOP    | LOW           | LOW           | **HIGH**       |

Device advertises as **TB-1E** when powered on.

## Protocol

Commands are newline-terminated ASCII strings:

| Button | Sent   |
|--------|--------|
| UP     | `UP`   |
| STOP   | `STOP` |
| DOWN   | `DOWN` |

ESP32 replies with `STATUS: UP`, `STATUS: DOWN`, or `STATUS: STOPPED`.

## Setup

### 1. ESP32 firmware

1. Install [ESP32 board support](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html) in Arduino IDE.
2. Open `esp32/bt_controller/bt_controller.ino`.
3. Select the board that matches your hardware:
   - **ESP32 Dev Module** — original ESP32 (WROOM-32)
   - **ESP32S3 Dev Module** — ESP32-S3 (most common new boards)
4. Select the correct **COM port** (CP2102 / CH340 USB driver if needed).
5. Upload the sketch.
6. Serial Monitor **115200 baud** — you should see:
   - `Bluetooth: Classic SPP` (WROOM-32), or
   - `Bluetooth: BLE UART` (S3/C3/C6)
   - `Bluetooth name: TB-1E`

### 2. Android app (v1.3+)

1. Install **app-debug.apk** (yellow bar shows **v1.3**).
2. Grant **Bluetooth** and **Location** permissions.
3. Power on ESP32.
4. In app **⋮** menu → **refresh** (waits ~5 s for BLE scan).
5. Select **TB-1E** → **KẾT NỐI MÁY THỬ DÂY AN TOÀN**.
6. Use **LÊN**, **DỪNG**, **XUỐNG**.

**WROOM-32 only:** you can also pair **TB-1E** in **Settings → Bluetooth** first (optional).

### 3. iOS app (v1.0)

Requires **ESP32-S3 / BLE** (iOS does not support Classic Bluetooth SPP).

1. On a Mac, open `ios/TB1EController.xcodeproj` in Xcode.
2. Set your **Signing Team** and run on your iPhone.
3. See [ios/README.md](ios/README.md) for full steps.

### 4. Web app (Chrome PC + Chrome Android)

Browser controller in `web/` — no install, **Web Bluetooth** (same protocol as Android app).

1. ESP32-S3 / C3 / C6 with BLE firmware, name **TB-1E**
2. Open **GitHub Pages** URL in **Chrome or Edge on PC**, or Chrome on Android — see [web/README.md](web/README.md)
3. **KẾT NỐI** → hold **LÊN / DỪNG / XUỐNG** (mouse/touch) or **↑ ↓ Space** on PC keyboard

**Safari iPhone:** Web Bluetooth not supported — use the iOS app instead.

### 5. Compile errors

| Error | Cause | Fix |
|-------|-------|-----|
| `BluetoothSerial.h: No such file` | Old sketch on S3 board | Use latest sketch — it switches to BLE on S3 |
| `#error Select original ESP32` | Old sketch | Update sketch from this repo |
| Upload fails | USB driver / boot mode | Hold **BOOT** during upload; install CP2102/CH340 driver |

## Troubleshooting

| Problem | Fix |
|---------|-----|
| **TB-1E** not in app menu | Power ESP32; tap **refresh** in app (BLE scan ~5 s) |
| App connect fails (WROOM-32) | Settings → **TB-1E** → **Disconnect**; connect only from app |
| App connect fails (S3) | Install app **v1.3+**; refresh; select scanned **TB-1E** |
| Motor not moving | Check GPIO 26/27/14 wiring to L298N |

## Project structure

```
arduino-bt-controller/
├── android/          # Kotlin app (Classic BT + BLE)
├── ios/              # SwiftUI iPhone app (BLE only)
├── web/              # Browser app (Web Bluetooth, Chrome Android)
├── esp32/            # ESP32 firmware
├── arduino/          # Legacy note (Nano + HC-05 removed)
└── README.md
```

## Safety

- App sends **RELEASE** on button release, disconnect, and when paused.
- ESP32 sets all GPIO **HIGH** when no app is connected.
