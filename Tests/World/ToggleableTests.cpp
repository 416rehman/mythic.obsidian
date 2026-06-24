// Mythic — toggleable (door/lever/gate/switch) unit tests
// Covers the pure toggle decision: lock, one-shot, and normal flip semantics.
// Run via: Session Frontend → Automation → Mythic.World.Toggleable

#include "Misc/AutomationTest.h"
#include "World/Interactables/MythicToggleable.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicToggleableResolveTest,
    "Mythic.World.Toggleable.ResolveToggle",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicToggleableResolveTest::RunTest(const FString &Parameters) {
    // Signature: ResolveToggle(bCurrentlyOn, bLocked, bOneShot, bHasActivated)

    // Normal toggle flips both ways.
    {
        const FMythicToggleOutcome R = AMythicToggleable::ResolveToggle(false, false, false, false);
        TestTrue(TEXT("off→on changes"), R.bChanged);
        TestTrue(TEXT("off→on new state is on"), R.bNewIsOn);
    }
    {
        const FMythicToggleOutcome R = AMythicToggleable::ResolveToggle(true, false, false, false);
        TestTrue(TEXT("on→off changes"), R.bChanged);
        TestFalse(TEXT("on→off new state is off"), R.bNewIsOn);
    }

    // Locked: never changes, state preserved (regardless of current state).
    {
        const FMythicToggleOutcome R = AMythicToggleable::ResolveToggle(false, true, false, false);
        TestFalse(TEXT("locked off does not change"), R.bChanged);
        TestFalse(TEXT("locked off stays off"), R.bNewIsOn);
    }
    {
        const FMythicToggleOutcome R = AMythicToggleable::ResolveToggle(true, true, false, false);
        TestFalse(TEXT("locked on does not change"), R.bChanged);
        TestTrue(TEXT("locked on stays on"), R.bNewIsOn);
    }

    // One-shot, not yet fired: goes ON (and only on).
    {
        const FMythicToggleOutcome R = AMythicToggleable::ResolveToggle(false, false, true, false);
        TestTrue(TEXT("fresh one-shot fires"), R.bChanged);
        TestTrue(TEXT("fresh one-shot turns on"), R.bNewIsOn);
    }
    // One-shot already on but not flagged-activated → target is on == current, so no change (never turns itself off).
    {
        const FMythicToggleOutcome R = AMythicToggleable::ResolveToggle(true, false, true, false);
        TestFalse(TEXT("one-shot already on does not toggle off"), R.bChanged);
        TestTrue(TEXT("one-shot stays on"), R.bNewIsOn);
    }
    // One-shot already fired (activated) → locked out, no change.
    {
        const FMythicToggleOutcome R = AMythicToggleable::ResolveToggle(false, false, true, true);
        TestFalse(TEXT("spent one-shot does not re-fire"), R.bChanged);
    }

    // Precedence: locked beats one-shot (a locked fresh one-shot still does nothing).
    {
        const FMythicToggleOutcome R = AMythicToggleable::ResolveToggle(false, true, true, false);
        TestFalse(TEXT("locked one-shot does not fire"), R.bChanged);
    }

    return true;
}
