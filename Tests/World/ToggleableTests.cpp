// Mythic — toggleable (door/lever/gate/switch) unit tests
// Covers the pure toggle decision: lock, one-shot, and normal flip semantics.
// Run via: Session Frontend → Automation → Mythic.World.Toggleable

#include "Misc/AutomationTest.h"
#include "GAS/MythicTags_GAS.h" // two registered tags for the pure key↔lock match (tag IDENTITY is irrelevant here)
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

// ─── Keyed unlock: the pure key↔lock match + unlock decision a held key drives ───
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicKeyedUnlockTest,
    "Mythic.World.Toggleable.KeyedUnlock",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicKeyedUnlockTest::RunTest(const FString &Parameters) {
    // Two distinct, definitely-registered tags stand in for "the lock's key tag" and "a different key" — the pure
    // DoesKeyOpenLock cares only about validity / containment, not which tag it is.
    const FGameplayTag LockKey = GAS_STATE_SPRINTING;
    const FGameplayTag OtherKey = GAS_STATE_DOWNED;

    // DoesKeyOpenLock(KeyTypeProbe, RequiredKeyTag): a probe containing the required tag opens it.
    FGameplayTagContainer Probe;
    Probe.AddTag(LockKey);
    TestTrue(TEXT("a key whose probe has the lock tag opens it"), AMythicToggleable::DoesKeyOpenLock(Probe, LockKey));
    TestFalse(TEXT("the wrong key does not open it"), AMythicToggleable::DoesKeyOpenLock(Probe, OtherKey));
    TestFalse(TEXT("an empty required tag is never opened (quest-gate lock)"),
              AMythicToggleable::DoesKeyOpenLock(Probe, FGameplayTag()));
    FGameplayTagContainer Empty;
    TestFalse(TEXT("a keyless probe opens nothing"), AMythicToggleable::DoesKeyOpenLock(Empty, LockKey));

    // PlanKeyedUnlock(bLocked, bHasMatchingKey, bConsumeKey)
    {
        const FMythicUnlockOutcome O = AMythicToggleable::PlanKeyedUnlock(true, true, false);
        TestTrue(TEXT("locked + key → unlock"), O.bUnlock);
        TestFalse(TEXT("reusable key not consumed"), O.bConsumeKey);
    }
    {
        const FMythicUnlockOutcome O = AMythicToggleable::PlanKeyedUnlock(true, true, true);
        TestTrue(TEXT("locked + key (consume) → unlock"), O.bUnlock);
        TestTrue(TEXT("single-use key consumed"), O.bConsumeKey);
    }
    {
        const FMythicUnlockOutcome O = AMythicToggleable::PlanKeyedUnlock(true, false, true);
        TestFalse(TEXT("locked + no key → stays locked"), O.bUnlock);
        TestFalse(TEXT("no key → nothing consumed"), O.bConsumeKey);
    }
    {
        const FMythicUnlockOutcome O = AMythicToggleable::PlanKeyedUnlock(false, true, true);
        TestFalse(TEXT("already unlocked → no unlock action"), O.bUnlock);
        TestFalse(TEXT("already unlocked → no key consumed"), O.bConsumeKey);
    }

    return true;
}
