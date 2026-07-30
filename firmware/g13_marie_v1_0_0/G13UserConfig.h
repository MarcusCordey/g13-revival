/*
==============================================================================
Teensy 4.1 Logitech G13 Adapter Project

by Marcus Cordey

Feel free to use, redistribute and/or edit.

This project contains reverse-engineered Logitech G13 protocol handling,
USB communication, key mapping and LCD support for Teensy 4.1.

The code is intentionally documented to help other developers understand
the protocol and implementation details.

No warranty is provided. Use at your own risk.
==============================================================================
*/

#pragma once

// ============================================================
// G13 USER CONFIGURATION
// Edit this file for normal user customization.
// Do not modify the firmware source files for normal setup.
// ============================================================

// -----------------------------------------------------------------------------
// G1 through G22 key mapping
//
// Use one normal-key Teensy KEY_* constant per key, for example KEY_W,
// KEY_SPACE or KEY_F1. These 16-bit constants identify USB HID keys directly.
// The computer's active keyboard layout determines the character produced by a
// physical key position. Character literals, modifier-only, media and system
// codes are intentionally not supported by this mapping interface.
//
// The defaults below reproduce the v1.1.0 mapping exactly.
// -----------------------------------------------------------------------------
#ifndef G13_KEY_G1
#define G13_KEY_G1  KEY_1     // G1 default: number 1
#endif
#ifndef G13_KEY_G2
#define G13_KEY_G2  KEY_2     // G2 default: number 2
#endif
#ifndef G13_KEY_G3
#define G13_KEY_G3  KEY_3     // G3 default: number 3
#endif
#ifndef G13_KEY_G4
#define G13_KEY_G4  KEY_4     // G4 default: number 4
#endif
#ifndef G13_KEY_G5
#define G13_KEY_G5  KEY_5     // G5 default: number 5
#endif
#ifndef G13_KEY_G6
#define G13_KEY_G6  KEY_6     // G6 default: number 6
#endif
#ifndef G13_KEY_G7
#define G13_KEY_G7  KEY_7     // G7 default: number 7
#endif
#ifndef G13_KEY_G8
#define G13_KEY_G8  KEY_K     // G8 default: lowercase k on German Mac
#endif
#ifndef G13_KEY_G9
#define G13_KEY_G9  KEY_L     // G9 default: lowercase l on German Mac
#endif
#ifndef G13_KEY_G10
#define G13_KEY_G10 KEY_A     // G10 default: lowercase a on German Mac
#endif
#ifndef G13_KEY_G11
#define G13_KEY_G11 KEY_W     // G11 default: lowercase w on German Mac
#endif
#ifndef G13_KEY_G12
#define G13_KEY_G12 KEY_D     // G12 default: lowercase d on German Mac
#endif
#ifndef G13_KEY_G13
#define G13_KEY_G13 KEY_M     // G13 default: lowercase m on German Mac
#endif
#ifndef G13_KEY_G14
#define G13_KEY_G14 KEY_N     // G14 default: lowercase n on German Mac
#endif
#ifndef G13_KEY_G15
#define G13_KEY_G15 KEY_O     // G15 default: lowercase o on German Mac
#endif
#ifndef G13_KEY_G16
#define G13_KEY_G16 KEY_P     // G16 default: lowercase p on German Mac
#endif
#ifndef G13_KEY_G17
#define G13_KEY_G17 KEY_S     // G17 default: lowercase s on German Mac
#endif
#ifndef G13_KEY_G18
#define G13_KEY_G18 KEY_Q     // G18 default: lowercase q on German Mac
#endif
#ifndef G13_KEY_G19
#define G13_KEY_G19 KEY_R     // G19 default: lowercase r on German Mac
#endif
#ifndef G13_KEY_G20
#define G13_KEY_G20 KEY_SPACE // G20 default: space bar
#endif
#ifndef G13_KEY_G21
#define G13_KEY_G21 KEY_U     // G21 default: lowercase u on German Mac
#endif
#ifndef G13_KEY_G22
#define G13_KEY_G22 KEY_T     // G22 default: lowercase t on German Mac
#endif

// -----------------------------------------------------------------------------
// G13 RGB/key backlight
//
// Valid range for each channel: 0..255.
// Default: RGB(0, 0, 255), the existing blue startup color.
// The G13 protocol may apply this value to key/global lighting rather than to
// the monochrome LCD itself.
// -----------------------------------------------------------------------------
#ifndef G13_BACKLIGHT_RED
#define G13_BACKLIGHT_RED   0
#endif
#ifndef G13_BACKLIGHT_GREEN
#define G13_BACKLIGHT_GREEN 0
#endif
#ifndef G13_BACKLIGHT_BLUE
#define G13_BACKLIGHT_BLUE  255
#endif

// -----------------------------------------------------------------------------
// LCD theme
//
// MARIE_LATTE: current eight-frame story and optional permanent M² signature.
// STATIC:       previous static G13 logo, without loading the animation assets.
// NONE:         initialize the LCD path but send no automatic startup graphic.
// -----------------------------------------------------------------------------
// Theme names/IDs are part of the public interface; do not edit these constants.
#define G13_LCD_THEME_MARIE_LATTE 1
#define G13_LCD_THEME_STATIC      2
#define G13_LCD_THEME_NONE        3

#ifndef G13_LCD_THEME
#define G13_LCD_THEME G13_LCD_THEME_MARIE_LATTE // Default v1.1.0 visual theme.
#endif

// -----------------------------------------------------------------------------
// Marie/Latte theme behavior
//
// Boolean options accept only 0 (off) or 1 (on).
// These options apply to G13_LCD_THEME_MARIE_LATTE. The STATIC and NONE themes
// deliberately override the story/permanent-frame choices.
// -----------------------------------------------------------------------------
#ifndef G13_LCD_ANIMATION_ENABLE
#define G13_LCD_ANIMATION_ENABLE 1 // Default: play all eight story frames.
#endif

#ifndef G13_LCD_PERMANENT_FRAME_ENABLE
#define G13_LCD_PERMANENT_FRAME_ENABLE 1 // Default: keep the M² signature.
#endif

#ifndef G13_LCD_ANIMATION_REPEAT
#define G13_LCD_ANIMATION_REPEAT 0 // Default: one-shot; 1 repeats after READY.
#endif

#ifndef G13_LCD_STATIC_FALLBACK_ENABLE
#define G13_LCD_STATIC_FALLBACK_ENABLE 1
// Default: if story and permanent frame are both off, show the old static logo.
// This fallback applies only to MARIE_LATTE; STATIC and NONE remain authoritative.
#endif

// -----------------------------------------------------------------------------
// Marie/Latte theme timing
//
// All values are milliseconds. Normal and LATTE values must be 1..60000.
// READY's additional hold may be 0..60000. Timing begins only after the
// preceding frame transfer has completed, so USB scheduling can add a little
// visible time.
// -----------------------------------------------------------------------------
#ifndef G13_LCD_ANIMATION_FRAME_MS
#define G13_LCD_ANIMATION_FRAME_MS 700
// Default: normal hold for story frames, including READY's first hold.
#endif

#ifndef G13_LATTE_OVERCLOCK_MS
#define G13_LATTE_OVERCLOCK_MS 1200
// Default: longer hold for frame 6, LATTE OVERCLOCK!
#endif

#ifndef G13_READY_HOLD_MS
#define G13_READY_HOLD_MS 2000
// Default: extra hold after READY FOR AZEROTH's normal frame time.
#endif
