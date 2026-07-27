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
