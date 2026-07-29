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

#include <stdint.h>

// Small, platform-independent timeline used by the LCD service. It owns no
// image memory and never advances a frame until the caller confirms completion.
class G13AnimationTimeline {
public:
  enum FrameKind : uint8_t {
    NO_FRAME,
    ANIMATION_FRAME,
    PERMANENT_FRAME
  };

  struct FrameRequest {
    FrameKind kind;
    uint8_t index;
  };

  enum Phase : uint8_t {
    READY_TO_SEND,
    TRANSFER_ACTIVE,
    HOLDING_FRAME,
    HOLDING_READY_EXTRA,
    FINISHED
  };

  G13AnimationTimeline(uint32_t normalFrameDurationMs,
                       uint32_t overclockFrameDurationMs,
                       uint32_t readyHoldMs,
                       bool animationEnabled,
                       bool repeat,
                       bool permanentFrameEnabled,
                       uint8_t overclockFrameIndex)
    : normalFrameDurationMs_(normalFrameDurationMs),
      overclockFrameDurationMs_(overclockFrameDurationMs),
      readyHoldMs_(readyHoldMs),
      animationEnabled_(animationEnabled),
      repeat_(repeat),
      permanentFrameEnabled_(permanentFrameEnabled),
      overclockFrameIndex_(overclockFrameIndex) {
    reset();
  }

  void reset() {
    frameIndex_ = 0;
    holdStartedAtMs_ = 0;
    holdDurationMs_ = 0;
    activeFrameKind_ = NO_FRAME;
    phase_ = READY_TO_SEND;
  }

  FrameRequest beginFrame(uint32_t now,
                          bool lcdTransferActive,
                          uint8_t animationFrameCount) {
    if (lcdTransferActive ||
        phase_ == TRANSFER_ACTIVE ||
        phase_ == FINISHED) {
      return noFrame();
    }

    if (phase_ == READY_TO_SEND) {
      if (animationEnabled_ && animationFrameCount > 0) {
        frameIndex_ = 0;
        return startAnimationFrame();
      }
      if (permanentFrameEnabled_) {
        return startPermanentFrame();
      }
      phase_ = FINISHED;
      return noFrame();
    }

    if (phase_ == HOLDING_FRAME) {
      if (!holdElapsed(now)) {
        return noFrame();
      }
      if (frameIndex_ + 1 < animationFrameCount) {
        frameIndex_++;
        return startAnimationFrame();
      }
      if (readyHoldMs_ > 0) {
        holdStartedAtMs_ = now;
        holdDurationMs_ = readyHoldMs_;
        phase_ = HOLDING_READY_EXTRA;
        return noFrame();
      }
      return startAfterReady();
    }

    if (phase_ == HOLDING_READY_EXTRA) {
      if (!holdElapsed(now)) {
        return noFrame();
      }
      return startAfterReady();
    }

    return noFrame();
  }

  bool transferCompleted(uint32_t now, uint8_t animationFrameCount) {
    if (phase_ != TRANSFER_ACTIVE) {
      return false;
    }

    if (activeFrameKind_ == PERMANENT_FRAME) {
      activeFrameKind_ = NO_FRAME;
      phase_ = FINISHED;
      return true;
    }

    if (activeFrameKind_ != ANIMATION_FRAME ||
        frameIndex_ >= animationFrameCount) {
      return false;
    }

    holdStartedAtMs_ = now;
    holdDurationMs_ =
      frameIndex_ == overclockFrameIndex_
        ? overclockFrameDurationMs_
        : normalFrameDurationMs_;
    activeFrameKind_ = NO_FRAME;
    phase_ = HOLDING_FRAME;
    return true;
  }

  uint8_t frameIndex() const {
    return frameIndex_;
  }

  FrameKind activeFrameKind() const {
    return activeFrameKind_;
  }

  Phase phase() const {
    return phase_;
  }

private:
  static FrameRequest noFrame() {
    FrameRequest request = {NO_FRAME, 0};
    return request;
  }

  FrameRequest startAnimationFrame() {
    activeFrameKind_ = ANIMATION_FRAME;
    phase_ = TRANSFER_ACTIVE;
    FrameRequest request = {ANIMATION_FRAME, frameIndex_};
    return request;
  }

  FrameRequest startPermanentFrame() {
    activeFrameKind_ = PERMANENT_FRAME;
    phase_ = TRANSFER_ACTIVE;
    FrameRequest request = {PERMANENT_FRAME, 0};
    return request;
  }

  FrameRequest startAfterReady() {
    if (animationEnabled_ && repeat_) {
      frameIndex_ = 0;
      return startAnimationFrame();
    }
    if (permanentFrameEnabled_) {
      return startPermanentFrame();
    }
    activeFrameKind_ = NO_FRAME;
    phase_ = FINISHED;
    return noFrame();
  }

  bool holdElapsed(uint32_t now) const {
    return (uint32_t)(now - holdStartedAtMs_) >= holdDurationMs_;
  }

  uint32_t normalFrameDurationMs_;
  uint32_t overclockFrameDurationMs_;
  uint32_t readyHoldMs_;
  uint32_t holdStartedAtMs_;
  uint32_t holdDurationMs_;
  uint8_t frameIndex_;
  bool animationEnabled_;
  bool repeat_;
  bool permanentFrameEnabled_;
  uint8_t overclockFrameIndex_;
  FrameKind activeFrameKind_;
  Phase phase_;
};

// Firmware-facing wrappers around one static timeline and the generated
// flash-resident frames. Disabled builds provide no-op implementations.
void g13StartupAnimationReset();
const uint8_t *g13StartupAnimationBeginFrame(uint32_t now,
                                              bool lcdTransferActive);
bool g13StartupAnimationTransferCompleted(uint32_t now);
