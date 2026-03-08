
# Tuner Calibration — RC Timing → Folder Selection

This project uses an **RC timing measurement** on the vintage radio tuning capacitor
to select an MP3 folder (1–4).  
The intent is to preserve the original tuning knob while mapping its electrical
position to discrete digital “stations”.

---

## Where the Logic Lives

Calibration and classification logic is implemented in:

- `Radio_Tuning.cpp`
- `Radio_Tuning.h`

This document describes **what the code currently does**, not historical or experimental values.

---

## Measurement Method (RC Timing)

Each tuning poll performs the following steps:

1. Configure the tuning pin as `OUTPUT`, drive **LOW**
2. Hold LOW for **10 ms** to fully discharge the capacitor
3. Switch the pin to `INPUT` (no pull‑ups)
4. Measure the time (µs) until the pin reads **HIGH**
5. Repeat the measurement **3 times**
6. Take the **median** of the three samples

This approach:
- Rejects single‑sample noise
- Avoids floating‑point math
- Is stable on the Arduino Nano

---

## Timing Constants (Current)

| Parameter | Value |
|---------|------|
| Discharge time | **10 ms** |
| Timeout | **2,000,000 µs (2 s)** |
| Samples per poll | **3 (median‑of‑three)** |
| Inter‑sample delay | **2 ms** |

If **any sample times out**, the entire poll is treated as a **FAULT**.

---

## Folder Classification (Latest Figures)

Measured time (`t_us`) is mapped to a folder or gap as follows:

| Measured time (µs) | Result |
|-------------------|--------|
| **0 – 52** | Folder **4** |
| **>52 – <60** | Gap |
| **60 – 76** | Folder **3** |
| **>76 – <96** | Gap |
| **96 – 140** | Folder **2** |
| **>140 – <172** | Gap |
| **≥172** | Folder **1** |

### Special values
- **99** → Gap / between stations
- **255** → Fault (timeout or invalid measurement)

Faults are treated the same as gaps by the rest of the system.

---

## Stability / Anti‑Flicker Logic

The system does **not** switch folders immediately.

Each classified result must repeat consecutively before it is committed:

| Class | Required consecutive hits |
|-----|---------------------------|
| Folder (1–4) | **4** |
| Gap (99) | **8** |
| Fault (255) | Immediate |

Internal state:
- `pendingClass` — candidate folder/gap
- `pendingHits` — consecutive matches
- `currentFolder` — committed, stable value

This prevents chatter when the tuning knob rests near a boundary.

---

## Runtime Outputs

Two values are always maintained internally:

| Value | Meaning |
|-----|--------|
| `currentFolder` | Stable, committed folder used by MP3 + display |
| `lastInstantClass` | Immediate classification (used for visuals) |

This allows:
- Stable audio playback
- Responsive “between stations” visual effects

---

## Debug Output (Calibration & Diagnostics)

Debug output is **compile‑time controlled** in `Config.h`.

### Relevant flags

```c
#define DEBUG 1

#define TUNING_DEBUG          1   // folder commit events
#define TUNING_STREAM_DEBUG  0   // continuous timing output (very noisy)
#define TUNING_VERBOSE_DEBUG 0   // raw samples and hit counts

