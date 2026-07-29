# Hardware validation

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

## Still open

The following special cases have not been marked as hardware-validated:

- disconnect during LCD initialization, each individual story frame, the READY
  hold or the permanent-frame transfer;
- rapid or repeated disconnect/reconnect sequences and a complete animation
  restart after each timing variant;
- deliberately forced LCD queue failures, completion errors and timeouts;
- lighting-state invalidation and resend under deliberately varied disconnect
  timing;
- physical operation with more than six mapped G-keys held;
- repeat, permanent-only, static-fallback, no-start-image and HID-only
  diagnostic variants on the physical device;
- Windows and Linux hosts; and
- alternative hubs, power supplies and cable combinations.

These cases should remain separate from the confirmed standard-path results
until they have been reproduced on the real hardware.
