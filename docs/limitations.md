# Known Limitations

The following limitations apply to `v1.2.0`:

- The primary tested use case is macOS. Windows and Linux compatibility is
  plausible through standard USB HID behavior but has not been verified for
  this repository snapshot.
- Only G1 through G22 are converted to keyboard events.
- The thumb joystick is ignored.
- M1, M2, M3, MR and the four buttons below the LCD are not mapped.
- G1 through G22 mappings are compile-time settings in `G13UserConfig.h`; there
  is no runtime profile editor, EEPROM profile or desktop configuration
  application.
- The public mapping accepts supported normal-key Teensy `KEY_*` constants.
  Modifier-only, media, system and quoted character values are not supported.
- Several G-key mappings are explicitly test mappings.
- The LCD master theme, Marie/Latte behavior and timing are compile-time
  settings. There is no runtime theme switching or application-facing dynamic
  display protocol.
- The animation is designed specifically for the G13's monochrome 160 x 48
  display. It contains no copyrighted World of Warcraft graphics or logos.
- RGB values are user-configurable, but the report is based on the earlier G13
  reference implementation. Its exact physical lighting target may vary between
  global/key and LCD-associated illumination.
- The selected Teensy USB keyboard profile represents at most six ordinary keys
  simultaneously. Further held G-keys wait for a free slot.
- LCD support prioritizes HID stability and disables its transfer path after a
  queue error or completion timeout. A confirmed detach/reconnect starts a new
  LCD session.
- A missing HID report stream is still diagnostic only. The firmware does not
  treat a stall report as proof of disconnect and does not release keys solely
  because of that timer.
- Host-side tests do not replace hardware-in-loop testing. The main `v1.1.0`
  keyboard, disconnect and display paths were validated manually, but no
  `v1.2.0` build or custom configuration was uploaded or physically tested in
  this development step.
- Serial diagnostics can expose USB descriptor strings and device serial
  numbers at runtime. Continuous HID report dumps are disabled by default.

The confirmed hardware results and explicitly open special cases are listed in
[`hardware-validation.md`](hardware-validation.md).
Normal compile-time customization is documented in
[`user-configuration.md`](user-configuration.md).
