# Troubleshooting

## The G13 is not detected

- Confirm that the G13 is connected to a device port on the powered hub.
- Confirm that the hub upstream connection leads to the Teensy USB host cable,
  not the Teensy Micro-USB device port.
- Confirm that the hub power supply is connected.
- Inspect the serial monitor at `115200` baud for VID/PID and HID claim
  messages.
- The expected G13 identifiers are vendor `0x046d`, product `0xc21c`.

## The firmware does not upload

- Connect the computer to the Teensy Micro-USB device/client port.
- Select **Teensy 4.1** in Arduino IDE.
- Confirm the documented USB type and board settings.
- Try a known data-capable Micro-USB cable.
- Press the Teensy program button once if Teensy Loader requests it.

## Keys produce unexpected characters

- Select the **German (Mac)** Teensy keyboard layout used by the reference
  build.
- Compare the result with `keymapping.md`.
- Remember that several mappings are intentionally marked as test mappings.

## The LCD stays blank

- Confirm that `G13_LCD_ENABLE` is `1` in `G13Config.h`.
- Look for `[G13] LCD attach`, `LCD init sent`, `LCD frame queued` and
  `LCD online` messages.
- A `LCD no driver` message means no suitable G13 interface with an OUT
  endpoint was attached.
- A `LCD error` or `LCD disabled` message means the safety guard stopped the LCD
  path. Keyboard input may continue.
- Power-cycle the complete setup before repeating an LCD test.

## Keyboard input stalls or disconnects

- Use the powered hub and short, reliable USB cables.
- Check for `[ERROR] G13 stall detected: no HID reports.` in the serial monitor.
- Disconnect optional USB devices from the hub during diagnosis.
- If LCD activity is suspected, temporarily set `G13_LCD_ENABLE` to `0` for a
  diagnostic build. This changes behavior and should not replace the tested
  `v1.0.0` reference without separate validation.

## Serial output is very verbose

The firmware retains PJRC HID diagnostic controls:

- `r` toggles raw HID output.
- `c` toggles changed-data-only mode.
- other input toggles formatted HID output.

Review logs before sharing them because attached USB devices can report serial
numbers.

