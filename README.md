# G13 Revival

G13 Revival is an independent Teensy 4.1 firmware project that connects a
Logitech G13 Advanced Gameboard to a modern computer without relying on the
legacy Logitech driver stack.

The Teensy operates simultaneously as:

- a USB host for the physical G13; and
- a USB HID keyboard device toward the computer.

Version `v1.1.0` is based on a firmware state that was successfully compiled,
uploaded to a Teensy 4.1 and tested with a physical Logitech G13.

## Current features

- decoding and keyboard mapping for G1 through G22;
- simultaneous key handling with deterministic six-key rollover and promotion
  of further held G-keys when a slot becomes free;
- LCD initialization, an eight-frame 160 x 48 pixel Marie/Latte-Overclock
  startup story and the permanent `M² inside | Powered by Marie` frame;
- RGB/key-backlight initialization used by the LCD startup path;
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
4. Select the documented Teensy 4.1 board options.
5. Compile and upload the sketch.
6. Connect the G13 through the powered USB hub to the Teensy USB host port.

The historical sketch and folder name `g13_marie_v1_0_0` is intentionally
retained for `v1.1.0`. Arduino requires the primary `.ino` file and its folder to
have matching names; renaming both would create unnecessary path churn and
release risk. The maintained project version is recorded in `info.txt`, this
README and the changelog.

## Build variants

The default build enables the LCD story, permanent frame and existing lighting
path. Compile-time controls in
[`G13Config.h`](firmware/g13_marie_v1_0_0/G13Config.h) also provide an HID-only
diagnostic build, repeat playback, permanent-only output, the previous static
fallback and a build without a startup image. The exact combinations are listed
in [Installation](docs/installation.md) and
[LCD startup animation](docs/startup-animation.md).

## Hardware validation

The `v1.1.0` standard build was exercised on a Teensy 4.1 and physical G13.
G1 through G22, simultaneous forward/turn and forward/jump input, confirmed
disconnect release, the complete Latte-Overclock animation, `READY FOR AZEROTH`
and the permanent signature all worked as intended. During a physical
disconnect while a movement key was held in World of Warcraft, movement stopped
immediately and no key remained active.

This validates the observed keyboard and display behavior, not every possible
USB timing condition. See [Hardware validation](docs/hardware-validation.md) for
the exact test boundary and open special cases.

## Repository structure

```text
firmware/g13_marie_v1_0_0/  Arduino sketch and supporting source files
firmware/SHA256SUMS-v1.0.0.txt  Fingerprints of the published v1.0.0 baseline
firmware/SHA256SUMS-v1.1.0.txt  Fingerprints of the v1.1.0 firmware directory
assets/startup-animation/   Ordered 1-bit source frames and enlarged preview
tools/                      Reproducible asset generation and conversion tools
tests/                      Host-side asset and animation-timeline tests
docs/                       Hardware, installation and operating documentation
LICENSE                     MIT license for the original project material
THIRD_PARTY_NOTICES.md      PJRC, Paul Stoffregen and khampf/ecraven notices
CHANGELOG.md                Version history
CONTRIBUTING.md             Contribution requirements
```

`SHA256SUMS-v1.0.0.txt` remains an unchanged historical record and is not
expected to validate modified files in a `v1.1.0` checkout.
`SHA256SUMS-v1.1.0.txt` uses SHA-256 and lists every regular file directly in
`firmware/g13_marie_v1_0_0/`, including firmware sources, metadata, the generated
native animation header and static fallback image. It was generated after the
successful standard and HID-only release builds and before the release commit.
From the `firmware/` directory, verify the current file with:

```sh
shasum -a 256 -c SHA256SUMS-v1.1.0.txt
```

## Documentation

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
