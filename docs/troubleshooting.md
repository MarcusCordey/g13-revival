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

## The firmware does not compile after customization

- Edit only `G13UserConfig.h` for normal configuration.
- Read the first named configuration error in the compiler output; later errors
  may be consequences of the first value.
- RGB channels must be `0` through `255`.
- Boolean animation, permanent, repeat and fallback options must be `0` or `1`.
- Use one of `G13_LCD_THEME_MARIE_LATTE`, `G13_LCD_THEME_STATIC` or
  `G13_LCD_THEME_NONE`.
- Do not edit the numeric definitions of the three `G13_LCD_THEME_*` constants.
- Normal and LATTE times must be `1` through `60000` milliseconds. The
  additional READY hold must be `0` through `60000`.
- Use a supported Teensy `KEY_*` constant for every G-key. A misspelling such as
  `KEY_SPCAE` produces the corresponding
  `G13_KEY_Gn must be a supported normal-key Teensy KEY_* constant` error.
- Restore the documented defaults, save, and click **Verify** again.

The exact messages, valid ranges and complete default block are in
[`user-configuration.md`](user-configuration.md).

## Keys produce unexpected characters

- Check `G13_KEY_G1` through `G13_KEY_G22` in `G13UserConfig.h`.
- Use supported Teensy `KEY_*` constants, not quoted ASCII character literals.
- Keep the **German (Mac)** Teensy setting used by the reference build, and
  check the host operating system's active layout. `KEY_*` values are direct
  USB-HID positions; the host layout interprets positions such as Y and Z.
- Compare the result with [`keymapping.md`](keymapping.md).
- Remember that several mappings are intentionally marked as test mappings.
- Compile and upload again after every mapping change.

## The LCD stays blank

- Check `G13_LCD_THEME` in `G13UserConfig.h`.
  `G13_LCD_THEME_NONE` intentionally sends no startup graphic.
- `G13_LCD_THEME_STATIC` sends the previous static image and ignores the
  Marie-specific animation choices.
- With `G13_LCD_THEME_MARIE_LATTE`, confirm that animation, permanent frame or
  both are enabled if an automatic story/signature is expected. If both are
  `0`, `G13_LCD_STATIC_FALLBACK_ENABLE=1` sends the previous static image and
  `0` sends no image.
- If an internal HID-only diagnostic build was intentionally compiled, set
  `G13_LCD_ENABLE` back to `1` in `G13Config.h` and rebuild. This is an advanced
  diagnostic control, not a normal user theme.
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

## The backlight color is unexpected

- Check all three `G13_BACKLIGHT_RED`, `G13_BACKLIGHT_GREEN` and
  `G13_BACKLIGHT_BLUE` values in `G13UserConfig.h`.
- Each value must be `0` through `255`; the default is blue `(0, 0, 255)`.
- Compile and upload after changing a color.
- The G13 protocol may apply this report to key/global lighting rather than the
  monochrome LCD itself. Non-default colors were not physically validated for
  `v1.2.0`.

## Keyboard input stalls or disconnects

- Use the powered hub and short, reliable USB cables.
- Check for `[ERROR] G13 stall detected: no HID reports.` in the serial monitor.
- Disconnect optional USB devices from the hub during diagnosis.
- If LCD activity is suspected, temporarily set `G13_LCD_ENABLE` to `0` for a
  diagnostic build. This changes behavior and should not replace the tested
  `v1.1.0` physical-hardware baseline without separate validation.

## Serial output is very verbose

Continuous HID report output is disabled by default. The firmware retains PJRC
HID diagnostic controls for temporary investigation:

- `r` toggles raw HID output.
- `c` toggles changed-data-only mode.
- other input toggles formatted HID output.

Review logs before sharing them because attached USB devices can report serial
numbers.
