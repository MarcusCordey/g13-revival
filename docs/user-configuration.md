# User Configuration

`G13UserConfig.h` is the supported interface for normal firmware
customization. It is located at:

```text
firmware/g13_marie_v1_0_0/G13UserConfig.h
```

Edit this file to change the G1 through G22 key assignments, RGB/key-backlight
color, LCD theme, animation behavior or display timing. Normal setup does not
require changes to any `.ino`, `.cpp` or internal header file.

Every change is compiled into the firmware. It takes effect only after the
sketch has been compiled again and uploaded to the Teensy.

## Before editing

- The checked-in defaults reproduce the `v1.1.0` key mapping, blue backlight,
  eight-frame Marie/Latte story, timing and permanent signature.
- Change one group of settings at a time, then compile before making further
  changes. Compile-time checks identify most invalid values by name.
- Use only Teensy `KEY_*` constants for G-key mappings. Do not replace them with
  quoted ASCII character literals.
- Keep a copy of your preferred values outside the firmware folder if you plan
  to update or replace the checkout later.

## Basic workflow

1. Open
   `firmware/g13_marie_v1_0_0/g13_marie_v1_0_0.ino`
   in Arduino IDE.
2. Open the `G13UserConfig.h` tab.
3. Change only the values after `#define`; keep the macro names unchanged.
4. Save the file.
5. Select the board settings documented under
   [Compile and upload](#compile-and-upload).
6. Click **Verify** and correct any configuration error reported by the
   compiler.
7. Click **Upload** and allow Teensy Loader to program the board.
8. Reconnect or restart the G13 setup and verify the changed behavior.

## G1 through G22 key mapping

Each physical G-key has one clearly named macro. The defaults are:

| G13 key | Macro | Default | Result with German (Mac) |
|---|---|---|---|
| G1 | `G13_KEY_G1` | `KEY_1` | `1` |
| G2 | `G13_KEY_G2` | `KEY_2` | `2` |
| G3 | `G13_KEY_G3` | `KEY_3` | `3` |
| G4 | `G13_KEY_G4` | `KEY_4` | `4` |
| G5 | `G13_KEY_G5` | `KEY_5` | `5` |
| G6 | `G13_KEY_G6` | `KEY_6` | `6` |
| G7 | `G13_KEY_G7` | `KEY_7` | `7` |
| G8 | `G13_KEY_G8` | `KEY_K` | `k` |
| G9 | `G13_KEY_G9` | `KEY_L` | `l` |
| G10 | `G13_KEY_G10` | `KEY_A` | `a` |
| G11 | `G13_KEY_G11` | `KEY_W` | `w` |
| G12 | `G13_KEY_G12` | `KEY_D` | `d` |
| G13 | `G13_KEY_G13` | `KEY_M` | `m` |
| G14 | `G13_KEY_G14` | `KEY_N` | `n` |
| G15 | `G13_KEY_G15` | `KEY_O` | `o` |
| G16 | `G13_KEY_G16` | `KEY_P` | `p` |
| G17 | `G13_KEY_G17` | `KEY_S` | `s` |
| G18 | `G13_KEY_G18` | `KEY_Q` | `q` |
| G19 | `G13_KEY_G19` | `KEY_R` | `r` |
| G20 | `G13_KEY_G20` | `KEY_SPACE` | Space |
| G21 | `G13_KEY_G21` | `KEY_U` | `u` |
| G22 | `G13_KEY_G22` | `KEY_T` | `t` |

### Letters, numbers and Space

Use Teensy key names without quotation marks:

```cpp
#define G13_KEY_G10 KEY_A
#define G13_KEY_G11 KEY_W
#define G13_KEY_G12 KEY_D
#define G13_KEY_G17 KEY_S
#define G13_KEY_G20 KEY_SPACE
```

The same pattern applies to other letters and numbers:

```cpp
#define G13_KEY_G1 KEY_1
#define G13_KEY_G2 KEY_2
#define G13_KEY_G8 KEY_B
#define G13_KEY_G9 KEY_C
```

Other normal-key Teensy constants such as `KEY_TAB`, `KEY_ENTER`, `KEY_ESC` or
`KEY_F1` can also be assigned. The supported named range is `KEY_A` through
`KEY_MENU`, plus `KEY_F13` through `KEY_F24`.
Mapping two G-keys to the same `KEY_*` value is supported. The shared keyboard
key uses one represented keyboard slot and remains pressed until both physical
G-keys have been released.

The firmware accepts only the corresponding Teensy 1.62 encodings
`0xF004` through `0xF065` and `0xF068` through `0xF073`. This catches quoted
character literals, modifier-only, media and system codes, raw out-of-range
numbers and misspelled, therefore undefined, `KEY_*` names. Use the supported
named constants rather than numeric values.

### German (Mac) keyboard layout

The verified board setting is:

```text
Keyboard Layout: German (Mac)
```

Keep this Arduino setting for the reproducible, physically tested `v1.1.0`
build environment and for any layout-aware `Keyboard` operations outside the
G-key table. The G-key table itself uses direct USB-HID `KEY_*` positions; those
values are not remapped by the Arduino keyboard-layout choice.

The host operating system interprets the direct positions using its active
layout. They are not layout-aware character strings. Positions that differ
between host layouts, especially Y and Z, can therefore produce a different
character when the host is not using the documented German (Mac) layout.

The default values above produce the documented characters with German (Mac).
After changing the firmware mapping or the host keyboard layout, test every
affected G-key.

The selected Teensy keyboard profile represents at most six ordinary keys
simultaneously. Additional held G-keys wait until a slot becomes free.

## RGB/key-backlight color

Set each channel to an integer from `0` through `255`:

```cpp
#define G13_BACKLIGHT_RED   0
#define G13_BACKLIGHT_GREEN 0
#define G13_BACKLIGHT_BLUE  255
```

The defaults preserve the existing blue startup color. Common examples are:

| Color | Red | Green | Blue |
|---|---:|---:|---:|
| Off / black | 0 | 0 | 0 |
| Red | 255 | 0 | 0 |
| Green | 0 | 255 | 0 |
| Blue, default | 0 | 0 | 255 |
| Yellow | 255 | 255 | 0 |
| Cyan | 0 | 255 | 255 |
| Magenta | 255 | 0 | 255 |
| White | 255 | 255 | 255 |
| Warm orange | 255 | 96 | 0 |

The control report comes from the earlier G13 reference implementation. On a
particular G13 it may affect global/key lighting rather than the monochrome LCD
itself. Non-default colors were compile-tested but not validated on physical
hardware in this development step.

## LCD theme

`G13_LCD_THEME` is the master choice for the automatic startup graphic:

| Theme value | Behavior |
|---|---|
| `G13_LCD_THEME_MARIE_LATTE` | Default. Use the eight-frame story and its Marie-specific options. |
| `G13_LCD_THEME_STATIC` | Show the previous static G13 image. Marie animation options are ignored. |
| `G13_LCD_THEME_NONE` | Send no automatic startup image. Marie animation options are ignored. |

The master theme is authoritative: `STATIC` always selects the static image,
`NONE` always selects no startup graphic, and only `MARIE_LATTE` consults the
animation, permanent, repeat and fallback settings below.

Default:

```cpp
#define G13_LCD_THEME G13_LCD_THEME_MARIE_LATTE
```

Change only this selection. Do not edit the numeric definitions of the three
`G13_LCD_THEME_*` constants.

`G13_LCD_THEME_NONE` suppresses the automatic image only. It does not disable
the internal LCD initialization, guarded USB transfer architecture or
RGB/key-backlight setup. The internal HID-only diagnostic mode is a separate
developer setting described in [Internal settings](#internal-settings).

The three themes select existing image data through compile-time guards. They
do not duplicate images or add runtime theme storage.

## Marie/Latte animation options

These settings are used only when
`G13_LCD_THEME` is `G13_LCD_THEME_MARIE_LATTE`:

| Option | Default | Valid values | Meaning |
|---|---:|---:|---|
| `G13_LCD_ANIMATION_ENABLE` | `1` | `0` or `1` | Play the eight story frames |
| `G13_LCD_PERMANENT_FRAME_ENABLE` | `1` | `0` or `1` | Show the permanent M² signature after one-shot playback, or directly when animation is off |
| `G13_LCD_ANIMATION_REPEAT` | `0` | `0` or `1` | Repeat the story after READY instead of entering the permanent state |
| `G13_LCD_STATIC_FALLBACK_ENABLE` | `1` | `0` or `1` | Show the previous static image when animation and permanent frame are both off |

The precedence within the Marie/Latte theme is:

| Animation | Permanent | Repeat | Fallback | Result |
|---:|---:|---:|---:|---|
| 1 | 1 | 0 | any | Default story once, then permanent M² signature |
| 1 | 0 | 0 | any | Story once; READY remains visible |
| 1 | any | 1 | any | Story repeats after READY; no permanent state |
| 0 | 1 | any | any | Permanent M² signature directly |
| 0 | 0 | any | 1 | Previous static image |
| 0 | 0 | any | 0 | No automatic startup image |

The master `STATIC` and `NONE` themes override this entire table.

Examples:

```cpp
// Repeat the Marie/Latte story.
#define G13_LCD_THEME G13_LCD_THEME_MARIE_LATTE
#define G13_LCD_ANIMATION_REPEAT 1
```

```cpp
// Use only the previous static image.
#define G13_LCD_THEME G13_LCD_THEME_STATIC
```

```cpp
// Send no automatic startup graphic.
#define G13_LCD_THEME G13_LCD_THEME_NONE
```

## Animation timing

All timing values are milliseconds and apply to the Marie/Latte theme:

| Option | Default | Valid range | Meaning |
|---|---:|---:|---|
| `G13_LCD_ANIMATION_FRAME_MS` | `700` | `1` through `60000` | Normal confirmed-frame hold, including READY's first hold |
| `G13_LATTE_OVERCLOCK_MS` | `1200` | `1` through `60000` | Longer hold for frame 6, `LATTE OVERCLOCK!` |
| `G13_READY_HOLD_MS` | `2000` | `0` through `60000` | Additional hold after READY's normal time |

Timing begins only after a complete frame transfer has been confirmed. USB and
cooperative-loop scheduling can make the physical display time slightly longer,
but the next frame is not scheduled early.

Very long values make startup appear paused. A zero READY hold is valid and
removes only the additional hold; READY still receives its normal frame time.

## Compile-time validation and common errors

The firmware stops compilation with a named message when a user value is
outside its supported range. Typical messages are:

| Compiler message | Cause and correction |
|---|---|
| `G13_BACKLIGHT_RED must be between 0 and 255` | Set the red channel to `0` through `255`; green and blue have equivalent messages. |
| `G13_LCD_THEME must be G13_LCD_THEME_MARIE_LATTE, G13_LCD_THEME_STATIC or G13_LCD_THEME_NONE` | Use one of the three named theme constants. |
| `G13_LCD_ANIMATION_ENABLE must be 0 or 1` | Use `0` or `1`; the other Boolean settings have equivalent messages. |
| `G13_LCD_STATIC_FALLBACK_ENABLE must be 0 or 1` | Use `0` for no fallback or `1` for the previous static image inside the Marie theme. |
| `G13_LCD_ANIMATION_FRAME_MS must be between 1 and 60000 milliseconds` | Choose a normal frame time in the stated range. |
| `G13_LATTE_OVERCLOCK_MS must be between 1 and 60000 milliseconds` | Choose a LATTE frame time in the stated range. |
| `G13_READY_HOLD_MS must be between 0 and 60000 milliseconds` | Choose an additional READY hold in the stated range. |
| `G13_KEY_G1 must be a supported normal-key Teensy KEY_* constant` | Use a named value from `KEY_A` through `KEY_MENU` or `KEY_F13` through `KEY_F24`, remove a quoted character literal, or restore the default; G2 through G22 have equivalent messages. |
| `G13 LCD theme IDs must remain distinct; do not edit the G13_LCD_THEME_* constants` | Restore the three named theme ID definitions and change only `G13_LCD_THEME`. |

For example, `KEY_SPCAE` is a misspelling of `KEY_SPACE` and is rejected.
Correct the value in `G13UserConfig.h`, save, and run **Verify** again. Returning
to valid values restores a successful build; no generated file needs manual
repair.

## Compile and upload

In Arduino IDE, open the main sketch and select:

| Setting | Value |
|---|---|
| Board | Teensy 4.1 |
| USB Type | Serial + Keyboard + Mouse + Joystick |
| CPU Speed | 450 MHz |
| Optimize | Faster |
| Keyboard Layout | German (Mac) |

Then:

1. Save `G13UserConfig.h`.
2. Click **Verify**.
3. Resolve any compile-time configuration error.
4. Click **Upload**.
5. Press the Teensy program button once if Teensy Loader requests it.
6. Test the changed keys, lighting and display behavior.

Changing `G13UserConfig.h` without recompiling and uploading cannot change the
firmware already stored on the Teensy.

## Restore the defaults

Restore the following values to return to the standard configuration:

```cpp
#define G13_KEY_G1  KEY_1
#define G13_KEY_G2  KEY_2
#define G13_KEY_G3  KEY_3
#define G13_KEY_G4  KEY_4
#define G13_KEY_G5  KEY_5
#define G13_KEY_G6  KEY_6
#define G13_KEY_G7  KEY_7
#define G13_KEY_G8  KEY_K
#define G13_KEY_G9  KEY_L
#define G13_KEY_G10 KEY_A
#define G13_KEY_G11 KEY_W
#define G13_KEY_G12 KEY_D
#define G13_KEY_G13 KEY_M
#define G13_KEY_G14 KEY_N
#define G13_KEY_G15 KEY_O
#define G13_KEY_G16 KEY_P
#define G13_KEY_G17 KEY_S
#define G13_KEY_G18 KEY_Q
#define G13_KEY_G19 KEY_R
#define G13_KEY_G20 KEY_SPACE
#define G13_KEY_G21 KEY_U
#define G13_KEY_G22 KEY_T

#define G13_BACKLIGHT_RED   0
#define G13_BACKLIGHT_GREEN 0
#define G13_BACKLIGHT_BLUE  255

#define G13_LCD_THEME G13_LCD_THEME_MARIE_LATTE
#define G13_LCD_ANIMATION_ENABLE 1
#define G13_LCD_PERMANENT_FRAME_ENABLE 1
#define G13_LCD_ANIMATION_REPEAT 0
#define G13_LCD_STATIC_FALLBACK_ENABLE 1
#define G13_LCD_ANIMATION_FRAME_MS 700
#define G13_LATTE_OVERCLOCK_MS 1200
#define G13_READY_HOLD_MS 2000
```

Alternatively, Git users can restore only the tracked configuration file:

```sh
git restore firmware/g13_marie_v1_0_0/G13UserConfig.h
```

That command discards all uncommitted custom values in this file. Review the
file before compiling and uploading the restored defaults.

## Internal settings

Do not edit these files for normal customization:

- `G13Config.h`
- `g13_marie_v1_0_0.ino`
- `G13Display.cpp` and `G13Display.h`
- `G13StartupAnimation.cpp`, its header and generated frame header
- `HIDDumper.*` or `USBDeviceInfo.*`
- `logo_g13_m2.h`

`G13Config.h` retains internal protocol, transfer, timeout, retry and safety
controls. Its `G13_LCD_ENABLE=0` path is used to compile the advanced HID-only
diagnostic build. It disables LCD initialization, OUT transfers and lighting,
and is not a normal theme setting.

## Validation boundary

For `v1.2.0`, the standard, HID-only, static-theme and no-start-image variants
compiled without warnings. The user-configuration tests, complete Python test
discovery and C++ animation-timeline test also passed.

This verifies compilation, defaults, value rejection and state-machine behavior.
It does not verify the new configuration layer on a physical Logitech G13 and
Teensy 4.1. No `v1.2.0` build was uploaded or physically tested during this
development step. The physical results documented for `v1.1.0` remain the
stability baseline.

See also:

- [Key mapping](keymapping.md)
- [Installation](installation.md)
- [LCD startup animation](startup-animation.md)
- [Troubleshooting](troubleshooting.md)
- [Hardware validation](hardware-validation.md)
