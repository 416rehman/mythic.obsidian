
#include "Misc/AutomationTest.h"
#include "Subsystem/SaveSystem/World/SavedWorldActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveReconcileDecisionTest,
    "Mythic.SaveSystem.Reconcile.ShouldDestroy",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSaveReconcileDecisionTest::RunTest(const FString &Parameters) {
    using FHelper = FSerializedWorldActorHelper;

    TestFalse(TEXT("placed actor kept (in save)"), FHelper::ShouldDestroyOnReconcile(false, false, true));
    TestFalse(TEXT("placed actor kept (absent from save)"), FHelper::ShouldDestroyOnReconcile(false, false, false));

    TestFalse(TEXT("just-spawned runtime actor is exempt even when absent from save"),
              FHelper::ShouldDestroyOnReconcile(true, true, false));
    TestFalse(TEXT("just-spawned runtime actor is exempt when present too"),
              FHelper::ShouldDestroyOnReconcile(true, true, true));

    TestFalse(TEXT("pre-existing runtime actor present in save is kept"),
              FHelper::ShouldDestroyOnReconcile(true, false, true));

    TestTrue(TEXT("pre-existing runtime orphan absent from save is destroyed"),
             FHelper::ShouldDestroyOnReconcile(true, false, false));

    return true;
}
