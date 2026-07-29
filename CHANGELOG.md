# Changelog

All notable project changes will be documented in this file.

## [Unreleased]

## [1.1.0] - 2026-07-29

### Added

- Added eight deterministic 160 x 48 pixel, 1-bit story frames, a separate
  permanent signature frame, individual 4x previews and a contact sheet.
- Added a standard-library PNG converter for the native 960-byte G13 frame
  layout and host-side conversion/timeline tests.
- Added a non-blocking, completion-gated, `millis()`-driven startup story in
  which Marie wakes, receives a Latte overclock, stabilizes and becomes ready
  for Azeroth.
- Added the permanent `M² inside | Powered by Marie` closing frame.
- Added compile-time controls for animation, frame timing, repeat mode,
  permanent frame and the previous static-logo fallback.

### Changed

- LCD attach, detach and transfer-completion events are now handed off safely
  from USBHost_t36 callbacks to the cooperative main-loop service.
- LCD initialization, image output and lighting control requests are serialized.
- LCD frame offsets and output caches advance only after confirmed transfer
  completion.
- Restricted the productive keyboard bridge to Logitech G13 VID `0x046d`, PID
  `0xc21c` and interface `0`.
- Preserved the six-key USB keyboard limit while allowing another held G-key to
  enter a slot as soon as one becomes free.
- Disabled continuous raw/formatted HID report dumps by default to avoid normal
  serial backpressure.
- Updated documentation and build data for the `v1.1.0` release.

### Fixed

- Confirmed G13 disconnects now release all Teensy keyboard state and clear the
  complete represented and pending G-key caches.
- Additional held G-keys now move into the six available keyboard slots after a
  represented key is released.
- Device output caches are invalidated on disconnect, pending lighting work is
  cancelled and the desired lighting value is sent again after reconnect.
- Removed races between USBHost_t36 callbacks and the main LCD state machine.
- Hardened HID report and USB descriptor parsing against truncated or malformed
  input.
- Queue acceptance is no longer treated as completed USB transmission.
- Queue failures now retry through bounded backoff, while missing completions
  time out safely instead of entering an unlimited fast retry loop.

### Known limitations

- Individual USB, LCD, lighting and reconnect edge cases still require further
  physical-hardware testing.
- Keyboard output continues to use the existing six-key rollover profile.
- A diagnostic HID stall is deliberately not treated as proof of disconnect and
  does not release keys automatically.
- The animation is specifically optimized for the monochrome 160 x 48 pixel G13
  display.
- No copyrighted World of Warcraft graphics or logos are included.

### Hardware validation

- The standard build, LCD, G1 through G22, simultaneous movement combinations,
  confirmed-disconnect key release, complete startup animation, `READY FOR
  AZEROTH` frame and permanent signature were tested successfully on a physical
  Logitech G13 and Teensy 4.1.
- Detailed results and still-open special cases are recorded in
  [`docs/hardware-validation.md`](docs/hardware-validation.md).

## [1.0.0]

Tested reference version prepared for repository publication.

### Included

- Teensy 4.1 USB host/device bridge for the Logitech G13.
- G1 through G22 report decoding and keyboard mapping.
- Simultaneous key state tracking.
- LCD initialization and custom splash-screen transfer.
- Blue RGB/key-backlight request used during LCD startup.
- Event-based USB/HID diagnostics.
- Status LED and HID-report stall monitoring.
- LCD failure guards that preserve the keyboard-input path.

### Documentation

- Added MIT project license.
- Added detailed PJRC, Paul Stoffregen, khampf and ecraven notices.
- Added installation, hardware, wiring, bill-of-materials, keymapping,
  troubleshooting and limitation documentation.

The historical `v1.0.0` firmware fingerprints remain preserved in
`firmware/SHA256SUMS-v1.0.0.txt`.
