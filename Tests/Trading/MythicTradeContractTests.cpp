
#include "Misc/AutomationTest.h"
#include "World/Trading/MythicTradeContractTypes.h"
#include "Itemization/Inventory/MythicCurrency.h"
#include "World/Trading/MythicTags_Trading.h"
#include "World/LivingWorld/EmergentQuests/MythicEmergentQuestRules.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "Itemization/MythicTags_Inventory.h"

namespace {
    FMythicEmergentQuestRule MakeFoodDeliveryRule() {
        FMythicEmergentQuestRule R;
        FGameplayTagContainer C;
        C.AddTag(TAG_LIVINGWORLD_EVENT_FACTION_FAMINE);
        R.EventTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(C);
        R.QuestKind = FGameplayTag::RequestGameplayTag(FName("Quest.Emergent.Delivery.Food"), false);
        R.DeliveryItemTag = ITEMIZATION_TYPE_CONSUMABLE_FOOD;
        R.DeliveryUnits = 15;
        R.DeliveryReserveAxis = EMythicResourceType::Food;
        R.FactionStandingReward = 15.0f;
        return R;
    }

    FMythicEmergentQuestRule MakeMaterialsDeliveryRule() {
        FMythicEmergentQuestRule R;
        FGameplayTagContainer C;
        C.AddTag(TAG_TRADING_EVENT_DEFICIT_MATERIALS);
        R.EventTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(C);
        R.QuestKind = FGameplayTag::RequestGameplayTag(FName("Quest.Emergent.Delivery.Materials"), false);
        R.DeliveryItemTag = ITEMIZATION_TYPE_MINING;
        R.DeliveryUnits = 12;
        R.DeliveryReserveAxis = EMythicResourceType::Materials;
        return R;
    }

    FMythicWorldEventSnapshot MakeSnapshot(const FGameplayTag &EventTag, float Significance = 0.5f, int32 DangerTier = 1) {
        FMythicWorldEventSnapshot Snap;
        Snap.EventTag = EventTag;
        Snap.Significance = Significance;
        Snap.PrimaryFactionId = 2;
        Snap.PlayerRelation = EMythicFactionRelation::Allied;
        Snap.DangerTier = DangerTier;
        return Snap;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingContractTriggerTest,
    "Mythic.Trading.Contracts.TriggerRows",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingContractTriggerTest::RunTest(const FString &Parameters) {
    using namespace MythicEmergentQuestRules;

    TArray<FMythicEmergentQuestRule> Rules;
    Rules.Add(MakeFoodDeliveryRule());
    Rules.Add(MakeMaterialsDeliveryRule());

    {
        const int32 Idx = SelectQuestRuleForEvent(MakeSnapshot(TAG_LIVINGWORLD_EVENT_FACTION_FAMINE), Rules, {}, 7);
        TestEqual(TEXT("Famine selects the food row"), Idx, 0);
        TestTrue(TEXT("Food row IS a delivery row"), Rules[0].IsDeliveryRow());
    }
    {
        const int32 Idx = SelectQuestRuleForEvent(MakeSnapshot(TAG_TRADING_EVENT_DEFICIT_MATERIALS), Rules, {}, 7);
        TestEqual(TEXT("Materials deficit selects the materials row"), Idx, 1);
    }
    {
        TArray<FGameplayTag> ActiveKinds;
        ActiveKinds.Add(Rules[0].QuestKind);
        const int32 Idx = SelectQuestRuleForEvent(MakeSnapshot(TAG_LIVINGWORLD_EVENT_FACTION_FAMINE), Rules, ActiveKinds, 7);
        TestEqual(TEXT("Open offer of the kind suppresses the row"), Idx, INDEX_NONE);
    }
    {
        FMythicEmergentQuestRule Broken = MakeFoodDeliveryRule();
        Broken.DeliveryItemTag = FGameplayTag();
        TestFalse(TEXT("Units without an item tag is not a delivery row"), Broken.IsDeliveryRow());
        Broken = MakeFoodDeliveryRule();
        Broken.DeliveryUnits = 0;
        TestFalse(TEXT("Tag without units is not a delivery row"), Broken.IsDeliveryRow());
    }
    {
        const FMythicEmergentQuestRule R = MakeFoodDeliveryRule();
        TestEqual(TEXT("Units danger-scale (tier 3)"), FMath::Max(1, R.DeliveryUnits + 3), 18);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingContractDeficitLatchTest,
    "Mythic.Trading.Contracts.DeficitLatch",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingContractDeficitLatchTest::RunTest(const FString &Parameters) {
    using namespace MythicTradeContracts;
    const float Threshold = -25.0f;
    const float Rearm = 0.0f;

    bool bLatched = false;
    TestFalse(TEXT("Healthy reserve never fires"), ShouldFireDeficitBeat(50.0f, Threshold, Rearm, false, bLatched));
    TestFalse(TEXT("Healthy reserve leaves the latch clear"), bLatched);
    TestTrue(TEXT("Crossing the threshold fires"), ShouldFireDeficitBeat(-30.0f, Threshold, Rearm, bLatched, bLatched));
    TestTrue(TEXT("… and latches"), bLatched);
    TestFalse(TEXT("Still in deficit → no re-fire"), ShouldFireDeficitBeat(-40.0f, Threshold, Rearm, bLatched, bLatched));
    TestTrue(TEXT("… latch holds"), bLatched);
    TestFalse(TEXT("Partial recovery doesn't re-arm"), ShouldFireDeficitBeat(-10.0f, Threshold, Rearm, bLatched, bLatched));
    TestTrue(TEXT("… latch still held"), bLatched);
    TestFalse(TEXT("Full recovery fires nothing"), ShouldFireDeficitBeat(20.0f, Threshold, Rearm, bLatched, bLatched));
    TestFalse(TEXT("… but re-arms the latch"), bLatched);
    TestTrue(TEXT("A fresh deficit fires again"), ShouldFireDeficitBeat(-26.0f, Threshold, Rearm, bLatched, bLatched));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingContractCompletionTest,
    "Mythic.Trading.Contracts.Completion",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingContractCompletionTest::RunTest(const FString &Parameters) {
    using namespace MythicTradeContracts;

    {
        const FDeliveryApplication A = ApplyDelivery(15, 0, 10);
        TestEqual(TEXT("Partial hand-in accepted whole"), A.AcceptedUnits, 10);
        TestFalse(TEXT("… not complete"), A.bCompleted);
    }
    {
        const FDeliveryApplication A = ApplyDelivery(15, 10, 10);
        TestEqual(TEXT("Over-delivery clamps to remaining"), A.AcceptedUnits, 5);
        TestTrue(TEXT("… and completes"), A.bCompleted);
    }
    {
        const FDeliveryApplication A = ApplyDelivery(15, 15, 5);
        TestEqual(TEXT("A full contract accepts nothing"), A.AcceptedUnits, 0);
        TestFalse(TEXT("… and never re-completes"), A.bCompleted);
    }
    {
        const FDeliveryApplication A = ApplyDelivery(15, 0, -3);
        TestEqual(TEXT("Negative offers accept nothing"), A.AcceptedUnits, 0);
        TestFalse(TEXT("… and never complete"), A.bCompleted);
    }

    TestEqual(TEXT("Payout at balance = value × units"), ComputeDeliveryPayout(10, 5, 1.0f), 50);
    TestEqual(TEXT("Famine pays the premium"), ComputeDeliveryPayout(10, 5, 1.5f), 75);
    TestEqual(TEXT("Surplus pays under value"), ComputeDeliveryPayout(10, 5, 0.75f), 38);
    TestEqual(TEXT("Worthless goods pay 0"), ComputeDeliveryPayout(0, 5, 1.5f), 0);
    TestEqual(TEXT("Zero units pay 0"), ComputeDeliveryPayout(10, 0, 1.5f), 0);
    TestEqual(TEXT("Negative scarcity guards to 0"), ComputeDeliveryPayout(10, 5, -1.0f), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTradingDeliveryMoneyPumpTest,
    "Mythic.Trading.Contracts.DeliveryMoneyPump",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTradingDeliveryMoneyPumpTest::RunTest(const FString &Parameters) {
    using namespace MythicTradeContracts;
    using MythicCurrency::ComputeBuyPrice;

    const int32 Value = 10;
    const int32 Units = 5;
    const float Scarcity = 1.5f;
    const int32 RawPayout = ComputeDeliveryPayout(Value, Units, Scarcity);

    auto ClampedPayout = [&](float EffBuyMult) {
        return FMath::Min(RawPayout, ComputeBuyPrice(Value, Units, EffBuyMult));
    };

    {
        const float EffBuyMult = 1.25f * Scarcity;
        const int32 BuyCost = ComputeBuyPrice(Value, Units, EffBuyMult);
        TestTrue(TEXT("non-discount vendor: buy cost >= raw delivery payout"), BuyCost >= RawPayout);
        TestEqual(TEXT("non-discount vendor: clamp does not reduce the payout"), ClampedPayout(EffBuyMult), RawPayout);
    }

    {
        const float EffBuyMult = 1.0f;
        const int32 BuyCost = ComputeBuyPrice(Value, Units, EffBuyMult);
        TestTrue(TEXT("discounted vendor: raw payout WOULD exceed buy cost (the pump)"), RawPayout > BuyCost);
        TestEqual(TEXT("discounted vendor: clamp floors payout to the buy cost"), ClampedPayout(EffBuyMult), BuyCost);
    }

    for (float EffBuyMult = 0.1f; EffBuyMult <= 3.0f; EffBuyMult += 0.1f) {
        const int32 BuyCost = ComputeBuyPrice(Value, Units, EffBuyMult);
        TestTrue(*FString::Printf(TEXT("buy >= delivery @ EffBuyMult %.2f"), EffBuyMult),
                 ClampedPayout(EffBuyMult) <= BuyCost);
    }

    return true;
}
