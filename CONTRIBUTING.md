# Contributing

Contributions are welcome after the repository is opened for collaboration.

## Priorities

The physically tested `v1.1.0` firmware remains the stability baseline. Version
`v1.2.0` adds a compile- and host-tested user-configuration layer, but it was
not physically validated during that development step. Changes must preserve
reliable G13-to-keyboard operation. LCD, theme or lighting work must not
compromise the HID input path.

## Before proposing a change

1. Open an issue or describe the intended change before substantial work.
2. Keep functional firmware changes separate from documentation or formatting
   changes.
3. State the Teensy board package, board options and host operating system used.
4. Compile the complete sketch.
5. For user-configuration, keymapping, theme or timing changes, compile the
   standard, HID-only, static-theme and no-start-image variants and run all host
   tests, including representative invalid-value checks.
6. For USB, keymapping, LCD or lighting changes, test with a physical Logitech
   G13 and describe the result. If hardware is unavailable, state that boundary
   explicitly and do not describe compile or host-test results as hardware
   validation.

## Licensing and provenance

- Contributions to original project material must be compatible with the MIT
  License.
- Do not remove existing copyright, license or attribution text.
- Identify copied or adapted code, protocol information, fonts, images and
  other third-party material in the contribution.
- Do not add Logitech, PJRC or other third-party logos unless their use has
  been reviewed and documented.

## Privacy and repository hygiene

- Do not commit passwords, tokens, keys, private paths or personal device data.
- Review serial logs before posting them; connected USB devices may expose
  serial numbers.
- Do not commit compiled firmware, Arduino build directories, IDE caches,
  `.DS_Store` files or local backup archives.
