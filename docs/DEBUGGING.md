DEBUGGING GUIDE — VINTAGE RADIO RETROFIT
This project uses compile‑time debug flags defined in Config.h.
When a debug option is disabled, all related debug code is removed at compile time.
There is no runtime cost, no branching, and no accidental serial output.
This document explains every available debug option, what it prints, and when it should be used.

GLOBAL DEBUG CONTROL
DEBUG
Set DEBUG to 1 to enable debug output.
Set DEBUG to 0 to completely remove all debug output.
When DEBUG is 0, no other debug flag has any effect.
Typical use:
DEBUG = 1 during development
DEBUG = 0 for final / production builds

BOOT AND MODE DEBUG
BOOT_DEBUG
Purpose:
Confirms the system has booted and serial output is working.
Typical output:
[BOOT] System ready
Notes:
Low volume.
Safe to leave enabled during development.

SRC_DEBUG
Purpose:
Logs high‑level system state changes.
Outputs:

Audio source changes (MP3 / Bluetooth)
Display mode switch changes

Typical output:
[SRC] MP3
[SRC] Bluetooth
Display mode: NORMAL
Display mode: ALTERNATE
Display mode: MATRIX_OFF
Notes:
Safe for long‑term use.
Very useful for validating switch wiring and logic.

TUNING AND FOLDER DEBUG
TUNING_DEBUG
Purpose:
Logs committed folder changes only (after stability filtering).
Outputs:

Folder numbers
Gap transitions

Typical output:
folder=1
folder=99
folder=4
Notes:
Recommended for normal development.
Low noise.
Does not print raw timing values.

TUNING_STREAM_DEBUG
WARNING: VERY NOISY — CALIBRATION ONLY
Purpose:
Streams every RC timing measurement and classification.
Outputs:

Raw timing value in microseconds
Instant classification
Current committed folder

Typical output:
t_us=83 inst=3 committed=3
t_us=90 inst=99 committed=3
t_us=145 inst=2 committed=3
Notes:
Use only while calibrating tuning thresholds.
Disable immediately afterward.

TUNING_VERBOSE_DEBUG
Purpose:
Deep inspection of tuning stability and hysteresis logic.
Outputs:

Individual RC samples
Stability hit counters
Commit diagnostics

Typical output:
samples: a=82 b=84 c=83
t_us=83 cls=3 hits=4
Notes:
Short‑term diagnostic use only.
Useful when adjusting thresholds or gap widths.

MP3 DEBUG
MP3_DEBUG
Purpose:
Logs high‑level MP3 subsystem behaviour.
Outputs:

MP3 online/offline status
Desired folder changes
Playback start events
Next‑track button events

Typical output:
MP3: Control Ready
MP3: Online
MP3: Desired folder = 3
MP3: Playback started (random-in-folder)
MP3: Next track
Notes:
Safe for day‑to‑day development.
Good balance of visibility and noise.

MP3_FRAME_DEBUG
WARNING: NOISY
Purpose:
Shows raw UART frames transmitted to the MP3 module.
Outputs:

Hex dumps of transmitted commands

Typical output:
MP3 TX: AA 06 00 B0
Notes:
Used when validating or troubleshooting MP3 protocol behaviour.
Not needed during normal operation.

MP3_RX_DEBUG
WARNING: VERY NOISY
Purpose:
Dumps raw bytes received from the MP3 module.
Outputs:
MP3 RX: 7E
MP3 RX: FF
Notes:
Rarely needed.
Mostly useful during protocol reverse‑engineering.
Disable immediately after use.

BLUETOOTH DEBUG
BT_DEBUG
Purpose:
Logs Bluetooth setup and passthrough activity.
Outputs:

AT command transmissions
Passthrough commands (when enabled)

Typical output:
[BT] Initial AT commands sent
[BT] Sent: AT+BDO'l Timey Radio
Notes:
Bluetooth normally runs without serial interaction.
Enable only when configuring or troubleshooting the module.

LED DEBUG
LED_STRIP_DEBUG
Purpose:
Logs LED strip initialisation and state changes.
Typical output:
[STRIP] Ready: 64
Notes:
Useful when bringing up or modifying strip patterns.
Low noise.

LED_MATRIX_DEBUG
Purpose:
Logs LED matrix initialisation and matrix‑level events.
Typical output:
[MATRIX] Initialised
Notes:
Optional.
Enable only when working on matrix logic.

LED_DIAL_DEBUG
Purpose:
Logs dial/tuner LED behaviour changes.
Outputs:

Mode transitions (solid, flicker, pulse)

Typical output:
[DIAL] Solid
[DIAL] Flicker
[DIAL] Pulse
Notes:
Useful when tuning PWM behaviour or diagnosing audible noise.

RECOMMENDED DEBUG PRESETS
Normal development:
DEBUG = 1
BOOT_DEBUG = 1
SRC_DEBUG = 1
MP3_DEBUG = 1
TUNING_DEBUG = 1
Tuning calibration:
DEBUG = 1
TUNING_STREAM_DEBUG = 1
Deep tuning diagnostics:
DEBUG = 1
TUNING_VERBOSE_DEBUG = 1
Final / production build:
DEBUG = 0

DESIGN RATIONALE

Compile‑time flags guarantee zero runtime overhead
Debug intent is explicit and visible in Config.h
No accidental serial flooding
Safe for long‑term maintenance
