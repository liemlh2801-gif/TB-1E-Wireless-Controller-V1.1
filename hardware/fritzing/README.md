# TB-1E — Fritzing wiring (ESP32 38-pin + CW-022)

Realistic breadboard wiring diagram for **TB-1E** using **Fritzing 0.9.3** at:

```
E:\fritzing.0.9.3b.64.pc\Fritzing.exe
```

## ESP32 part (import first)

This sketch uses the **ESP32 DevKitc V4** part from your **Mine** parts bin (not the broken embedded TB1E part).

1. In Fritzing: **File → Open** → `C:\Users\Admin\Downloads\ESP32 DevKitc V4.fzpz`  
   (or **Mine → Import** if already imported once)
2. Then open the wiring sketch via `open_in_fritzing.bat`

| Wire | ESP32 DevKitc V4 pin | CW-022 |
|------|----------------------|--------|
| Red | **5V** | VCC |
| Black | **GND** | GND |
| Blue | **IO26** | IN1 |
| Blue | **IO27** | IN2 |
| Green | **IO14** | IN3 |

> **Note:** You imported `ESP32 DevKitc V4.fzpz` (in Downloads), not `ESP32-38PinWide-fixed.fzpz` from the Fritzing forum. Both are 38-pin ESP32 boards; DevKitc V4 has a photo-realistic breadboard view and is valid for TB-1E wiring.

## Quick start

1. **Generate** (if `TB-1E_ESP32_CW022_Wiring.fzz` is missing):
   ```
   python hardware/fritzing/generate_tb1e_fritzing.py
   ```

2. **Open** — double-click:
   ```
   hardware/fritzing/open_in_fritzing.bat
   ```
   Or in Fritzing: **File → Open** → `C:\TB1E_Fritzing\TB-1E_ESP32_CW022_Wiring.fzz`

   **Important:** Fritzing 0.9.3 on Windows cannot open `.fzz` files from paths with Vietnamese/Unicode characters (`Gia công`, `bản vẽ`, …). You will see `zip.open(): %d`. Always use the batch file or the copy under `C:\TB1E_Fritzing\`.

3. Switch to **Breadboard** view for the physical wiring layout.

## What's in the sketch

| Part | Pins shown |
|------|------------|
| **ESP32 DevKitc V4** | Imported user part — 38 pins (IO26, IO27, IO14, 5V, GND, …) |
| **CW-022 4CH relay** | GND, IN1, IN2, IN3, IN4, VCC |

### Pre-wired (motor outputs)

| Wire color | ESP32 | CW-022 | Function |
|------------|-------|--------|----------|
| Red | 5V | VCC | 5V power |
| Black | GND | GND | Ground |
| Blue | IO26 | IN1 | Motor IN1 (LÊN) |
| Blue | IO27 | IN2 | Motor IN2 (XUỐNG) |
| Green | IO14 | IN3 | STOP (DỪNG) |
| — | — | IN4 | Spare |

**CW-022 is active LOW:** GPIO HIGH = relay off, GPIO LOW = relay on.

## Add panel wiring in Fritzing

Drag wires from these ESP32 pins to switches/buttons (switch other terminal → **GND**):

| GPIO | Function |
|------|----------|
| 32 | Manual / Auto |
| 18 | Top limit |
| 19 | Bottom limit |
| 25 | ON-HOLD |
| 21 | Panel LÊN |
| 22 | Panel XUỐNG |
| 23 | Panel DỪNG |
| 4 | AUTO-DOWN |
| 33 | BLE connected output (LOW = connected) |

## Custom parts

- `parts/TB1E_CW022_4ch/` — CW-022 4-channel relay (embedded in `.fzz`)
- ESP32: use imported **ESP32 DevKitc V4** (see above). Legacy `TB1E_ESP32_38pin/` is no longer used by the sketch.

## Optional: ESP32-38PinWide-fixed (forum)

If you prefer Peter van Epp's forum part instead of DevKitc V4, download `ESP32-38PinWide-fixed.fzpz` from the [Fritzing forum](https://forum.fritzing.org/t/esp32-wroom-32u/12978), import it, then reconnect wires manually in the sketch.

## Regenerate

```
python hardware/fritzing/generate_tb1e_fritzing.py
```
