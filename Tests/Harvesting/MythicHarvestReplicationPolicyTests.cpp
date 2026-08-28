#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "HAL/IConsoleManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestWorldPartitionVisibilityHandshakeTest,
    "Mythic.Harvesting.Replication.WorldPartitionVisibilityHandshake",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestWorldPartitionVisibilityHandshakeTest::RunTest(
    const FString &Parameters) {
    const IConsoleVariable *VisibilityTransactions =
        IConsoleManager::Get().FindConsoleVariable(
            TEXT("wp.Runtime.UseMakingVisibleTransactionRequests"));
    TestNotNull(TEXT("World Partition visibility transaction CVar exists"),
                VisibilityTransactions);
    if (VisibilityTransactions) {
        TestTrue(
            TEXT("clients wait for authority to register a streamed cell before exposing it"),
            VisibilityTransactions->GetInt() != 0);
    }
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
