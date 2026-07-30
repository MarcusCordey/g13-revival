# G13 startup-animation assets

The eight animation frames and separate permanent signature in `frames/` are
exact 160 x 43 pixel, monochrome, 1-bit PNG files matching the physical G13
LCD's visible area. The numeric prefixes define animation playback order; the
`permanent_` file is deliberately not a ninth animation frame.

Every source was individually re-laid out for the 43-row canvas. Borders, text
and scene details were repositioned rather than simply cropping five rows from
the previous 160 x 48 source images.

`preview/startup_animation_contact_sheet_4x.png` shows all frames at integer
4x scale without smoothing. The 3x3 sheet is 1984 x 652 pixels; it is a review
artifact and is not compiled into the firmware. `preview/frames/` contains one
separate 640 x 172 pixel preview for every source graphic.

Regenerate the PNG sources and contact sheet from the repository root:

```sh
python3 tools/generate_startup_animation.py
```

Regenerate the native firmware header:

```sh
python3 tools/png_to_g13.py \
  assets/startup-animation/frames/frame_01_marie_inside.png \
  assets/startup-animation/frames/frame_02_marie_wakes.png \
  assets/startup-animation/frames/frame_03_latte_arrives.png \
  assets/startup-animation/frames/frame_04_marie_drinks.png \
  assets/startup-animation/frames/frame_05_overclock_chaos.png \
  assets/startup-animation/frames/frame_06_latte_overclock.png \
  assets/startup-animation/frames/frame_07_marie_stable.png \
  assets/startup-animation/frames/frame_08_ready_for_azeroth.png \
  assets/startup-animation/frames/permanent_m2_inside_powered_by_marie.png \
  --strict-monochrome --language cpp --array-prefix g13_startup_animation \
  --output firmware/g13_marie_v1_0_0/G13StartupAnimationFrames.h
```

The converter embeds each 160 x 43 visible source in the G13's native 160 x 48
storage layout. Its six 8-row vertical banks occupy 960 bytes; the five
non-visible bit rows `y=43` through `y=47` remain clear. LCD transmission is
unchanged: the firmware adds the existing 32-byte header to each 960-byte native
frame, producing the same 992-byte USB payload.

Both scripts use only the Python standard library. See
[`docs/startup-animation.md`](../../docs/startup-animation.md) for the native
byte layout and asset validation procedure, and
[`docs/user-configuration.md`](../../docs/user-configuration.md) for theme,
playback and timing settings.

The regenerated sources, previews and native arrays have host-side coverage but
have not been uploaded to a Teensy or validated on a physical G13.
