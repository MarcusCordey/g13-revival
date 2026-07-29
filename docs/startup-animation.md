# LCD startup animation

The non-blocking animation and permanent signature were introduced in
`v1.1.0`.

## Story and permanent frame

The startup sequence consists of eight ordered 160 x 48 pixel animation frames:

1. `MARIE INSIDE` opens the technical boot sequence.
2. Marie appears as a friendly microchip waking up with large pixel eyes.
3. A steaming Latte Macchiato cup moves in from the right while Marie looks at it.
4. A thick energy line connects the cup to Marie and the eyes become much larger.
5. Bold circuit paths and four large lightning shapes show humorous overclocking.
6. `LATTE OVERCLOCK!` forms the visual and timing climax.
7. Marie returns to friendly eyes and symmetrical, orderly circuit paths.
8. `READY FOR AZEROTH` presents the calm operational state.

After READY's normal duration and an additional hold, the default one-shot mode
sends a separate permanent signature exactly once. Its left side is an original
clipped-corner `M² inside` technology emblem; the right side reads `Powered by
Marie`. It does not reproduce an Intel logo, World of Warcraft artwork or any
other third-party logo.

The source PNGs are in `assets/startup-animation/frames/`. The permanent
signature uses a `permanent_` filename rather than a `frame_09_` prefix so that
it cannot be mistaken for part of the repeatable story.

Every source has a separate, unsmoothed 4x preview in
`assets/startup-animation/preview/frames/`. The complete 3x3 contact sheet is:

```text
assets/startup-animation/preview/startup_animation_contact_sheet_4x.png
```

All source and preview graphics are true grayscale-color-type 0, bit-depth 1
PNGs. The animation generator uses a fixed bitmap font, integer line primitives
and integer scaling:

```sh
python3 tools/generate_startup_animation.py
```

It uses only the Python standard library and writes no antialiasing, grayscale
or dithered pixels.

## Native G13 frame format

The converter follows the byte order already used by the working LCD pixel
routine:

```text
width              160 pixels
height              48 pixels
pages                6 groups of 8 vertical pixels
frame size         960 bytes
byte offset        x + (y / 8) * 160
bit within byte    y & 7
set bit              white/on pixel
```

The LCD USB payload adds the existing 32-byte header in front of these 960
bytes. Header byte 0 is `0x03`; the other header bytes remain zero.

`tools/png_to_g13.py` validates PNG chunks, CRCs, dimensions and modes. Strict
mode accepts only opaque pure black and white; non-strict mode can
deterministically threshold supported grayscale, indexed, RGB and alpha PNGs.
It can emit C or C++ arrays:

```sh
python3 tools/png_to_g13.py FRAME.png --strict-monochrome --language cpp
python3 tools/png_to_g13.py FRAME.png --strict-monochrome --language c
```

The complete nine-file conversion command is in
`assets/startup-animation/README.md`. Each generated array contains exactly 960
bytes. On Teensy 4.x the immutable arrays and their pointer table use the core's
`PROGMEM` section in memory-mapped flash.

## Timing and transfer behavior

`G13StartupAnimation` owns only a small timeline; it does not own image buffers.
The existing 992-byte USB transfer buffer receives one selected flash frame
when that transfer begins. There is no runtime image conversion, allocation or
additional full-frame RAM cache.

Visibility timing starts only after all chunks of the preceding LCD frame have
completed successfully:

| Frame | Default hold after confirmed transfer |
|---|---:|
| 1–5 | 700 ms |
| 6 `LATTE OVERCLOCK!` | 1200 ms |
| 7 | 700 ms |
| 8 `READY FOR AZEROTH` | 700 ms normal, then 2000 ms additional |

The additional READY phase is logically a full 2000 ms. Cooperative loop and
USB scheduling can make the physical display time a few milliseconds longer,
but the permanent frame is never scheduled early.

The permanent frame becomes `FINISHED` only after its own complete transfer is
confirmed. Polling in `FINISHED` produces no further frame request. Existing
completion callbacks, timeouts, bounded retries and backoff remain responsible
for USB traffic, and no new frame starts while an LCD transfer is active.

Detach clears pending animation work and resets the timeline through the
existing LCD detach path. Attach or reconnect therefore starts again with frame
1. A completion event from the previous USB connection is ignored by the
existing connection-generation check and cannot advance the reset timeline.

## Compile-time options

The options are in `firmware/g13_marie_v1_0_0/G13Config.h`:

| Option | Default | Meaning |
|---|---:|---|
| `G13_LCD_ANIMATION_ENABLE` | `1` | Play the eight story frames |
| `G13_LCD_PERMANENT_FRAME_ENABLE` | `1` | Send the signature after one-shot playback |
| `G13_LCD_ANIMATION_FRAME_MS` | `700` | Normal confirmed-frame hold |
| `G13_LATTE_OVERCLOCK_MS` | `1200` | Confirmed hold for frame 6 |
| `G13_READY_HOLD_MS` | `2000` | Additional hold after READY's normal time |
| `G13_LCD_ANIMATION_REPEAT` | `0` | Repeat frames 1–8 instead of entering the permanent state |
| `G13_LCD_STATIC_FALLBACK_ENABLE` | `1` | Use the former static logo when animation and permanent frame are disabled |

With animation disabled and the permanent frame enabled, the signature is sent
directly once after the established initialization sequence. Repeat mode loops
frames 1–8 after both READY hold phases and therefore intentionally does not
enter the permanent state. With animation and permanent frame disabled, the
old static fallback is used if enabled. `G13_LCD_ENABLE=0` still removes the
complete LCD and lighting path for HID-only diagnosis.

## Storage impact

Eight animation frames plus one permanent frame require `9 x 960 = 8640` bytes
of immutable native image data. This is only 1920 bytes more than the preceding
seven-frame implementation, so compression would add complexity without a
useful Teensy 4.1 resource benefit.

The previous static fallback remains a 960-byte source asset, but it is excluded
from the standard animation build by compile-time guards. The existing runtime
buffers remain one 960-byte logical framebuffer and one 992-byte USB payload
buffer.

Using the build environment documented in `installation.md`:

| Reported region | Seven-frame pre-release build | v1.1.0 standard build | Difference |
|---|---:|---:|---:|
| Total FLASH (`code + data + headers`) | 101,372 bytes | 103,420 bytes | +2,048 bytes |
| RAM1 variables | 60,192 bytes | 60,224 bytes | +32 bytes |
| RAM2 variables | 12,864 bytes | 12,864 bytes | 0 bytes |

The flash increase includes two additional native images, the larger pointer
table and the extended timeline logic. The images are not duplicated in RAM.

## Host-side validation

Run the deterministic asset and timeline tests from the repository root:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest -v tests/test_animation_assets.py
c++ -std=c++17 -Wall -Wextra -Werror \
  tests/test_g13_animation_timeline.cpp \
  -o /tmp/test_g13_animation_timeline
/tmp/test_g13_animation_timeline
```

The tests verify all nine source modes and sizes, each 960-byte native array,
the explicit frame order, native pixel mapping, exact 4x previews,
deterministic regeneration, normal/special/READY timing, transfer exclusion,
one-time permanent-frame scheduling, repeat behavior, reset/reconnect behavior
and `millis()` wraparound.

## Hardware validation

The standard animation completed successfully on a physical Logitech G13 and
Teensy 4.1. The LCD operated, `READY FOR AZEROTH` appeared correctly and the
permanent `M² inside | Powered by Marie` signature followed as intended. Normal
G-key input and the tested simultaneous-key combinations also remained
functional in the validated standard configuration.

Compile and host tests cannot cover every physical USB timing path. Disconnect
during initialization or individual animation phases, rapid reconnects, forced
transfer failures, lighting resend timing and the alternative build modes
remain open. The canonical list of confirmed results and untested special cases
is maintained in [`hardware-validation.md`](hardware-validation.md).
