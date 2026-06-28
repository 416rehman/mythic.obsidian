// Mythic — co-op revive channel unit tests.
// Covers the pure rules behind the proximity-maintained teammate-revive channel: progress accrual (clamped), the
// completion gate, and the per-tick keep-going gate. The live channel timer + server re-validation are PIE-verified;
// this locks the decision math so a downed ally is revived after exactly ReviveChannelSeconds of maintained proximity.
// Run via: Session Frontend → Automation → Mythic.Combat.ReviveChannel

#include "Misc/AutomationTest.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicReviveChannelTest,
    "Mythic.Combat.ReviveChannel",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicReviveChannelTest::RunTest(const FString &Parameters) {
    using R = UMythicLifeComponent;

    // ── ComputeReviveProgressAfterTick(Current, Delta, Channel) = clamp(Current + max(0,Delta), 0, max(0,Channel)) ──
    TestEqual(TEXT("accrue one tick"), R::ComputeReviveProgressAfterTick(0.0f, 0.1f, 3.0f), 0.1f);
    TestEqual(TEXT("progress clamps to the channel length"), R::ComputeReviveProgressAfterTick(2.95f, 0.1f, 3.0f), 3.0f);
    TestEqual(TEXT("negative delta never rewinds progress"), R::ComputeReviveProgressAfterTick(1.0f, -0.5f, 3.0f), 1.0f);
    TestEqual(TEXT("a disabled channel (0) clamps progress to 0"), R::ComputeReviveProgressAfterTick(1.0f, 0.1f, 0.0f), 0.0f);

    // ── IsReviveComplete(Progress, Channel): enabled channel AND progress reached it ──
    TestTrue(TEXT("complete when progress reaches the channel"), R::IsReviveComplete(3.0f, 3.0f));
    TestFalse(TEXT("not complete just under"), R::IsReviveComplete(2.9f, 3.0f));
    TestFalse(TEXT("a disabled channel (0) is never complete"), R::IsReviveComplete(5.0f, 0.0f));

    // ── ShouldContinueReviveChannel(targetDowned, reviverValid, reviverDowned, reviverInRange) ──
    TestTrue(TEXT("continue when all conditions hold"), R::ShouldContinueReviveChannel(true, true, false, true));
    TestFalse(TEXT("stop if the target is no longer downed (already revived / died)"), R::ShouldContinueReviveChannel(false, true, false, true));
    TestFalse(TEXT("stop if the reviver vanished"), R::ShouldContinueReviveChannel(true, false, false, true));
    TestFalse(TEXT("stop if the reviver themselves went down"), R::ShouldContinueReviveChannel(true, true, true, true));
    TestFalse(TEXT("stop if the reviver moved out of range"), R::ShouldContinueReviveChannel(true, true, false, false));

    // ── ShouldInterruptReviveOnDamage(healthNow, healthAtLastTick): the reviver took damage → break the channel ──
    TestTrue(TEXT("a health drop (reviver hit) interrupts"), R::ShouldInterruptReviveOnDamage(80.0f, 100.0f));
    TestFalse(TEXT("steady health does not interrupt"), R::ShouldInterruptReviveOnDamage(100.0f, 100.0f));
    TestFalse(TEXT("healing upward does not interrupt"), R::ShouldInterruptReviveOnDamage(120.0f, 100.0f));
    TestFalse(TEXT("sub-epsilon jitter does not spuriously interrupt"), R::ShouldInterruptReviveOnDamage(100.0f - 1e-5f, 100.0f));
    TestTrue(TEXT("even a 1-point hit interrupts"), R::ShouldInterruptReviveOnDamage(99.0f, 100.0f));

    // ── CanReviveTarget (revive-eligibility gate, reused by the interaction) ──
    TestTrue(TEXT("a downed target + an upright reviver is revivable"), R::CanReviveTarget(true, false));
    TestFalse(TEXT("a healthy target is not revivable"), R::CanReviveTarget(false, false));
    TestFalse(TEXT("a downed reviver cannot revive"), R::CanReviveTarget(true, true));

    // ── End-to-end: maintained-proximity ticks accrue to completion. Float-robust (0.1f accumulation undershoots 3.0 at
    //    exactly 30 ticks; the clamp caps progress at 3.0 once it's crossed, so completion lands within one tick of target
    //    — the live channel finishes ~0.1s late at worst, which is fine). Assert partway-incomplete + eventually-complete. ──
    float Progress = 0.0f;
    for (int32 i = 0; i < 10; ++i) { // ~1.0s of a 3.0s channel
        Progress = R::ComputeReviveProgressAfterTick(Progress, 0.1f, 3.0f);
    }
    TestFalse(TEXT("partway (~1.0s of 3.0s) the revive is not yet complete"), R::IsReviveComplete(Progress, 3.0f));
    for (int32 i = 0; i < 25; ++i) { // 35 ticks total → comfortably past 3.0s (clamped to exactly 3.0)
        Progress = R::ComputeReviveProgressAfterTick(Progress, 0.1f, 3.0f);
    }
    TestEqual(TEXT("progress clamps at exactly the channel length"), Progress, 3.0f);
    TestTrue(TEXT("after enough held proximity (>=3.0s) the revive completes"), R::IsReviveComplete(Progress, 3.0f));

    return true;
}
