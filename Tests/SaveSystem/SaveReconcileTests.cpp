// Mythic — World save reconciliation unit tests
// Covers FSerializedWorldActorHelper::ShouldDestroyOnReconcile — the pure subtractive-reconciliation decision that
// decides whether a live actor is a stale orphan to destroy after a world load. The bug this locks: a runtime actor
// RESPAWNED by the load (cross-session, fresh path-name id absent from the saved id-set) must NOT be destroyed.
// Run via: Session Frontend → Automation → Mythic.SaveSystem.Reconcile

#include "Misc/AutomationTest.h"
#include "Subsystem/SaveSystem/World/SavedWorldActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveReconcileDecisionTest,
    "Mythic.SaveSystem.Reconcile.ShouldDestroy",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveReconcileDecisionTest::RunTest(const FString &Parameters) {
    using FHelper = FSerializedWorldActorHelper;

    // Level-placed actors (not runtime-spawned) are NEVER reconciled away, regardless of the other flags.
    TestFalse(TEXT("placed actor kept (in save)"), FHelper::ShouldDestroyOnReconcile(false, false, true));
    TestFalse(TEXT("placed actor kept (absent from save)"), FHelper::ShouldDestroyOnReconcile(false, false, false));

    // A runtime actor SPAWNED by this load is the restored state — never destroy it, even though (cross-session) its
    // fresh path-name id is absent from the saved id-set. THIS is the bug being locked.
    TestFalse(TEXT("just-spawned runtime actor is exempt even when absent from save"),
              FHelper::ShouldDestroyOnReconcile(true, true, false));
    TestFalse(TEXT("just-spawned runtime actor is exempt when present too"),
              FHelper::ShouldDestroyOnReconcile(true, true, true));

    // A pre-existing runtime actor the save knows about is kept.
    TestFalse(TEXT("pre-existing runtime actor present in save is kept"),
              FHelper::ShouldDestroyOnReconcile(true, false, true));

    // A pre-existing runtime actor ABSENT from the save is a stale orphan -> destroy (the legitimate purpose).
    TestTrue(TEXT("pre-existing runtime orphan absent from save is destroyed"),
             FHelper::ShouldDestroyOnReconcile(true, false, false));

    return true;
}
