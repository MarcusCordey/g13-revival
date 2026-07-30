# LCD startup animation

The non-blocking animation and permanent signature were introduced in
`v1.1.0`. Version `v1.2.0` added a central user configuration and master-theme
selection. The current unreleased asset correction preserves the story, frame
order, timing and transfer state machine while re-laying out every image for
the physical LCD's 160 x 43 visible area.

## Story and permanent frame

The startup sequence consists of eight ordered 160 x 43 visible-pixel animation
frames:

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

The 160 x 43 source PNGs are in `assets/startup-animation/frames/`. The
permanent signature uses a `permanent_` filename rather than a `frame_09_`
prefix so that it cannot be mistaken for part of the repeatable story. Each
scene and the permanent signature were individually re-laid out for the 43-row
visible area; the revised sources are not merely bottom-cropped versions of the
previous 48-row canvases.

Every source has a separate, unsmoothed 4x preview in
`assets/startup-animation/preview/frames/`. Each preview is 640 x 172 pixels.
The complete 1984 x 652 pixel 3x3 contact sheet is:

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

## Visible image and native G13 frame format

The physical G13 GamePanel LCD exposes 160 x 43 visible pixels. The USB
protocol nevertheless stores a frame in six complete banks of eight vertical
bit positions, so its native transfer layout is 160 x 48 bits and 960 bytes.
The converter embeds each visible source in that native layout and leaves
`y=43` through `y=47` clear:

```text
visible width            160 pixels
visible height            43 pixels
native storage width     160 columns
native storage height     48 bit rows
pages                       6 groups of 8 vertical bits
native frame size         960 bytes
byte offset              x + (y / 8) * 160
bit within byte          y & 7
set bit                    white/on pixel
padding                  y=43 through y=47, all clear
```

The LCD USB payload is unchanged: it adds the existing 32-byte header in front
of the 960-byte native frame for a total of 992 bytes. Header byte 0 is `0x03`;
the other header bytes remain zero. The visible-geometry correction therefore
does not change the USB transfer size, framing, timing or state machine.

`tools/png_to_g13.py` validates PNG chunks, CRCs, dimensions and modes. Strict
mode accepts only opaque pure black and white; non-strict mode can
deterministically threshold supported grayscale, indexed, RGB and alpha PNGs.
The normal source size is 160 x 43. A 160 x 48 compatibility source is accepted
only when all five non-visible padding rows are clear. The tool can emit C or
C++ arrays:

```sh
python3 tools/png_to_g13.py FRAME.png --strict-monochrome --language cpp
python3 tools/png_to_g13.py FRAME.png --strict-monochrome --language c
```

The complete nine-file conversion command is in
`assets/startup-animation/README.md`. Each generated array still contains
exactly 960 bytes even though its source has only 43 visible rows. On Teensy
4.x the immutable arrays and their pointer table use the core's `PROGMEM`
section in memory-mapped flash.

## Timing and transfer behavior

`G13StartupAnimation` owns only a small timeline; it does not own image buffers.
The existing 992-byte USB transfer buffer consists of its 32-byte header
followed by one selected 960-byte native flash frame. There is no runtime image
conversion, allocation or additional full-frame RAM cache.

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

## Themes and compile-time options

Normal presentation options are in
`firmware/g13_marie_v1_0_0/G13UserConfig.h`. The master theme is selected with
`G13_LCD_THEME`:

| Theme | Meaning |
|---|---|
| `G13_LCD_THEME_MARIE_LATTE` | Default eight-frame story and Marie-specific options |
| `G13_LCD_THEME_STATIC` | Previous static G13 image; Marie options are ignored |
| `G13_LCD_THEME_NONE` | No automatic startup graphic; Marie options are ignored |

Change only the `G13_LCD_THEME` selection. The three named theme ID constants
are implementation identifiers and must not be edited.

Within `G13_LCD_THEME_MARIE_LATTE`, these options apply:

| Option | Default | Meaning |
|---|---:|---|
| `G13_LCD_ANIMATION_ENABLE` | `1` | Play the eight story frames |
| `G13_LCD_PERMANENT_FRAME_ENABLE` | `1` | Send the signature after one-shot playback |
| `G13_LCD_ANIMATION_FRAME_MS` | `700` | Normal confirmed-frame hold |
| `G13_LATTE_OVERCLOCK_MS` | `1200` | Confirmed hold for frame 6 |
| `G13_READY_HOLD_MS` | `2000` | Additional hold after READY's normal time |
| `G13_LCD_ANIMATION_REPEAT` | `0` | Repeat frames 1–8 instead of entering the permanent state |
| `G13_LCD_STATIC_FALLBACK_ENABLE` | `1` | Use the previous static image when animation and permanent frame are both disabled |

With animation disabled and the permanent frame enabled, the signature is sent
directly once after the established initialization sequence. Repeat mode loops
frames 1–8 after both READY hold phases and therefore intentionally does not
enter the permanent state. With animation and permanent frame disabled, the
fallback value selects the previous static image (`1`) or no startup image
(`0`).

The master `STATIC` and `NONE` themes override all Marie-specific choices.

`G13_LCD_THEME_NONE` still permits the guarded LCD initialization and
RGB/key-backlight path. The internal `G13_LCD_ENABLE=0` setting in
`G13Config.h` removes the complete LCD and lighting path for HID-only diagnosis.
It is not a normal user theme.

All Boolean values accept only `0` or `1`. Normal and LATTE timing values accept
`1` through `60000` milliseconds; the additional READY hold accepts `0` through
`60000`. The complete precedence table and examples are in
[`user-configuration.md`](user-configuration.md).

## Storage impact

Eight animation frames plus one permanent frame require `9 x 960 = 8640` bytes
of immutable native image data. This is only 1920 bytes more than the preceding
seven-frame implementation, so compression would add complexity without a
useful Teensy 4.1 resource benefit.

The previous static image remains a 960-byte native asset, but it is excluded
from the standard animation build by compile-time guards. The existing runtime
buffers remain one 960-byte native transfer framebuffer and one 992-byte USB
payload buffer.

Using the build environment documented in `installation.md`:

| Reported region | Seven-frame pre-release build | v1.1.0 standard build | Difference |
|---|---:|---:|---:|
| Total FLASH (`code + data + headers`) | 101,372 bytes | 103,420 bytes | +2,048 bytes |
| RAM1 variables | 60,192 bytes | 60,224 bytes | +32 bytes |
| RAM2 variables | 12,864 bytes | 12,864 bytes | 0 bytes |

The flash increase includes two additional native images, the larger pointer
table and the extended timeline logic. The images are not duplicated in RAM.

## Host-side validation

Run all Python asset and user-configuration tests plus the deterministic
timeline test from the repository root:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 -m unittest discover -s tests -p 'test_*.py' -v
c++ -std=c++17 -Wall -Wextra -Werror \
  tests/test_g13_animation_timeline.cpp \
  -o /tmp/test_g13_animation_timeline
/tmp/test_g13_animation_timeline
```

The recorded `v1.2.0` validation passed 14 of 14 Python tests, including all
four user-configuration tests. For the unreleased geometry correction, the
asset tests cover 160 x 43 sources, zero padding in each 960-byte native array,
the explicit frame order, native pixel mapping, exact 640 x 172 previews, the
1984 x 652 contact sheet and deterministic regeneration. The strict C++ test
covers normal/special/READY timing, transfer exclusion, one-time
permanent-frame scheduling, repeat behavior, reset/reconnect behavior and
`millis()` wraparound. These host-side checks do not constitute physical
hardware validation.

## Hardware validation

The `v1.1.0` standard animation completed successfully on a physical Logitech
G13 and Teensy 4.1. The LCD operated, `READY FOR AZEROTH` appeared correctly and
the permanent `M² inside | Powered by Marie` signature followed as intended.
Normal G-key input and the tested simultaneous-key combinations also remained
functional in that validated standard configuration.

Compile and host tests cannot cover every physical USB timing path. Disconnect
during initialization or individual animation phases, rapid reconnects, forced
transfer failures, lighting resend timing and the alternative build modes
remain open. The canonical list of confirmed results and untested special cases
is maintained in [`hardware-validation.md`](hardware-validation.md).

No `v1.2.0` build was uploaded to the Teensy or tested with the physical G13 in
this development step. The unchanged default timeline, `STATIC` and `NONE`
themes, custom timing and the new configuration path therefore still require
physical validation. The unreleased 160 x 43 source re-layout and regenerated
native frames likewise have not been uploaded to a Teensy or tested on a
physical G13.
