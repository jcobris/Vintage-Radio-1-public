We’re continuing my Vintage Radio project. Work one step at a time.

Public mirror repo: https://github.com/jcobris/Vintage-Radio-1-public
Please read:
- ASSISTANT_START_HERE.md
- PROJECT_STATE.md
- docs/REQUIREMENTS.md
- docs/ARCHITECTURE.md
- docs/PINOUT.md
- docs/README.md

Current sketch: sketch/Vintage-Radio-1.ino
Today’s goal: 

Hi Copilot — we’re continuing my Vintage Radio Arduino Nano retrofit.
Work one step at a time. Don’t redesign everything; keep behaviour close to what works.
Repo + files
Public mirror repo: https://github.com/jcobris/Vintage-Radio-1-public
Please read:

ASSISTANT_START_HERE.md
PROJECT_STATE.md
docs/REQUIREMENTS.md
docs/ARCHITECTURE.md
docs/PINOUT.md
docs/README.md

Current sketch + modules are already integrated and stable:

Arduino Nano (ATmega328P)
Serial debug 115200 set only once in main .ino
MP3 DY-SV5W on SoftwareSerial: D12 RX, D11 TX, 9600
Bluetooth BT201 on SoftwareSerial: D10 RX, D9 TX, 57600 (UART slept unless passthrough)
Source detect on D2: LOW=MP3, HIGH=Bluetooth (INPUT_PULLUP)
Display mode switch to GND: D3 NORMAL, D4 ALT (same for now), D5 MATRIX_OFF
Matrix data D7, Tuner LED string D6
Folder numbering standard: 1..4, 99 = gap, 255 = fault; treat 255 as 99 before calling MP3

Today’s goal
Physically test the tuner RC timing input and calibrate/refine the tuner logic so it:

reliably returns 1..4 for the 4 knob positions
reliably returns 99 in “dead space” between bands
reliably returns 255 only for genuine measurement failures (timeouts etc.)
keeps folder mapping consistent with “Option A”: folder 1 = old “00”, folder 4 = old “03”

Hardware under test

Tuner RC timing input is wired to Config::PIN_TUNING_INPUT = D8
The knob has 4 stable positions + dead zones between them

Current state + constraints

Code is stable and runs long-term (memory headroom improved); avoid reintroducing String or heap churn.
Debug should be predictable: boot messages, mode changes, folder changes.
Avoid spam unless behind explicit debug flags.
If you change files: return full replacement contents for each changed file.
If you need a new file: provide full content and exact path.

How we’ll work

You tell me Step 1 only: the minimum instrumentation needed to measure real tuner timings safely (without spamming), and exactly what I should do on the bench.
I will run the test and paste the Serial logs you ask for.
You’ll then propose Step 2 only (threshold adjustments / stability logic tweaks), and provide full file replacements if needed.

What I can provide

Serial logs at 115200
I can rotate the knob slowly/quickly and hold at each position
I can report observed behaviour (folder changes, dead-zone behaviour, fault occurrences)

First task for you (Step 1 only)
The tuning has been tested. When the capacitor tuning is disabled and the folder is set manually the MP3 plays as expected. When the capacitor is enabled the folder selection works properly (although needs refinment) but the music can't be heard. I suspect the mute setting from when the folder is at 99 (between channels) is this issue. Lets investigate this first.

Next step will be to turn on debugging of the tuner so we can get the folder changes 100% 

Design a minimal, safe data-collection approach to capture RC timing values and classification results for each knob position + dead spaces:

Then we should look at ways to free up some arduino resources. Possibly by reducing some of the timing.

Also there is an issue with the public github mirror. It doesn't seem to be updating.