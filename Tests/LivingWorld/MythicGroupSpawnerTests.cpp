
#include "Misc/AutomationTest.h"
#include "Mass/Processors/GroupSpawnerProcessor.h"
#include "World/LivingWorld/Groups/GroupTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"

namespace {
    FMythicFactionData MakeStrongFaction() {
        FMythicFactionData F;
        F.bAlive = true;
        F.Status = EMythicFactionStatus::Active;
        F.MilitaryStrength = 1.0f;
        F.Population = 500;
        F.Reserves.Wealth = 1000.0f;
        return F;
    }
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicGroupDefaultsWellFormedTest,
    "Mythic.LivingWorld.Groups.Defaults.WellFormed",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicGroupDefaultsWellFormedTest::RunTest(const FString &Parameters) {
    TArray<FMythicGroupTemplate> Templates;
    MythicGroupDefaults::BuildDefaultTemplates(Templates);

    TestTrue(TEXT("Code defaults produce at least one template"), Templates.Num() > 0);

    for (const FMythicGroupTemplate &T : Templates) {
        TestTrue(TEXT("Template has a valid group tag"), T.GroupTag.IsValid());
        TestTrue(TEXT("Template has at least one member spec"), T.Members.Num() > 0);
        TestTrue(TEXT("Template weight is positive"), T.RelativeWeight > 0.0f);

        int32 MaxPossible = 0;
        int32 LeaderSpecs = 0;
        for (const FMythicGroupMemberSpec &S : T.Members) {
            TestTrue(TEXT("Member spec has a valid role"), S.RoleTag.IsValid());
            TestTrue(TEXT("Member MinCount <= MaxCount"), S.MinCount <= S.MaxCount);
            TestTrue(TEXT("Member MinCount >= 1"), S.MinCount >= 1);
            MaxPossible += S.MaxCount;
            if (S.bIsLeader) {
                ++LeaderSpecs;
            }
        }
        TestTrue(TEXT("Template max member count <= MaxGroupMembers"), MaxPossible <= MaxGroupMembers);
        TestTrue(TEXT("Template has at most one leader spec"), LeaderSpecs <= 1);
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicGroupTemplateEligibilityTest,
    "Mythic.LivingWorld.Groups.TemplateEligible.Gates",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicGroupTemplateEligibilityTest::RunTest(const FString &Parameters) {
    FMythicGroupTemplate Retinue;
    Retinue.GroupTag = TAG_NPC_GROUP_RETINUE;
    Retinue.RelativeWeight = 1.0f;
    Retinue.MinFactionMilitaryStrength = 0.4f;
    Retinue.MinFactionPopulation = 30;
    Retinue.MinReserveWealth = 50.0f;
    FMythicGroupMemberSpec Lead;
    Lead.RoleTag = TAG_NPC_ROLE_NOBLE;
    Lead.bIsLeader = true;
    Retinue.Members.Add(Lead);

    const FMythicFactionData Strong = MakeStrongFaction();
    TestTrue(TEXT("Strong faction can field the retinue"),
             UMythicGroupSpawnerProcessor::TemplateEligible(Retinue, Strong, EMythicSettlementEconomy::Generic));

    {
        FMythicFactionData Weak = Strong;
        Weak.MilitaryStrength = 0.1f;
        TestFalse(TEXT("Weak military fails the retinue gate"),
                  UMythicGroupSpawnerProcessor::TemplateEligible(Retinue, Weak, EMythicSettlementEconomy::Generic));
    }
    {
        FMythicFactionData Tiny = Strong;
        Tiny.Population = 5;
        TestFalse(TEXT("Low population fails the retinue gate"),
                  UMythicGroupSpawnerProcessor::TemplateEligible(Retinue, Tiny, EMythicSettlementEconomy::Generic));
    }
    {
        FMythicFactionData Poor = Strong;
        Poor.Reserves.Wealth = 1.0f;
        TestFalse(TEXT("Low reserves fails the retinue gate"),
                  UMythicGroupSpawnerProcessor::TemplateEligible(Retinue, Poor, EMythicSettlementEconomy::Generic));
    }

    {
        FMythicGroupTemplate TradeOnly = Retinue;
        TradeOnly.MinFactionMilitaryStrength = 0.0f;
        TradeOnly.MinFactionPopulation = 0;
        TradeOnly.MinReserveWealth = 0.0f;
        TradeOnly.AllowedEconomies.Add(EMythicSettlementEconomy::Trade);
        TestFalse(TEXT("Trade-only template rejected in a Farming cell"),
                  UMythicGroupSpawnerProcessor::TemplateEligible(TradeOnly, Strong, EMythicSettlementEconomy::Farming));
        TestTrue(TEXT("Trade-only template accepted in a Trade cell"),
                 UMythicGroupSpawnerProcessor::TemplateEligible(TradeOnly, Strong, EMythicSettlementEconomy::Trade));
    }

    {
        FMythicGroupTemplate Zero = Retinue;
        Zero.RelativeWeight = 0.0f;
        TestFalse(TEXT("Zero-weight template is ineligible"),
                  UMythicGroupSpawnerProcessor::TemplateEligible(Zero, Strong, EMythicSettlementEconomy::Generic));

        FMythicGroupTemplate Empty = Retinue;
        Empty.Members.Reset();
        TestFalse(TEXT("Member-less template is ineligible"),
                  UMythicGroupSpawnerProcessor::TemplateEligible(Empty, Strong, EMythicSettlementEconomy::Generic));
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicGroupPickTemplateTest,
    "Mythic.LivingWorld.Groups.PickTemplate.EligibleDeterministic",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicGroupPickTemplateTest::RunTest(const FString &Parameters) {
    TArray<FMythicGroupTemplate> Templates;
    MythicGroupDefaults::BuildDefaultTemplates(Templates);

    const FMythicFactionData Strong = MakeStrongFaction();

    bool bSawRetinue = false;
    bool bSawSocial = false;
    for (uint32 i = 0; i < 256; ++i) {
        const uint32 Seed = (i + 1) * 2654435761u;
        int32 Index = INDEX_NONE;
        const bool bPicked =
            UMythicGroupSpawnerProcessor::PickTemplateIndex(Templates, EMythicSettlementEconomy::Generic, Strong, Seed, Index);
        TestTrue(TEXT("Pick succeeds for a fully-eligible faction"), bPicked);
        TestTrue(TEXT("Picked index is in range"), Templates.IsValidIndex(Index));
        if (Templates.IsValidIndex(Index)) {
            bSawRetinue |= (Templates[Index].GroupTag == TAG_NPC_GROUP_RETINUE);
            bSawSocial |= (Templates[Index].GroupTag == TAG_NPC_GROUP_SOCIAL);
        }
    }
    TestTrue(TEXT("Weighted pick reaches the retinue template over the seed sweep"), bSawRetinue);
    TestTrue(TEXT("Weighted pick reaches the social template over the seed sweep"), bSawSocial);

    int32 A = INDEX_NONE;
    int32 B = INDEX_NONE;
    UMythicGroupSpawnerProcessor::PickTemplateIndex(Templates, EMythicSettlementEconomy::Generic, Strong, 12345u, A);
    UMythicGroupSpawnerProcessor::PickTemplateIndex(Templates, EMythicSettlementEconomy::Generic, Strong, 12345u, B);
    TestEqual(TEXT("PickTemplateIndex is deterministic"), A, B);

    FMythicFactionData Destitute;
    Destitute.bAlive = true;
    Destitute.Status = EMythicFactionStatus::Active;
    Destitute.MilitaryStrength = 0.0f;
    Destitute.Population = 0;
    Destitute.Reserves.Wealth = 0.0f;
    for (uint32 Seed = 0; Seed < 64; ++Seed) {
        int32 Index = INDEX_NONE;
        const bool bPicked =
            UMythicGroupSpawnerProcessor::PickTemplateIndex(Templates, EMythicSettlementEconomy::Generic, Destitute, Seed, Index);
        TestTrue(TEXT("Destitute faction still picks the always-eligible trio"), bPicked);
        if (Templates.IsValidIndex(Index)) {
            TestTrue(TEXT("Destitute faction can only field the friend trio"),
                     Templates[Index].GroupTag == TAG_NPC_GROUP_SOCIAL);
        }
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicGroupRollMemberCountTest,
    "Mythic.LivingWorld.Groups.RollMemberCount.RangeDeterministic",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicGroupRollMemberCountTest::RunTest(const FString &Parameters) {
    FMythicGroupMemberSpec Spec;
    Spec.RoleTag = TAG_NPC_ROLE_GUARD;
    Spec.MinCount = 2;
    Spec.MaxCount = 4;

    bool bSawMin = false;
    bool bSawMax = false;
    for (uint32 Seed = 0; Seed < 512; ++Seed) {
        const int32 Count = UMythicGroupSpawnerProcessor::RollMemberCount(Spec, Seed);
        TestTrue(TEXT("Rolled count >= MinCount"), Count >= Spec.MinCount);
        TestTrue(TEXT("Rolled count <= MaxCount"), Count <= Spec.MaxCount);
        bSawMin |= (Count == Spec.MinCount);
        bSawMax |= (Count == Spec.MaxCount);

        TestEqual(TEXT("RollMemberCount is deterministic"), Count,
                  UMythicGroupSpawnerProcessor::RollMemberCount(Spec, Seed));
    }
    TestTrue(TEXT("Roll reaches the minimum across the seed sweep"), bSawMin);
    TestTrue(TEXT("Roll reaches the maximum across the seed sweep"), bSawMax);

    FMythicGroupMemberSpec Fixed;
    Fixed.RoleTag = TAG_NPC_ROLE_NOBLE;
    Fixed.MinCount = 1;
    Fixed.MaxCount = 1;
    TestEqual(TEXT("Min==Max yields exactly that count"),
              UMythicGroupSpawnerProcessor::RollMemberCount(Fixed, 999u), 1);

    return true;
}
