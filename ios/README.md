# TB-1E Controller — iOS

SwiftUI iPhone app for controlling the **MAY THU DAY AN TOAN (TB-1E)** machine over **Bluetooth LE**.

Matches the Android app: **LÊN / DỪNG / XUỐNG** momentary buttons, connect to **TB-1E**, same command protocol (`UP`, `STOP`, `DOWN`, `RELEASE`).

## Important: iOS and Bluetooth type

| Platform | Classic Bluetooth (ESP32-WROOM SPP) | BLE (ESP32-S3 / C3 / C6) |
|----------|-------------------------------------|---------------------------|
| Android  | Yes                                 | Yes                       |
| **iOS**  | **No** (Apple restriction)          | **Yes**                   |

Use **ESP32-S3** firmware from `esp32/bt_controller/bt_controller.ino` with board **ESP32S3 Dev Module**.

## Requirements

- Mac with **Xcode 15+**
- iPhone with **iOS 16+**
- Apple Developer account (free tier works for device testing)
- ESP32-S3 running TB-1E BLE firmware

## Setup in Xcode

1. Open `ios/TB1EController.xcodeproj` in Xcode.
2. Target **TB1EController** → **Signing & Capabilities** → select your **Team**.
3. Connect iPhone → **Run** (▶).
4. Allow **Bluetooth** when prompted.

### Machine panel image

If the machine image is missing:

```bash
cp android/app/src/main/res/drawable/asset1.jpg \
   ios/TB1EController/Assets.xcassets/asset1.imageset/asset1.jpg
```

## Using the app

1. Power on ESP32 (**TB-1E**).
2. Tap **⋯** → **Refresh devices** (~5 s scan).
3. Select **TB-1E**.
4. Tap **KẾT NỐI MÁY THỬ DÂY AN TOÀN**.
5. **Hold** **LÊN / DỪNG / XUỐNG** — release sends `RELEASE`.

## Protocol

| Action         | Command   |
|----------------|-----------|
| LÊN pressed    | `UP`      |
| XUỐNG pressed  | `DOWN`    |
| DỪNG pressed   | `STOP`    |
| Button release | `RELEASE` |

BLE: Nordic UART `6E400001-B5A3-F393-E0A9-E50E24DCCA9E`
