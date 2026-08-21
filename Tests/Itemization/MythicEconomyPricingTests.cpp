
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "Itemization/Vendor/MythicEconomyPricing.h"
#include "Itemization/MythicTags_Inventory.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEconomyScarcityMultiplierTest,
    "Mythic.Itemization.EconomyPricing.ScarcityMultiplier",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEconomyScarcityMultiplierTest::RunTest(const FString &Parameters) {
    using P = FMythicEconomyPricing;
    using Axis = EMythicEconomyAxis;

    FMythicEconomyPricingParams Params;
    Params.Elasticity = 0.1f;
    Params.MinBand = 0.75f;
    Params.MaxBand = 1.5f;
    Params.ReserveReference = 100.0f;

    const float Famine = P::ComputeScarcityMultiplier( -50.0f, 80.0f, 3.0f, Axis::Food, Params);
    TestEqual(TEXT("famine multiplier"), Famine, 1.43f, 0.0001f);
    TestTrue(TEXT("famine raises price above base"), Famine > 1.0f);

    const float MoreDemand = P::ComputeScarcityMultiplier(-50.0f, 120.0f, 3.0f, Axis::Food, Params);
    TestTrue(TEXT("higher demand -> higher multiplier"), MoreDemand > Famine);
    const float HigherPrice = P::ComputeScarcityMultiplier(-50.0f, 80.0f, 4.0f, Axis::Food, Params);
    TestTrue(TEXT("higher price -> higher multiplier"), HigherPrice > Famine);
    const float LowerReserves = P::ComputeScarcityMultiplier(-120.0f, 80.0f, 3.0f, Axis::Food, Params);
    TestTrue(TEXT("lower reserves -> higher multiplier"), LowerReserves > Famine);

    const float Surplus = P::ComputeScarcityMultiplier( 200.0f, 10.0f, 0.5f, Axis::Materials, Params);
    TestEqual(TEXT("surplus multiplier"), Surplus, 0.86f, 0.0001f);
    TestTrue(TEXT("surplus drops price below base"), Surplus < 1.0f);

    FMythicEconomyPricingParams Inert = Params;
    Inert.Elasticity = 0.0f;
    TestEqual(TEXT("Elasticity 0 -> exactly 1.0"), P::ComputeScarcityMultiplier(-500.0f, 999.0f, 50.0f, Axis::Food, Inert), 1.0f);

    TestEqual(TEXT("Axis None -> 1.0"), P::ComputeScarcityMultiplier(-500.0f, 999.0f, 50.0f, Axis::None, Params), 1.0f);

    TestEqual(TEXT("no market signal -> 1.0"), P::ComputeScarcityMultiplier(300.0f, 0.0f, 0.0f, Axis::Food, Params), 1.0f);

    FMythicEconomyPricingParams Hard = Params;
    Hard.Elasticity = 1.0f;
    const float Clamped = P::ComputeScarcityMultiplier(-1000.0f, 1000.0f, 50.0f, Axis::Food, Hard);
    TestEqual(TEXT("extreme famine clamps to MaxBand"), Clamped, Hard.MaxBand, 0.0001f);
    const float ClampedLow = P::ComputeScarcityMultiplier(100000.0f, 1.0f, 0.01f, Axis::Materials, Hard);
    TestEqual(TEXT("extreme surplus clamps to MinBand"), ClampedLow, Hard.MinBand, 0.0001f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEconomySellPressureTest,
    "Mythic.Itemization.EconomyPricing.SellPressure",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEconomySellPressureTest::RunTest(const FString &Parameters) {
    using P = FMythicEconomyPricing;

    const float Base = 0.4f;
    const float Floor = 0.1f;
    const float PerUnit = 0.05f;

    TestEqual(TEXT("zero units -> unchanged"), P::ApplyLocalSellPressure(Base, 0.0f, PerUnit, Floor), Base);

    TestEqual(TEXT("feature off -> unchanged"), P::ApplyLocalSellPressure(Base, 500.0f, 0.0f, Floor), Base);

    const float P1 = P::ApplyLocalSellPressure(Base, 1.0f, PerUnit, Floor);
    const float P5 = P::ApplyLocalSellPressure(Base, 5.0f, PerUnit, Floor);
    const float P20 = P::ApplyLocalSellPressure(Base, 20.0f, PerUnit, Floor);
    TestTrue(TEXT("1 unit already below base"), P1 < Base);
    TestTrue(TEXT("monotonic: 5 units < 1 unit"), P5 < P1);
    TestTrue(TEXT("monotonic: 20 units < 5 units"), P20 < P5);

    const float Flood = P::ApplyLocalSellPressure(Base, 100000.0f, PerUnit, Floor);
    TestTrue(TEXT("flood never drops below floor"), Flood >= Floor);
    TestTrue(TEXT("flood approaches the floor"), Flood <= Floor + 0.001f);

    TestEqual(TEXT("base below floor -> unchanged"), P::ApplyLocalSellPressure(0.05f, 500.0f, PerUnit, Floor), 0.05f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEconomyAxisForItemTest,
    "Mythic.Itemization.EconomyPricing.AxisForItem",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEconomyAxisForItemTest::RunTest(const FString &Parameters) {
    using P = FMythicEconomyPricing;
    using Axis = EMythicEconomyAxis;

    TMap<FGameplayTag, Axis> Map;
    Map.Add(ITEMIZATION_TYPE_CONSUMABLE_FOOD.GetTag(), Axis::Food);
    Map.Add(ITEMIZATION_TYPE_MINING.GetTag(), Axis::Materials);
    Map.Add(ITEMIZATION_TYPE_EQUIPMENT_WEAPON.GetTag(), Axis::Arms);
    Map.Add(ITEMIZATION_TYPE_CURRENCY.GetTag(), Axis::Wealth);

    FGameplayTagContainer FoodTags;
    FoodTags.AddTag(ITEMIZATION_TYPE_CONSUMABLE_FOOD.GetTag());
    TestEqual(TEXT("food type tag -> Food"), P::AxisForItem(FoodTags, Map), Axis::Food);

    FGameplayTagContainer OreTags;
    OreTags.AddTag(ITEMIZATION_TYPE_MINING_ORE.GetTag());
    TestEqual(TEXT("ore (child of Mining) -> Materials"), P::AxisForItem(OreTags, Map), Axis::Materials);

    FGameplayTagContainer UnmappedTags;
    UnmappedTags.AddTag(ITEMIZATION_TYPE_LEARNING_TOME.GetTag());
    TestEqual(TEXT("unmapped type -> None"), P::AxisForItem(UnmappedTags, Map), Axis::None);

    FGameplayTagContainer Empty;
    TestEqual(TEXT("empty container -> None"), P::AxisForItem(Empty, Map), Axis::None);
    TMap<FGameplayTag, Axis> EmptyMap;
    TestEqual(TEXT("empty map -> None"), P::AxisForItem(FoodTags, EmptyMap), Axis::None);

    return true;
}
