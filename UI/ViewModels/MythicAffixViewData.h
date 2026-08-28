// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Stats/MythicStatTypes.h"
#include "MythicAffixViewData.generated.h"

class UMythicItemizationDataRegistrySubsystem;

/** Display-ready projection of the single stat modifier owned by an Affix Definition. */
USTRUCT(BlueprintType, meta = (DisplayName = "Affix Stat Modifier View"))
struct MYTHIC_API FMythicAffixValueViewData {
    GENERATED_BODY()

    /** Canonical identity of the stat currently targeted by the live Affix Definition. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FGameplayTag StatTag;

    /** Localized player-facing name supplied by the live Stat Definition. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FText StatLabel;

    /** Rolled magnitude formatted with the target Stat Definition and current modifier operation. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FText FormattedValue;

    /** Live roll range formatted when item context was supplied; empty for context-free projections. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FText FormattedRange;

    /** Permanent-stat operation currently authored by the live Affix Definition. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    TEnumAsByte<EGameplayModOp::Type> ModifierOp = EGameplayModOp::AddBase;

    /** Canonical modifier-aware number presentation derived from the live target Stat Definition. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FMythicStatNumberPresentation NumberPresentation;

    /** Immutable rolled magnitude used by gameplay and player-facing presentation. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    float RawValue = 0.0f;

    /** Modifier value normalized into target-stat space; division stores its reciprocal for honest comparisons. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    float ComparisonValue = 0.0f;

    /** Baseline of the final GAS stat used by stat-sheet visibility and override presentation. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    float FinalStatNeutralValue = 0.0f;

    /** Identity used when folding this modifier: zero for addition and one for multiplication or division. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    float ContributionIdentity = 0.0f;

    /** Defines whether a larger, smaller, or neither contribution is presented as the better roll. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    EMythicStatComparisonDirection ComparisonDirection = EMythicStatComparisonDirection::Neutral;
};

/**
 * Player-facing projection of one rolled affix.
 *
 * The snapshot contributes only its direct Affix Definition reference, tier rank, magnitude, and audit provenance.
 * Target stat, operation, localized text, category, formatting, and comparison semantics are always resolved from the
 * currently loaded Affix Definition -> Stat Definition chain, so presentation cannot drift from gameplay authority.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixViewData {
    GENERATED_BODY()

    /** Stable identity of this individual rolled affix instance. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FGuid RollGuid;

    /** Canonical semantic tag resolved from the referenced live Affix Definition. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FGameplayTag AffixTag;

    /** Localized affix name with its stat and value placeholders resolved. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FText DisplayName;

    /** Localized affix explanation with its stat and value placeholders resolved. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FText Description;

    /** Complete display line for tooltip widgets that support the project's rich-text markup. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FText RichText;

    /** Stat currently targeted by the live Affix Definition. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FGameplayTag PrimaryStatTag;

    /** Data-driven Stat Category used to group and style this affix in player-facing UI. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FGameplayTag SemanticCategoryTag;

    /** Roll group that produced the affix, such as prefix or suffix. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes", meta = (DisplayName = "Roll Group"))
    FGameplayTag RollGroup;

    /** Origin of the affix, such as implicit, random, crafted, socketed, or gem-granted. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FGameplayTag SourceKind;

    /** One-based rank selected from the matching contextual progression at generation time. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes", meta = (ClampMin = "1"))
    int32 TierRank = 0;

    /** Localized tier label resolved by rank from live definition data, with a localized numeric fallback. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    FText TierDisplayName;

    /** True when gameplay rules prohibit rerolling or replacing this affix. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    bool bLocked = false;

    /** Canonical modifier projection; the one-affix/one-stat architecture always produces exactly one entry. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Affixes")
    TArray<FMythicAffixValueViewData> Values;
};

UCLASS()
class MYTHIC_API UMythicAffixViewDataLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    /**
     * Builds presentation from an already-loaded semantic closure. Returns false and no partial result when the
     * referenced Affix Definition or target Stat Definition is unavailable or inconsistent with the snapshot.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Affixes", meta = (DisplayName = "Build Affix View Data"))
    static bool BuildViewData(const FRolledAffix &Snapshot,
                              const UMythicItemizationDataRegistrySubsystem *Registry,
                              FMythicAffixViewData &OutViewData);

    /**
     * Builds the same live semantic projection and also resolves roll quality from the progression selected by the
     * supplied item tags and item level. The range is recomputed from current data and is never cached on the roll.
    */
    UFUNCTION(BlueprintPure, Category = "Mythic|Affixes",
              meta = (DisplayName = "Build Affix View Data With Item Context"))
    static bool BuildViewDataWithItemContext(
        const FRolledAffix &Snapshot,
        const FGameplayTagContainer &ItemContextTags,
        int32 ItemLevel,
        const UMythicItemizationDataRegistrySubsystem *Registry,
        FMythicAffixViewData &OutViewData);
};
