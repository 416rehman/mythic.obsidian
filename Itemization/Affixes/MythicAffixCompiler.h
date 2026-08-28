#pragma once

#include "CoreMinimal.h"
#include "Curves/RichCurve.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicAffixPool.h"
#include "Itemization/Affixes/MythicAffixProfile.h"
#include "Itemization/Affixes/MythicAffixRollPolicy.h"
#include "MythicAffixCompiler.generated.h"

class UMythicItemizationDataRegistrySubsystem;
class FMythicStatRegistry;

/** Immutable value-copy of one tier magnitude range. */
struct MYTHIC_API FCompiledAffixMagnitudeBand {
    float Min = 0.0f;
    float Max = 0.0f;
    EMythicAffixScaleMode ScaleMode = EMythicAffixScaleMode::None;
    float LinearPerItemLevel = 0.0f;
    float CurveTailGrowth = 1.0f;
    FRichCurve LevelScalingCurve;

    bool Resolve(int32 ItemLevel, float &OutMin, float &OutMax) const;
};

/** Immutable dependency and content-hash record for one concrete profile. */
USTRUCT()
struct MYTHIC_API FMythicAffixDependencyManifest {
    GENERATED_BODY()

    /** Concrete profile represented by this closure. */
    UPROPERTY(VisibleAnywhere) FPrimaryAssetId ProfileId;

    /** Runtime assets that must remain resident with this closure. */
    UPROPERTY(VisibleAnywhere) TArray<FPrimaryAssetId> RuntimePrimaryAssets;

    /** Presentation assets that may be streamed for UI. */
    UPROPERTY(VisibleAnywhere) TArray<FPrimaryAssetId> PresentationPrimaryAssets;

    /** Gameplay-semantic closure fingerprint. */
    UPROPERTY(VisibleAnywhere) FMythicContentHash GameplayContentHash;

    /** Presentation-only closure fingerprint. */
    UPROPERTY(VisibleAnywhere) FMythicContentHash PresentationContentHash;
};

/** One ordered tier in a compiled contextual progression. */
struct MYTHIC_API FCompiledAffixTier {
    int32 TierRank = 0;
    FName DeveloperName;
    int32 PresentationRevision = 1;
    int32 MinItemLevel = 1;
    int64 TierWeight = 0;
    int64 BudgetCost = 0;
    FCompiledAffixMagnitudeBand Magnitude;
};

/** One context-selected tier progression copied from an Affix Definition. */
struct MYTHIC_API FCompiledAffixTierProgression {
    FName DeveloperName;
    FName TuningContext;
    FGameplayTagQuery ApplicabilityQuery;
    int32 SelectionPriority = 0;
    TArray<FCompiledAffixTier> Tiers;
};

/** Canonical semantics copied for deterministic generation; application re-resolves the live assets. */
struct MYTHIC_API FCompiledAffixDefinition {
    FMythicAffixDefinitionHandle Definition;
    FPrimaryAssetId DefinitionId;
    FGameplayTag AffixTag;
    int32 Revision = 1;
    int32 PresentationRevision = 1;
    FPrimaryAssetId TargetStatId;
    FPrimaryAssetId TargetCategoryId;
    FGameplayTag TargetStatTag;
    FGameplayAttribute TargetAttribute;
    int32 TargetStatRevision = 1;
    int32 TargetStatPresentationRevision = 1;
    int32 TargetCategoryPresentationRevision = 1;
    EGameplayModOp::Type ModifierOp = EGameplayModOp::AddBase;
    FMythicAffixQuantization Quantization;
    EMythicStatComparisonDirection ComparisonDirection = EMythicStatComparisonDirection::HigherIsBetter;
    float NeutralValue = 0.0f;
    FGameplayTag StackingGroup;
    EMythicAffixStackingRule StackingRule = EMythicAffixStackingRule::UniquePerItem;
    FGameplayTagContainer ConflictGroups;
};

/** Fully compiled Affix Definition containing every contextual tier progression. */
struct MYTHIC_API FCompiledAffix {
    FCompiledAffixDefinition Definition;
    TArray<FCompiledAffixTierProgression> Progressions;

    /** Resolves one highest-priority progression; ties and no-match fail closed. */
    const FCompiledAffixTierProgression *ResolveProgression(
        const FGameplayTagContainer &ContextTags) const;
};

/** Compiled guaranteed affix source. */
struct MYTHIC_API FCompiledAffixGrant {
    FMythicAffixGrantSpec Spec;
    FCompiledAffix Affix;
};

/** Immutable closure used by gems, sockets, crafting, and other exact-grant sources. */
struct MYTHIC_API FCompiledAffixGrantClosure {
    FMythicAffixGrantSpec Spec;
    FCompiledAffix Affix;
    FMythicContentHash GameplayContentHash;
    FMythicContentHash PresentationContentHash;
    int32 AlgorithmVersion = 1;
};

/** Compiled weighted pool row. */
struct MYTHIC_API FCompiledAffixPoolRow {
    FGuid PoolRowGuid;
    FName DeveloperName;
    int32 RowRevision = 1;
    FGameplayTag RollGroup;
    int64 SelectionWeight = 0;
    FGameplayTagQuery EligibilityQuery;
    FCompiledAffix Affix;
};

/** Compiled profile slice and its resident pool rows. */
struct MYTHIC_API FCompiledAffixSlice {
    FGuid SliceGuid;
    FName DeveloperName;
    FPrimaryAssetId PoolId;
    int32 PoolRevision = 1;
    FGameplayTag SourceKind;
    int32 Priority = 0;
    int32 SliceWeight = 1;
    TArray<FCompiledAffixPoolRow> Rows;
};

/** Fixed-point rarity generation budget. */
struct MYTHIC_API FCompiledAffixRarityBudget {
    EItemRarity Rarity = EItemRarity::Common;
    int32 RandomRollCount = 0;
    TMap<FGameplayTag, int32> RollGroupCaps;
    bool bUnlimitedMagnitudeBudget = true;
    int64 MagnitudeBudget = 0;
};

/** Immutable roll-policy copy. */
struct MYTHIC_API FCompiledAffixPolicy {
    FPrimaryAssetId PolicyId;
    int32 Revision = 1;
    int32 AlgorithmVersion = 1;
    bool bGuaranteedConsumesMagnitudeBudget = false;
    bool bDisallowDuplicateAffixDefinition = true;
    bool bDisallowDuplicateTargetStat = true;
    EMythicAffixShortfallMode ShortfallMode = EMythicAffixShortfallMode::AllowPartial;
    bool bAllowLowerPriorityFallback = false;
    TMap<EItemRarity, FCompiledAffixRarityBudget> Budgets;
};

/** Immutable generation closure for one concrete profile asset. */
struct MYTHIC_API FCompiledAffixProfile {
    FPrimaryAssetId ProfileId;
    int32 ProfileRevision = 1;
    FCompiledAffixPolicy Policy;
    TArray<FCompiledAffixGrant> GuaranteedGrants;
    TArray<FCompiledAffixSlice> RandomSlices;
    FMythicAffixDependencyManifest DependencyManifest;
    FMythicContentHash GameplayContentHash;
    FMythicContentHash PresentationContentHash;
};

/** Validates authoring and publishes deterministic immutable affix closures. */
struct MYTHIC_API FMythicAffixCompiler {
    static constexpr int64 FixedPointScale = 1000000;

    static bool TryCompileFixedPoint(double Value, bool bRequirePositive, int64 &OutValue);
    static bool CompileMagnitudeBand(const FMythicAffixMagnitudeBand &Source,
                                     FCompiledAffixMagnitudeBand &OutBand,
                                     TArray<FText> &OutErrors);
    static bool ValidateDefinitionAuthoring(const UMythicAffixDefinition &Definition,
                                            const FMythicStatRegistry &StatRegistry,
                                            TArray<FText> &OutErrors);
    static bool ValidatePoolAuthoring(const UMythicAffixPool &Pool, TArray<FText> &OutErrors);
    static bool CompileDefinition(const FMythicAffixDefinitionHandle &Handle,
                                  const UMythicItemizationDataRegistrySubsystem &Registry,
                                  FCompiledAffix &OutAffix,
                                  TArray<FText> &OutErrors);
    static bool CompileGrant(const FMythicAffixGrantSpec &Spec,
                             const UMythicItemizationDataRegistrySubsystem &Registry,
                             TSharedPtr<const FCompiledAffixGrantClosure> &OutGrant,
                             TArray<FText> &OutErrors);
    static bool ValidateStructuralFeasibility(const FCompiledAffixProfile &Profile,
                                              TArray<FText> &OutErrors);
    static bool Compile(const UMythicAffixProfile &Profile,
                        const UMythicItemizationDataRegistrySubsystem &Registry,
                        TSharedPtr<const FCompiledAffixProfile> &OutProfile,
                        TArray<FText> &OutErrors);
};
