
#include "Misc/AutomationTest.h"
#include "World/Death/MythicCemeteryRules.h"
#include "World/Death/MythicEpitaph.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCemeteryTest,
    "Mythic.Death.Cemetery",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCemeteryTest::RunTest(const FString &Parameters) {
    const FGameplayTag NobleRole = TAG_NPC_ROLE_NOBLE.GetTag();
    const FGameplayTag MerchantRole = TAG_NPC_ROLE_MERCHANT.GetTag();
    const FGameplayTag FarmerRole = TAG_NPC_ROLE_FARMER.GetTag();
    const FGameplayTag GuardRole = TAG_NPC_ROLE_GUARD.GetTag();

    {
        FMythicCemeteryConfig Cfg;
        Cfg.NotabilityGate = 3.0f;
        Cfg.NotableRoleTags.AddTag(NobleRole);

        TestTrue(TEXT("noble role → notable"), FMythicCemeteryRules::IsNotableDeath(NobleRole, 0.0f, Cfg));

        TestFalse(TEXT("farmer + low sig → not notable"), FMythicCemeteryRules::IsNotableDeath(FarmerRole, 1.0f, Cfg));
        TestFalse(TEXT("farmer + just-below gate → not notable"), FMythicCemeteryRules::IsNotableDeath(FarmerRole, 2.99f, Cfg));

        TestTrue(TEXT("farmer + at gate → notable"), FMythicCemeteryRules::IsNotableDeath(FarmerRole, 3.0f, Cfg));
        TestTrue(TEXT("farmer + boss sig → notable"), FMythicCemeteryRules::IsNotableDeath(FarmerRole, 5.0f, Cfg));

        TestFalse(TEXT("no role + low sig → not notable"), FMythicCemeteryRules::IsNotableDeath(FGameplayTag(), 0.0f, Cfg));
        TestTrue(TEXT("no role + high sig → notable"), FMythicCemeteryRules::IsNotableDeath(FGameplayTag(), 4.0f, Cfg));

        TestFalse(TEXT("merchant not notable pre-add"), FMythicCemeteryRules::IsNotableDeath(MerchantRole, 0.0f, Cfg));
        Cfg.NotableRoleTags.AddTag(MerchantRole);
        TestTrue(TEXT("merchant notable post-add"), FMythicCemeteryRules::IsNotableDeath(MerchantRole, 0.0f, Cfg));

        const FGameplayTag RoleParent = FGameplayTag::RequestGameplayTag(FName("NPC.Role"), false);
        if (RoleParent.IsValid()) {
            FMythicCemeteryConfig ParentCfg;
            ParentCfg.NotabilityGate = 999.0f;
            ParentCfg.NotableRoleTags.AddTag(RoleParent);
            TestTrue(TEXT("child role matches notable parent"), FMythicCemeteryRules::IsNotableDeath(FarmerRole, 0.0f, ParentCfg));
        }
    }

    {
        const float Spacing = 150.0f;
        const int32 PerRow = 5;

        TestEqual(TEXT("index 0 → origin"), FMythicCemeteryRules::ComputeGraveSlotOffset(0, Spacing, PerRow), FVector::ZeroVector);

        TestEqual(TEXT("index 1 → (0,150,0)"), FMythicCemeteryRules::ComputeGraveSlotOffset(1, Spacing, PerRow), FVector(0.f, 150.f, 0.f));
        TestEqual(TEXT("index 4 → (0,600,0)"), FMythicCemeteryRules::ComputeGraveSlotOffset(4, Spacing, PerRow), FVector(0.f, 600.f, 0.f));

        TestEqual(TEXT("index 5 → (150,0,0)"), FMythicCemeteryRules::ComputeGraveSlotOffset(5, Spacing, PerRow), FVector(150.f, 0.f, 0.f));
        TestEqual(TEXT("index 6 → (150,150,0)"), FMythicCemeteryRules::ComputeGraveSlotOffset(6, Spacing, PerRow), FVector(150.f, 150.f, 0.f));
        TestEqual(TEXT("index 10 → (300,0,0)"), FMythicCemeteryRules::ComputeGraveSlotOffset(10, Spacing, PerRow), FVector(300.f, 0.f, 0.f));

        TSet<FVector> Seen;
        const int32 SweepCount = 50;
        for (int32 i = 0; i < SweepCount; ++i) {
            Seen.Add(FMythicCemeteryRules::ComputeGraveSlotOffset(i, Spacing, PerRow));
        }
        TestEqual(TEXT("distinct indices → distinct offsets"), Seen.Num(), SweepCount);

        TestEqual(TEXT("negative index → origin"), FMythicCemeteryRules::ComputeGraveSlotOffset(-3, Spacing, PerRow), FVector::ZeroVector);
    }

    {
        TArray<FMythicEpitaphTemplate> Templates;

        FMythicEpitaphTemplate Wild;
        Wild.BodyFormat = FText::FromString(TEXT("Here lies {name}. Day {day}."));
        Templates.Add(Wild);

        FMythicEpitaphTemplate NobleT;
        NobleT.RoleTag = NobleRole;
        NobleT.BodyFormat = FText::FromString(TEXT("{name}, noble, day {day}."));
        Templates.Add(NobleT);

        FMythicEpitaphTemplate NobleFactionT;
        NobleFactionT.RoleTag = NobleRole;
        NobleFactionT.Faction = GuardRole;
        NobleFactionT.BodyFormat = FText::FromString(TEXT("{name}, noble of {faction}, day {day}."));
        Templates.Add(NobleFactionT);

        TestEqual(TEXT("role+faction picks most specific"),
                  FMythicEpitaph::SelectEpitaphTemplate(NobleRole, GuardRole, Templates), 2);

        TestEqual(TEXT("role-only when faction absent"),
                  FMythicEpitaph::SelectEpitaphTemplate(NobleRole, FGameplayTag(), Templates), 1);

        TestEqual(TEXT("wildcard fallback for unmatched role"),
                  FMythicEpitaph::SelectEpitaphTemplate(FarmerRole, FGameplayTag(), Templates), 0);

        TArray<FMythicEpitaphTemplate> StrictOnly;
        FMythicEpitaphTemplate OnlyNoble;
        OnlyNoble.RoleTag = NobleRole;
        OnlyNoble.BodyFormat = FText::FromString(TEXT("noble only"));
        StrictOnly.Add(OnlyNoble);
        TestEqual(TEXT("no match + no wildcard → -1"),
                  FMythicEpitaph::SelectEpitaphTemplate(FarmerRole, FGameplayTag(), StrictOnly), -1);

        FMythicEpitaphTemplate Full;
        Full.BodyFormat = FText::FromString(TEXT("Here lies {name}, {role} of {faction}, day {day}."));
        const FString Composed = FMythicEpitaph::Compose(Full, FName(TEXT("Aldric")), NobleRole, GuardRole, 7).ToString();
        TestEqual(TEXT("compose fills all tokens exactly"), Composed, FString(TEXT("Here lies Aldric, Noble of Guard, day 7.")));
        TestFalse(TEXT("compose leaves no unfilled token"), Composed.Contains(TEXT("{")));

        TestEqual(TEXT("tag leaf of NPC.Role.Noble"), FMythicEpitaph::TagLeaf(NobleRole), FString(TEXT("Noble")));
        TestEqual(TEXT("tag leaf of invalid tag"), FMythicEpitaph::TagLeaf(FGameplayTag()), FString());
    }

    {
        TestEqual(TEXT("t=0 → day 1"), FMythicCemeteryRules::WorldDayForSeconds(0.0, 1200.0f), 1);
        TestEqual(TEXT("t just under 1 day → day 1"), FMythicCemeteryRules::WorldDayForSeconds(1199.0, 1200.0f), 1);
        TestEqual(TEXT("t = 1 day → day 2"), FMythicCemeteryRules::WorldDayForSeconds(1200.0, 1200.0f), 2);
        TestEqual(TEXT("t = 3.5 days → day 4"), FMythicCemeteryRules::WorldDayForSeconds(4200.0, 1200.0f), 4);
        TestEqual(TEXT("guarded day length → day 1"), FMythicCemeteryRules::WorldDayForSeconds(9999.0, 0.0f), 1);
    }

    return true;
}
