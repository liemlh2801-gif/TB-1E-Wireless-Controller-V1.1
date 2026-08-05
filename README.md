# ESP32 Bluetooth Controller

Android app + **iOS app** + **web app** + ESP32 firmware for controlling a motor/actuator with **UP**, **STOP**, and **DOWN** commands over Bluetooth.

No HC-05 module required — Bluetooth is built into the ESP32.

**Recommended project path (ASCII only, for Gradle/Fritzing on Windows):**

`D:\01 Lam viec\Gia-cong\drawings\safety-belt-tester\APP\arduino-bt-controller`

## Hardware

### Which ESP32 board?

| Board in Arduino IDE | Chip | Bluetooth | App connection |
|----------------------|------|-----------|----------------|
| **ESP32 Dev Module** | WROOM-32 | **BLE** (NUS) | Android, **Chrome web**, pair via app/browser |
| **ESP32S3 Dev Module** | S3 | BLE only | Same as WROOM |
| ESP32-C3 / C6 | C3 / C6 | BLE only | Same as S3 |

**Important:** If you see `BluetoothSerial.h: No such file`, you selected **ESP32-S3** (or C3/C6). That is normal — the sketch now uses **BLE** on those boards automatically. Keep your **ESP32S3 Dev Module** board selected and upload again.

### Wiring — ESP32 to L298N

| ESP32 GPIO | L298N / function |
|------------|------------------|
| **3.3V or 5V** | VCC (logic; module power separate) |
| **GND** | GND |
| **GPIO 26** | IN1 |
| **GPIO 27** | IN2 |
| **GPIO 14** | Stop signal |
| **GPIO 25** | BLE connected (idle HIGH; LOW while phone/PC is connected) |
| **GPIO 32** | Manual / Auto mode switch (see below) |
| **GPIO 18** | Top limit switch (see below) |
| **GPIO 19** | Bottom limit switch (see below) |
| **GPIO 33** | ON-HOLD switch (see below) |

| Command | GPIO 26 (IN1) | GPIO 27 (IN2) | GPIO 14 (STOP) |
|---------|---------------|---------------|----------------|
| UP      | LOW           | HIGH          | HIGH           |
| DOWN    | HIGH          | LOW           | HIGH           |
| STOP    | HIGH          | HIGH          | **LOW**        |
| RELEASE / idle | HIGH     | HIGH          | HIGH           |

### Manual / Auto mode switch (GPIO 32)

Physical **Manual / Auto** selector on the machine panel. ESP32 reads **GPIO 32** with internal pull-down (Manual is default):

| Switch position | GPIO 32 | Mode | App behavior |
|-----------------|---------|------|----------------|
| **Manual** (default) | **Open** or **GND** | Manual | Momentary — hold LÊN/DỪNG/XUỐNG, release sends `RELEASE` |
| **Auto** | **3.3V only** | Auto | Latch — press/release LÊN or XUỐNG keeps output until DỪNG, opposite direction, or limit |

**Wiring:**

- **Manual:** leave GPIO 32 **open**, or connect to **GND**
- **Auto:** connect GPIO 32 to **3.3V** only (do not use open/float for Auto)

On mode change or BLE connect, ESP32 notifies the app with `MODE: MANUAL` or `MODE: AUTO`.

### App buttons (web / Android / iOS)

App / web buttons match the physical panel (GPIO 21 / 22 / 23). The ESP32 applies the same rules to BLE commands via `maintainAppPanelButtons()` (Manual: hold to run; Auto: latch on press; DỪNG priority). Physical panel inputs take priority if pressed at the same time.

**Manual** (GPIO 32 open or GND):

| Button | Press | Release |
|--------|-------|---------|
| **LÊN** | `UP` — GPIO 26 LOW, GPIO 27/14 HIGH | `RELEASE` — all HIGH |
| **DỪNG** | `STOP` — GPIO 14 LOW | `RELEASE` — all HIGH |
| **XUỐNG** | `DOWN` — GPIO 27 LOW, GPIO 26/14 HIGH | `RELEASE` — all HIGH |

Priority while held: **DỪNG** > **LÊN** > **XUỐNG** (same as panel).

**Auto** (GPIO 32 on 3.3V):

| Button | Press | Release |
|--------|-------|---------|
| **LÊN** | `UP` — GPIO 26 LOW, GPIO 27/14 HIGH | No change — latched |
| **DỪNG** | `STOP` — GPIO 14 LOW | `RELEASE` — all HIGH |
| **XUỐNG** | `DOWN` — GPIO 27 LOW, GPIO 26/14 HIGH | No change — latched |

While **DỪNG** is held, **LÊN** / **XUỐNG** are ignored (same as panel).

### Limit switches (GPIO 18 / GPIO 19)

Two **normally-open** limit switches. ESP32 reads **GPIO 18** (top) and **GPIO 19** (bottom) with internal pull-up:

| Switch | GPIO | Active (limit hit) | Inactive (normal) |
|--------|------|--------------------|-------------------|
| **Top limit** | **18** | **LOW** (pin to **GND**) | **Open** or **HIGH** |
| **Bottom limit** | **19** | **LOW** (pin to **GND**) | **Open** or **HIGH** |

**While top limit active:** GPIO 26 set HIGH, then disabled (cannot go LOW); GPIO 27 and GPIO 14 unchanged.

**While bottom limit active:** GPIO 27 set HIGH, then disabled (cannot go LOW); GPIO 26 and GPIO 14 unchanged.

**Wiring (each limit switch):**

- One terminal → **GPIO 18** (top) or **GPIO 19** (bottom)
- Other terminal → **GND**
- Switch closes to GND when the carriage hits the limit (NO switch)

When top limit clears, GPIO 26 is enabled again; if **GPIO 21** is still connected to **GND**, GPIO 26 is allowed to go **LOW**.

When bottom limit clears, GPIO 27 is enabled again; if **GPIO 22** is still connected to **GND**, GPIO 27 is allowed to go **LOW**.

On limit change or BLE connect, ESP32 sends `LIMIT: TOP`, `LIMIT: TOP OFF`, `LIMIT: BOT`, or `LIMIT: BOT OFF`.

### ON-HOLD switch (GPIO 33)

ESP32 reads **GPIO 33** with internal pull-up:

| Switch | GPIO 33 | Effect |
|--------|---------|--------|
| **ON-HOLD active** | **LOW** (pin to **GND**) | **UP input and output stay enabled**; AUTO-DOWN latch cleared |
| **Inactive** | **Open** or **HIGH** | Normal operation |
| **ON-HOLD release** (active→inactive) | LOW→open | If ON-HOLD stays **inactive for 3 seconds**: GPIO 26 inactive (HIGH), GPIO 27 **GND** (LOW), GPIO 14 inactive (HIGH) — same as panel XUỐNG press-release once in Auto (if GPIO 4 enabled). Count resets if ON-HOLD goes active again. |

**Wiring:** one terminal → **GPIO 33**, other → **GND** (closes to GND when hold is engaged).

Panel **LÊN** (GPIO 21), app **UP**, and GPIO 26 output are **never disabled** while ON-HOLD is active.

Device advertises as **TB-1E** when powered on.

## Protocol

Commands are newline-terminated ASCII strings:

| Button | Sent   |
|--------|--------|
| UP     | `UP`   |
| STOP   | `STOP` |
| DOWN   | `DOWN` |

Mode notifications from ESP32:

| Message | Meaning |
|---------|---------|
| `MODE: MANUAL` | Manual mode — momentary buttons |
| `MODE: AUTO` | Auto mode — UP/DOWN latch after release |
| `LIMIT: TOP` / `LIMIT: TOP OFF` | Top limit active / cleared |
| `LIMIT: BOT` / `LIMIT: BOT OFF` | Bottom limit active / cleared |

ESP32 replies with `STATUS: UP`, `STATUS: DOWN`, or `STATUS: STOP`.

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
   - `Bluetooth: BLE UART (TB-1E)`
   - `Bluetooth name: TB-1E`

### 2. Android app (v1.3+)

1. Install **app-debug.apk** (yellow bar shows **v1.3**).
2. Grant **Bluetooth** and **Location** permissions.
3. Power on ESP32.
4. In app **⋮** menu → **refresh** (waits ~5 s for BLE scan).
5. Select **TB-1E** → **KẾT NỐI MÁY THỬ DÂY AN TOÀN**.
6. Use **LÊN**, **DỪNG**, **XUỐNG**.

**WROOM-32 / S3:** connect from the app or Chrome — do not rely on Windows Settings pairing alone.

### 3. iOS app (v1.0)

Requires **ESP32 BLE** firmware (WROOM-32 or S3/C3/C6).

1. On a Mac, open `ios/TB1EController.xcodeproj` in Xcode.
2. Set your **Signing Team** and run on your iPhone.
3. See [ios/README.md](ios/README.md) for full steps.

### 4. Web app (Chrome PC + Chrome Android)

Browser controller in `web/` — no install, **Web Bluetooth** (same protocol as Android app).

1. ESP32 with BLE firmware (WROOM-32 or S3/C3/C6), name **TB-1E**
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
