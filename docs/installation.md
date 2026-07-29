# Installation

## Verified v1.1.0 build environment

The `v1.1.0` release state was compiled successfully on 29 July 2026 with:

- Arduino IDE `2.3.10`
- bundled Arduino CLI `1.5.1`
- Teensy board package `1.62.0`
- Teensy 4.1 board definition
- `USBHost_t36` library metadata version `0.2`, supplied by the Teensy package

Both the standard LCD build and the HID-only diagnostic build completed without
compiler warnings:

| Build | FLASH total | FLASH code | FLASH data | FLASH headers | RAM1 variables | RAM2 variables |
|---|---:|---:|---:|---:|---:|---:|
| Standard LCD | 103,420 | 68,924 | 26,220 | 8,276 | 60,224 | 12,864 |
| HID-only | 91,132 | 65,144 | 17,544 | 8,444 | 58,144 | 12,864 |

Other versions may work, but they have not been verified for this repository
snapshot.

Animation storage and comparison with the immediately preceding implementation
are documented in
[`startup-animation.md`](startup-animation.md).

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
as required by Arduino. Their historical `g13_marie_v1_0_0` name is retained for
`v1.1.0` because renaming both would create unnecessary path churn and release
risk. The current version is maintained in the sketch's `info.txt`, README and
changelog.

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

## Build variants

The compile-time controls are in `G13Config.h`:

| Variant | Relevant settings |
|---|---|
| Standard | documented defaults |
| HID-only diagnostic | `G13_LCD_ENABLE=0` |
| Repeating story | `G13_LCD_ANIMATION_REPEAT=1` |
| Permanent frame only | `G13_LCD_ANIMATION_ENABLE=0`, `G13_LCD_PERMANENT_FRAME_ENABLE=1` |
| Previous static fallback | `G13_LCD_ANIMATION_ENABLE=0`, `G13_LCD_PERMANENT_FRAME_ENABLE=0`, `G13_LCD_STATIC_FALLBACK_ENABLE=1` |
| No startup image | `G13_LCD_ANIMATION_ENABLE=0`, `G13_LCD_PERMANENT_FRAME_ENABLE=0`, `G13_LCD_STATIC_FALLBACK_ENABLE=0` |

The standard and HID-only variants are mandatory release builds. The other
variants are compile-checked configuration paths; their physical-device status
is documented separately in
[`hardware-validation.md`](hardware-validation.md).

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
- The LCD startup path should play the bundled eight-frame story, hold
  `READY FOR AZEROTH`, then keep `M² inside | Powered by Marie` visible.
- Pressing G1 through G22 should emit the documented keyboard characters.
- The serial monitor at `115200` baud can be used for connection, LCD and stall
  diagnostics.

Continuous raw and formatted HID report output is disabled by default so serial
backpressure cannot affect normal key handling. It can still be enabled
temporarily with the controls described in the troubleshooting guide.

The main `v1.1.0` keyboard, disconnect and animation paths have been exercised
on a physical G13. See [`hardware-validation.md`](hardware-validation.md) for
the exact successful checks and the special cases that remain open.

## Release checksums

`firmware/SHA256SUMS-v1.1.0.txt` contains SHA-256 fingerprints for every regular
file directly in the firmware sketch directory. It was generated after the
successful standard and HID-only builds and before the release commit. The
separate `SHA256SUMS-v1.0.0.txt` remains unchanged as a historical baseline.

Verify `v1.1.0` from the `firmware/` directory:

```sh
shasum -a 256 -c SHA256SUMS-v1.1.0.txt
```

Do not publish an unreviewed serial log. USB descriptors may include a connected
device serial number.
