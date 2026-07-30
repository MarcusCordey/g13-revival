# Third-Party Notices

This project contains third-party example code and uses protocol information
from earlier open-source Logitech G13 projects. The project-level MIT license
does not replace or remove the notices described below.

## PJRC USBHost_t36 and Paul Stoffregen

Upstream project:

- Repository: <https://github.com/PaulStoffregen/USBHost_t36>
- Example: `examples/HIDDeviceInfo/`
- Author and maintainer: Paul Stoffregen / PJRC
- Local dependency used for the verified build: Teensy board package `1.61.0`,
  `USBHost_t36` library metadata version `0.2`
- Upstream comparison reference used during the repository audit:
  commit `0463fc1f6591c0d303698d7454f6a130cf84dd67`

### Files derived from the PJRC example

- `firmware/g13_marie_v1_0_0/HIDDumper.cpp`
  - Derived from `examples/HIDDeviceInfo/HIDDumper.cpp`.
  - Retains the 2017 Paul Stoffregen copyright and MIT license text.
  - Adds project-specific, event-based Logitech G13 HID claim diagnostics.

- `firmware/g13_marie_v1_0_0/HIDDumper.h`
  - Derived from `examples/HIDDeviceInfo/HIDDumper.h`.
  - Includes the Paul Stoffregen MIT notice retained for this project copy.

- `firmware/g13_marie_v1_0_0/USBDeviceInfo.cpp`
- `firmware/g13_marie_v1_0_0/USBDeviceInfo.h`
  - Copied from the PJRC `HIDDeviceInfo` example.
  - These files contain their MIT-style permission and warranty text.
  - The audited project copies matched the corresponding files in both the
    installed Teensy `1.61.0` package and the audited upstream reference.

- `firmware/g13_marie_v1_0_0/g13_marie_v1_0_0.ino`
  - Adapts the USB host object registry, diagnostic-controller arrangement,
    serial output controls and connect/disconnect reporting structure from
    `examples/HIDDeviceInfo/HIDDeviceInfo.ino`.
  - The upstream example identifies the example sketch as public-domain
    material.
  - The G13 report decoding, keyboard mapping, monitoring and LCD integration
    are project-specific additions.

The original notices embedded in the source files must be retained in copies
and substantial portions of those files.

## khampf/g13 and ecraven/g13

Reference projects:

- khampf/g13: <https://github.com/khampf/g13>
- ecraven/g13: <https://github.com/ecraven/g13>
- `khampf/g13` is a fork and later refactoring of `ecraven/g13`.
- Current khampf comparison reference used during the repository audit:
  commit `acea10b7ae7b673a0b41545c71084c9b9b63d1d7`

The G13 LCD and lighting implementation in
`firmware/g13_marie_v1_0_0/G13Display.cpp` uses protocol information verified
against the following khampf/g13 files:

- `g13.hpp`
  - Logitech vendor ID `0x046d`
  - Logitech G13 product ID `0xc21c`
  - key and LCD endpoint constants

- `g13_lcd.cpp` and `g13_lcd.hpp`
  - LCD initialization control request
  - 160 x 43 visible pixels in a 160 x 48 monochrome native transfer framebuffer
  - 960-byte framebuffer size
  - 32-byte transfer header
  - header byte `0x03`
  - LCD output-transfer shape

- `g13_device.cpp`
  - RGB/key-backlight control report and payload shape

The Arduino implementation does not vendor or reproduce the complete
Linux/libusb implementation. It adapts the protocol values to a small
Teensy/USBHost_t36 state machine.

The khampf/g13 README states that files without an individual copyright notice
are placed in the public domain and that files carrying separate MIT-style
notices remain subject to those notices. The protocol-reference files listed
above do not contain separate copyright or license headers in the audited
upstream version.

The historical local reference was previously described only as
`g13-master`. Its exact historical commit was not preserved. The two repository
links and audited comparison commit above are therefore recorded explicitly for
traceability; they do not claim that the audited commit was the exact original
download.

## Arduino and Teensy dependencies

The firmware depends on the Arduino/Teensy core, `Keyboard` and `USBHost_t36`.
These dependencies are installed separately and are not vendored in this
repository. Their own licenses and notices are distributed with the Teensy
board package.

## Trademarks

Logitech and Logitech product names are used only for compatibility and
technical identification. G13 Revival is not affiliated with, endorsed by or
sponsored by Logitech, PJRC or the third-party projects named above.
