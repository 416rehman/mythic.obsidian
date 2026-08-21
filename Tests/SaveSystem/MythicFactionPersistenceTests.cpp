
#include "Misc/AutomationTest.h"
#include "Subsystem/SaveSystem/Character/SavedFactionStanding.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFactionPersistenceTest,
    "Mythic.SaveSystem.FactionPersistence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFactionPersistenceTest::RunTest(const FString &Parameters) {
    using H = FSerializedFactionStandingHelper;

    TestFalse(TEXT("invalid index (sentinel), positive value"), H::ShouldPersist(FMythicFactionId::InvalidIndex, 50.0f));
    TestFalse(TEXT("invalid index (sentinel), negative value"), H::ShouldPersist(FMythicFactionId::InvalidIndex, -25.0f));
    TestFalse(TEXT("invalid index (sentinel), zero value"), H::ShouldPersist(FMythicFactionId::InvalidIndex, 0.0f));

    TestFalse(TEXT("valid index (0), zero value"), H::ShouldPersist(0, 0.0f));
    TestFalse(TEXT("valid index (3), zero value"), H::ShouldPersist(3, 0.0f));

    TestTrue(TEXT("valid index (0), positive value"), H::ShouldPersist(0, 50.0f));
    TestTrue(TEXT("valid index (3), negative value"), H::ShouldPersist(3, -25.0f));
    TestTrue(TEXT("valid index (7), small non-zero value"), H::ShouldPersist(7, -0.01f));

    return true;
}
