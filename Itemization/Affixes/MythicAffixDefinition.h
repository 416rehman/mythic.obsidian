#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "Itemization/MythicDataAsset.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "MythicAffixDefinition.generated.h"

/** Definition-owned numeric roll range and item-level scaling rule for one affix tier. */
USTRUCT(BlueprintType, meta = (DisplayName = "Affix Modifier Range"))
struct MYTHIC_API FMythicAffixMagnitudeBand {
    GENERATED_BODY()

    /** Inclusive minimum roll before the selected item-level scaling mode is applied. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Min = 0.0f;

    /** Inclusive maximum roll before the selected item-level scaling mode is applied. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) float Max = 0.0f;

    /** Chooses whether this range is fixed, grows linearly, or is multiplied by a level curve. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EMythicAffixScaleMode ScaleMode = EMythicAffixScaleMode::None;

    /** Amount added to both range endpoints per item level when Linear scaling is selected. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (EditCondition = "ScaleMode == EMythicAffixScaleMode::Linear", EditConditionHides))
    float LinearPerItemLevel = 0.0f;

    /** Inline definition-owned scaling curve; validation rejects its optional engine-level External Curve reference. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (EditCondition = "ScaleMode == EMythicAffixScaleMode::Curve", EditConditionHides))
    FRuntimeFloatCurve LevelScalingCurve;

    /** Minimum open-ended growth factor used to extrapolate the curve beyond its last authored key. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              meta = (EditCondition = "ScaleMode == EMythicAffixScaleMode::Curve", EditConditionHides,
                      ClampMin = "1.0"))
    float CurveTailGrowth = 1.0f;
};

/** One selectable item-level tier on an affix definition, including its weight, cost, and roll range. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAffixTierDefinition {
    GENERATED_BODY()

    /** Internal, non-localized tier label used in authoring, validation, and diagnostics. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DeveloperName;

    /** Revision of this tier's localized label and other presentation-only data. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 PresentationRevision = 1;

    /** Optional localized player-facing tier name used by detailed or debug presentation. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;

    /** Lowest item level at which this tier can be selected. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "1")) int32 MinItemLevel = 1;

    /** Relative deterministic-selection weight among eligible tiers on this Affix Definition. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.000001")) float TierWeight = 1.0f;

    /** Amount consumed from the rarity's magnitude budget when this tier is selected. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0")) float BudgetCost = 0.0f;

    /** Numeric range for the owning Affix Definition's single authoritative stat modifier. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Roll Range"))
    FMythicAffixMagnitudeBand Magnitude;

};

/** Context-selected ordered tier ladder owned directly by one canonical affix definition. */
USTRUCT(BlueprintType, meta = (DisplayName = "Affix Tier Progression"))
struct MYTHIC_API FMythicAffixTierProgressionDefinition {
    GENERATED_BODY()

    /** Internal, non-localized label describing the tuning context, such as Core, Weapon, or Armour. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DeveloperName;

    /** Short semantic role that distinguishes this ladder from other ladders on the same affix. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName TuningContext;

    /**
     * Data-driven item/source conditions under which this progression applies. An empty query is the fallback;
     * validation rejects contexts where multiple progressions tie at the same priority.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Applies When"))
    FGameplayTagQuery ApplicabilityQuery;

    /** Highest matching conditional progression wins; the empty-query fallback is used only when none match. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
    int32 SelectionPriority = 0;

    /** Free-form tuning guidance for designers; never shown to players. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true)) FString DesignerNotes;

    /** Ordered item-level tiers available whenever this contextual progression is selected. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FMythicAffixTierDefinition> Tiers;
};

/** Canonical Primary Data Asset that owns one player-facing affix's stat semantics and all contextual tiers. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicAffixDefinition : public UMythicDataAsset {
    GENERATED_BODY()

public:
    /** Internal, non-localized label used to find this definition in authoring and diagnostics. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FName DeveloperName;

    /** Explains the intended player fantasy and tuning role to content authors; never shown to players. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true)) FString DesignerPurpose;

    /** Gameplay-semantic revision stored in rolled-affix provenance for telemetry and tuning diagnostics. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 Revision = 1;

    /** Revision of localized text and presentation-only choices, independent of gameplay tuning. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) int32 PresentationRevision = 1;

    /** Stable gameplay-tag identity for this player-facing affix across pools and profiles. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.Affix"))
    FGameplayTag AffixTag;

    /** Localized player-facing affix name, formatted from the canonical stat and rolled value. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayNameTemplate;

    /** Localized player-facing explanation, formatted from the canonical stat and rolled value. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true)) FText DescriptionTemplate;

    /** Canonical data-driven stat modified by this player-facing affix. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Target Stat"))
    FMythicStatDefinitionHandle TargetStat;

    /** Permanent-stat operation composed by the equipment stat-source ledger before temporary Gameplay Effects. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Operation"))
    TEnumAsByte<EGameplayModOp::Type> ModifierOp = EGameplayModOp::AddBase;

    /** Rounding applied once when rolled so save, network, tooltip, stat sheet, and gameplay values stay identical. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Value Rounding"))
    FMythicAffixQuantization Quantization;

    /**
     * Optional cross-affix family used to share one stacking decision. When unset, every non-StackAll rule
     * automatically uses AffixTag, so a definition never has to duplicate its own identity here.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTag StackingGroup;

    /** Controls whether copies stack, are unique per item, or resolve to the strongest copy. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    EMythicAffixStackingRule StackingRule = EMythicAffixStackingRule::UniquePerItem;

    /** Semantic groups that make this affix mutually exclusive with otherwise unrelated affixes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FGameplayTagContainer ConflictGroups;

    /** Contextual tier progressions owned by this affix; pools reference only the Affix Definition. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayName = "Tier Progressions"))
    TArray<FMythicAffixTierProgressionDefinition> TierProgressions;

    /** Resolves exactly one contextual tier progression; returns null when no progression matches or a priority ties. */
    const FMythicAffixTierProgressionDefinition *ResolveTierProgression(
        const FGameplayTagContainer &ContextTags) const;

    /** Returns the single typed stacking identity used by compilation and runtime duplicate resolution. */
    FGameplayTag GetEffectiveStackingGroup() const;

    static bool ResolveMagnitudeBand(const FMythicAffixMagnitudeBand &Band, int32 ItemLevel,
                                     float &OutMin, float &OutMax);
    virtual FPrimaryAssetId GetPrimaryAssetId() const override;
#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};
