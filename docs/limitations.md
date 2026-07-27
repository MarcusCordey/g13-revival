# Known Limitations

The following limitations apply to `v1.0.0`:

- The primary tested use case is macOS. Windows and Linux compatibility is
  plausible through standard USB HID behavior but has not been verified for
  this repository snapshot.
- Only G1 through G22 are converted to keyboard events.
- The thumb joystick is ignored.
- M1, M2, M3, MR and the four buttons below the LCD are not mapped.
- Key mappings are fixed in the source; there is no runtime profile editor.
- Several G-key mappings are explicitly test mappings.
- The LCD displays one startup splash frame. There is no application-facing
  dynamic display protocol in this version.
- The RGB report is based on the earlier G13 reference implementation. Its exact
  physical lighting target may vary between global/key and LCD-associated
  illumination.
- LCD support prioritizes HID stability and can disable itself after transfer or
  reconnect errors.
- There are no automated hardware tests. Meaningful USB, LCD and keymapping
  changes require a physical Logitech G13 and Teensy 4.1.
- Serial diagnostics can expose USB descriptor strings and device serial
  numbers at runtime.

