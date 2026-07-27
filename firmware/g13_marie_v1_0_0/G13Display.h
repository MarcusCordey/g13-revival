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

#include "G13Config.h"
#include <Arduino.h>
#include <USBHost_t36.h>

// -----------------------------------------------------------------------------
// G13 LCD public API
// Purpose:
// Exposes a small, Arduino-style interface for the optional Logitech G13 LCD
// module while keeping implementation details in G13Display.cpp.
//
// Design notes:
// - All functions are non-blocking from the caller's perspective.
// - updateDisplay() must be called regularly from loop().
// - lcdAttach()/lcdDetach() are called from the HID claim/disconnect path only
//   when G13_LCD_ENABLE is enabled.
// - When LCD support is compiled out, these functions become safe no-op stubs.
// -----------------------------------------------------------------------------

// Returns true when a USBHIDParser appears to own the G13 interface with an OUT
// endpoint suitable for LCD transfers.
bool lcdCanAttachTo(USBHIDParser *driver);

// Attach/detach the LCD state machine to the verified G13 HID parser.
void lcdAttach(USBHIDParser *driver);
void lcdDetach();

// High-level LCD drawing API. The framebuffer is local until lcdUpdate() marks
// it for transfer.
void lcdInit();
void lcdClear();
void lcdDrawBitmap(const uint8_t *bitmap);
void lcdDrawText(int x, int y, const char *text);
void lcdUpdate();

// Cooperative LCD/backlight service function; call once per main loop.
void updateDisplay();

// Compatibility helper for brightness-only callers. Current implementation maps
// brightness to the blue channel: RGB(0, 0, brightness).
void setBacklight(uint8_t brightness);
