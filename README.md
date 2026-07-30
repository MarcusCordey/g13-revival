# G13 Revival

G13 Revival is an independent Teensy 4.1 firmware project that connects a
Logitech G13 Advanced Gameboard to a modern computer without relying on the
legacy Logitech driver stack.

The Teensy operates simultaneously as:

- a USB host for the physical G13; and
- a USB HID keyboard device toward the computer.

Version `v1.2.0` adds one documented user-configuration interface while
preserving the `v1.1.0` default behavior. The `v1.2.0` standard, HID-only,
static-theme and no-start-image builds compiled without warnings, and all host
tests passed. This version was not uploaded to a Teensy or tested on a physical
G13 during this development step. The physical-hardware stability baseline
remains `v1.1.0`.

## Current features

- decoding and user-configurable keyboard mapping for G1 through G22;
- simultaneous key handling with deterministic six-key rollover and promotion
  of further held G-keys when a slot becomes free;
- a central
  [`G13UserConfig.h`](firmware/g13_marie_v1_0_0/G13UserConfig.h) for normal
  key, backlight, LCD-theme and animation customization;
- LCD initialization, an eight-frame Marie/Latte-Overclock startup story and
  the permanent `M² inside | Powered by Marie` frame, authored for the G13's
  160 x 43 visible LCD area and converted to its native 160 x 48, 960-byte
  transfer layout;
- user-configurable RGB/key-backlight initialization used by the LCD startup
  path;
- event-based USB and HID diagnostics;
- status LED and HID-report stall monitoring;
- G13-specific HID claiming and disconnect cleanup;
- completion-checked LCD/lighting transfers with guarded failure handling so
  keyboard input remains the priority.

See [Known limitations](docs/limitations.md) for controls and operating systems
that are not yet fully supported.

## Quick start

1. Read the [hardware requirements](docs/hardware.md) and
   [wiring guide](docs/wiring.md).
2. Install the toolchain described in [Installation](docs/installation.md).
3. Open
   `firmware/g13_marie_v1_0_0/g13_marie_v1_0_0.ino`
   in Arduino IDE.
4. Optionally edit only
   `firmware/g13_marie_v1_0_0/G13UserConfig.h` as described in
   [User configuration](docs/user-configuration.md). Leave it unchanged for the
   documented defaults.
5. Select the documented Teensy 4.1 board options.
6. Compile and upload the sketch.
7. Connect the G13 through the powered USB hub to the Teensy USB host port.

The historical sketch and folder name `g13_marie_v1_0_0` is intentionally
retained for `v1.2.0`. Arduino requires the primary `.ino` file and its folder to
have matching names; renaming both would create unnecessary path churn and
release risk. The maintained project version is recorded in `info.txt`, this
README and the changelog.

## User configuration and build variants

Normal customization belongs only in
[`G13UserConfig.h`](firmware/g13_marie_v1_0_0/G13UserConfig.h). Its defaults
preserve the established G1 through G22 mapping, blue RGB value, one-shot
Marie/Latte story and permanent signature. It can also select the previous
static image or no automatic startup graphic.

[`G13Config.h`](firmware/g13_marie_v1_0_0/G13Config.h) contains internal
protocol and safety controls. Its `G13_LCD_ENABLE=0` setting is retained for the
HID-only diagnostic build, not for normal setup. See
[User configuration](docs/user-configuration.md),
[Installation](docs/installation.md) and
[LCD startup animation](docs/startup-animation.md) for the exact options.

## Hardware validation

The `v1.1.0` standard build was exercised on a Teensy 4.1 and physical G13.
G1 through G22, simultaneous forward/turn and forward/jump input, confirmed
disconnect release, the complete Latte-Overclock animation, `READY FOR AZEROTH`
and the permanent signature all worked as intended. During a physical
disconnect while a movement key was held in World of Warcraft, movement stopped
immediately and no key remained active.

The `v1.2.0` configuration layer and its four release build variants were
validated by compilation and host tests only. They were not uploaded or tested
on the physical G13/Teensy setup in this development step. The earlier hardware
results therefore do not constitute physical validation of `v1.2.0` or of
custom key, color, theme or timing values. See
[Hardware validation](docs/hardware-validation.md) for the exact boundary.

## Repository structure

```text
firmware/g13_marie_v1_0_0/  Arduino sketch and supporting source files
firmware/g13_marie_v1_0_0/G13UserConfig.h  Normal user customization
firmware/SHA256SUMS-v1.0.0.txt  Fingerprints of the published v1.0.0 baseline
firmware/SHA256SUMS-v1.1.0.txt  Historical v1.1.0 firmware fingerprints
firmware/SHA256SUMS-v1.2.0.txt  Fingerprints of the v1.2.0 firmware directory
assets/startup-animation/   Ordered 160 x 43 1-bit sources and enlarged preview
tools/                      Reproducible asset generation and conversion tools
tests/                      Host-side configuration, asset and timeline tests
docs/                       Hardware, installation and operating documentation
LICENSE                     MIT license for the original project material
THIRD_PARTY_NOTICES.md      PJRC, Paul Stoffregen and khampf/ecraven notices
CHANGELOG.md                Version history
CONTRIBUTING.md             Contribution requirements
```

`SHA256SUMS-v1.0.0.txt` and `SHA256SUMS-v1.1.0.txt` remain unchanged historical
records and are not expected to validate modified files in a `v1.2.0` checkout.
`SHA256SUMS-v1.2.0.txt` uses SHA-256 and lists every regular file directly in
`firmware/g13_marie_v1_0_0/`, including `G13UserConfig.h`, firmware sources,
metadata, the generated native animation header and previous static image. It
was generated after the successful release builds and host tests. From the
`firmware/` directory, verify the current file with:

```sh
shasum -a 256 -c SHA256SUMS-v1.2.0.txt
```

## Documentation

- [User configuration](docs/user-configuration.md)
- [Installation](docs/installation.md)
- [Required hardware](docs/hardware.md)
- [Wiring](docs/wiring.md)
- [Bill of materials](docs/bom.md)
- [Key mapping](docs/keymapping.md)
- [LCD startup animation](docs/startup-animation.md)
- [Hardware validation](docs/hardware-validation.md)
- [Troubleshooting](docs/troubleshooting.md)
- [Known limitations](docs/limitations.md)
- [Image authorship](docs/images/README.md)

## Authorship

Original project code, documentation, photographs and LCD motif:

Copyright (c) 2026 Marcus Cordey

ChatGPT was used as an assisting tool during development and documentation.
Marcus Cordey is the author and rights holder of the original project material.

The repository also contains and adapts third-party example code and protocol
information. Those parts remain subject to their respective notices. See
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Independence and trademarks

G13 Revival is an independent community project. It is not affiliated with,
endorsed by or sponsored by Logitech, PJRC or OpenAI.

Logitech, Logi and their logos are trademarks or registered trademarks of
Logitech Europe S.A. and/or its affiliates in the United States and other
countries. Teensy is a trademark of PJRC.COM, LLC. All other trademarks are the
property of their respective owners.

No Logitech corporate logo is included in the firmware LCD motif. Product names
are used only to identify compatibility with the Logitech G13 hardware.

## License

The original project material is released under the [MIT License](LICENSE).
Third-party components and adapted material are documented separately in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
