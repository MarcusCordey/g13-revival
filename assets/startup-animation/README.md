# G13 startup-animation assets

The eight animation frames and separate permanent signature in `frames/` are
exact 160 x 48 pixel, monochrome, 1-bit PNG files. The numeric prefixes define
animation playback order; the `permanent_` file is deliberately not a ninth
animation frame.

`preview/startup_animation_contact_sheet_4x.png` shows all frames at integer
4x scale without smoothing. It is a review artifact and is not compiled into
the firmware. `preview/frames/` contains one separate 640 x 192 pixel preview
for every source graphic.

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

Both scripts use only the Python standard library. See
[`docs/startup-animation.md`](../../docs/startup-animation.md) for the native
byte layout, configuration and validation procedure.
