
#include "Misc/AutomationTest.h"
#include "Containers/Set.h"
#include "GameplayTagContainer.h"
#include "Math/RandomStream.h"
#include "Itemization/Inventory/Fragments/Passive/TalentFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/MythicLootSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTalentRollTest,
    "Mythic.Itemization.Talents",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTalentRollTest::RunTest(const FString &Parameters) {
    UMythicLootSettings *Settings = NewObject<UMythicLootSettings>();
    Settings->TalentCountByRarity = {0, 1, 1, 1, 2};
    TestEqual(TEXT("Common rolls 0 talents"), UTalentFragment::ResolveTalentCount(Common, Settings), 0);
    TestEqual(TEXT("Rare rolls 1 talent"), UTalentFragment::ResolveTalentCount(Rare, Settings), 1);
    TestEqual(TEXT("Epic rolls 1 talent"), UTalentFragment::ResolveTalentCount(Epic, Settings), 1);
    TestEqual(TEXT("Legendary rolls 1 talent"), UTalentFragment::ResolveTalentCount(Legendary, Settings), 1);
    TestEqual(TEXT("Mythic rolls 2 talents"), UTalentFragment::ResolveTalentCount(Mythic, Settings), 2);

    TestEqual(TEXT("above-Mythic rarity → clamps to the Mythic count"), UTalentFragment::ResolveTalentCount(5, Settings), 2);

    TestEqual(TEXT("null settings, Legendary → 1"), UTalentFragment::ResolveTalentCount(Legendary, nullptr), 1);
    TestEqual(TEXT("null settings, Mythic → 2"), UTalentFragment::ResolveTalentCount(Mythic, nullptr), 2);
    TestEqual(TEXT("null settings, Common → 0"), UTalentFragment::ResolveTalentCount(Common, nullptr), 0);
    TestEqual(TEXT("null settings, Rare → 1 (matches the data-driven default)"), UTalentFragment::ResolveTalentCount(Rare, nullptr), 1);
    TestEqual(TEXT("null settings, Epic → 1 (matches the data-driven default)"), UTalentFragment::ResolveTalentCount(Epic, nullptr), 1);

    const UMythicLootSettings *Defaults = GetDefault<UMythicLootSettings>();
    for (int32 R = Common; R <= Mythic; ++R) {
        TestEqual(*FString::Printf(TEXT("fallback matches shipped default at rarity %d"), R),
                  UTalentFragment::ResolveTalentCount(R, nullptr), UTalentFragment::ResolveTalentCount(R, Defaults));
    }

    const FGameplayTag SwordTag = FGameplayTag::RequestGameplayTag(FName("Itemization.Type.Equipment.Weapon.Sword"));
    const FGameplayTag AxeTag = FGameplayTag::RequestGameplayTag(FName("Itemization.Type.Equipment.Weapon.Axe"));

    FGameplayTagContainer SwordProbe;
    SwordProbe.AddTag(SwordTag);

    TestTrue(TEXT("empty query is universal (sword probe)"),
             UTalentFragment::IsTalentAllowedOnItem(FGameplayTagQuery(), SwordProbe));
    TestTrue(TEXT("empty query is universal (empty probe)"),
             UTalentFragment::IsTalentAllowedOnItem(FGameplayTagQuery(), FGameplayTagContainer()));

    const FGameplayTagQuery SwordOnly = FGameplayTagQuery::MakeQuery_MatchAnyTags(FGameplayTagContainer(SwordTag));
    TestTrue(TEXT("sword-gated talent rolls on a sword"), UTalentFragment::IsTalentAllowedOnItem(SwordOnly, SwordProbe));

    FGameplayTagContainer AxeProbe;
    AxeProbe.AddTag(AxeTag);
    TestFalse(TEXT("sword-gated talent does NOT roll on an axe"), UTalentFragment::IsTalentAllowedOnItem(SwordOnly, AxeProbe));
    TestFalse(TEXT("a gated talent does not roll on an untyped item"),
              UTalentFragment::IsTalentAllowedOnItem(SwordOnly, FGameplayTagContainer()));

    TestTrue(TEXT("Mythic item eligible for a Common-gated talent"), UTalentFragment::IsTalentEligible(Mythic, Common));
    TestTrue(TEXT("Rare item eligible for a Rare-gated talent (equal)"), UTalentFragment::IsTalentEligible(Rare, Rare));
    TestTrue(TEXT("Legendary item eligible for a Legendary-gated talent"), UTalentFragment::IsTalentEligible(Legendary, Legendary));
    TestFalse(TEXT("Common item NOT eligible for a Rare-gated talent"), UTalentFragment::IsTalentEligible(Common, Rare));
    TestFalse(TEXT("Epic item NOT eligible for a Legendary-gated talent"), UTalentFragment::IsTalentEligible(Epic, Legendary));

    const TArray<int32> Eligible = {3, 5, 8, 13, 21};

    {
        FRandomStream A(1234);
        FRandomStream B(1234);
        const TArray<int32> PickA = UTalentFragment::SampleWithoutReplacement(Eligible, 3, A);
        const TArray<int32> PickB = UTalentFragment::SampleWithoutReplacement(Eligible, 3, B);
        TestEqual(TEXT("same seed → same pick count"), PickA.Num(), PickB.Num());

        bool bIdentical = PickA.Num() == PickB.Num();
        for (int32 i = 0; bIdentical && i < PickA.Num(); i++) {
            bIdentical = (PickA[i] == PickB[i]);
        }
        TestTrue(TEXT("same seed → identical picks (deterministic RNG)"), bIdentical);
        TestEqual(TEXT("picked exactly 3 from a pool of 5"), PickA.Num(), 3);

        TSet<int32> Seen;
        bool bUniqueAndValid = true;
        for (int32 V : PickA) {
            if (Seen.Contains(V) || !Eligible.Contains(V)) {
                bUniqueAndValid = false;
                break;
            }
            Seen.Add(V);
        }
        TestTrue(TEXT("picks are unique + drawn from the eligible set"), bUniqueAndValid);
    }

    {
        FRandomStream R(99);
        const TArray<int32> Pick = UTalentFragment::SampleWithoutReplacement(Eligible, 100, R);
        TestEqual(TEXT("over-request clamps to eligible count"), Pick.Num(), Eligible.Num());
    }

    {
        FRandomStream R(7);
        const TArray<int32> Pick = UTalentFragment::SampleWithoutReplacement(TArray<int32>(), 2, R);
        TestEqual(TEXT("zero eligible → empty pick"), Pick.Num(), 0);
    }

    {
        FRandomStream R(7);
        const TArray<int32> Pick = UTalentFragment::SampleWithoutReplacement(Eligible, 0, R);
        TestEqual(TEXT("zero requested → empty pick"), Pick.Num(), 0);
    }

    {
        FRandomStream R(7);
        const TArray<int32> Pick = UTalentFragment::SampleWithoutReplacement(Eligible, -5, R);
        TestEqual(TEXT("negative requested → empty pick"), Pick.Num(), 0);
    }

    return true;
}
