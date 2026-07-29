# Known Limitations

The following limitations apply to `v1.1.0`:

- The primary tested use case is macOS. Windows and Linux compatibility is
  plausible through standard USB HID behavior but has not been verified for
  this repository snapshot.
- Only G1 through G22 are converted to keyboard events.
- The thumb joystick is ignored.
- M1, M2, M3, MR and the four buttons below the LCD are not mapped.
- Key mappings are fixed in the source; there is no runtime profile editor.
- Several G-key mappings are explicitly test mappings.
- The LCD plays a fixed compile-time startup story and then keeps its permanent
  signature frame. There is no application-facing dynamic display protocol in
  this version.
- The animation is designed specifically for the G13's monochrome 160 x 48
  display. It contains no copyrighted World of Warcraft graphics or logos.
- The RGB report is based on the earlier G13 reference implementation. Its exact
  physical lighting target may vary between global/key and LCD-associated
  illumination.
- The selected Teensy USB keyboard profile represents at most six ordinary keys
  simultaneously. Further held G-keys wait for a free slot.
- LCD support prioritizes HID stability and disables its transfer path after a
  queue error or completion timeout. A confirmed detach/reconnect starts a new
  LCD session.
- A missing HID report stream is still diagnostic only. The firmware does not
  treat a stall report as proof of disconnect and does not release keys solely
  because of that timer.
- Host-side tests do not replace hardware-in-loop testing. Individual USB, LCD,
  lighting and reconnect timing cases remain open even though the main
  `v1.1.0` keyboard, disconnect and display paths were validated manually.
- Serial diagnostics can expose USB descriptor strings and device serial
  numbers at runtime. Continuous HID report dumps are disabled by default.

The confirmed hardware results and explicitly open special cases are listed in
[`hardware-validation.md`](hardware-validation.md).
