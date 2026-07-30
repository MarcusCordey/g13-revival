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

#include "G13StartupAnimation.h"
#include "G13Config.h"

#if G13_LCD_ENABLE && \
    (G13_INTERNAL_LCD_ANIMATION_ENABLE || \
     G13_INTERNAL_LCD_PERMANENT_FRAME_ENABLE)

#include "G13StartupAnimationFrames.h"

static const uint8_t G13_ANIMATION_FRAME_COUNT = 8;
static const uint8_t G13_PERMANENT_FRAME_INDEX = 8;
static const uint8_t G13_LATTE_OVERCLOCK_FRAME_INDEX = 5;

static_assert(g13_startup_animation_width == 160,
              "G13 animation frames must be 160 pixels wide");
static_assert(g13_startup_animation_height == 48,
              "G13 animation frames must be 48 pixels high");
static_assert(g13_startup_animation_frame_bytes == 960,
              "G13 animation frames must contain 960 bytes");
static_assert(g13_startup_animation_count ==
                G13_ANIMATION_FRAME_COUNT + 1,
              "G13 startup assets must contain 8 animation frames "
              "and 1 permanent frame");

static G13AnimationTimeline animationTimeline(
  G13_LCD_ANIMATION_FRAME_MS,
  G13_LATTE_OVERCLOCK_MS,
  G13_READY_HOLD_MS,
  G13_INTERNAL_LCD_ANIMATION_ENABLE != 0,
  G13_LCD_ANIMATION_REPEAT != 0,
  G13_INTERNAL_LCD_PERMANENT_FRAME_ENABLE != 0,
  G13_LATTE_OVERCLOCK_FRAME_INDEX
);

void g13StartupAnimationReset() {
  animationTimeline.reset();
}

const uint8_t *g13StartupAnimationBeginFrame(uint32_t now,
                                              bool lcdTransferActive) {
  const G13AnimationTimeline::FrameRequest request =
    animationTimeline.beginFrame(
    now,
    lcdTransferActive,
    G13_ANIMATION_FRAME_COUNT
  );
  if (request.kind == G13AnimationTimeline::ANIMATION_FRAME) {
    return g13_startup_animation_frames[request.index];
  }
  if (request.kind == G13AnimationTimeline::PERMANENT_FRAME) {
    return g13_startup_animation_frames[G13_PERMANENT_FRAME_INDEX];
  }
  return nullptr;
}

bool g13StartupAnimationTransferCompleted(uint32_t now) {
  return animationTimeline.transferCompleted(
    now,
    G13_ANIMATION_FRAME_COUNT
  );
}

#else

void g13StartupAnimationReset() {}

const uint8_t *g13StartupAnimationBeginFrame(uint32_t now,
                                              bool lcdTransferActive) {
  (void)now;
  (void)lcdTransferActive;
  return nullptr;
}

bool g13StartupAnimationTransferCompleted(uint32_t now) {
  (void)now;
  return false;
}

#endif
