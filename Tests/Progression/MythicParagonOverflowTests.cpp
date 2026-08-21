
#include "Misc/AutomationTest.h"
#include "Player/Proficiency/ProficiencyComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicParagonOverflowTest,
    "Mythic.Progression.ParagonOverflow",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicParagonOverflowTest::RunTest(const FString &Parameters) {
    using C = UProficiencyComponent;

    TestEqual(TEXT("MaxXP 0 (uninitialized) forwards nothing"), C::ComputeXpOverflow(0.0f, 100.0f, 0.0f), 0.0f);
    TestEqual(TEXT("negative MaxXP forwards nothing"), C::ComputeXpOverflow(50.0f, 100.0f, -1.0f), 0.0f);

    TestEqual(TEXT("grant lands under the cap"), C::ComputeXpOverflow(100.0f, 50.0f, 1000.0f), 0.0f);
    TestEqual(TEXT("grant lands exactly on the cap"), C::ComputeXpOverflow(950.0f, 50.0f, 1000.0f), 0.0f);

    TestEqual(TEXT("partial overflow forwards only the excess"), C::ComputeXpOverflow(980.0f, 50.0f, 1000.0f), 30.0f);

    TestEqual(TEXT("already capped forwards the whole grant"), C::ComputeXpOverflow(1000.0f, 25.0f, 1000.0f), 25.0f);
    TestEqual(TEXT("already over cap forwards the whole grant"), C::ComputeXpOverflow(1200.0f, 25.0f, 1000.0f), 25.0f);

    TestEqual(TEXT("zero grant forwards nothing"), C::ComputeXpOverflow(1000.0f, 0.0f, 1000.0f), 0.0f);
    TestEqual(TEXT("negative grant forwards nothing"), C::ComputeXpOverflow(1000.0f, -50.0f, 1000.0f), 0.0f);

    TestTrue(TEXT("overflow is never negative"), C::ComputeXpOverflow(0.0f, 1.0f, 1000.0f) >= 0.0f);

    return true;
}
