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

// -----------------------------------------------------------------------------
// Compile-time feature switches
// Purpose:
// Keeps risky or optional protocol features behind a single project-local flag.
//
// G13_LCD_ENABLE:
// 0 = HID-only mode. No LCD init, no OUT transfers and no lighting reports.
// 1 = Enable the documented LCD state machine and guarded G13 display support.
//
// Safety note:
// HID input stability has priority. If LCD experiments cause USB instability,
// set this flag to 0 and rebuild.
// -----------------------------------------------------------------------------
// Set to 0 to force the LCD/backlight path completely off.
#ifndef G13_LCD_ENABLE
#define G13_LCD_ENABLE 1
#endif

// -----------------------------------------------------------------------------
// Startup animation
//
// G13_LCD_ANIMATION_ENABLE:
// 1 = play the generated monochrome startup frames after each attach/reconnect.
// 0 = skip the animation.
//
// G13_LCD_PERMANENT_FRAME_ENABLE:
// 1 = show M² inside | Powered by Marie once after a one-shot animation.
//     If the animation is disabled, show the permanent frame directly.
// 0 = leave READY FOR AZEROTH visible after a one-shot animation.
//
// G13_LCD_ANIMATION_FRAME_MS:
// Normal visibility time after a confirmed animation-frame transfer.
//
// G13_LATTE_OVERCLOCK_MS:
// Longer visibility time for frame 6, LATTE OVERCLOCK!
//
// G13_READY_HOLD_MS:
// Additional visibility time after READY FOR AZEROTH's normal frame time.
//
// G13_LCD_ANIMATION_REPEAT:
// 0 = play once, then show the permanent frame when enabled.
// 1 = restart after READY's normal and additional hold times. Repeat mode
//     intentionally does not enter the permanent-frame state.
//
// G13_LCD_STATIC_FALLBACK_ENABLE:
// When animation and permanent frame are both disabled, 1 keeps the previous
// static logo startup frame; 0 leaves the display unchanged after initialization.
// -----------------------------------------------------------------------------
#ifndef G13_LCD_ANIMATION_ENABLE
#define G13_LCD_ANIMATION_ENABLE 1
#endif

#ifndef G13_LCD_PERMANENT_FRAME_ENABLE
#define G13_LCD_PERMANENT_FRAME_ENABLE 1
#endif

#ifndef G13_LCD_ANIMATION_FRAME_MS
#define G13_LCD_ANIMATION_FRAME_MS 700
#endif

#ifndef G13_LATTE_OVERCLOCK_MS
#define G13_LATTE_OVERCLOCK_MS 1200
#endif

#ifndef G13_READY_HOLD_MS
#define G13_READY_HOLD_MS 2000
#endif

#ifndef G13_LCD_ANIMATION_REPEAT
#define G13_LCD_ANIMATION_REPEAT 0
#endif

#ifndef G13_LCD_STATIC_FALLBACK_ENABLE
#define G13_LCD_STATIC_FALLBACK_ENABLE 1
#endif

#if (G13_LCD_ENABLE != 0) && (G13_LCD_ENABLE != 1)
#error "G13_LCD_ENABLE must be 0 or 1"
#endif

#if (G13_LCD_ANIMATION_ENABLE != 0) && (G13_LCD_ANIMATION_ENABLE != 1)
#error "G13_LCD_ANIMATION_ENABLE must be 0 or 1"
#endif

#if (G13_LCD_PERMANENT_FRAME_ENABLE != 0) && (G13_LCD_PERMANENT_FRAME_ENABLE != 1)
#error "G13_LCD_PERMANENT_FRAME_ENABLE must be 0 or 1"
#endif

#if G13_LCD_ANIMATION_FRAME_MS < 1
#error "G13_LCD_ANIMATION_FRAME_MS must be at least 1"
#endif

#if G13_LATTE_OVERCLOCK_MS < 1
#error "G13_LATTE_OVERCLOCK_MS must be at least 1"
#endif

#if G13_READY_HOLD_MS < 0
#error "G13_READY_HOLD_MS must not be negative"
#endif

#if (G13_LCD_ANIMATION_REPEAT != 0) && (G13_LCD_ANIMATION_REPEAT != 1)
#error "G13_LCD_ANIMATION_REPEAT must be 0 or 1"
#endif

#if (G13_LCD_STATIC_FALLBACK_ENABLE != 0) && (G13_LCD_STATIC_FALLBACK_ENABLE != 1)
#error "G13_LCD_STATIC_FALLBACK_ENABLE must be 0 or 1"
#endif
