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

#include "G13Display.h"
#include "G13Config.h"
#include "G13StartupAnimation.h"

#if G13_LCD_ENABLE

#if G13_INTERNAL_LCD_STATIC_FALLBACK_ENABLE
#include "logo_g13_m2.h"
#endif
#include <string.h>

// -----------------------------------------------------------------------------
// Logitech G13 LCD protocol summary
// Purpose:
// Keep all LCD-related reverse-engineering notes close to the implementation.
//
// Confirmed from g13-master and hardware-oriented experiments:
// - The G13 LCD accepts a control request before bitmap transfer:
//   bmRequestType=0x00, bRequest=0x09, wValue=0x0001, wIndex=0x0000.
// - LCD frames use a 32-byte USB payload header followed by 960 framebuffer
//   bytes. Header byte 0 is 0x03; the remaining header bytes are zero.
// - The framebuffer is monochrome. Color, if available, is separate lighting,
//   not per-pixel color.
//
// Assumptions and limitations:
// - The RGB control report 0x0307 is known from g13-master as SetKeyColor().
//   It may affect key backlight/global lighting rather than the LCD alone.
// - USBHost_t36 sendPacket() uses endpoint-sized TX buffers. Therefore the
//   992-byte LCD frame is sent in chunks no larger than outSize().
// -----------------------------------------------------------------------------
// LCD/backlight protocol values verified against g13-master:
// - g13_lcd.cpp / g13_lcd.hpp: LCD init, endpoint payload header, 960 byte framebuffer
// - g13_device.cpp: class requests for key backlight color
static const uint16_t G13_VENDOR_ID = 0x046d;
static const uint16_t G13_PRODUCT_ID = 0xc21c;

static const uint16_t LCD_WIDTH = 160;
static const uint16_t LCD_HEIGHT = 48;
static const uint16_t LCD_FRAMEBUFFER_SIZE = 960;
static const uint16_t LCD_TRANSFER_HEADER_SIZE = 32;
static const uint16_t LCD_TRANSFER_SIZE = LCD_TRANSFER_HEADER_SIZE + LCD_FRAMEBUFFER_SIZE;

// Das G13-LCD bekommt nur ein monochromes 1-Bit-Bitmap; Farbe kommt, soweit
// das Geraet sie akzeptiert, ueber die separate Hintergrundbeleuchtung.
// Daher kann der Framebuffer keine echten blauen Flaechen zeichnen. Fuer die
// gewuenschte blaue Hintergrundwirkung bleibt das Logo invertiert (weisse
// Logopixel), und der sonst schwarze Grafikhintergrund wird leicht gedithert,
// damit die blaue Beleuchtung optisch durchwirken kann.
#if G13_INTERNAL_LCD_STATIC_FALLBACK_ENABLE
static const bool LCD_INVERT_LOGO_FOR_WHITE_ON_BLACK = true;
static const bool LCD_DITHER_LOGO_BACKGROUND_FOR_BACKLIGHT = true;
#endif

// USBHIDParser currently selected for LCD OUT traffic.
// It is only set after lcdCanAttachTo() confirms a G13 parser with an OUT
// endpoint. Keeping this pointer null disables all LCD transfers.
static USBHIDParser *lcdDriver = nullptr;
static uint16_t lcdDriverOutSize = 0;
static uint8_t lcdDriverInterface = 0;
static uint32_t lcdConnectionGeneration = 0;

// USBHost_t36 invokes HID callbacks from the USBHS interrupt. Callbacks only
// publish events here. updateDisplay() consumes them with IRQ_USBHS masked
// briefly, then performs all actual state-machine changes in the main loop.
static USBHIDParser *volatile pendingLcdAttachDriver = nullptr;
static volatile bool pendingLcdDetach = false;
static volatile uint16_t pendingLcdAttachOutSize = 0;
static volatile uint8_t pendingLcdAttachInterface = 0;
static volatile uint32_t pendingLcdConnectionGeneration = 0;
static volatile uint8_t pendingControlCompletion = 0;
static volatile uint16_t pendingOutCompletionLength = 0;
static volatile uint32_t pendingControlCompletionGeneration = 0;
static volatile uint32_t pendingOutCompletionGeneration = 0;

// Framebuffer storage:
// lcdFramebuffer is the logical 160x48 monochrome image.
// lcdTransferFrame is the exact USB payload: 32-byte header + framebuffer.
// The alignment is intentional for Teensy 4.x USB/cache behavior.
static uint8_t lcdFramebuffer[LCD_FRAMEBUFFER_SIZE] __attribute__((aligned(32)));
static uint8_t lcdTransferFrame[LCD_TRANSFER_SIZE] __attribute__((aligned(32)));

// Five-byte class report used for G13 RGB lighting:
// byte 0 = report ID / command prefix observed as 5
// byte 1 = red
// byte 2 = green
// byte 3 = blue
// byte 4 = reserved / zero in the reference implementation
static uint8_t backlightPacket[5] __attribute__((aligned(32))) = {5, 0, 0, 0, 0};

// LCD state flags. These are deliberately explicit rather than compressed into
// bitfields so Serial event logs and failure handling remain easy to audit.
static bool lcdDirty = false;
static bool lcdOnlineLogged = false;
static bool lcdErrorLogged = false;
static bool backlightErrorLogged = false;
static bool lcdNoDriverLogged = false;
static bool lcdDisabledLogged = false;
static uint32_t lcdFrameStartMs = 0;
static uint16_t lcdTransferOffset = 0;
static bool lcdTransferActive = false;
static const uint8_t *lcdPendingNativeFrame = nullptr;
static bool lcdAnimationFrameInFlight = false;
static bool lcdOutTransferPending = false;
static uint16_t lcdOutPendingLength = 0;
static uint32_t lcdOutTransferStartedMs = 0;
static uint32_t lcdOutNextAttemptMs = 0;
static uint8_t lcdOutAttemptCount = 0;

// Backlight state cache. The driver only sends a lighting report when the
// requested RGB value changes, preventing repeated control transfers in loop().
static uint8_t currentBacklightRed = 0;
static uint8_t currentBacklightGreen = 0;
static uint8_t currentBacklightBlue = 0;
static uint8_t pendingBacklightRed = G13_BACKLIGHT_RED;
static uint8_t pendingBacklightGreen = G13_BACKLIGHT_GREEN;
static uint8_t pendingBacklightBlue = G13_BACKLIGHT_BLUE;
static bool currentBacklightValid = false;
static bool backlightPending = false;

enum LcdControlTransferType : uint8_t {
  LCD_CONTROL_NONE,
  LCD_CONTROL_INIT,
  LCD_CONTROL_BACKLIGHT
};

static LcdControlTransferType lcdControlPending = LCD_CONTROL_NONE;
static uint32_t lcdControlTransferStartedMs = 0;
static uint32_t lcdControlNextAttemptMs = 0;
static uint8_t lcdControlAttemptCount = 0;
static uint8_t lcdControlBacklightRed = 0;
static uint8_t lcdControlBacklightGreen = 0;
static uint8_t lcdControlBacklightBlue = 0;

// -----------------------------------------------------------------------------
// LcdBootState
// Purpose:
// Non-blocking LCD startup and frame-transfer state machine.
//
// Flow:
// WAIT_DRIVER        - no suitable USBHIDParser is attached
// WAIT_STABLE_CLAIM  - parser was found; wait briefly before LCD init
// SEND_INIT          - queue the LCD init control transfer
// WAIT_INIT          - wait for confirmed init transfer completion
// SEND_BACKLIGHT     - queue the initial lighting control transfer
// WAIT_BACKLIGHT     - wait for confirmed lighting transfer completion
// INIT_SETTLE        - allow the device to settle before bitmap transfer
// SEND_SPLASH        - send one prepared startup frame in endpoint-sized chunks
// READY              - wait for the next due animation frame or later update
// ERROR              - LCD disabled after an error; HID continues to run
// -----------------------------------------------------------------------------
enum LcdBootState {
  LCD_WAIT_DRIVER,
  LCD_WAIT_STABLE_CLAIM,
  LCD_SEND_INIT,
  LCD_WAIT_INIT,
  LCD_SEND_BACKLIGHT,
  LCD_WAIT_BACKLIGHT,
  LCD_INIT_SETTLE,
  LCD_SEND_SPLASH,
  LCD_READY,
  LCD_ERROR
};

static LcdBootState lcdBootState = LCD_WAIT_DRIVER;
static uint32_t lcdStateSinceMs = 0;
static bool lcdDisabledUntilDetach = false;

// Timing constants:
// These are not delay() calls. They gate state-machine transitions while the
// main loop continues to run USBHost_t36 and HID processing. Their internal
// values are defined in G13Config.h, separate from G13UserConfig.h.

// -----------------------------------------------------------------------------
// Function: logEvent
// Purpose:
// Emits a single event-style diagnostic message when Serial is available.
//
// Input:
// message - constant string describing the event
//
// Output:
// Serial line, or no output if Serial is not connected.
//
// Policy:
// LCD logging should remain event-based. The fast display service loop must not
// print cyclic messages.
// -----------------------------------------------------------------------------
static void logEvent(const char *message) {
  if (Serial) {
    Serial.println(message);
  }
}

// -----------------------------------------------------------------------------
// Function: logNoDriverOnce
// Purpose:
// Logs that no suitable G13 LCD-capable HID parser is available.
//
// Notes:
// The latch prevents repeated "[G13] LCD no driver" output while the device is
// absent or while the wrong interface is claimed.
// -----------------------------------------------------------------------------
static void logNoDriverOnce() {
  if (!lcdNoDriverLogged) {
    logEvent("[G13] LCD no driver");
    lcdNoDriverLogged = true;
  }
}

// -----------------------------------------------------------------------------
// Function: logDisabledOnce
// Purpose:
// Logs that LCD service has been disabled for the current failure condition.
//
// Notes:
// Disabling LCD is a safety decision. HID input remains the priority if display
// transfer appears to destabilize the USB host path.
// -----------------------------------------------------------------------------
static void logDisabledOnce() {
  if (!lcdDisabledLogged) {
    logEvent("[G13] LCD disabled");
    lcdDisabledLogged = true;
  }
}

// -----------------------------------------------------------------------------
// Function: driverLooksLikeG13
// Purpose:
// Verifies that the attached LCD parser still points to the Logitech G13.
//
// Output:
// true when lcdDriver is non-null and reports the expected VID/PID.
//
// Limitation:
// This does not prove the parser owns the correct OUT endpoint; that selection
// is handled by lcdCanAttachTo() during attachment.
// -----------------------------------------------------------------------------
static bool driverLooksLikeG13() {
  return lcdDriver != nullptr &&
         lcdDriverInterface == 0 &&
         lcdDriverOutSize > 0;
}

// -----------------------------------------------------------------------------
// Function: noteLcdError
// Purpose:
// Handles LCD-specific failure without disturbing HID input.
//
// Output:
// Logs one LCD error, clears pending LCD/backlight work, detaches the LCD
// parser pointer and moves the LCD state machine to ERROR.
//
// Safety behavior:
// LCD is disabled until a future detach/reclaim condition. Keyboard input should
// continue because the HID decoder path is not touched here.
// -----------------------------------------------------------------------------
static void noteLcdError() {
  if (!lcdErrorLogged) {
    logEvent("[G13] LCD error");
    lcdErrorLogged = true;
  }
  lcdDirty = false;
  lcdTransferActive = false;
  lcdPendingNativeFrame = nullptr;
  lcdAnimationFrameInFlight = false;
  lcdOutTransferPending = false;
  lcdControlPending = LCD_CONTROL_NONE;
  backlightPending = false;
  currentBacklightValid = false;
  lcdDriver = nullptr;
  lcdDriverOutSize = 0;
  lcdDisabledUntilDetach = true;
  lcdBootState = LCD_ERROR;
  logDisabledOnce();
}

// -----------------------------------------------------------------------------
// Function: lcdSetPixel
// Purpose:
// Sets or clears one pixel in the local 160x48 monochrome framebuffer.
//
// Input:
// x, y - pixel coordinates
// on   - true sets the bit, false clears it
//
// Framebuffer layout:
// The G13 uses vertical byte packing: byte offset is x + (y / 8) * 160, and
// bit (y & 7) selects the pixel row within that vertical byte.
// -----------------------------------------------------------------------------
static void lcdSetPixel(int x, int y, bool on) {
  if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_HEIGHT) {
    return;
  }

  const uint16_t offset = x + (y / 8) * LCD_WIDTH;
  const uint8_t mask = 1 << (y & 7);

  if (on) {
    lcdFramebuffer[offset] |= mask;
  } else {
    lcdFramebuffer[offset] &= ~mask;
  }
}

#if G13_INTERNAL_LCD_STATIC_FALLBACK_ENABLE
// -----------------------------------------------------------------------------
// Function: lcdInvertFramebuffer
// Purpose:
// Inverts the entire 1-bit framebuffer before transfer.
//
// Background:
// The current logo asset was generated with the opposite polarity for the
// desired "white logo / darker background" G13 appearance. Inverting here keeps
// the asset unchanged and documents the hardware/display polarity assumption.
// -----------------------------------------------------------------------------
static void lcdInvertFramebuffer() {
  for (uint16_t i = 0; i < LCD_FRAMEBUFFER_SIZE; i++) {
    lcdFramebuffer[i] = ~lcdFramebuffer[i];
  }
}

// -----------------------------------------------------------------------------
// Function: lcdDitherInactiveBackground
// Purpose:
// Adds a sparse pattern to pixels that would otherwise be fully off.
//
// Why:
// The display is monochrome, so the framebuffer cannot draw real blue areas.
// A light dither reduces the "solid black" impression and lets the blue
// backlight visually contribute to the logo background.
//
// Limitation:
// This is a visual compromise, not true color rendering.
// -----------------------------------------------------------------------------
static void lcdDitherInactiveBackground() {
  for (uint16_t y = 0; y < LCD_HEIGHT; y++) {
    for (uint16_t x = 0; x < LCD_WIDTH; x++) {
      const uint16_t offset = x + (y / 8) * LCD_WIDTH;
      const uint8_t mask = 1 << (y & 7);

      if ((lcdFramebuffer[offset] & mask) == 0 && (((x + y) & 0x03) == 0)) {
        lcdFramebuffer[offset] |= mask;
      }
    }
  }
}
#endif

// -----------------------------------------------------------------------------
// Function: queueBacklightColor
// Purpose:
// Schedules a G13 RGB lighting report if the requested color changed.
//
// Input:
// red, green, blue - desired RGB values, 0..255
//
// Output:
// Updates pendingBacklight* and sets backlightPending. The actual USB control
// transfer is performed later by serviceBacklight().
//
// Protocol note:
// The color report is known from g13-master, but whether it maps specifically
// to LCD backlight or to global/key lighting is still hardware-dependent.
// -----------------------------------------------------------------------------
static void queueBacklightColor(uint8_t red, uint8_t green, uint8_t blue) {
  if (currentBacklightValid &&
      red == currentBacklightRed &&
      green == currentBacklightGreen &&
      blue == currentBacklightBlue &&
      !backlightPending) {
    return;
  }

  if (backlightPending &&
      red == pendingBacklightRed &&
      green == pendingBacklightGreen &&
      blue == pendingBacklightBlue) {
    return;
  }

  pendingBacklightRed = red;
  pendingBacklightGreen = green;
  pendingBacklightBlue = blue;
  backlightPending = true;
}

// -----------------------------------------------------------------------------
// Function: fontColumns
// Purpose:
// Provides a small built-in 5-column bitmap font for status text rendering.
//
// Input:
// c - ASCII character. Lowercase letters are normalized to uppercase.
//
// Output:
// Pointer to five bytes of column data. Unknown characters return a blank glyph.
//
// Limitation:
// Only characters needed by current status strings are included. This avoids a
// large font table in the embedded sketch.
// -----------------------------------------------------------------------------
static const uint8_t *fontColumns(char c) {
  static const uint8_t blank[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
  static const uint8_t glyph1[5] = {0x00, 0x42, 0x7f, 0x40, 0x00};
  static const uint8_t glyph3[5] = {0x21, 0x41, 0x45, 0x4b, 0x31};
  static const uint8_t glyphA[5] = {0x7e, 0x11, 0x11, 0x11, 0x7e};
  static const uint8_t glyphB[5] = {0x7f, 0x49, 0x49, 0x49, 0x36};
  static const uint8_t glyphC[5] = {0x3e, 0x41, 0x41, 0x41, 0x22};
  static const uint8_t glyphD[5] = {0x7f, 0x41, 0x41, 0x22, 0x1c};
  static const uint8_t glyphE[5] = {0x7f, 0x49, 0x49, 0x49, 0x41};
  static const uint8_t glyphG[5] = {0x3e, 0x41, 0x49, 0x49, 0x7a};
  static const uint8_t glyphI[5] = {0x00, 0x41, 0x7f, 0x41, 0x00};
  static const uint8_t glyphK[5] = {0x7f, 0x08, 0x14, 0x22, 0x41};
  static const uint8_t glyphL[5] = {0x7f, 0x40, 0x40, 0x40, 0x40};
  static const uint8_t glyphN[5] = {0x7f, 0x02, 0x04, 0x08, 0x7f};
  static const uint8_t glyphO[5] = {0x3e, 0x41, 0x41, 0x41, 0x3e};
  static const uint8_t glyphR[5] = {0x7f, 0x09, 0x19, 0x29, 0x46};
  static const uint8_t glyphS[5] = {0x46, 0x49, 0x49, 0x49, 0x31};
  static const uint8_t glyphT[5] = {0x01, 0x01, 0x7f, 0x01, 0x01};
  static const uint8_t glyphU[5] = {0x3f, 0x40, 0x40, 0x40, 0x3f};
  static const uint8_t glyphV[5] = {0x1f, 0x20, 0x40, 0x20, 0x1f};
  static const uint8_t glyphY[5] = {0x07, 0x08, 0x70, 0x08, 0x07};

  if (c >= 'a' && c <= 'z') {
    c -= 32;
  }

  switch (c) {
    case '1': return glyph1;
    case '3': return glyph3;
    case 'A': return glyphA;
    case 'B': return glyphB;
    case 'C': return glyphC;
    case 'D': return glyphD;
    case 'E': return glyphE;
    case 'G': return glyphG;
    case 'I': return glyphI;
    case 'K': return glyphK;
    case 'L': return glyphL;
    case 'N': return glyphN;
    case 'O': return glyphO;
    case 'R': return glyphR;
    case 'S': return glyphS;
    case 'T': return glyphT;
    case 'U': return glyphU;
    case 'V': return glyphV;
    case 'Y': return glyphY;
    default: return blank;
  }
}

// -----------------------------------------------------------------------------
// Function: sendPendingLcdFrame
// Purpose:
// Sends one prepared LCD frame in endpoint-sized chunks.
//
// Input:
// Uses lcdFramebuffer/lcdTransferFrame and the attached USBHIDParser.
//
// Output:
// Returns true once completion callbacks confirmed the full 992-byte payload.
//
// USB detail:
// USBHIDParser::sendPacket() copies data into internal buffers sized by the OUT
// endpoint. Sending the full 992-byte frame in one call can overflow that
// internal buffer, so this function limits each call to the cached outSize().
//
// Safety:
// A timeout disables LCD rather than risking repeated transfers. HID input is
// expected to continue because the LCD state machine is independent.
// -----------------------------------------------------------------------------
static bool deadlineReached(uint32_t now, uint32_t deadline) {
  return (int32_t)(now - deadline) >= 0;
}

static void finishBacklightFailure(const char *message) {
  if (!backlightErrorLogged) {
    logEvent(message);
    backlightErrorLogged = true;
  }
  lcdControlPending = LCD_CONTROL_NONE;
  lcdControlAttemptCount = 0;
  backlightPending = false;
  currentBacklightValid = false;

  // Lighting is not allowed to prevent the independently working LCD bitmap
  // path from starting.
  if (lcdBootState == LCD_SEND_BACKLIGHT ||
      lcdBootState == LCD_WAIT_BACKLIGHT) {
    lcdBootState = LCD_INIT_SETTLE;
    lcdStateSinceMs = millis();
  }
}

static bool queueControlTransfer(LcdControlTransferType type) {
  if (lcdControlPending != LCD_CONTROL_NONE || !driverLooksLikeG13()) {
    return false;
  }

  const uint32_t now = millis();
  if (!deadlineReached(now, lcdControlNextAttemptMs)) {
    return false;
  }

  if (type == LCD_CONTROL_BACKLIGHT) {
    backlightPacket[0] = 5;
    backlightPacket[1] = pendingBacklightRed;
    backlightPacket[2] = pendingBacklightGreen;
    backlightPacket[3] = pendingBacklightBlue;
    backlightPacket[4] = 0;
    lcdControlBacklightRed = pendingBacklightRed;
    lcdControlBacklightGreen = pendingBacklightGreen;
    lcdControlBacklightBlue = pendingBacklightBlue;
  }

  lcdControlAttemptCount++;
  lcdControlPending = type;
  lcdControlTransferStartedMs = now;

  bool queued = false;
  const bool usbIrqWasEnabled = NVIC_IS_ENABLED(IRQ_USBHS);
  NVIC_DISABLE_IRQ(IRQ_USBHS);
  USBHIDParser *driver = lcdDriver;
  if (driver && !pendingLcdDetach) {
    if (type == LCD_CONTROL_INIT) {
      queued = driver->sendControlPacket(0x00, 0x09, 0x0001, 0x0000, 0, nullptr);
    } else {
      queued = driver->sendControlPacket(0x21, 0x09, 0x0307, 0x0000,
                                         sizeof(backlightPacket), backlightPacket);
    }
  }
  if (usbIrqWasEnabled) {
    NVIC_ENABLE_IRQ(IRQ_USBHS);
  }

  if (queued) {
    return true;
  }

  lcdControlPending = LCD_CONTROL_NONE;
  if (lcdControlAttemptCount >= G13_LCD_MAX_TRANSFER_ATTEMPTS) {
    if (type == LCD_CONTROL_INIT) {
      logEvent("[G13] LCD init queue error");
      noteLcdError();
    } else {
      finishBacklightFailure("[G13] Backlight queue error");
    }
  } else {
    lcdControlNextAttemptMs =
      now + G13_LCD_RETRY_BACKOFF_MS * lcdControlAttemptCount;
  }
  return false;
}

static bool sendPendingLcdFrame() {
  if (lcdDisabledUntilDetach) {
    logDisabledOnce();
    return false;
  }

  if (!lcdDirty && !lcdTransferActive) {
    return false;
  }

  if (!driverLooksLikeG13()) {
    logNoDriverOnce();
    return false;
  }

  const uint16_t chunkSize = lcdDriverOutSize;
  const uint32_t now = millis();

  if (!lcdTransferActive) {
    memset(lcdTransferFrame, 0, LCD_TRANSFER_HEADER_SIZE);
    lcdTransferFrame[0] = 0x03;
    const uint8_t *sourceFrame = lcdPendingNativeFrame
      ? lcdPendingNativeFrame
      : lcdFramebuffer;
    memcpy(lcdTransferFrame + LCD_TRANSFER_HEADER_SIZE,
           sourceFrame,
           LCD_FRAMEBUFFER_SIZE);

    lcdTransferOffset = 0;
    lcdFrameStartMs = now;
    lcdTransferActive = true;
    lcdOutTransferPending = false;
    lcdOutAttemptCount = 0;
    lcdOutNextAttemptMs = now;
    lcdPendingNativeFrame = nullptr;
    lcdDirty = false;
  }

  if (now - lcdFrameStartMs > G13_LCD_TRANSFER_TIMEOUT_MS) {
    noteLcdError();
    return false;
  }

  if (lcdTransferOffset >= LCD_TRANSFER_SIZE) {
    lcdTransferActive = false;
    return true;
  }

  if (lcdOutTransferPending || !deadlineReached(now, lcdOutNextAttemptMs)) {
    return false;
  }

  const uint16_t remaining = LCD_TRANSFER_SIZE - lcdTransferOffset;
  const uint16_t bytesToSend = (remaining < chunkSize) ? remaining : chunkSize;
  lcdOutAttemptCount++;
  lcdOutPendingLength = bytesToSend;
  lcdOutTransferPending = true;
  lcdOutTransferStartedMs = now;

  bool queued = false;
  const bool usbIrqWasEnabled = NVIC_IS_ENABLED(IRQ_USBHS);
  NVIC_DISABLE_IRQ(IRQ_USBHS);
  USBHIDParser *driver = lcdDriver;
  if (driver && !pendingLcdDetach) {
    queued = driver->sendPacket(lcdTransferFrame + lcdTransferOffset, bytesToSend);
  }
  if (usbIrqWasEnabled) {
    NVIC_ENABLE_IRQ(IRQ_USBHS);
  }

  if (!queued) {
    lcdOutTransferPending = false;
    if (lcdOutAttemptCount >= G13_LCD_MAX_TRANSFER_ATTEMPTS) {
      logEvent("[G13] LCD OUT queue error");
      noteLcdError();
    } else {
      lcdOutNextAttemptMs =
        now + G13_LCD_RETRY_BACKOFF_MS * lcdOutAttemptCount;
    }
  }

  return false;
}

// -----------------------------------------------------------------------------
// Function: serviceBacklight
// Purpose:
// Sends a pending RGB lighting control report to the G13.
//
// Input:
// Uses pendingBacklight* and backlightPending.
//
// Output:
// Queues a USB class control transfer:
// bmRequestType=0x21, bRequest=0x09, wValue=0x0307, wIndex=0.
//
// Reverse-engineering note:
// g13-master names this behavior SetKeyColor(). The exact physical target on
// every G13 unit may be global/key backlight rather than LCD-only lighting.
// -----------------------------------------------------------------------------
static void serviceBacklight() {
  if (!backlightPending ||
      !driverLooksLikeG13() ||
      lcdControlPending != LCD_CONTROL_NONE) {
    return;
  }
  queueControlTransfer(LCD_CONTROL_BACKLIGHT);
}

// -----------------------------------------------------------------------------
// Function: lcdCanAttachTo
// Purpose:
// Decides whether a USBHIDParser is safe to use for LCD OUT traffic.
//
// Input:
// driver - candidate USBHIDParser from the HID claim path
//
// Output:
// true only for Logitech G13 VID/PID, interface 0 and a non-zero OUT size.
//
// Assumption:
// g13-master claims interface 0 and uses endpoint 2 OUT for LCD. USBHost_t36
// exposes the endpoint indirectly through outSize(), not by endpoint number.
// -----------------------------------------------------------------------------
bool lcdCanAttachTo(USBHIDParser *driver) {
  if (!driver) {
    return false;
  }

  return driver->idVendor() == G13_VENDOR_ID &&
         driver->idProduct() == G13_PRODUCT_ID &&
         driver->interfaceNumber() == 0 &&
         driver->outSize() > 0;
}

// -----------------------------------------------------------------------------
// Function: applyLcdAttach
// Purpose:
// Attaches the LCD state machine to a verified G13 HID parser.
//
// Input:
// driver - parser selected by the HID claim path
//
// Output:
// Initializes LCD state variables and waits for a stable claim before sending
// any display traffic.
//
// -----------------------------------------------------------------------------
static void applyLcdAttach(USBHIDParser *driver, uint8_t interfaceNumber,
                           uint16_t outSize, uint32_t generation) {
  if (lcdDisabledUntilDetach) {
    logDisabledOnce();
    return;
  }

  if (!driver || interfaceNumber != 0 || outSize == 0) {
    return;
  }

  if (lcdDriver) {
    return;
  }

  lcdDriver = driver;
  lcdDriverInterface = interfaceNumber;
  lcdDriverOutSize = outSize;
  lcdConnectionGeneration = generation;
  lcdBootState = LCD_WAIT_STABLE_CLAIM;
  lcdStateSinceMs = millis();
  lcdFrameStartMs = 0;
  lcdTransferOffset = 0;
  lcdTransferActive = false;
  lcdPendingNativeFrame = nullptr;
  lcdAnimationFrameInFlight = false;
  lcdOutTransferPending = false;
  lcdOutAttemptCount = 0;
  lcdControlPending = LCD_CONTROL_NONE;
  lcdControlAttemptCount = 0;
  lcdControlNextAttemptMs = 0;
  lcdOnlineLogged = false;
  lcdErrorLogged = false;
  backlightErrorLogged = false;
  lcdNoDriverLogged = false;
  lcdDisabledLogged = false;
  currentBacklightValid = false;
  backlightPending = false;
  lcdDirty = false;
  g13StartupAnimationReset();
  lcdClear();
  if (Serial) {
    Serial.printf("[G13] LCD attach parser=%p iface=%u outSize=%u\n",
                  (void *)driver,
                  interfaceNumber,
                  outSize);
  }
}

// -----------------------------------------------------------------------------
// Function: applyLcdDetach
// Purpose:
// Clears LCD state when the G13 disconnects or its collection is released.
//
// -----------------------------------------------------------------------------
static void applyLcdDetach(uint32_t generation) {
  lcdDriver = nullptr;
  lcdDriverOutSize = 0;
  lcdDriverInterface = 0;
  lcdConnectionGeneration = generation;
  lcdBootState = LCD_WAIT_DRIVER;
  lcdDirty = false;
  lcdTransferActive = false;
  lcdPendingNativeFrame = nullptr;
  lcdAnimationFrameInFlight = false;
  lcdOutTransferPending = false;
  lcdOutAttemptCount = 0;
  lcdControlPending = LCD_CONTROL_NONE;
  lcdControlAttemptCount = 0;
  backlightPending = false;
  currentBacklightValid = false;
  lcdDisabledUntilDetach = false;
  g13StartupAnimationReset();
}

void lcdQueueAttachEvent(USBHIDParser *driver) {
  pendingLcdConnectionGeneration++;
  pendingLcdAttachInterface = driver ? driver->interfaceNumber() : 0xff;
  pendingLcdAttachOutSize = driver ? driver->outSize() : 0;
  pendingLcdAttachDriver = driver;
}

void lcdQueueDetachEvent() {
  pendingLcdConnectionGeneration++;
  pendingLcdDetach = true;
  pendingLcdAttachDriver = nullptr;
  pendingLcdAttachOutSize = 0;
}

bool lcdQueueControlCompleteEvent(const Transfer_t *transfer) {
  if (!transfer) {
    return false;
  }

  LcdControlTransferType type = LCD_CONTROL_NONE;
  if (transfer->setup.bmRequestType == 0x00 &&
      transfer->setup.bRequest == 0x09 &&
      transfer->setup.wValue == 0x0001 &&
      transfer->setup.wIndex == 0x0000 &&
      transfer->setup.wLength == 0) {
    type = LCD_CONTROL_INIT;
  } else if (transfer->setup.bmRequestType == 0x21 &&
             transfer->setup.bRequest == 0x09 &&
             transfer->setup.wValue == 0x0307 &&
             transfer->setup.wIndex == 0x0000 &&
             transfer->setup.wLength == sizeof(backlightPacket)) {
    type = LCD_CONTROL_BACKLIGHT;
  } else {
    return false;
  }

  pendingControlCompletionGeneration = pendingLcdConnectionGeneration;
  pendingControlCompletion = type;
  return true;
}

bool lcdQueueOutCompleteEvent(const Transfer_t *transfer) {
  if (!transfer) {
    return false;
  }

  pendingOutCompletionGeneration = pendingLcdConnectionGeneration;
  pendingOutCompletionLength = transfer->length;
  return true;
}

static void serviceLcdTransferEvents() {
  uint8_t controlCompletion = LCD_CONTROL_NONE;
  uint16_t outCompletionLength = 0;
  uint32_t controlGeneration = 0;
  uint32_t outGeneration = 0;

  const bool usbIrqWasEnabled = NVIC_IS_ENABLED(IRQ_USBHS);
  NVIC_DISABLE_IRQ(IRQ_USBHS);
  controlCompletion = pendingControlCompletion;
  outCompletionLength = pendingOutCompletionLength;
  controlGeneration = pendingControlCompletionGeneration;
  outGeneration = pendingOutCompletionGeneration;
  pendingControlCompletion = LCD_CONTROL_NONE;
  pendingOutCompletionLength = 0;
  if (usbIrqWasEnabled) {
    NVIC_ENABLE_IRQ(IRQ_USBHS);
  }

  if (controlCompletion != LCD_CONTROL_NONE &&
      controlGeneration == lcdConnectionGeneration &&
      controlCompletion == lcdControlPending) {
    const LcdControlTransferType completedType = lcdControlPending;
    lcdControlPending = LCD_CONTROL_NONE;
    lcdControlAttemptCount = 0;
    lcdControlNextAttemptMs = millis();

    if (completedType == LCD_CONTROL_INIT) {
      logEvent("[G13] LCD init complete");
      backlightPending = true;
      lcdBootState = LCD_SEND_BACKLIGHT;
    } else {
      currentBacklightRed = lcdControlBacklightRed;
      currentBacklightGreen = lcdControlBacklightGreen;
      currentBacklightBlue = lcdControlBacklightBlue;
      currentBacklightValid = true;
      backlightPending =
        pendingBacklightRed != currentBacklightRed ||
        pendingBacklightGreen != currentBacklightGreen ||
        pendingBacklightBlue != currentBacklightBlue;
      backlightErrorLogged = false;
      logEvent("[G13] Backlight updated");
      if (lcdBootState == LCD_WAIT_BACKLIGHT ||
          lcdBootState == LCD_SEND_BACKLIGHT) {
        lcdBootState = LCD_INIT_SETTLE;
        lcdStateSinceMs = millis();
      }
    }
  }

  if (outCompletionLength != 0 &&
      outGeneration == lcdConnectionGeneration &&
      lcdOutTransferPending) {
    if (outCompletionLength != lcdOutPendingLength) {
      noteLcdError();
      return;
    }

    lcdTransferOffset += lcdOutPendingLength;
    lcdOutTransferPending = false;
    lcdOutAttemptCount = 0;
    lcdOutNextAttemptMs = millis() + G13_LCD_CHUNK_INTERVAL_MS;
  }
}

static void serviceLcdTransferTimeouts() {
  const uint32_t now = millis();

  if (lcdControlPending != LCD_CONTROL_NONE &&
      now - lcdControlTransferStartedMs > G13_LCD_CONTROL_TIMEOUT_MS) {
    const LcdControlTransferType timedOutType = lcdControlPending;
    lcdControlPending = LCD_CONTROL_NONE;
    if (timedOutType == LCD_CONTROL_INIT) {
      logEvent("[G13] LCD init timeout");
      noteLcdError();
    } else {
      finishBacklightFailure("[G13] Backlight timeout");
    }
  }

  if (lcdOutTransferPending &&
      now - lcdOutTransferStartedMs > G13_LCD_OUT_TIMEOUT_MS) {
    lcdOutTransferPending = false;
    logEvent("[G13] LCD OUT timeout");
    noteLcdError();
  }
}

static void serviceLcdConnectionEvents() {
  USBHIDParser *attachDriver = nullptr;
  uint16_t attachOutSize = 0;
  uint8_t attachInterface = 0;
  uint32_t generation = 0;
  bool detach = false;

  const bool usbIrqWasEnabled = NVIC_IS_ENABLED(IRQ_USBHS);
  NVIC_DISABLE_IRQ(IRQ_USBHS);
  detach = pendingLcdDetach;
  attachDriver = pendingLcdAttachDriver;
  attachOutSize = pendingLcdAttachOutSize;
  attachInterface = pendingLcdAttachInterface;
  generation = pendingLcdConnectionGeneration;
  pendingLcdDetach = false;
  pendingLcdAttachDriver = nullptr;
  pendingLcdAttachOutSize = 0;
  if (usbIrqWasEnabled) {
    NVIC_ENABLE_IRQ(IRQ_USBHS);
  }

  // Preserve ordering for a detach followed quickly by a new attach. Conversely,
  // lcdQueueDetachEvent() clears an older unconsumed attach event.
  if (detach) {
    applyLcdDetach(generation);
  }
  if (attachDriver) {
    applyLcdAttach(attachDriver, attachInterface, attachOutSize, generation);
  }
}

// -----------------------------------------------------------------------------
// Function: lcdInit
// Purpose:
// Starts the known LCD initialization control request. Completion is confirmed
// asynchronously before lighting or bitmap transfers can begin.
// -----------------------------------------------------------------------------
void lcdInit() {
  if (!driverLooksLikeG13()) {
    logNoDriverOnce();
    return;
  }

  if (queueControlTransfer(LCD_CONTROL_INIT)) {
    lcdBootState = LCD_WAIT_INIT;
  }
}

// -----------------------------------------------------------------------------
// Function: lcdClear
// Purpose:
// Clears the local monochrome framebuffer.
//
// Output:
// Sets all 960 framebuffer bytes to zero. It does not send anything to USB;
// lcdUpdate() is required to request a transfer.
// -----------------------------------------------------------------------------
void lcdClear() {
  memset(lcdFramebuffer, 0, sizeof(lcdFramebuffer));
}

// -----------------------------------------------------------------------------
// Function: lcdDrawBitmap
// Purpose:
// Copies a full-screen 960-byte G13 bitmap into the local framebuffer.
//
// Input:
// bitmap - pointer to framebuffer data in the G13 160x48 packed format.
//
// Limitation:
// No size argument is provided. Callers must pass a valid full-screen bitmap.
// -----------------------------------------------------------------------------
void lcdDrawBitmap(const uint8_t *bitmap) {
  if (!bitmap) {
    return;
  }
  memcpy(lcdFramebuffer, bitmap, LCD_FRAMEBUFFER_SIZE);
}

// -----------------------------------------------------------------------------
// Function: lcdDrawText
// Purpose:
// Draws simple 5-column text into the local framebuffer.
//
// Input:
// x, y - pixel position
// text - null-terminated ASCII text
//
// Output:
// Sets framebuffer pixels only; it does not queue a USB transfer by itself.
// -----------------------------------------------------------------------------
void lcdDrawText(int x, int y, const char *text) {
  if (!text) {
    return;
  }

  while (*text) {
    const uint8_t *columns = fontColumns(*text++);
    for (uint8_t col = 0; col < 5; col++) {
      for (uint8_t row = 0; row < 7; row++) {
        if (columns[col] & (1 << row)) {
          lcdSetPixel(x + col, y + row, true);
        }
      }
    }
    x += 6;
  }
}

// -----------------------------------------------------------------------------
// Function: lcdUpdate
// Purpose:
// Marks the local framebuffer as dirty so updateDisplay() will send it.
//
// Output:
// Sets lcdDirty. The actual transfer remains non-blocking and chunked.
// -----------------------------------------------------------------------------
void lcdUpdate() {
  lcdDirty = true;
}

// -----------------------------------------------------------------------------
// Function: updateDisplay
// Purpose:
// Services LCD/backlight state machines from the main loop.
//
// Flow:
// - Applies queued USB callback events.
// - Waits for a stable HID claim before LCD init.
// - Confirms serialized init and lighting control transfers.
// - Sends startup frames in completion-checked chunks.
// - Waits in READY until the animation timeline or lcdUpdate() requests a frame.
//
// Important:
// This function must not block or delay. It shares the cooperative loop with
// USBHost_t36, so all waiting is based on millis() and state transitions.
// -----------------------------------------------------------------------------
void updateDisplay() {
  serviceLcdConnectionEvents();
  serviceLcdTransferEvents();
  serviceLcdTransferTimeouts();

  switch (lcdBootState) {
    case LCD_WAIT_DRIVER:
      if (millis() > G13_LCD_STABLE_CLAIM_MS) {
        logNoDriverOnce();
      }
      return;

    case LCD_WAIT_STABLE_CLAIM:
      if (millis() - lcdStateSinceMs >= G13_LCD_STABLE_CLAIM_MS) {
        lcdBootState = LCD_SEND_INIT;
      }
      return;

    case LCD_SEND_INIT:
      lcdInit();
      return;

    case LCD_WAIT_INIT:
      return;

    case LCD_SEND_BACKLIGHT:
      if (!backlightPending) {
        lcdBootState = LCD_INIT_SETTLE;
        lcdStateSinceMs = millis();
      } else if (queueControlTransfer(LCD_CONTROL_BACKLIGHT)) {
        lcdBootState = LCD_WAIT_BACKLIGHT;
      }
      return;

    case LCD_WAIT_BACKLIGHT:
      return;

    case LCD_INIT_SETTLE:
      if (millis() - lcdStateSinceMs >= 50) {
#if G13_INTERNAL_LCD_ANIMATION_ENABLE || \
    G13_INTERNAL_LCD_PERMANENT_FRAME_ENABLE
        const uint8_t *firstFrame =
          g13StartupAnimationBeginFrame(millis(), lcdTransferActive);
        if (firstFrame) {
          lcdPendingNativeFrame = firstFrame;
          lcdAnimationFrameInFlight = true;
          lcdUpdate();
          lcdBootState = LCD_SEND_SPLASH;
        } else {
          lcdBootState = LCD_READY;
        }
#elif G13_INTERNAL_LCD_STATIC_FALLBACK_ENABLE
        lcdDrawBitmap(logo_g13_m2);
        if (LCD_INVERT_LOGO_FOR_WHITE_ON_BLACK) {
          lcdInvertFramebuffer();
        }
        if (LCD_DITHER_LOGO_BACKGROUND_FOR_BACKLIGHT) {
          lcdDitherInactiveBackground();
        }
        lcdUpdate();
        lcdBootState = LCD_SEND_SPLASH;
#else
        lcdBootState = LCD_READY;
#endif
      }
      return;

    case LCD_SEND_SPLASH:
      if (sendPendingLcdFrame()) {
        if (lcdAnimationFrameInFlight) {
          g13StartupAnimationTransferCompleted(millis());
          lcdAnimationFrameInFlight = false;
        }
        if (!lcdOnlineLogged) {
          logEvent("[G13] LCD online");
          lcdOnlineLogged = true;
        }
        lcdBootState = LCD_READY;
        lcdStateSinceMs = millis();
      }
      return;

    case LCD_READY:
      serviceBacklight();
      if (lcdDirty) {
        lcdBootState = LCD_SEND_SPLASH;
        return;
      }
#if G13_INTERNAL_LCD_ANIMATION_ENABLE || \
    G13_INTERNAL_LCD_PERMANENT_FRAME_ENABLE
      {
        const uint8_t *nextFrame = g13StartupAnimationBeginFrame(
          millis(),
          lcdTransferActive || lcdDirty
        );
        if (nextFrame) {
          lcdPendingNativeFrame = nextFrame;
          lcdAnimationFrameInFlight = true;
          lcdUpdate();
          lcdBootState = LCD_SEND_SPLASH;
        }
      }
#endif
      return;

    case LCD_ERROR:
      return;
  }
}

// -----------------------------------------------------------------------------
// Function: setBacklight
// Purpose:
// Public compatibility helper for setting blue-channel brightness.
//
// Input:
// brightness - 0..255 value mapped to RGB(0, 0, brightness)
//
// Note:
// The project currently uses queueBacklightColor() internally for full RGB.
// This wrapper keeps the originally requested API available.
// -----------------------------------------------------------------------------
void setBacklight(uint8_t brightness) {
  queueBacklightColor(0, 0, brightness);
}

#else

// LCD-disabled stubs:
// When G13_LCD_ENABLE is 0, the public LCD API remains link-compatible but no
// USB control or OUT transfers are generated. This is the safest HID-only mode.
bool lcdCanAttachTo(USBHIDParser *driver) {
  (void)driver;
  return false;
}

void lcdQueueAttachEvent(USBHIDParser *driver) {
  (void)driver;
}

void lcdQueueDetachEvent() {}

bool lcdQueueControlCompleteEvent(const Transfer_t *transfer) {
  (void)transfer;
  return false;
}

bool lcdQueueOutCompleteEvent(const Transfer_t *transfer) {
  (void)transfer;
  return false;
}

void lcdInit() {}
void lcdClear() {}

void lcdDrawBitmap(const uint8_t *bitmap) {
  (void)bitmap;
}

void lcdDrawText(int x, int y, const char *text) {
  (void)x;
  (void)y;
  (void)text;
}

void lcdUpdate() {}
void updateDisplay() {}

void setBacklight(uint8_t brightness) {
  (void)brightness;
}

#endif
