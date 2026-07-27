# Installation

## Verified build environment

The repository copy was compiled successfully on 27 July 2026 with:

- Arduino IDE `2.3.10`
- bundled Arduino CLI `1.5.1`
- Teensy board package `1.61.0`
- Teensy 4.1 board definition
- `USBHost_t36` library metadata version `0.2`, supplied by the Teensy package

Successful compile memory report:

```text
FLASH: code 67600, data 18568, headers 9060
RAM1:  variables 60256, code 65072, padding 464
RAM2:  variables 12864
```

Other versions may work, but they have not been verified for this repository
snapshot.

## Install Arduino IDE and Teensy support

1. Install Arduino IDE 2.x from <https://www.arduino.cc/en/software/>.
2. Open Arduino IDE settings.
3. Add the PJRC package URL to **Additional Boards Manager URLs**:

   ```text
   https://www.pjrc.com/teensy/package_teensy_index.json
   ```

4. Open Boards Manager, search for `Teensy` and install the Teensy board
   package.

## Open the sketch

Open:

```text
firmware/g13_marie_v1_0_0/g13_marie_v1_0_0.ino
```

The sketch folder and primary `.ino` file intentionally have identical names,
as required by Arduino.

## Required board settings

Select:

| Setting | Value |
|---|---|
| Board | Teensy 4.1 |
| USB Type | Serial + Keyboard + Mouse + Joystick |
| CPU Speed | 450 MHz |
| Optimize | Faster |
| Keyboard Layout | German (Mac) |

The firmware uses both serial diagnostics and the Teensy `Keyboard` API. The
documented USB type must therefore be selected for the verified configuration.

If a different keyboard layout is selected, emitted characters may differ from
the mappings in `keymapping.md`.

## Compile and upload

1. Connect the Teensy USB device/client port directly to the computer.
2. Confirm the board and options above.
3. Click **Verify** to compile.
4. Click **Upload**.
5. If requested by the Teensy Loader, press the Teensy program button once.
6. After upload, connect the G13 through the powered hub as described in the
   wiring guide.

## Initial verification

- The Teensy onboard LED should blink quickly during startup, then settle to a
  slower pulse.
- The G13 should be detected on the Teensy USB host side.
- The LCD startup path should attempt to display the bundled splash screen.
- Pressing G1 through G22 should emit the documented keyboard characters.
- The serial monitor at `115200` baud can be used for connection, LCD and stall
  diagnostics.

Do not publish an unreviewed serial log. USB descriptors may include a connected
device serial number.

