#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameplayTagsManager.h"
#include "Itemization/Affixes/MythicAffixCompiler.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicAffixGeneration.h"
#include "Itemization/Affixes/MythicAffixPool.h"
#include "Itemization/Affixes/MythicAffixRng.h"
#include "Itemization/Affixes/MythicItemizationHash.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "System/MythicAssetManager.h"

namespace {
FGameplayTag RequireTag(const TCHAR *Name) {
    return UGameplayTagsManager::Get().RequestGameplayTag(FName(Name), true);
}

FCompiledAffix MakeCompiledAffix(UMythicAffixDefinition *Definition,
                                 const FGameplayTag StatTag, const float Min,
                                 const float Max) {
    FCompiledAffix Affix;
    Affix.Definition.Definition.SetAsset(Definition);
    Affix.Definition.DefinitionId = Definition->GetPrimaryAssetId();
    Affix.Definition.Revision = 5;
    Affix.Definition.TargetStatTag = StatTag;
    Affix.Definition.StackingGroup = Definition->AffixTag;
    Affix.Definition.StackingRule = EMythicAffixStackingRule::UniquePerItem;

    FCompiledAffixTierProgression &Progression = Affix.Progressions.AddDefaulted_GetRef();
    Progression.DeveloperName = TEXT("Fallback");
    Progression.TuningContext = TEXT("Core");
    Progression.SelectionPriority = 0;
    FCompiledAffixTier &Tier = Progression.Tiers.AddDefaulted_GetRef();
    Tier.TierRank = 1;
    Tier.DeveloperName = TEXT("TestTier");
    Tier.MinItemLevel = 1;
    Tier.TierWeight = FMythicAffixCompiler::FixedPointScale;
    Tier.BudgetCost = 0;
    Tier.Magnitude.Min = Min;
    Tier.Magnitude.Max = Max;
    Tier.Magnitude.ScaleMode = EMythicAffixScaleMode::None;
    return Affix;
}

FCompiledAffixProfile MakeTwoStageProfile(UMythicAffixDefinition *Power,
                                          UMythicAffixDefinition *Strength) {
    FCompiledAffixProfile Profile;
    Profile.ProfileId = FPrimaryAssetId(
        UMythicAssetManager::AffixProfileType,
        FName(TEXT("Itemization.AffixProfile.GlobalWeapon.S0")));
    Profile.ProfileRevision = 2;
    Profile.Policy.PolicyId = FPrimaryAssetId(
        UMythicAssetManager::AffixRollPolicyType,
        FName(TEXT("Itemization.AffixRollPolicy.Default.S0")));
    Profile.Policy.Revision = 3;
    Profile.Policy.AlgorithmVersion = 1;
    Profile.Policy.ShortfallMode = EMythicAffixShortfallMode::FailGeneration;
    FCompiledAffixRarityBudget Budget;
    Budget.Rarity = EItemRarity::Common;
    Budget.RandomRollCount = 1;
    Budget.bUnlimitedMagnitudeBudget = true;
    Budget.RollGroupCaps.Add(AFFIX_ROLL_GROUP_PREFIX, 1);
    Budget.RollGroupCaps.Add(AFFIX_ROLL_GROUP_SUFFIX, 1);
    Profile.Policy.Budgets.Add(EItemRarity::Common, MoveTemp(Budget));
    Profile.GameplayContentHash.Word0 = 1;

    FCompiledAffixSlice &FirstSlice = Profile.RandomSlices.AddDefaulted_GetRef();
    FirstSlice.SliceGuid = FGuid(1, 0, 0, 0);
    FirstSlice.PoolId = FPrimaryAssetId(
        UMythicAssetManager::AffixPoolType,
        FName(TEXT("Itemization.AffixPool.GlobalWeapon.S0")));
    FirstSlice.PoolRevision = 17;
    FirstSlice.SourceKind = AFFIX_SOURCE_EXPLICIT;
    FirstSlice.Priority = 10;
    FirstSlice.SliceWeight = 1;
    FCompiledAffixPoolRow &FirstRow = FirstSlice.Rows.AddDefaulted_GetRef();
    FirstRow.PoolRowGuid = FGuid(10, 0, 0, 0);
    FirstRow.RowRevision = 19;
    FirstRow.RollGroup = AFFIX_ROLL_GROUP_PREFIX;
    FirstRow.SelectionWeight = 1000000000000LL;
    FirstRow.Affix = MakeCompiledAffix(
        Power, RequireTag(TEXT("Stat.Attribute.Power")), 4.0f, 8.0f);

    FCompiledAffixSlice &SecondSlice = Profile.RandomSlices.AddDefaulted_GetRef();
    SecondSlice.SliceGuid = FGuid(2, 0, 0, 0);
    SecondSlice.PoolId = FPrimaryAssetId(
        UMythicAssetManager::AffixPoolType,
        FName(TEXT("Itemization.AffixPool.SpecializedWeapon.S0")));
    SecondSlice.PoolRevision = 23;
    SecondSlice.SourceKind = AFFIX_SOURCE_EXPLICIT;
    SecondSlice.Priority = 10;
    SecondSlice.SliceWeight = 3;
    FCompiledAffixPoolRow &SecondRow = SecondSlice.Rows.AddDefaulted_GetRef();
    SecondRow.PoolRowGuid = FGuid(20, 0, 0, 0);
    SecondRow.RowRevision = 29;
    SecondRow.RollGroup = AFFIX_ROLL_GROUP_SUFFIX;
    SecondRow.SelectionWeight = 1;
    SecondRow.Affix = MakeCompiledAffix(
        Strength, RequireTag(TEXT("Stat.Attribute.Strength")), 9.0f, 12.0f);
    return Profile;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixIdentityAndMathTest,
    "Mythic.Itemization.Affixes.PrimaryAssets.IdentityAndMath",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixIdentityAndMathTest::RunTest(const FString &Parameters) {
    UMythicAffixDefinition *Definition = NewObject<UMythicAffixDefinition>();
    Definition->AffixTag = RequireTag(TEXT("Itemization.Affix.Power"));
    TestEqual(TEXT("definition primary type is exact"),
              Definition->GetPrimaryAssetId().PrimaryAssetType,
              UMythicAssetManager::AffixDefinitionType);
    TestEqual(TEXT("definition logical identity is derived from its semantic tag"),
              Definition->GetPrimaryAssetId().PrimaryAssetName,
              Definition->AffixTag.GetTagName());
    Definition->StackingRule = EMythicAffixStackingRule::UniquePerItem;
    TestEqual(TEXT("an omitted stacking family derives from the affix identity"),
              Definition->GetEffectiveStackingGroup(), Definition->AffixTag);
    const FGameplayTag SharedFamily = RequireTag(TEXT("Itemization.Affix.Strength"));
    Definition->StackingGroup = SharedFamily;
    TestEqual(TEXT("an explicit cross-affix stacking family overrides the identity default"),
              Definition->GetEffectiveStackingGroup(), SharedFamily);
    Definition->StackingRule = EMythicAffixStackingRule::StackAll;
    TestFalse(TEXT("StackAll has no effective exclusive stacking family"),
              Definition->GetEffectiveStackingGroup().IsValid());

    UMythicAffixPool *Pool = NewObject<UMythicAffixPool>();
    Pool->PoolTag = RequireTag(TEXT("Itemization.AffixPool.GlobalWeapon.S0"));
    TestEqual(TEXT("pool primary type is exact"),
              Pool->GetPrimaryAssetId().PrimaryAssetType,
              UMythicAssetManager::AffixPoolType);

    int64 Fixed = 0;
    TestTrue(TEXT("fixed-point half rounds away from zero"),
             FMythicAffixCompiler::TryCompileFixedPoint(0.0000005, true, Fixed));
    TestEqual(TEXT("smallest representable positive fixed-point value"), Fixed, int64(1));
    TestFalse(TEXT("positive value that rounds to zero fails"),
              FMythicAffixCompiler::TryCompileFixedPoint(0.0000004, true, Fixed));

    FMythicAffixQuantization Whole;
    Whole.Mode = EMythicAffixQuantizationMode::WholeNumber;
    TestEqual(TEXT("positive halves round away"), Whole.Apply(2.5f), 3.0f);
    TestEqual(TEXT("negative halves round away"), Whole.Apply(-2.5f), -3.0f);

    FMythicAffixQuantization HalfStep;
    HalfStep.Mode = EMythicAffixQuantizationMode::Step;
    HalfStep.Step = 0.5f;
    TestEqual(TEXT("positive half-step ties round away"), HalfStep.Apply(0.25f), 0.5f);
    TestEqual(TEXT("negative half-step ties round away"), HalfStep.Apply(-0.25f), -0.5f);
    TestEqual(TEXT("negative values below a half-step round to canonical zero"),
              FMath::AsUInt(HalfStep.Apply(-0.24f)), FMath::AsUInt(0.0f));

    FMythicAffixMagnitudeBand Band;
    Band.Min = 2.0f;
    Band.Max = 4.0f;
    float Min = 0.0f;
    float Max = 0.0f;
    TestTrue(TEXT("fixed range resolves"),
             UMythicAffixDefinition::ResolveMagnitudeBand(Band, 50, Min, Max));
    TestEqual(TEXT("fixed minimum is unscaled"), Min, 2.0f);
    TestEqual(TEXT("fixed maximum is unscaled"), Max, 4.0f);
    Band.ScaleMode = EMythicAffixScaleMode::Linear;
    Band.LinearPerItemLevel = 0.5f;
    TestTrue(TEXT("linear range resolves"),
             UMythicAffixDefinition::ResolveMagnitudeBand(Band, 10, Min, Max));
    TestEqual(TEXT("linear minimum uses item level"), Min, 7.0f);
    TestEqual(TEXT("linear maximum uses item level"), Max, 9.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixRngAndSelectionTest,
    "Mythic.Itemization.Affixes.PrimaryAssets.RngAndTwoStageSelection",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixRngAndSelectionTest::RunTest(const FString &Parameters) {
    const TArray<uint8> Sha256AbcInput{'a', 'b', 'c'};
    const uint8 Sha256AbcExpected[] = {
        0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea,
        0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
        0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c,
        0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad,
    };
    FSHA256Signature Digest{};
    TestTrue(TEXT("project-local SHA-256 accepts the standard abc vector"),
             MythicItemizationHash::Sha256(Sha256AbcInput, Digest));
    TestTrue(TEXT("project-local SHA-256 matches the standard abc digest"),
             FMemory::Memcmp(Digest.Signature, Sha256AbcExpected,
                             UE_ARRAY_COUNT(Sha256AbcExpected)) == 0);

    FMythicAffixRngV1 Golden(42, 54);
    const uint32 Expected[] = {
        0xa15c02b7u, 0x7b47f409u, 0xba1d3330u, 0x83d2f293u, 0xbfa4784bu};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Expected); ++Index) {
        TestEqual(FString::Printf(TEXT("PCG32 golden output %d"), Index),
                  Golden.NextUInt32(), Expected[Index]);
    }

    UMythicAffixDefinition *Power = NewObject<UMythicAffixDefinition>();
    Power->AffixTag = RequireTag(TEXT("Itemization.Affix.Power"));
    UMythicAffixDefinition *Strength = NewObject<UMythicAffixDefinition>();
    Strength->AffixTag = RequireTag(TEXT("Itemization.Affix.Strength"));
    const FCompiledAffixProfile Profile = MakeTwoStageProfile(Power, Strength);
    const FGuid ItemGuid(0x11111111, 0x22222222, 0x33333333, 0x44444444);
    int32 SecondSliceCount = 0;
    for (uint64 Seed = 1; Seed <= 2000; ++Seed) {
        FMythicAffixRollRequest Request;
        Request.ItemInstanceGuid = ItemGuid;
        Request.ItemLevel = 25;
        Request.Rarity = EItemRarity::Common;
        Request.ProfileId = Profile.ProfileId;
        Request.Seed = Seed;
        Request.AlgorithmVersion = 1;
        TArray<FRolledAffix> Results;
        if (!FMythicAffixGenerator::Generate(Request, Profile, Results, nullptr)) {
            AddError(FString::Printf(TEXT("generation failed for seed %llu"), Seed));
            return false;
        }
        if (!TestEqual(TEXT("one requested roll lands"), Results.Num(), 1)) {
            return false;
        }
        if (Results[0].AffixDefinition.GetAsset() == Strength) {
            ++SecondSliceCount;
            TestEqual(TEXT("pool revision reaches audit provenance"),
                      Results[0].Provenance.PoolRevision, 23);
            TestEqual(TEXT("row revision reaches audit provenance"),
                      Results[0].Provenance.PoolRowRevision, 29);
        }
    }
    TestTrue(TEXT("slice selection follows 1:3 slice weights independent of row weights"),
             SecondSliceCount >= 1400 && SecondSliceCount <= 1600);

    FMythicAffixRollRequest Request;
    Request.ItemInstanceGuid = ItemGuid;
    Request.ItemLevel = 25;
    Request.Rarity = EItemRarity::Common;
    Request.ProfileId = Profile.ProfileId;
    Request.Seed = 999;
    Request.AlgorithmVersion = 1;
    TArray<FRolledAffix> First;
    TArray<FRolledAffix> Second;
    if (!TestTrue(TEXT("first deterministic roll succeeds"),
                  FMythicAffixGenerator::Generate(Request, Profile, First, nullptr))
        || !TestTrue(TEXT("second deterministic roll succeeds"),
                     FMythicAffixGenerator::Generate(Request, Profile, Second, nullptr))
        || !TestEqual(TEXT("both deterministic attempts materialize one roll"),
                      First.Num() + Second.Num(), 2)) {
        return false;
    }
    TestEqual(TEXT("same seed binds the same Definition asset"),
              First[0].AffixDefinition.GetAsset(), Second[0].AffixDefinition.GetAsset());
    TestEqual(TEXT("same seed materializes the same singular magnitude"),
              First[0].Magnitude, Second[0].Magnitude);
    TestEqual(TEXT("same origin yields stable RollGuid"),
              First[0].RollGuid, Second[0].RollGuid);
    TestEqual(TEXT("generation stores the derived one-based rank"), First[0].TierRank, 1);
    TestEqual(TEXT("generation records the physical item as source provenance"),
              First[0].Provenance.SourceItemGuid, ItemGuid);
    return true;
}

#endif
