#include <assert.h>
#include <stdint.h>

#include "../firmware/g13_marie_v1_0_0/G13StartupAnimation.h"

static const uint8_t ANIMATION_FRAME_COUNT = 8;

static void assertNoFrame(
    const G13AnimationTimeline::FrameRequest &request) {
  assert(request.kind == G13AnimationTimeline::NO_FRAME);
}

static void assertAnimationFrame(
    const G13AnimationTimeline::FrameRequest &request,
    uint8_t expectedIndex) {
  assert(request.kind == G13AnimationTimeline::ANIMATION_FRAME);
  assert(request.index == expectedIndex);
}

static void assertPermanentFrame(
    const G13AnimationTimeline::FrameRequest &request) {
  assert(request.kind == G13AnimationTimeline::PERMANENT_FRAME);
}

static void testOneShotTimingAndPermanentFrame() {
  G13AnimationTimeline timeline(
    700,   // normal frames
    1200,  // LATTE OVERCLOCK!
    2000,  // additional READY hold
    true,
    false,
    true,
    5
  );

  assertAnimationFrame(
    timeline.beginFrame(0, false, ANIMATION_FRAME_COUNT),
    0
  );
  assertNoFrame(timeline.beginFrame(1, false, ANIMATION_FRAME_COUNT));
  assertNoFrame(timeline.beginFrame(1, true, ANIMATION_FRAME_COUNT));

  uint32_t completedAt = 1000;
  for (uint8_t current = 0; current < 5; current++) {
    assert(timeline.transferCompleted(completedAt, ANIMATION_FRAME_COUNT));
    assertNoFrame(
      timeline.beginFrame(completedAt + 699, false, ANIMATION_FRAME_COUNT)
    );
    assertNoFrame(
      timeline.beginFrame(completedAt + 700, true, ANIMATION_FRAME_COUNT)
    );
    assertAnimationFrame(
      timeline.beginFrame(completedAt + 700, false, ANIMATION_FRAME_COUNT),
      current + 1
    );
    completedAt += 2000;
  }

  // Frame 6 is zero-based index 5 and uses the longer Latte hold.
  assert(timeline.transferCompleted(completedAt, ANIMATION_FRAME_COUNT));
  assertNoFrame(
    timeline.beginFrame(completedAt + 1199, false, ANIMATION_FRAME_COUNT)
  );
  assertAnimationFrame(
    timeline.beginFrame(completedAt + 1200, false, ANIMATION_FRAME_COUNT),
    6
  );

  completedAt += 3000;
  assert(timeline.transferCompleted(completedAt, ANIMATION_FRAME_COUNT));
  assertNoFrame(
    timeline.beginFrame(completedAt + 699, false, ANIMATION_FRAME_COUNT)
  );
  assertAnimationFrame(
    timeline.beginFrame(completedAt + 700, false, ANIMATION_FRAME_COUNT),
    7
  );

  // READY receives its normal 700 ms, followed by an additional full 2000 ms.
  completedAt += 3000;
  assert(timeline.transferCompleted(completedAt, ANIMATION_FRAME_COUNT));
  assertNoFrame(
    timeline.beginFrame(completedAt + 699, false, ANIMATION_FRAME_COUNT)
  );
  assertNoFrame(
    timeline.beginFrame(completedAt + 700, false, ANIMATION_FRAME_COUNT)
  );
  assert(timeline.phase() == G13AnimationTimeline::HOLDING_READY_EXTRA);
  assertNoFrame(
    timeline.beginFrame(completedAt + 2699, false, ANIMATION_FRAME_COUNT)
  );
  assertPermanentFrame(
    timeline.beginFrame(completedAt + 2700, false, ANIMATION_FRAME_COUNT)
  );

  // The signature is one logical frame request and only finishes after the
  // caller confirms its complete LCD transfer.
  assertNoFrame(
    timeline.beginFrame(completedAt + 2701, false, ANIMATION_FRAME_COUNT)
  );
  assert(timeline.transferCompleted(completedAt + 2800, ANIMATION_FRAME_COUNT));
  assert(timeline.phase() == G13AnimationTimeline::FINISHED);
  assertNoFrame(
    timeline.beginFrame(0xFFFFFFFFU, false, ANIMATION_FRAME_COUNT)
  );
  assert(!timeline.transferCompleted(
    completedAt + 2801,
    ANIMATION_FRAME_COUNT
  ));
}

static uint32_t reachReadyExtraHold(G13AnimationTimeline &timeline) {
  assertAnimationFrame(
    timeline.beginFrame(0, false, ANIMATION_FRAME_COUNT),
    0
  );

  uint32_t now = 100;
  for (uint8_t current = 0; current < ANIMATION_FRAME_COUNT - 1; current++) {
    assert(timeline.transferCompleted(now, ANIMATION_FRAME_COUNT));
    now += current == 5 ? 20 : 10;
    assertAnimationFrame(
      timeline.beginFrame(now, false, ANIMATION_FRAME_COUNT),
      current + 1
    );
    now += 100;
  }

  assert(timeline.transferCompleted(now, ANIMATION_FRAME_COUNT));
  assertNoFrame(timeline.beginFrame(now + 10, false, ANIMATION_FRAME_COUNT));
  assert(timeline.phase() == G13AnimationTimeline::HOLDING_READY_EXTRA);
  return now + 10;
}

static void testPermanentDisabledAndRepeatModes() {
  G13AnimationTimeline oneShotWithoutPermanent(
    10, 20, 30, true, false, false, 5
  );
  const uint32_t oneShotExtraStartedAt =
    reachReadyExtraHold(oneShotWithoutPermanent);
  assertNoFrame(
    oneShotWithoutPermanent.beginFrame(
      oneShotExtraStartedAt + 29,
      false,
      ANIMATION_FRAME_COUNT
    )
  );
  assertNoFrame(
    oneShotWithoutPermanent.beginFrame(
      oneShotExtraStartedAt + 30,
      false,
      ANIMATION_FRAME_COUNT
    )
  );
  assert(oneShotWithoutPermanent.phase() == G13AnimationTimeline::FINISHED);

  G13AnimationTimeline repeating(
    10, 20, 30, true, true, true, 5
  );
  const uint32_t repeatExtraStartedAt = reachReadyExtraHold(repeating);
  assertNoFrame(
    repeating.beginFrame(
      repeatExtraStartedAt + 29,
      false,
      ANIMATION_FRAME_COUNT
    )
  );
  assertAnimationFrame(
    repeating.beginFrame(
      repeatExtraStartedAt + 30,
      false,
      ANIMATION_FRAME_COUNT
    ),
    0
  );
}

static void testPermanentOnlyAndNoStartupFrame() {
  G13AnimationTimeline permanentOnly(
    700, 1200, 2000, false, false, true, 5
  );
  assertPermanentFrame(
    permanentOnly.beginFrame(10, false, ANIMATION_FRAME_COUNT)
  );
  assertNoFrame(
    permanentOnly.beginFrame(11, false, ANIMATION_FRAME_COUNT)
  );
  assert(permanentOnly.transferCompleted(20, ANIMATION_FRAME_COUNT));
  assertNoFrame(
    permanentOnly.beginFrame(1000, false, ANIMATION_FRAME_COUNT)
  );

  G13AnimationTimeline noStartupFrame(
    700, 1200, 2000, false, false, false, 5
  );
  assertNoFrame(
    noStartupFrame.beginFrame(10, false, ANIMATION_FRAME_COUNT)
  );
  assert(noStartupFrame.phase() == G13AnimationTimeline::FINISHED);
}

static void testResetAndStaleCompletion() {
  G13AnimationTimeline timeline(
    10, 20, 30, true, false, true, 5
  );

  assertAnimationFrame(
    timeline.beginFrame(0, false, ANIMATION_FRAME_COUNT),
    0
  );
  timeline.reset();
  assert(timeline.phase() == G13AnimationTimeline::READY_TO_SEND);
  assert(!timeline.transferCompleted(1, ANIMATION_FRAME_COUNT));
  assertAnimationFrame(
    timeline.beginFrame(1, false, ANIMATION_FRAME_COUNT),
    0
  );

  timeline.reset();
  const uint32_t extraStartedAt = reachReadyExtraHold(timeline);
  assertPermanentFrame(
    timeline.beginFrame(
      extraStartedAt + 30,
      false,
      ANIMATION_FRAME_COUNT
    )
  );
  timeline.reset();
  assert(!timeline.transferCompleted(extraStartedAt + 31, ANIMATION_FRAME_COUNT));
  assertAnimationFrame(
    timeline.beginFrame(extraStartedAt + 31, false, ANIMATION_FRAME_COUNT),
    0
  );
}

static void testMillisWraparound() {
  G13AnimationTimeline timeline(
    10, 20, 30, true, false, true, 0
  );

  assertAnimationFrame(
    timeline.beginFrame(0xFFFFFFF0U, false, 2),
    0
  );
  assert(timeline.transferCompleted(0xFFFFFFF0U, 2));
  assertNoFrame(timeline.beginFrame(3, false, 2));
  assertAnimationFrame(timeline.beginFrame(4, false, 2), 1);

  assert(timeline.transferCompleted(5, 2));
  assertNoFrame(timeline.beginFrame(14, false, 2));
  assertNoFrame(timeline.beginFrame(15, false, 2));
  assertNoFrame(timeline.beginFrame(44, false, 2));
  assertPermanentFrame(timeline.beginFrame(45, false, 2));
}

int main() {
  testOneShotTimingAndPermanentFrame();
  testPermanentDisabledAndRepeatModes();
  testPermanentOnlyAndNoStartupFrame();
  testResetAndStaleCompletion();
  testMillisWraparound();
  return 0;
}
