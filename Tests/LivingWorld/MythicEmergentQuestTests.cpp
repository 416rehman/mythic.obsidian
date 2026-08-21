
#include "Misc/AutomationTest.h"
#include "World/LivingWorld/EmergentQuests/MythicEmergentQuestRules.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"

namespace {
    FMythicEmergentQuestRule MakeRule(const FGameplayTag &EventTag, float MinSig, const FGameplayTag &Kind,
                                      EMythicFactionRelation Req = EMythicFactionRelation::Hostile) {
        FMythicEmergentQuestRule R;
        FGameplayTagContainer C;
        C.AddTag(EventTag);
        R.EventTagQuery = FGameplayTagQuery::MakeQuery_MatchAnyTags(C);
        R.MinSignificance = MinSig;
        R.QuestKind = Kind;
        R.ReqPlayerRelationToPrimary = Req;
        return R;
    }

    FMythicWorldEventSnapshot MakeSnap(const FGameplayTag &EventTag, float Significance,
                                       EMythicFactionRelation Relation = EMythicFactionRelation::Allied) {
        FMythicWorldEventSnapshot S;
        S.EventTag = EventTag;
        S.Significance = Significance;
        S.PlayerRelation = Relation;
        return S;
    }
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEmergentQuestSelectTest,
    "Mythic.LivingWorld.EmergentQuest.SelectRule",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEmergentQuestSelectTest::RunTest(const FString &Parameters) {
    const FGameplayTag EventTag = TAG_LIVINGWORLD_EVENT_DIPLOMACY_SHIFT;
    const FGameplayTag OtherTag = TAG_LIVINGWORLD_EVENT_FACTION_FAMINE;
    const FGameplayTag KindA = TAG_WORLD_EVENT_DEATH_PERMANENT;
    const FGameplayTag KindB = TAG_LIVINGWORLD_EVENT_SCHEME_COMPLETED;

    {
        TArray<FMythicEmergentQuestRule> Rules;
        Rules.Add(MakeRule(OtherTag, 0.0f, KindA));
        Rules.Add(MakeRule(EventTag, 0.3f, KindB));
        const FMythicWorldEventSnapshot Snap = MakeSnap(EventTag, 0.5f);
        TestEqual(TEXT("matching event → correct index"),
                  MythicEmergentQuestRules::SelectQuestRuleForEvent(Snap, Rules, {}, 0), 1);
    }

    {
        TArray<FMythicEmergentQuestRule> Rules;
        Rules.Add(MakeRule(EventTag, 0.5f, KindA));
        const FMythicWorldEventSnapshot Snap = MakeSnap(EventTag, 0.2f);
        TestEqual(TEXT("below-significance → INDEX_NONE"),
                  MythicEmergentQuestRules::SelectQuestRuleForEvent(Snap, Rules, {}, 0), (int32)INDEX_NONE);
    }

    {
        TArray<FMythicEmergentQuestRule> Rules;
        Rules.Add(MakeRule(OtherTag, 0.0f, KindA));
        const FMythicWorldEventSnapshot Snap = MakeSnap(EventTag, 1.0f);
        TestEqual(TEXT("non-matching event tag → INDEX_NONE"),
                  MythicEmergentQuestRules::SelectQuestRuleForEvent(Snap, Rules, {}, 0), (int32)INDEX_NONE);
    }

    {
        TArray<FMythicEmergentQuestRule> Rules;
        Rules.Add(MakeRule(EventTag, 0.0f, KindA));
        const FMythicWorldEventSnapshot Snap = MakeSnap(EventTag, 1.0f);
        TArray<FGameplayTag> Active;
        Active.Add(KindA);
        TestEqual(TEXT("active kind → excluded (INDEX_NONE)"),
                  MythicEmergentQuestRules::SelectQuestRuleForEvent(Snap, Rules, Active, 0), (int32)INDEX_NONE);
        TArray<FGameplayTag> OtherActive;
        OtherActive.Add(KindB);
        TestEqual(TEXT("a different active kind does not exclude"),
                  MythicEmergentQuestRules::SelectQuestRuleForEvent(Snap, Rules, OtherActive, 0), 0);
    }

    {
        TArray<FMythicEmergentQuestRule> Rules;
        Rules.Add(MakeRule(EventTag, 0.0f, KindA, EMythicFactionRelation::Friendly));
        TestEqual(TEXT("Neutral player fails a Friendly-gated rule"),
                  MythicEmergentQuestRules::SelectQuestRuleForEvent(MakeSnap(EventTag, 1.0f, EMythicFactionRelation::Neutral), Rules, {}, 0),
                  (int32)INDEX_NONE);
        TestEqual(TEXT("Friendly player passes"),
                  MythicEmergentQuestRules::SelectQuestRuleForEvent(MakeSnap(EventTag, 1.0f, EMythicFactionRelation::Friendly), Rules, {}, 0), 0);
        TestEqual(TEXT("Allied (friendlier) player passes"),
                  MythicEmergentQuestRules::SelectQuestRuleForEvent(MakeSnap(EventTag, 1.0f, EMythicFactionRelation::Allied), Rules, {}, 0), 0);
        TArray<FMythicEmergentQuestRule> Ungated;
        Ungated.Add(MakeRule(EventTag, 0.0f, KindA));
        TestEqual(TEXT("default (Hostile) gate admits a Hostile player"),
                  MythicEmergentQuestRules::SelectQuestRuleForEvent(MakeSnap(EventTag, 1.0f, EMythicFactionRelation::Hostile), Ungated, {}, 0), 0);
    }

    {
        TArray<FMythicEmergentQuestRule> Rules;
        Rules.Add(MakeRule(EventTag, 0.0f, KindA));
        Rules.Add(MakeRule(EventTag, 0.0f, KindB));
        const FMythicWorldEventSnapshot Snap = MakeSnap(EventTag, 1.0f);
        for (uint32 Seed = 0; Seed < 6; ++Seed) {
            const int32 First = MythicEmergentQuestRules::SelectQuestRuleForEvent(Snap, Rules, {}, Seed);
            const int32 Second = MythicEmergentQuestRules::SelectQuestRuleForEvent(Snap, Rules, {}, Seed);
            TestEqual(FString::Printf(TEXT("same snapshot+seed %u → same pick"), Seed), First, Second);
            TestTrue(TEXT("pick is a valid candidate index"), First == 0 || First == 1);
        }
        TestEqual(TEXT("seed 0 → candidate 0"), MythicEmergentQuestRules::SelectQuestRuleForEvent(Snap, Rules, {}, 0), 0);
        TestEqual(TEXT("seed 1 → candidate 1"), MythicEmergentQuestRules::SelectQuestRuleForEvent(Snap, Rules, {}, 1), 1);
        TestEqual(TEXT("seed 2 → wraps to candidate 0"), MythicEmergentQuestRules::SelectQuestRuleForEvent(Snap, Rules, {}, 2), 0);
    }

    {
        const FMythicWorldEventSnapshot Snap = MakeSnap(EventTag, 1.0f);
        TestEqual(TEXT("empty rule pool → INDEX_NONE"),
                  MythicEmergentQuestRules::SelectQuestRuleForEvent(Snap, {}, {}, 0), (int32)INDEX_NONE);
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEmergentQuestRewardTest,
    "Mythic.LivingWorld.EmergentQuest.Reward",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEmergentQuestRewardTest::RunTest(const FString &Parameters) {
    FMythicEmergentQuestRule Rule;
    Rule.BaseCount = 2;
    Rule.RewardTierMultiplier = 1.0f;

    {
        const int32 C0 = MythicEmergentQuestRules::ComputeEmergentReward(Rule, 0, 0.0f).QuestCount;
        const int32 C1 = MythicEmergentQuestRules::ComputeEmergentReward(Rule, 1, 0.0f).QuestCount;
        const int32 C2 = MythicEmergentQuestRules::ComputeEmergentReward(Rule, 2, 0.0f).QuestCount;
        TestTrue(TEXT("count non-decreasing in danger (0<=1)"), C0 <= C1);
        TestTrue(TEXT("count non-decreasing in danger (1<=2)"), C1 <= C2);
        TestTrue(TEXT("count always >= 1"), C0 >= 1);
        TestEqual(TEXT("count = base + danger"), C2, Rule.BaseCount + 2);
    }

    {
        const int32 T0 = MythicEmergentQuestRules::ComputeEmergentReward(Rule, 0, 0.0f).RewardTier;
        const int32 T1 = MythicEmergentQuestRules::ComputeEmergentReward(Rule, 1, 0.0f).RewardTier;
        const int32 T2 = MythicEmergentQuestRules::ComputeEmergentReward(Rule, 2, 0.0f).RewardTier;
        TestTrue(TEXT("tier increasing in danger (0<1)"), T0 < T1);
        TestTrue(TEXT("tier increasing in danger (1<2)"), T1 < T2);
    }

    {
        const int32 S0 = MythicEmergentQuestRules::ComputeEmergentReward(Rule, 1, 0.0f).RewardTier;
        const int32 S1 = MythicEmergentQuestRules::ComputeEmergentReward(Rule, 1, 1.0f).RewardTier;
        TestTrue(TEXT("tier non-decreasing in faction strength"), S0 <= S1);
        TestTrue(TEXT("tier strictly increases with a full-strength faction"), S1 > S0);
    }

    {
        FMythicEmergentQuestRule Zero;
        Zero.RewardTierMultiplier = 0.0f;
        TestEqual(TEXT("zero multiplier → tier 0 (high danger/strength)"),
                  MythicEmergentQuestRules::ComputeEmergentReward(Zero, 4, 1.0f).RewardTier, 0);
    }

    {
        FMythicEmergentQuestRule Default;
        const FMythicEmergentReward R = MythicEmergentQuestRules::ComputeEmergentReward(Default, 0, 0.0f);
        TestTrue(TEXT("default rule count >= 1"), R.QuestCount >= 1);
        TestTrue(TEXT("default rule tier >= 0"), R.RewardTier >= 0);
        const FMythicEmergentReward Neg = MythicEmergentQuestRules::ComputeEmergentReward(Default, -5, -5.0f);
        TestTrue(TEXT("negative inputs clamp: count >= 1"), Neg.QuestCount >= 1);
        TestTrue(TEXT("negative inputs clamp: tier >= 0"), Neg.RewardTier >= 0);
    }

    return true;
}
