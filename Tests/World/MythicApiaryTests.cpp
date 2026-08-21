
#include "Misc/AutomationTest.h"
#include "World/Farming/MythicApiaryRules.h"
#include "World/Farming/MythicFarmingRules.h"
#include "World/Farming/MythicTags_Farming.h"
#include "World/Gathering/MythicYieldQuality.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicApiaryTest,
    "Mythic.World.Apiary",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicApiaryTest::RunTest(const FString &Parameters) {
    using Rules = FMythicApiaryRules;

    {
        const FGameplayTag A = TAG_Crop_Type;
        const FGameplayTag B = TAG_Item_Seed;
        const FGameplayTag C = TAG_Item_Feed;

        TestEqual(TEXT("empty → 0"), Rules::CountDistinctCropTypes(TArray<FGameplayTag>()), 0);
        TestEqual(TEXT("one type"), Rules::CountDistinctCropTypes({A}), 1);
        TestEqual(TEXT("duplicates collapse"), Rules::CountDistinctCropTypes({A, A, A}), 1);
        TestEqual(TEXT("three distinct"), Rules::CountDistinctCropTypes({A, B, C, B, A}), 3);
        TestEqual(TEXT("invalid tags don't count"), Rules::CountDistinctCropTypes({FGameplayTag(), A, FGameplayTag()}), 1);
    }

    {
        const TArray<int32> Ladder = {0, 2, 3};
        TestEqual(TEXT("no crops → plain honey"), Rules::ResolveHoneyVariety(0, Ladder), 0);
        TestEqual(TEXT("1 crop → still plain"), Rules::ResolveHoneyVariety(1, Ladder), 0);
        TestEqual(TEXT("2 distinct → Meadow Blend"), Rules::ResolveHoneyVariety(2, Ladder), 1);
        TestEqual(TEXT("3 distinct → Wildflower Reserve"), Rules::ResolveHoneyVariety(3, Ladder), 2);
        TestEqual(TEXT("5 distinct → still the top row"), Rules::ResolveHoneyVariety(5, Ladder), 2);

        const TArray<int32> Shuffled = {3, 0, 2};
        TestEqual(TEXT("shuffled rows, 3 distinct → the 3-row"), Rules::ResolveHoneyVariety(3, Shuffled), 0);
        TestEqual(TEXT("shuffled rows, 2 distinct → the 2-row"), Rules::ResolveHoneyVariety(2, Shuffled), 2);
        TestEqual(TEXT("shuffled rows, 0 distinct → the 0-row"), Rules::ResolveHoneyVariety(0, Shuffled), 1);

        TestEqual(TEXT("no rows → -1"), Rules::ResolveHoneyVariety(5, TArray<int32>()), -1);
        TestEqual(TEXT("only high rows, low count → -1"), Rules::ResolveHoneyVariety(1, TArray<int32>({2, 3})), -1);
    }

    {
        FMythicProductionAccrual Accrual = Rules::AccrueUnits(0.0f, 3700.0f, 1.0f, 1800.0f, 0, 5);
        TestEqual(TEXT("3700s → 2 units"), Accrual.StoredUnits, 2);
        TestTrue(TEXT("3700s → 100s carryover"), FMath::IsNearlyEqual(Accrual.CarryoverSeconds, 100.0f, 0.01f));

        Accrual = Rules::AccrueUnits(1700.0f, 200.0f, 1.0f, 1800.0f, 0, 5);
        TestEqual(TEXT("carryover completes the unit"), Accrual.StoredUnits, 1);
        TestTrue(TEXT("carryover remainder banks"), FMath::IsNearlyEqual(Accrual.CarryoverSeconds, 100.0f, 0.01f));

        Accrual = Rules::AccrueUnits(0.0f, 604800.0f, 1.0f, 1800.0f, 3, 5);
        TestEqual(TEXT("week away → clamped at cap"), Accrual.StoredUnits, 5);
        TestEqual(TEXT("full store drops the carryover"), Accrual.CarryoverSeconds, 0.0f);

        Accrual = Rules::AccrueUnits(500.0f, 10000.0f, 0.0f, 1800.0f, 1, 5);
        TestEqual(TEXT("paused window → no new units"), Accrual.StoredUnits, 1);
        TestTrue(TEXT("paused window → bank unchanged"), FMath::IsNearlyEqual(Accrual.CarryoverSeconds, 500.0f, 0.01f));

        Accrual = Rules::AccrueUnits(0.0f, 99999.0f, 1.0f, 0.0f, 0, 5);
        TestEqual(TEXT("SecondsPerUnit 0 → nothing"), Accrual.StoredUnits, 0);

        const int32 Short = Rules::AccrueUnits(0.0f, 1000.0f, 1.0f, 1800.0f, 0, 5).StoredUnits;
        const int32 Long = Rules::AccrueUnits(0.0f, 8000.0f, 1.0f, 1800.0f, 0, 5).StoredUnits;
        TestTrue(TEXT("more time never yields less"), Long >= Short);
    }

    {
        TestEqual(TEXT("fresh comb → full interval"), Rules::SecondsToNextUnit(0.0f, 1800.0f, 0, 5), 1800.0f);
        TestEqual(TEXT("600 banked → 1200 to go"), Rules::SecondsToNextUnit(600.0f, 1800.0f, 0, 5), 1200.0f);
        TestEqual(TEXT("full store → nothing to schedule"), Rules::SecondsToNextUnit(0.0f, 1800.0f, 5, 5), 0.0f);
        TestEqual(TEXT("unauthored → nothing to schedule"), Rules::SecondsToNextUnit(0.0f, 0.0f, 0, 5), 0.0f);
    }

    {
        const FMythicFarmingConfig Config;
        const FMythicYieldQualityRules QualityRules;
        FMythicCropQualityInputs Bare;
        FMythicCropQualityInputs Pollinated = Bare;
        Pollinated.PollinationMagnitude = 1.0f;

        TestEqual(TEXT("no hive → Common"), FMythicFarmingRules::ComputeCropQuality(Bare, QualityRules, Config, 0.15f),
                  EMythicYieldQuality::Common);
        TestEqual(TEXT("hive coverage → Fine"), FMythicFarmingRules::ComputeCropQuality(Pollinated, QualityRules, Config, 0.15f),
                  EMythicYieldQuality::Fine);
        TestEqual(TEXT("hive coverage → +1 yield on a low roll"), FMythicFarmingRules::PollinationBonusYield(1.0f, Config, 0.05f), 1);
    }

    return true;
}
