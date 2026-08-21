
#include "Misc/AutomationTest.h"
#include "World/Trading/MythicCargoRisk.h"
#include "Player/FastTravel/MythicFastTravelRules.h"
#include "Itemization/Vendor/MythicVendor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingCargoHeatTest,
    "Mythic.Trading.Risk.CargoHeat",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingCargoHeatTest::RunTest(const FString &Parameters) {
    using namespace MythicCargoRisk;
    const float ValueRef = 2000.0f;

    TestEqual(TEXT("Safe cell → 0"), ComputeCargoHeat(1.0e6f, 0, ValueRef), 0.0f);
    TestEqual(TEXT("Low cell → 0"), ComputeCargoHeat(1.0e6f, 1, ValueRef), 0.0f);
    TestEqual(TEXT("No cargo → 0"), ComputeCargoHeat(0.0f, 4, ValueRef), 0.0f);
    TestEqual(TEXT("Negative cargo guards to 0"), ComputeCargoHeat(-100.0f, 4, ValueRef), 0.0f);

    TestTrue(TEXT("Saturated cargo in Extreme = 1"), FMath::IsNearlyEqual(ComputeCargoHeat(ValueRef, 4, ValueRef), 1.0f, 1e-4f));
    TestTrue(TEXT("Monotonic in value"),
             ComputeCargoHeat(1500.0f, 3, ValueRef) > ComputeCargoHeat(500.0f, 3, ValueRef));
    TestTrue(TEXT("Monotonic in danger"),
             ComputeCargoHeat(1000.0f, 4, ValueRef) > ComputeCargoHeat(1000.0f, 2, ValueRef));
    TestTrue(TEXT("Never above 1"), ComputeCargoHeat(1.0e9f, 99, ValueRef) <= 1.0f);
    TestTrue(TEXT("Never negative"), ComputeCargoHeat(1.0f, 2, ValueRef) >= 0.0f);
    TestTrue(TEXT("Min-tier saturated cargo = 1/3"),
             FMath::IsNearlyEqual(ComputeCargoHeat(ValueRef, 2, ValueRef), 1.0f / 3.0f, 1e-4f));
    TestTrue(TEXT("Degenerate ValueReference guarded"), ComputeCargoHeat(10.0f, 4, 0.0f) <= 1.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingFastTravelCargoGateTest,
    "Mythic.Trading.Risk.FastTravelCargoGate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingFastTravelCargoGateTest::RunTest(const FString &Parameters) {
    using namespace MythicFastTravel;

    TestTrue(TEXT("Eligible + unencumbered → travels"), CanFastTravelWithCargo(true, false));
    TestFalse(TEXT("Eligible + OVERLOADED → walks (the cargo game)"), CanFastTravelWithCargo(true, true));
    TestFalse(TEXT("Ineligible + unencumbered → blocked"), CanFastTravelWithCargo(false, false));
    TestFalse(TEXT("Ineligible + overloaded → blocked"), CanFastTravelWithCargo(false, true));

    TSet<int32> Unlocked;
    Unlocked.Add(3);
    Unlocked.Add(7);
    TestTrue(TEXT("Between unlocked points composes through"),
             CanFastTravelWithCargo(CanFastTravelBetween(Unlocked, 3, 7, false), false));
    TestFalse(TEXT("Wilderness source composes to blocked"),
              CanFastTravelWithCargo(CanFastTravelBetween(Unlocked, INDEX_NONE, 7, false), false));
    TestFalse(TEXT("Unlocked route + overloaded still walks"),
              CanFastTravelWithCargo(CanFastTravelBetween(Unlocked, 3, 7, false), true));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingContrabandPremiumTest,
    "Mythic.Trading.Risk.ContrabandPremium",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingContrabandPremiumTest::RunTest(const FString &Parameters) {
    TestEqual(TEXT("1.25 premium on 100"), AMythicVendor::ApplyContrabandPremium(100, 1.25f), 125);
    TestEqual(TEXT("Rounding (1.25 × 10 = 13)"), AMythicVendor::ApplyContrabandPremium(10, 1.25f), 13);
    TestEqual(TEXT("Premium below 1 clamps to par"), AMythicVendor::ApplyContrabandPremium(100, 0.5f), 100);
    TestEqual(TEXT("Zero payout stays zero"), AMythicVendor::ApplyContrabandPremium(0, 2.0f), 0);
    TestEqual(TEXT("Negative payout clamps to 0"), AMythicVendor::ApplyContrabandPremium(-10, 2.0f), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingHagglingTest,
    "Mythic.Trading.Risk.Haggling",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingHagglingTest::RunTest(const FString &Parameters) {
    TestEqual(TEXT("Buy fold inert at 0/level"), AMythicVendor::ComputeHagglingBuyMultiplier(50, 0.0f), 1.0f);
    TestEqual(TEXT("Sell fold inert at 0/level"), AMythicVendor::ComputeHagglingSellMultiplier(50, 0.0f), 1.0f);
    TestEqual(TEXT("Level 0 buys at par"), AMythicVendor::ComputeHagglingBuyMultiplier(0, 0.01f), 1.0f);
    TestEqual(TEXT("Level 0 sells at par"), AMythicVendor::ComputeHagglingSellMultiplier(0, 0.01f), 1.0f);

    TestTrue(TEXT("Buy discount lowers the multiplier"),
             AMythicVendor::ComputeHagglingBuyMultiplier(10, 0.01f) < 1.0f);
    TestTrue(TEXT("Sell bonus raises the multiplier"),
             AMythicVendor::ComputeHagglingSellMultiplier(10, 0.01f) > 1.0f);
    TestTrue(TEXT("Buy discount floors at the min multiplier"),
             FMath::IsNearlyEqual(AMythicVendor::ComputeHagglingBuyMultiplier(1000, 0.01f, 0.5f), 0.5f, 1e-4f));
    TestTrue(TEXT("Sell bonus ceilings at the max multiplier"),
             FMath::IsNearlyEqual(AMythicVendor::ComputeHagglingSellMultiplier(1000, 0.01f, 1.5f), 1.5f, 1e-4f));

    TestTrue(TEXT("10 levels of 0.5% buys at 95%"),
             FMath::IsNearlyEqual(AMythicVendor::ComputeHagglingBuyMultiplier(10, 0.005f), 0.95f, 1e-4f));
    TestTrue(TEXT("10 levels of 0.5% sells at 105%"),
             FMath::IsNearlyEqual(AMythicVendor::ComputeHagglingSellMultiplier(10, 0.005f), 1.05f, 1e-4f));

    return true;
}
