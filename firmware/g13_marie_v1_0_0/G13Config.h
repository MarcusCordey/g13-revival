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
// Internal firmware configuration
// Purpose:
// Keeps protocol, transfer and safety controls separate from normal user setup.
// Edit G13UserConfig.h for key mapping, lighting and LCD presentation.
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

// HID health monitoring. These conservative thresholds are part of the tested
// internal behavior, not normal user customization.
#ifndef G13_STALL_TIMEOUT_MS
#define G13_STALL_TIMEOUT_MS 5000
#endif

#ifndef G13_RECOVER_GRACE_MS
#define G13_RECOVER_GRACE_MS 200
#endif

// LCD/USB state-machine timing, retry and safety limits. Keep these values in
// the internal configuration so G13UserConfig.h remains presentation-focused.
#ifndef G13_LCD_STABLE_CLAIM_MS
#define G13_LCD_STABLE_CLAIM_MS 1000
#endif

#ifndef G13_LCD_CHUNK_INTERVAL_MS
#define G13_LCD_CHUNK_INTERVAL_MS 2
#endif

#ifndef G13_LCD_TRANSFER_TIMEOUT_MS
#define G13_LCD_TRANSFER_TIMEOUT_MS 2500
#endif

#ifndef G13_LCD_CONTROL_TIMEOUT_MS
#define G13_LCD_CONTROL_TIMEOUT_MS 500
#endif

#ifndef G13_LCD_OUT_TIMEOUT_MS
#define G13_LCD_OUT_TIMEOUT_MS 250
#endif

#ifndef G13_LCD_RETRY_BACKOFF_MS
#define G13_LCD_RETRY_BACKOFF_MS 100
#endif

#ifndef G13_LCD_MAX_TRANSFER_ATTEMPTS
#define G13_LCD_MAX_TRANSFER_ATTEMPTS 3
#endif

#include <Arduino.h>
#include "G13UserConfig.h"

#if (G13_LCD_ENABLE != 0) && (G13_LCD_ENABLE != 1)
#error "G13_LCD_ENABLE must be 0 or 1"
#endif

#if G13_BACKLIGHT_RED < 0 || G13_BACKLIGHT_RED > 255
#error "G13_BACKLIGHT_RED must be between 0 and 255"
#endif

#if G13_BACKLIGHT_GREEN < 0 || G13_BACKLIGHT_GREEN > 255
#error "G13_BACKLIGHT_GREEN must be between 0 and 255"
#endif

#if G13_BACKLIGHT_BLUE < 0 || G13_BACKLIGHT_BLUE > 255
#error "G13_BACKLIGHT_BLUE must be between 0 and 255"
#endif

#if G13_LCD_THEME_MARIE_LATTE == G13_LCD_THEME_STATIC || \
    G13_LCD_THEME_MARIE_LATTE == G13_LCD_THEME_NONE || \
    G13_LCD_THEME_STATIC == G13_LCD_THEME_NONE
#error "G13 LCD theme IDs must remain distinct; do not edit the G13_LCD_THEME_* constants"
#endif

#if G13_LCD_THEME != G13_LCD_THEME_MARIE_LATTE && \
    G13_LCD_THEME != G13_LCD_THEME_STATIC && \
    G13_LCD_THEME != G13_LCD_THEME_NONE
#error "G13_LCD_THEME must be G13_LCD_THEME_MARIE_LATTE, G13_LCD_THEME_STATIC or G13_LCD_THEME_NONE"
#endif

#if (G13_LCD_ANIMATION_ENABLE != 0) && (G13_LCD_ANIMATION_ENABLE != 1)
#error "G13_LCD_ANIMATION_ENABLE must be 0 or 1"
#endif

#if (G13_LCD_PERMANENT_FRAME_ENABLE != 0) && (G13_LCD_PERMANENT_FRAME_ENABLE != 1)
#error "G13_LCD_PERMANENT_FRAME_ENABLE must be 0 or 1"
#endif

#if G13_LCD_ANIMATION_FRAME_MS < 1 || G13_LCD_ANIMATION_FRAME_MS > 60000
#error "G13_LCD_ANIMATION_FRAME_MS must be between 1 and 60000 milliseconds"
#endif

#if G13_LATTE_OVERCLOCK_MS < 1 || G13_LATTE_OVERCLOCK_MS > 60000
#error "G13_LATTE_OVERCLOCK_MS must be between 1 and 60000 milliseconds"
#endif

#if G13_READY_HOLD_MS < 0 || G13_READY_HOLD_MS > 60000
#error "G13_READY_HOLD_MS must be between 0 and 60000 milliseconds"
#endif

#if (G13_LCD_ANIMATION_REPEAT != 0) && (G13_LCD_ANIMATION_REPEAT != 1)
#error "G13_LCD_ANIMATION_REPEAT must be 0 or 1"
#endif

#if (G13_LCD_STATIC_FALLBACK_ENABLE != 0) && (G13_LCD_STATIC_FALLBACK_ENABLE != 1)
#error "G13_LCD_STATIC_FALLBACK_ENABLE must be 0 or 1"
#endif

// Teensy 1.62 defines named normal-key constants for HID usages 4..101 and
// 104..115. Keeping the public mapping to these exact 0xF0xx ranges preserves
// six-key rollover accounting and gives duplicate mappings one representation.
#define G13_INTERNAL_KEYCODE_IS_SUPPORTED(value) \
  (((value) >= 0xF004 && (value) <= 0xF065) || \
   ((value) >= 0xF068 && (value) <= 0xF073))

#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G1)
#error "G13_KEY_G1 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G2)
#error "G13_KEY_G2 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G3)
#error "G13_KEY_G3 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G4)
#error "G13_KEY_G4 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G5)
#error "G13_KEY_G5 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G6)
#error "G13_KEY_G6 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G7)
#error "G13_KEY_G7 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G8)
#error "G13_KEY_G8 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G9)
#error "G13_KEY_G9 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G10)
#error "G13_KEY_G10 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G11)
#error "G13_KEY_G11 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G12)
#error "G13_KEY_G12 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G13)
#error "G13_KEY_G13 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G14)
#error "G13_KEY_G14 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G15)
#error "G13_KEY_G15 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G16)
#error "G13_KEY_G16 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G17)
#error "G13_KEY_G17 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G18)
#error "G13_KEY_G18 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G19)
#error "G13_KEY_G19 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G20)
#error "G13_KEY_G20 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G21)
#error "G13_KEY_G21 must be a supported normal-key Teensy KEY_* constant"
#endif
#if !G13_INTERNAL_KEYCODE_IS_SUPPORTED(G13_KEY_G22)
#error "G13_KEY_G22 must be a supported normal-key Teensy KEY_* constant"
#endif

// Theme-derived implementation switches. Firmware files consume only these
// internal values, so a theme change cannot create conflicting startup paths.
#define G13_INTERNAL_LCD_ANIMATION_ENABLE \
  (G13_LCD_THEME == G13_LCD_THEME_MARIE_LATTE && G13_LCD_ANIMATION_ENABLE)

#define G13_INTERNAL_LCD_PERMANENT_FRAME_ENABLE \
  (G13_LCD_THEME == G13_LCD_THEME_MARIE_LATTE && \
   G13_LCD_PERMANENT_FRAME_ENABLE)

#define G13_INTERNAL_LCD_STATIC_FALLBACK_ENABLE \
  (G13_LCD_THEME == G13_LCD_THEME_STATIC || \
   (G13_LCD_THEME == G13_LCD_THEME_MARIE_LATTE && \
    !G13_LCD_ANIMATION_ENABLE && \
    !G13_LCD_PERMANENT_FRAME_ENABLE && \
    G13_LCD_STATIC_FALLBACK_ENABLE))
