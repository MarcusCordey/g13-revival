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
- Confirm that `G13_LCD_ANIMATION_ENABLE` or
  `G13_LCD_PERMANENT_FRAME_ENABLE` is `1`. The previous static startup image is
  used only when both are disabled and `G13_LCD_STATIC_FALLBACK_ENABLE` is `1`.
- Look for `[G13] LCD attach`, `LCD init complete`, `Backlight updated` and
  `LCD online` messages. These success messages are emitted after confirmed
  transfer completion.
- A `LCD no driver` message means no suitable G13 interface with an OUT
  endpoint was attached.
- An `init timeout`, `OUT timeout`, `queue error`, `LCD error` or
  `LCD disabled` message means the safety guard stopped the affected transfer
  path. Keyboard input may continue.
- Power-cycle the complete setup before repeating an LCD test.

The default animation plays once after attach or reconnect, holds
`READY FOR AZEROTH` for its normal duration plus two seconds, and then leaves
`M² inside | Powered by Marie` on screen. Timing and repeat behavior are
documented in [`startup-animation.md`](startup-animation.md).

## Keyboard input stalls or disconnects

- Use the powered hub and short, reliable USB cables.
- Check for `[ERROR] G13 stall detected: no HID reports.` in the serial monitor.
- Disconnect optional USB devices from the hub during diagnosis.
- If LCD activity is suspected, temporarily set `G13_LCD_ENABLE` to `0` for a
  diagnostic build. This changes behavior and should not replace the tested
  `v1.1.0` standard configuration without separate validation.

## Serial output is very verbose

Continuous HID report output is disabled by default. The firmware retains PJRC
HID diagnostic controls for temporary investigation:

- `r` toggles raw HID output.
- `c` toggles changed-data-only mode.
- other input toggles formatted HID output.

Review logs before sharing them because attached USB devices can report serial
numbers.
