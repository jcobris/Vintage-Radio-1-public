
# Vintage Radio Retrofit — Pinout

**Target:** Arduino Nano (ATmega328P)

---

## Inputs

| Pin | Purpose |
|---|---|
| D2 | Source detect (LOW=MP3, HIGH=Bluetooth) |
| D3 | Display mode: Normal |
| D4 | Display mode: Reserved |
| D5 | Display mode: Matrix OFF |
| D8 | Tuning capacitor timing input |
| D13 | MP3 next‑track button (active‑low) |

---

## Outputs

| Pin | Purpose |
|---|---|
| D6 | Dial / tuner LED strip (PWM) |
| D7 | WS2812B LED matrix data |
| A0 | Additional WS2812B LED strip |

---

## UART (SoftwareSerial)

| Module | Pins | Baud |
|---|---|---|
| Bluetooth (BT210) | D9 / D10 | 57600 |
| MP3 (DY‑SV5W) | D11 / D12 | 9600 |

---

## Notes

- All LED power supplies share common ground with Nano
- Series resistor recommended on WS2812 data lines
- Pin conflicts are enforced at compile time in `Config.h`
