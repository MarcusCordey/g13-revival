# Hardware validation

## v1.2.0 software validation

The following checks completed successfully for `v1.2.0`:

- the standard Marie/Latte build compiled without warnings;
- the internal HID-only build compiled without warnings;
- the static-theme build compiled without warnings;
- the no-start-image build compiled without warnings;
- all four user-configuration host tests passed;
- complete Python test discovery passed 14 of 14 tests; and
- the strict C++ animation-timeline test passed.

The tests cover the unchanged default key mapping, RGB and timing values, all
three master themes, supported user overrides, invalid-value error messages and
the existing animation state machine.

No `v1.2.0` build was uploaded to a Teensy or exercised with a physical Logitech
G13 during this development step. Compile and host-test success must not be
reported as physical hardware validation.

## Confirmed for v1.1.0

The following checks were completed with the standard build on a physical
Logitech G13 connected to a Teensy 4.1:

- the standard firmware build compiled successfully;
- the G13 LCD operated;
- normal G1 through G22 keyboard input operated;
- simultaneous forward/turn and forward/jump combinations operated;
- during a flight over Stormwind in World of Warcraft, a movement key was held
  while the G13 was physically disconnected;
- the character stopped immediately and no keyboard key remained active;
- the complete new LCD startup animation played without an observed error;
- `READY FOR AZEROTH` appeared correctly; and
- the permanent `M² inside | Powered by Marie` frame appeared as intended.

These are observed functional results, not automated hardware-in-loop tests.
The World of Warcraft scenario was used only to observe standard keyboard
behavior; the repository contains no World of Warcraft artwork or logos.

## Still open for v1.2.0

The following `v1.2.0` checks require a physical Logitech G13 and Teensy 4.1:

- upload and operation of the unchanged standard configuration;
- G1 through G22 output using the new centralized mapping and shared-key
  reconciliation;
- custom `KEY_*` mappings, including two G-keys mapped to the same key;
- the default blue RGB value and non-default RGB colors;
- complete Marie/Latte playback, permanent-only, repeat and fallback choices;
- the `STATIC` and `NONE` master themes;
- minimum, maximum and representative custom timing values; and
- disconnect/reconnect behavior with each configuration variant.

The successful physical `v1.1.0` results are the stability baseline, but they do
not automatically validate the new `v1.2.0` configuration layer.

## Additional open hardware cases

The following special cases have not been marked as hardware-validated:

- disconnect during LCD initialization, each individual story frame, the READY
  hold or the permanent-frame transfer;
- rapid or repeated disconnect/reconnect sequences and a complete animation
  restart after each timing variant;
- deliberately forced LCD queue failures, completion errors and timeouts;
- lighting-state invalidation and resend under deliberately varied disconnect
  timing;
- physical operation with more than six mapped G-keys held;
- repeat, permanent-only, static-image, no-start-image and HID-only
  diagnostic variants on the physical device;
- Windows and Linux hosts; and
- alternative hubs, power supplies and cable combinations.

These cases should remain separate from the confirmed standard-path results
until they have been reproduced on the real hardware.
