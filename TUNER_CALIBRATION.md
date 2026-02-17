
# Tuner Calibration (RC Timing → Folder Selection)

This project uses an RC timing measurement on the tuning-capacitor mechanism to select an MP3 folder.
The logic lives in:

- `sketch/Vintage-Radio-1/Radio_Tuning.cpp`
- `sketch/Vintage-Radio-1/Radio_Tuning.h`

## What is being measured?

Each “sample” measures how long the RC node stays LOW after being released to INPUT (no pullups):

1. Drive tuning pin LOW as OUTPUT for `DISCHARGE_MS` to discharge the capacitor.
2. Switch pin to INPUT (no pullups).
3. Time until `digitalRead()` becomes HIGH.
4. Repeat 3 times and take the **median** (reduces noise).

Key parameters (current values in code):

- `DISCHARGE_MS = 10` ms
- `TIMEOUT_US   = 2,000,000` µs (2s safety timeout)
- Median-of-3 with `delay(2)` between samples

If timeout occurs, the code treats the measurement as **FAULT** (`t_us == 0`).

## Folder buckets (µs)

The tuner is classified into one of these buckets based on the measured `t_us`:

| Bucket | Meaning   | Range (µs) |
|--------|-----------|------------|
| `03`   | folder 3  | 0 → 30     |
| `99`   | dead-gap  | 31 → 40    |
| `02`   | folder 2  | 41 → 51    |
| `99`   | dead-gap  | 52 → 64    |
| `01`   | folder 1  | 65 → 89    |
| `99`   | dead-gap  | 90 → 122   |
| `00`   | folder 0  | >= 123     |

Current constants:

- `F03_LOWER_US = 0`
- `F03_UPPER_US = 30`

- `F02_LOWER_US = 41`
- `F02_UPPER_US = 51`

- `F01_LOWER_US = 65`
- `F01_UPPER_US = 89`

- `F00_LOWER_US = 123`  (no upper bound)

The “dead gaps” are intentional hysteresis zones between bands.

## Stability / hysteresis (anti-flicker)

The code does not instantly switch folders. It requires consecutive hits of the same bucket:

- `STABLE_COUNT_FOLDER = 4` for buckets `00/01/02/03`
- `STABLE_COUNT_GAP    = 4` for bucket `99`

Internal state:

- `pendingClass` tracks the current candidate bucket
- `pendingHits` counts consecutive matches
- `currentFolder` only changes once `pendingHits >= need`

This prevents jitter when the tuning knob sits near a boundary.

## Debug output (calibration mode)

Two compile-time flags control serial output:

### 1) Streaming timing output (recommended while calibrating)
```c
#define DEBUG_TIMING_STREAM 1
