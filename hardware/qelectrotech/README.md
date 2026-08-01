# TB-1E — ESP32 ↔ CW-022 Wiring (QElectroTech)

Wiring sketch for **ESP32 Dev Module** connected to **CW-022** 2-channel 5V relay modules, for the TB-1E wireless controller firmware.

## Open in QElectroTech

1. Install [QElectroTech](https://qelectrotech.org/) (0.8 or newer recommended).
2. Open file:
   ```
   hardware/qelectrotech/TB-1E_ESP32_CW022_Wiring.qet
   ```
3. Three folios (tabs):
   - **F1 — ESP32 to CW-022 Control** — main control wiring
   - **F2 — CW-022 Relay Outputs** — relay contacts to machine
   - **F3 — Panel Inputs Reference** — switches and buttons

Custom symbols are embedded in the project (`tb1e` collection). Standalone copies are in `elements/`.

## CW-022 module

Standard **2-channel 5V opto-isolated relay board** (often labeled CW-022 / 2CH relay):

| Pin | Connect to |
|-----|------------|
| **VCC** | ESP32 **5V** (VIN / USB 5V) |
| **GND** | ESP32 **GND** (common ground) |
| **IN1** | ESP32 GPIO (see table) |
| **IN2** | ESP32 GPIO (see table) |
| **JD-VCC** | Jumper **ON** (default) — shares 5V with VCC |

**Trigger:** Active **LOW** — GPIO **LOW** = relay ON, GPIO **HIGH** = relay OFF.

TB-1E firmware needs **3 relay channels** → use **2× CW-022** modules.

## Control wiring (Folio 1)

| ESP32 | CW-022 module | Channel | Function |
|-------|---------------|---------|----------|
| **5V** | #1 VCC, #2 VCC | — | Logic + coil power |
| **GND** | #1 GND, #2 GND | — | Common ground |
| **GPIO 26** | #1 **IN1** | R1 | Motor IN1 (LÊN) |
| **GPIO 27** | #1 **IN2** | R2 | Motor IN2 (XUỐNG) |
| **GPIO 14** | #2 **IN1** | R1 | Motor STOP (DỪNG) |
| — | #2 IN2 | R2 | Spare |

## Relay outputs (Folio 2)

Wire **NO** (normally open) contacts unless your machine needs NC:

| Relay | COM | NO → |
|-------|-----|------|
| CW-022 #1 R1 | Machine IN1 common | IN1 active (LÊN) |
| CW-022 #1 R2 | Machine IN2 common | IN2 active (XUỐNG) |
| CW-022 #2 R1 | Machine STOP common | STOP active (DỪNG) |

## Power notes

- Use **5V / ≥1A** supply when both relay modules may energize together (~70 mA per channel).
- Keep **GND** shared between ESP32 and all CW-022 boards.
- For full opto-isolation: remove JD-VCC jumper and feed JD-VCC from a separate 5V supply (GND still common).

## Signal table (firmware)

| Command | GPIO 26 | GPIO 27 | GPIO 14 |
|---------|---------|---------|---------|
| LÊN (UP) | LOW | HIGH | HIGH |
| XUỐNG (DOWN) | HIGH | LOW | HIGH |
| DỪNG (STOP) | HIGH | HIGH | **LOW** |
| Idle | HIGH | HIGH | HIGH |

## ASCII overview

```
                    ┌─────────────────┐
    USB 5V ────────►│  ESP32 Dev Kit  │
                    │                 │
         GPIO26 ───►│ IO26 ───────────┼──► CW-022 #1 IN1 ──► R1 → IN1 (LÊN)
         GPIO27 ───►│ IO27 ───────────┼──► CW-022 #1 IN2 ──► R2 → IN2 (XUỐNG)
         GPIO14 ───►│ IO14 ───────────┼──► CW-022 #2 IN1 ──► R1 → STOP
                    │ 5V  ────────────┼──► VCC (both modules)
                    │ GND ────────────┼──► GND (both modules)
                    └─────────────────┘
```

## Edit symbols

To change symbols: **Projet → Collection embarquée → tb1e**, or edit files in `elements/` and re-import.
