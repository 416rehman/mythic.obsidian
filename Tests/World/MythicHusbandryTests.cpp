
#include "Misc/AutomationTest.h"
#include "World/Farming/MythicApiaryRules.h"
#include "World/Farming/MythicHusbandryRules.h"
#include "World/Gathering/MythicYieldQuality.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHusbandryTest,
    "Mythic.World.Husbandry",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHusbandryTest::RunTest(const FString &Parameters) {
    using Rules = FMythicHusbandryRules;

    {
        TestEqual(TEXT("Ragged feed floors at Common (Ragged is hunting-only)"),
                  Rules::FeedToProduceTier(EMythicYieldQuality::Ragged), EMythicYieldQuality::Common);
        TestEqual(TEXT("Common feed → Common produce"), Rules::FeedToProduceTier(EMythicYieldQuality::Common), EMythicYieldQuality::Common);
        TestEqual(TEXT("Fine feed → Fine produce"), Rules::FeedToProduceTier(EMythicYieldQuality::Fine), EMythicYieldQuality::Fine);
        TestEqual(TEXT("Pristine feed → Pristine produce"), Rules::FeedToProduceTier(EMythicYieldQuality::Pristine), EMythicYieldQuality::Pristine);

        int32 Prev = -1;
        for (int32 i = 0; i < FMythicYieldQuality::NumTiers; ++i) {
            const int32 Out = FMythicYieldQuality::TierIndex(Rules::FeedToProduceTier(FMythicYieldQuality::TierFromIndex(i)));
            TestTrue(TEXT("produce tier monotonic in feed tier"), Out >= Prev);
            Prev = Out;
        }
    }

    {
        TestEqual(TEXT("fed through the gap → full window"), Rules::FedWindowSeconds(100.0, 400.0, 1000.0), 300.0f);
        TestEqual(TEXT("feed ran out mid-gap → clamped window"), Rules::FedWindowSeconds(100.0, 400.0, 250.0), 150.0f);
        TestEqual(TEXT("unfed → zero window (stalls)"), Rules::FedWindowSeconds(100.0, 400.0, 50.0), 0.0f);
        TestEqual(TEXT("never-fed → zero window"), Rules::FedWindowSeconds(100.0, 400.0, 0.0), 0.0f);
        TestEqual(TEXT("inverted clocks clamp to 0"), Rules::FedWindowSeconds(400.0, 100.0, 1000.0), 0.0f);
    }

    {
        const float Window = Rules::FedWindowSeconds(0.0, 604800.0, 0.0);
        const FMythicProductionAccrual Accrual = FMythicApiaryRules::AccrueUnits(432.0f, Window, 1.0f, 1800.0f, 2, 3);
        TestEqual(TEXT("unfed week → units untouched"), Accrual.StoredUnits, 2);
        TestTrue(TEXT("unfed week → carryover untouched"), FMath::IsNearlyEqual(Accrual.CarryoverSeconds, 432.0f, 0.01f));

        const float FedWindow = Rules::FedWindowSeconds(0.0, 3600.0, 3600.0);
        const FMythicProductionAccrual Fed = FMythicApiaryRules::AccrueUnits(0.0f, FedWindow, 1.0f, 1800.0f, 0, 3);
        TestEqual(TEXT("one feed unit → two eggs"), Fed.StoredUnits, 2);
    }

    {
        TestEqual(TEXT("hungry animal → now + feed span"), Rules::ExtendFedUntil(1000.0, 0.0, 3600.0f, 7200.0f), 4600.0);
        TestEqual(TEXT("fed animal → extends the clock"), Rules::ExtendFedUntil(1000.0, 2000.0, 3600.0f, 7200.0f), 5600.0);
        TestEqual(TEXT("feed-dump caps at the bank"), Rules::ExtendFedUntil(1000.0, 8000.0, 3600.0f, 7200.0f), 8200.0);
        TestEqual(TEXT("cap is now-relative"), Rules::ExtendFedUntil(1000.0, 100000.0, 3600.0f, 7200.0f), 8200.0);
    }

    return true;
}
