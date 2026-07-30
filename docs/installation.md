# Installation

## Verified v1.2.0 build environment

The `v1.2.0` release state was compiled successfully on 30 July 2026 with:

- Arduino IDE `2.3.10`
- bundled Arduino CLI `1.5.1`
- Teensy board package `1.62.0`
- Teensy 4.1 board definition
- `USBHost_t36` library metadata version `0.2`, supplied by the Teensy package

All four release variants completed without compiler warnings:

| Build | FLASH total | FLASH code | FLASH data | FLASH headers | RAM1 variables | RAM2 variables |
|---|---:|---:|---:|---:|---:|---:|
| Standard Marie/Latte | 104,444 | 69,244 | 26,220 | 8,980 | 60,224 | 12,864 |
| HID-only | 92,156 | 65,464 | 17,544 | 9,148 | 58,144 | 12,864 |
| Static theme | 96,252 | 68,984 | 18,568 | 8,700 | 60,192 | 12,864 |
| No startup image | 95,228 | 68,792 | 17,544 | 8,892 | 60,192 | 12,864 |

Complete Python test discovery passed 14 of 14 tests, including all four
user-configuration tests, and the strict C++ animation-timeline test passed.
These are compile and host-test results. No `v1.2.0` build was uploaded to a
Teensy or tested with a physical G13 during this development step.

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
`v1.2.0` because renaming both would create unnecessary path churn and release
risk. The current version is maintained in the sketch's `info.txt`, README and
changelog.

## Configure normal user settings

The standard defaults can be compiled without editing anything. For normal
customization, edit only:

```text
firmware/g13_marie_v1_0_0/G13UserConfig.h
```

It contains the G1 through G22 mapping, RGB/key-backlight color, LCD master
theme, Marie/Latte playback options and frame timing. See
[`user-configuration.md`](user-configuration.md) for every setting, supported
Teensy keycode examples and the default values.

Do not edit `.ino`, `.cpp` or internal header files for normal setup. Every
configuration change requires a new compile and upload.

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

Keep **German (Mac)** selected to reproduce the documented build environment.
The G-key table uses direct `KEY_*` positions, so its characters are interpreted
by the host operating system's active keyboard layout. A different host layout
can therefore differ from the results in `keymapping.md`, especially at Y/Z.

## Themes and build variants

Normal presentation variants are selected in `G13UserConfig.h`:

| Variant | Relevant settings |
|---|---|
| Standard Marie/Latte | documented defaults; `G13_LCD_THEME=G13_LCD_THEME_MARIE_LATTE` |
| Repeating story | Marie/Latte theme and `G13_LCD_ANIMATION_REPEAT=1` |
| Permanent frame only | Marie/Latte theme, `G13_LCD_ANIMATION_ENABLE=0`, `G13_LCD_PERMANENT_FRAME_ENABLE=1` |
| Marie static fallback | Marie/Latte theme, animation and permanent frame `0`, `G13_LCD_STATIC_FALLBACK_ENABLE=1` |
| Previous static image | `G13_LCD_THEME=G13_LCD_THEME_STATIC` |
| No startup image | `G13_LCD_THEME=G13_LCD_THEME_NONE` |

The theme IDs are fixed named constants; change only the `G13_LCD_THEME`
selection, not their numeric definitions.

The HID-only diagnostic uses the internal `G13_LCD_ENABLE=0` control in
`G13Config.h`. It removes LCD initialization, LCD OUT transfers and lighting,
and is not a normal user theme. The standard, HID-only, static-theme and
no-start-image release variants were compiled without warnings. Their physical
device status is documented separately in
[`hardware-validation.md`](hardware-validation.md).

## Compile and upload

1. Connect the Teensy USB device/client port directly to the computer.
2. Confirm the board and options above.
3. If desired, edit and save only `G13UserConfig.h`.
4. Click **Verify** to compile.
5. Correct any named configuration error before continuing.
6. Click **Upload**.
7. If requested by the Teensy Loader, press the Teensy program button once.
8. After upload, connect the G13 through the powered hub as described in the
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

The behavior above was exercised on a physical G13 for `v1.1.0`. The `v1.2.0`
defaults are protected by automated tests, but the new configuration layer and
its build variants were not uploaded or physically tested in this development
step. See [`hardware-validation.md`](hardware-validation.md) for the exact
boundary.

## Release checksums

`firmware/SHA256SUMS-v1.2.0.txt` contains SHA-256 fingerprints for every regular
file directly in the firmware sketch directory, including `G13UserConfig.h`. It
was generated after the successful release builds and host tests. The separate
`SHA256SUMS-v1.0.0.txt` and `SHA256SUMS-v1.1.0.txt` files remain unchanged
historical baselines.

Verify `v1.2.0` from the `firmware/` directory:

```sh
shasum -a 256 -c SHA256SUMS-v1.2.0.txt
```

Do not publish an unreviewed serial log. USB descriptors may include a connected
device serial number.
