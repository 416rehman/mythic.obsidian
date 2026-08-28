#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "Internationalization/StringTableRegistry.h"
#include "Itemization/Affixes/MythicAffixCompiler.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicAffixGeneration.h"
#include "Itemization/Affixes/MythicAffixPool.h"
#include "Itemization/Affixes/MythicAffixProfile.h"
#include "Itemization/Affixes/MythicAffixRollPolicy.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"
#include "System/MythicAssetManager.h"
#include "UObject/StrongObjectPtr.h"

namespace {
FGameplayTag RequireHardeningTag(const TCHAR *Name) {
    return FGameplayTag::RequestGameplayTag(FName(Name), true);
}

const FName HardeningStringTableId(TEXT("MythicAffixRegistryHardeningTests"));

class FScopedHardeningStringTable {
public:
    FScopedHardeningStringTable() {
        FStringTableRegistry::Get().UnregisterStringTable(HardeningStringTableId);
        FStringTableRegistry::Get().Internal_NewLocTable(
            HardeningStringTableId, TEXT("MythicAffixRegistryHardeningTests"));
    }

    ~FScopedHardeningStringTable() {
        FStringTableRegistry::Get().UnregisterStringTable(HardeningStringTableId);
    }

    FText Add(const TCHAR *Key, const TCHAR *Source) const {
        FStringTableRegistry::Get().Internal_SetLocTableEntry(
            HardeningStringTableId, Key, Source);
        return FText::FromStringTable(HardeningStringTableId, Key);
    }
};

struct FRegistryFixture {
    UMythicItemizationDataRegistrySubsystem *CreateRegistry() {
        check(GEngine);
        GameInstance.Reset(NewObject<UGameInstance>(GEngine, NAME_None, RF_Transient));
        Registry.Reset(NewObject<UMythicItemizationDataRegistrySubsystem>(
            GameInstance.Get(), NAME_None, RF_Transient));
        return Registry.Get();
    }

    TStrongObjectPtr<UGameInstance> GameInstance;
    TStrongObjectPtr<UMythicItemizationDataRegistrySubsystem> Registry;
};

FGameplayTagQuery MatchTagQuery(const FGameplayTag Tag) {
    FGameplayTagQueryExpression Expression;
    Expression.AllTagsMatch().AddTag(Tag);
    FGameplayTagQuery Query;
    Query.Build(Expression);
    return Query;
}

void AddTier(FMythicAffixTierProgressionDefinition &Progression, const TCHAR *Name,
             const int32 MinItemLevel, const float Min, const float Max) {
    FMythicAffixTierDefinition &Tier = Progression.Tiers.AddDefaulted_GetRef();
    Tier.DeveloperName = FName(Name);
    Tier.MinItemLevel = MinItemLevel;
    Tier.TierWeight = 1.0f;
    Tier.BudgetCost = 0.0f;
    Tier.Magnitude.Min = Min;
    Tier.Magnitude.Max = Max;
}

void BuildSemanticGraph(const FScopedHardeningStringTable &Texts,
                        UMythicStatCategoryDefinition *&OutCategory,
                        UMythicStatDefinition *&OutStat,
                        UMythicAffixDefinition *&OutDefinition) {
    OutCategory = NewObject<UMythicStatCategoryDefinition>(GetTransientPackage());
    OutCategory->DeveloperName = TEXT("Defense");
    OutCategory->DesignerPurpose = TEXT("Registry hardening fixture.");
    OutCategory->CategoryTag = RequireHardeningTag(TEXT("Stat.Category.Defense"));
    OutCategory->DisplayName = Texts.Add(TEXT("DefenseCategory"), TEXT("Defense"));

    OutStat = NewObject<UMythicStatDefinition>(GetTransientPackage());
    OutStat->DeveloperName = TEXT("IncomingDamageMultiplier");
    OutStat->DesignerPurpose = TEXT("Registry hardening fixture.");
    OutStat->StatTag = RequireHardeningTag(TEXT("Stat.Attribute.IncomingDamageMultiplier"));
    OutStat->Attribute = UMythicAttributeSet_Defense::GetIncomingDamageMultiplierAttribute();
    OutStat->DisplayName = Texts.Add(TEXT("IncomingDamage"), TEXT("Incoming Damage"));
    OutStat->Category.SetAsset(OutCategory);
    OutStat->NumberPresentation.Format = EMythicStatFormat::Multiplier;
    OutStat->NeutralValue = 1.0f;
    OutStat->bCanBeAffixTarget = true;

    OutDefinition = NewObject<UMythicAffixDefinition>(GetTransientPackage());
    OutDefinition->DeveloperName = TEXT("IncomingDamageMultiplier");
    OutDefinition->DesignerPurpose = TEXT("Registry hardening fixture.");
    OutDefinition->AffixTag =
        RequireHardeningTag(TEXT("Itemization.Affix.IncomingDamageMultiplier"));
    OutDefinition->DisplayNameTemplate = Texts.Add(
        TEXT("IncomingDamageAffix"), TEXT("Incoming Damage"));
    OutDefinition->TargetStat.SetAsset(OutStat);
    OutDefinition->ModifierOp = EGameplayModOp::MultiplyCompound;
    OutDefinition->Quantization.Mode = EMythicAffixQuantizationMode::Step;
    OutDefinition->Quantization.Step = 0.1f;
    OutDefinition->StackingGroup = OutDefinition->AffixTag;

    FMythicAffixTierProgressionDefinition &Fallback =
        OutDefinition->TierProgressions.AddDefaulted_GetRef();
    Fallback.DeveloperName = TEXT("Fallback");
    Fallback.TuningContext = TEXT("Core");
    Fallback.SelectionPriority = 0;
    AddTier(Fallback, TEXT("Rank1"), 1, 0.9f, 0.9f);
    AddTier(Fallback, TEXT("Rank2"), 10, 0.8f, 0.8f);

    FMythicAffixTierProgressionDefinition &Specialized =
        OutDefinition->TierProgressions.AddDefaulted_GetRef();
    Specialized.DeveloperName = TEXT("DefensiveGear");
    Specialized.TuningContext = TEXT("Armour");
    Specialized.ApplicabilityQuery = MatchTagQuery(AFFIX_ROLL_GROUP_SUFFIX);
    Specialized.SelectionPriority = 20;
    AddTier(Specialized, TEXT("Rank1"), 1, 0.85f, 0.85f);
    AddTier(Specialized, TEXT("Rank2"), 10, 0.7f, 0.7f);
}

FCompiledAffix MakeFeasibilityAffix(const FName DefinitionName,
                                    const FName StatName,
                                    const int64 FallbackCost,
                                    const TOptional<int64> ConditionalCost = {}) {
    FCompiledAffix Affix;
    Affix.Definition.DefinitionId = FPrimaryAssetId(
        UMythicAssetManager::AffixDefinitionType, DefinitionName);
    Affix.Definition.TargetStatId = FPrimaryAssetId(
        UMythicAssetManager::StatDefinitionType, StatName);
    Affix.Definition.StackingRule = EMythicAffixStackingRule::StackAll;

    FCompiledAffixTierProgression &Fallback = Affix.Progressions.AddDefaulted_GetRef();
    Fallback.DeveloperName = TEXT("Fallback");
    Fallback.SelectionPriority = 0;
    FCompiledAffixTier &FallbackTier = Fallback.Tiers.AddDefaulted_GetRef();
    FallbackTier.TierRank = 1;
    FallbackTier.MinItemLevel = 1;
    FallbackTier.TierWeight = FMythicAffixCompiler::FixedPointScale;
    FallbackTier.BudgetCost = FallbackCost;

    if (ConditionalCost.IsSet()) {
        FCompiledAffixTierProgression &Conditional =
            Affix.Progressions.AddDefaulted_GetRef();
        Conditional.DeveloperName = TEXT("Conditional");
        Conditional.ApplicabilityQuery = MatchTagQuery(AFFIX_ROLL_GROUP_SUFFIX);
        Conditional.SelectionPriority = 10;
        FCompiledAffixTier &ConditionalTier = Conditional.Tiers.AddDefaulted_GetRef();
        ConditionalTier.TierRank = 1;
        ConditionalTier.MinItemLevel = 1;
        ConditionalTier.TierWeight = FMythicAffixCompiler::FixedPointScale;
        ConditionalTier.BudgetCost = ConditionalCost.GetValue();
    }
    return Affix;
}

FCompiledAffixProfile MakeFeasibilityProfile(const bool bUnlimitedBudget,
                                              const int64 MagnitudeBudget) {
    FCompiledAffixProfile Profile;
    Profile.ProfileId = FPrimaryAssetId(
        UMythicAssetManager::AffixProfileType,
        TEXT("Itemization.AffixProfile.CompilerHardening"));
    Profile.ProfileRevision = 1;
    Profile.Policy.ShortfallMode = EMythicAffixShortfallMode::FailGeneration;
    Profile.Policy.bAllowLowerPriorityFallback = false;
    for (uint8 RarityByte = static_cast<uint8>(EItemRarity::Common);
         RarityByte <= static_cast<uint8>(EItemRarity::Mythic); ++RarityByte) {
        FCompiledAffixRarityBudget Budget;
        Budget.Rarity = static_cast<EItemRarity>(RarityByte);
        Budget.RandomRollCount = 1;
        Budget.bUnlimitedMagnitudeBudget = bUnlimitedBudget;
        Budget.MagnitudeBudget = MagnitudeBudget;
        Budget.RollGroupCaps.Add(AFFIX_ROLL_GROUP_PREFIX, 1);
        Profile.Policy.Budgets.Add(Budget.Rarity, MoveTemp(Budget));
    }
    return Profile;
}

void AddFeasibilitySlice(FCompiledAffixProfile &Profile, const uint32 GuidWord,
                         const int32 Priority, const FCompiledAffix &Affix,
                         const FGameplayTagQuery &EligibilityQuery = FGameplayTagQuery()) {
    FCompiledAffixSlice &Slice = Profile.RandomSlices.AddDefaulted_GetRef();
    Slice.SliceGuid = FGuid(GuidWord, 0, 0, 1);
    Slice.Priority = Priority;
    Slice.SliceWeight = 1;
    FCompiledAffixPoolRow &Row = Slice.Rows.AddDefaulted_GetRef();
    Row.PoolRowGuid = FGuid(GuidWord, 0, 0, 2);
    Row.RollGroup = AFFIX_ROLL_GROUP_PREFIX;
    Row.SelectionWeight = FMythicAffixCompiler::FixedPointScale;
    Row.EligibilityQuery = EligibilityQuery;
    Row.Affix = Affix;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixCompiledCurveHardeningTest,
    "Mythic.Itemization.Affixes.Compiler.CompiledCurveIsImmutable",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixCompiledCurveHardeningTest::RunTest(const FString &Parameters) {
    FMythicAffixMagnitudeBand Source;
    Source.Min = 2.0f;
    Source.Max = 4.0f;
    Source.ScaleMode = EMythicAffixScaleMode::Curve;
    FRichCurve *SourceCurve = Source.LevelScalingCurve.GetRichCurve();
    SourceCurve->AddKey(1.0f, 1.0f);
    SourceCurve->AddKey(10.0f, 2.0f);
    Source.CurveTailGrowth = 1.05f;

    FCompiledAffixMagnitudeBand Compiled;
    TArray<FText> Errors;
    TestTrue(TEXT("finite embedded curve compiles into a value copy"),
             FMythicAffixCompiler::CompileMagnitudeBand(Source, Compiled, Errors));
    float BeforeMin = 0.0f;
    float BeforeMax = 0.0f;
    TestTrue(TEXT("compiled curve resolves at its final key"),
             Compiled.Resolve(10, BeforeMin, BeforeMax));
    SourceCurve->UpdateOrAddKey(10.0f, 100.0f);
    float AfterMin = 0.0f;
    float AfterMax = 0.0f;
    TestTrue(TEXT("compiled curve still resolves after source mutation"),
             Compiled.Resolve(10, AfterMin, AfterMax));
    TestEqual(TEXT("source mutation cannot change compiled minimum"), AfterMin, BeforeMin);
    TestEqual(TEXT("source mutation cannot change compiled maximum"), AfterMax, BeforeMax);
    TestTrue(TEXT("compiled evaluator preserves open-ended tail"),
             Compiled.Resolve(11, AfterMin, AfterMax));
    TestTrue(TEXT("tail is applied to the copied final-key value"),
             FMath::IsNearlyEqual(AfterMin, 4.2f, 1.e-4f));

    Source.LevelScalingCurve.ExternalCurve =
        NewObject<UCurveFloat>(GetTransientPackage());
    Errors.Reset();
    TestFalse(TEXT("an external curve dependency is rejected as a second tuning source"),
              FMythicAffixCompiler::CompileMagnitudeBand(Source, Compiled, Errors));
    TestTrue(TEXT("external-curve rejection is actionable"), !Errors.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixContextualProgressionCompilationTest,
    "Mythic.Itemization.Affixes.Compiler.ContextualProgressionsAndDerivedRanks",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixContextualProgressionCompilationTest::RunTest(const FString &Parameters) {
    FScopedHardeningStringTable Texts;
    UMythicStatCategoryDefinition *Category = nullptr;
    UMythicStatDefinition *Stat = nullptr;
    UMythicAffixDefinition *Definition = nullptr;
    BuildSemanticGraph(Texts, Category, Stat, Definition);

    FRegistryFixture Fixture;
    UMythicItemizationDataRegistrySubsystem *Registry = Fixture.CreateRegistry();
    TArray<UObject *> Assets{Category, Stat, Definition};
    TArray<FText> Errors;
    if (!TestTrue(TEXT("typed semantic graph publishes transactionally"),
                  Registry->PublishLoadedAssets(Assets, Errors))) return false;

    FMythicAffixDefinitionHandle Handle;
    Handle.SetAsset(Definition);
    FCompiledAffix Compiled;
    if (!TestTrue(TEXT("Definition compiles through its direct typed reference"),
                  FMythicAffixCompiler::CompileDefinition(
                      Handle, *Registry, Compiled, Errors))) return false;
    TestEqual(TEXT("both contextual progressions compile"), Compiled.Progressions.Num(), 2);
    TestEqual(TEXT("tier rank is derived from fallback array order"),
              Compiled.Progressions[1].Tiers[1].TierRank, 2);

    FGameplayTagContainer EmptyContext;
    const FCompiledAffixTierProgression *Fallback = Compiled.ResolveProgression(EmptyContext);
    TestTrue(TEXT("empty context selects the fallback progression"),
             Fallback && Fallback->DeveloperName == TEXT("Fallback"));
    FGameplayTagContainer DefensiveContext;
    DefensiveContext.AddTag(AFFIX_ROLL_GROUP_SUFFIX);
    const FCompiledAffixTierProgression *Specialized =
        Compiled.ResolveProgression(DefensiveContext);
    TestTrue(TEXT("matching context selects the highest-priority progression"),
             Specialized && Specialized->DeveloperName == TEXT("DefensiveGear"));

    FCompiledAffix Ambiguous = Compiled;
    FCompiledAffixTierProgression DuplicateConditional = Ambiguous.Progressions[0];
    DuplicateConditional.DeveloperName = TEXT("DefensiveGearTie");
    Ambiguous.Progressions.Add(MoveTemp(DuplicateConditional));
    TestNull(TEXT("equal-priority matches fail closed instead of using array order"),
             Ambiguous.ResolveProgression(DefensiveContext));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixExactRankGrantHardeningTest,
    "Mythic.Itemization.Affixes.Compiler.ExactRankGrantMaterialization",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixExactRankGrantHardeningTest::RunTest(const FString &Parameters) {
    FScopedHardeningStringTable Texts;
    UMythicStatCategoryDefinition *Category = nullptr;
    UMythicStatDefinition *Stat = nullptr;
    UMythicAffixDefinition *Definition = nullptr;
    BuildSemanticGraph(Texts, Category, Stat, Definition);

    FRegistryFixture Fixture;
    UMythicItemizationDataRegistrySubsystem *Registry = Fixture.CreateRegistry();
    TArray<UObject *> Assets{Category, Stat, Definition};
    TArray<FText> Errors;
    if (!TestTrue(TEXT("typed semantic graph publishes"),
                  Registry->PublishLoadedAssets(Assets, Errors))) return false;

    FMythicAffixGrantSpec Grant;
    Grant.GrantGuid = FGuid(0x900, 0xA00, 0xB00, 0xC00);
    Grant.DeveloperName = TEXT("ExactRankTwo");
    Grant.AffixDefinition.SetAsset(Definition);
    Grant.TierMode = EMythicAffixGrantTierMode::ExactTier;
    Grant.ExactTierRank = 2;
    Grant.RollGroup = AFFIX_ROLL_GROUP_SUFFIX;
    Grant.SourceKind = AFFIX_SOURCE_GEM;
    Grant.bLocked = true;
    if (!TestTrue(TEXT("exact rank compiles across every contextual progression"),
                  Registry->CompileGrantClosures(MakeArrayView(&Grant, 1), Errors))) return false;

    const TSharedPtr<const FCompiledAffixGrantClosure> Closure =
        Registry->FindCompiledGrant(Grant);
    if (!TestTrue(TEXT("compiled grant has a nonzero gameplay hash"),
                  Closure.IsValid() && !Closure->GameplayContentHash.IsZero())) return false;
    TestEqual(TEXT("compiled grant keeps the direct Definition asset"),
              Closure->Affix.Definition.Definition.GetAsset(), Definition);
    TestEqual(TEXT("modifier operation remains canonical compiler data"),
              Closure->Affix.Definition.ModifierOp, EGameplayModOp::MultiplyCompound);

    FMythicAffixGrantContext Context;
    Context.ItemInstanceGuid = FGuid(1, 2, 3, 4);
    Context.ItemLevel = 1;
    Context.Seed = 123;
    Context.ContextTags.AddTag(AFFIX_ROLL_GROUP_SUFFIX);
    FRolledAffix Snapshot;
    if (!TestTrue(TEXT("explicit exact-rank grant bypasses random tier item-level gating"),
                  FMythicAffixGrantService::Materialize(
                      Grant, Context, *Registry, Snapshot, nullptr))) return false;
    TestEqual(TEXT("snapshot references the authoritative Definition asset"),
              Snapshot.AffixDefinition.GetAsset(), Definition);
    TestEqual(TEXT("snapshot persists the one-based rank"), Snapshot.TierRank, 2);
    TestEqual(TEXT("matching contextual progression supplies the singular magnitude"),
              Snapshot.Magnitude, 0.7f);
    TestEqual(TEXT("grant materialization records the physical source item"),
              Snapshot.Provenance.SourceItemGuid, Context.ItemInstanceGuid);
    TestTrue(TEXT("snapshot persists its compiled gameplay hash"),
             Snapshot.Provenance.GameplayContentHash == Closure->GameplayContentHash);

    FMythicAffixGrantSpec InvalidRank = Grant;
    InvalidRank.GrantGuid = FGuid(5, 6, 7, 8);
    InvalidRank.ExactTierRank = 3;
    TSharedPtr<const FCompiledAffixGrantClosure> InvalidClosure;
    Errors.Reset();
    TestFalse(TEXT("an exact rank missing from any progression fails compilation"),
              FMythicAffixCompiler::CompileGrant(
                  InvalidRank, *Registry, InvalidClosure, Errors));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixStrictFeasibilityContextTest,
    "Mythic.Itemization.Affixes.Compiler.StrictFeasibilityRejectsUnprovenContext",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixStrictFeasibilityContextTest::RunTest(const FString &Parameters) {
    const FCompiledAffix Affix = MakeFeasibilityAffix(
        TEXT("ConditionalOnly"), TEXT("ConditionalOnlyStat"), 0);

    FCompiledAffixProfile ConditionalOnly = MakeFeasibilityProfile(true, 0);
    AddFeasibilitySlice(ConditionalOnly, 1, 10, Affix,
                        MatchTagQuery(AFFIX_ROLL_GROUP_SUFFIX));
    TArray<FText> Errors;
    TestFalse(TEXT("FailGeneration cannot promise a roll from only context-gated rows"),
              FMythicAffixCompiler::ValidateStructuralFeasibility(
                  ConditionalOnly, Errors));
    ConditionalOnly.Policy.ShortfallMode = EMythicAffixShortfallMode::AllowPartial;
    Errors.Reset();
    TestTrue(TEXT("AllowPartial explicitly permits a context-gated shortfall"),
             FMythicAffixCompiler::ValidateStructuralFeasibility(
                 ConditionalOnly, Errors));

    FCompiledAffixProfile BlockedLowerPriority = MakeFeasibilityProfile(true, 0);
    AddFeasibilitySlice(BlockedLowerPriority, 2, 10, Affix,
                        MatchTagQuery(AFFIX_ROLL_GROUP_SUFFIX));
    AddFeasibilitySlice(
        BlockedLowerPriority, 3, 0,
        MakeFeasibilityAffix(TEXT("UnconditionalFallback"),
                             TEXT("UnconditionalFallbackStat"), 0));
    Errors.Reset();
    TestFalse(
        TEXT("an unmatched higher-priority slice blocks a lower slice when fallback is disabled"),
        FMythicAffixCompiler::ValidateStructuralFeasibility(
            BlockedLowerPriority, Errors));
    BlockedLowerPriority.Policy.bAllowLowerPriorityFallback = true;
    Errors.Reset();
    TestTrue(TEXT("enabling lower-priority fallback makes the unconditional backbone provable"),
             FMythicAffixCompiler::ValidateStructuralFeasibility(
                 BlockedLowerPriority, Errors));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixStrictFeasibilityProgressionCostTest,
    "Mythic.Itemization.Affixes.Compiler.StrictFeasibilityUsesFallbackProgressionCost",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixStrictFeasibilityProgressionCostTest::RunTest(
    const FString &Parameters) {
    const int64 CheapConditionalCost = FMythicAffixCompiler::FixedPointScale;
    const int64 ExpensiveFallbackCost = 5 * FMythicAffixCompiler::FixedPointScale;
    FCompiledAffixProfile Profile = MakeFeasibilityProfile(
        false, CheapConditionalCost);
    AddFeasibilitySlice(
        Profile, 4, 0,
        MakeFeasibilityAffix(TEXT("ConditionalDiscount"),
                             TEXT("ConditionalDiscountStat"),
                             ExpensiveFallbackCost, CheapConditionalCost));

    TArray<FText> Errors;
    TestFalse(
        TEXT("a cheap conditional progression cannot hide an unaffordable fallback progression"),
        FMythicAffixCompiler::ValidateStructuralFeasibility(Profile, Errors));
    for (TPair<EItemRarity, FCompiledAffixRarityBudget> &Pair
         : Profile.Policy.Budgets) {
        Pair.Value.MagnitudeBudget = ExpensiveFallbackCost;
    }
    Errors.Reset();
    TestTrue(TEXT("funding every selectable progression makes the profile feasible"),
             FMythicAffixCompiler::ValidateStructuralFeasibility(Profile, Errors));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixSliceMetadataCompilationTest,
    "Mythic.Itemization.Affixes.Compiler.RejectsInvalidSliceMetadata",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixSliceMetadataCompilationTest::RunTest(const FString &Parameters) {
    FScopedHardeningStringTable Texts;
    UMythicStatCategoryDefinition *Category = nullptr;
    UMythicStatDefinition *Stat = nullptr;
    UMythicAffixDefinition *Definition = nullptr;
    BuildSemanticGraph(Texts, Category, Stat, Definition);

    UMythicAffixPool *Pool = NewObject<UMythicAffixPool>(GetTransientPackage());
    Pool->DeveloperName = TEXT("CompilerHardeningPool");
    Pool->DesignerPurpose = TEXT("Compiler slice-metadata fixture.");
    Pool->PoolTag = RequireHardeningTag(TEXT("Itemization.AffixPool.Armour.S0"));
    FMythicAffixPoolEntry &Entry = Pool->Entries.AddDefaulted_GetRef();
    Entry.PoolRowGuid = FGuid(0x100, 0x200, 0x300, 0x400);
    Entry.DeveloperName = TEXT("IncomingDamageMultiplier");
    Entry.AffixDefinition.SetAsset(Definition);
    Entry.RollGroup = AFFIX_ROLL_GROUP_PREFIX;

    UMythicAffixRollPolicy *Policy =
        NewObject<UMythicAffixRollPolicy>(GetTransientPackage());
    Policy->DeveloperName = TEXT("CompilerHardeningPolicy");
    Policy->DesignerPurpose = TEXT("Compiler slice-metadata fixture.");
    Policy->PolicyTag = RequireHardeningTag(
        TEXT("Itemization.AffixRollPolicy.Default.S0"));
    Policy->ShortfallMode = EMythicAffixShortfallMode::FailGeneration;
    for (uint8 RarityByte = static_cast<uint8>(EItemRarity::Common);
         RarityByte <= static_cast<uint8>(EItemRarity::Mythic); ++RarityByte) {
        FMythicAffixRarityBudget &Budget =
            Policy->BudgetsByRarity.AddDefaulted_GetRef();
        Budget.Rarity = static_cast<EItemRarity>(RarityByte);
        Budget.RandomRollCount = 1;
        FMythicAffixRollGroupBudget &RollGroupBudget =
            Budget.RollGroupBudgets.AddDefaulted_GetRef();
        RollGroupBudget.RollGroup = AFFIX_ROLL_GROUP_PREFIX;
        RollGroupBudget.MaxRolls = 1;
    }

    UMythicAffixProfile *Profile =
        NewObject<UMythicAffixProfile>(GetTransientPackage());
    Profile->DeveloperName = TEXT("CompilerHardeningProfile");
    Profile->DesignerPurpose = TEXT("Compiler slice-metadata fixture.");
    Profile->ProfileTag = RequireHardeningTag(
        TEXT("Itemization.AffixProfile.Armour.S0"));
    Profile->RollPolicy.SetAsset(Policy);
    FMythicAffixPoolSlice &Slice = Profile->RandomPoolSlices.AddDefaulted_GetRef();
    Slice.SliceGuid = FGuid(0x500, 0x600, 0x700, 0x800);
    Slice.DeveloperName = TEXT("Armour");
    Slice.Pool.SetAsset(Pool);
    Slice.SourceKind = AFFIX_SOURCE_EXPLICIT;

    FRegistryFixture Fixture;
    UMythicItemizationDataRegistrySubsystem *Registry = Fixture.CreateRegistry();
    TArray<UObject *> Assets{Category, Stat, Definition, Pool, Policy, Profile};
    TArray<FText> Errors;
    if (!TestTrue(TEXT("valid slice fixture publishes"),
                  Registry->PublishLoadedAssets(Assets, Errors))) return false;

    TSharedPtr<const FCompiledAffixProfile> Compiled;
    TestTrue(TEXT("valid slice fixture compiles"),
             FMythicAffixCompiler::Compile(*Profile, *Registry, Compiled, Errors));

    Slice.DeveloperName = NAME_None;
    Errors.Reset();
    TestFalse(TEXT("a slice without a developer name is rejected by compilation"),
              FMythicAffixCompiler::Compile(
                  *Profile, *Registry, Compiled, Errors));

    Slice.DeveloperName = TEXT("Armour");
    Slice.SourceKind = FGameplayTag();
    Errors.Reset();
    TestFalse(TEXT("a slice without a source kind is rejected by compilation"),
              FMythicAffixCompiler::Compile(
                  *Profile, *Registry, Compiled, Errors));
    return true;
}

#endif
