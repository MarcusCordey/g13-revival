# G13 Revival

G13 Revival is an independent Teensy 4.1 firmware project that connects a
Logitech G13 Advanced Gameboard to a modern computer without relying on the
legacy Logitech driver stack.

The Teensy operates simultaneously as:

- a USB host for the physical G13; and
- a USB HID keyboard device toward the computer.

Version `v1.0.0` is based on a firmware state that was successfully compiled,
uploaded to a Teensy 4.1 and tested with the real hardware.

## Current features

- decoding and keyboard mapping for G1 through G22;
- simultaneous key handling;
- LCD initialization and a custom 160 x 48 pixel splash screen;
- RGB/key-backlight initialization used by the LCD startup path;
- event-based USB and HID diagnostics;
- status LED and HID-report stall monitoring;
- guarded LCD failure handling so keyboard input remains the priority.

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

## Repository structure

```text
firmware/g13_marie_v1_0_0/  Arduino sketch and supporting source files
firmware/SHA256SUMS-v1.0.0.txt  Fingerprints of the verified firmware copy
docs/                       Hardware, installation and operating documentation
LICENSE                     MIT license for the original project material
THIRD_PARTY_NOTICES.md      PJRC, Paul Stoffregen and khampf/ecraven notices
CHANGELOG.md                Version history
CONTRIBUTING.md             Contribution requirements
```

## Documentation

- [Installation](docs/installation.md)
- [Required hardware](docs/hardware.md)
- [Wiring](docs/wiring.md)
- [Bill of materials](docs/bom.md)
- [Key mapping](docs/keymapping.md)
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
