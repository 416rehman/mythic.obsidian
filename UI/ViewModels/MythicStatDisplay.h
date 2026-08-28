// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Stats/MythicStatTypes.h"
#include "MythicStatDisplay.generated.h"

class UMythicStatDefinition;
class UMythicStatSummaryLibrary;
class UTexture2D;

/** Display-ready state for one canonical StatDefinition and the ASC values it addresses. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatLine {
    GENERATED_BODY()

    /** Canonical gameplay tag identity of the stat represented by this row. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FGameplayTag StatTag;

    /** Canonical category tag used to group and style this row. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FGameplayTag CategoryTag;

    /** Localized player-facing stat name. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Label;

    /** Localized explanation of the stat's gameplay effect. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Description;

    /** Display-ready current value, including pair-aware composition where applicable. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Value;

    /** Display-ready total difference between current and base values for the rendered row. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText BonusText;

    /** Bonus text for the row's primary/current attribute before pair-aware composition. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText PrimaryBonusText;

    /** Canonical identity and presentation for a capacity folded into this current-stat row. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FGameplayTag PairedStatTag;

    /** Gameplay Ability System attribute addressed by PairedStatTag. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FGameplayAttribute PairedAttribute;

    /** Canonical formatting rules for the paired capacity value. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FMythicStatNumberPresentation PairedNumberPresentation;

    /** Display-ready current-minus-base delta for the paired capacity stat. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText PairedBonusText;

    /** Unmodified base value of the row's primary Gameplay Ability System attribute. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float BaseValue = 0.0f;

    /** GAS base after permanent equipment affixes are composed and before temporary Gameplay Effects. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float EquipmentBaseValue = 0.0f;

    /** Live final value of the row's primary Gameplay Ability System attribute. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float CurrentValue = 0.0f;

    /** CurrentValue minus BaseValue for the row's primary stat. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float BonusValue = 0.0f;

    /** EquipmentBaseValue minus BaseValue for the row's primary stat. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float EquipmentBonusValue = 0.0f;

    /** CurrentValue minus EquipmentBaseValue for temporary effects on the row's primary stat. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float TemporaryBonusValue = 0.0f;

    /** Unmodified base value of the paired capacity stat, or zero when no pair is rendered. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float PairedBaseValue = 0.0f;

    /** Equipment-composed GAS base of the paired capacity stat, or zero when no pair is rendered. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float PairedEquipmentBaseValue = 0.0f;

    /** Live final value of the paired capacity stat, or zero when no pair is rendered. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float PairedCurrentValue = 0.0f;

    /** PairedCurrentValue minus PairedBaseValue. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float PairedBonusValue = 0.0f;

    /** Equipment-only delta for the paired capacity stat. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float PairedEquipmentBonusValue = 0.0f;

    /** Temporary-effect delta for the paired capacity stat. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float PairedTemporaryBonusValue = 0.0f;

    /** True when permanent equipment changes either attribute represented by this row. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    bool bHasEquipmentBonus = false;

    /** True when temporary Gameplay Effects change either attribute represented by this row. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    bool bHasTemporaryBonus = false;

    /** True when either the primary stat or its rendered pair differs meaningfully from its base value. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    bool bHasBonus = false;

    /** True when the row's primary stat differs meaningfully from its base value. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    bool bPrimaryStatHasBonus = false;

    /** True when the paired capacity stat differs meaningfully from its base value. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    bool bPairedStatHasBonus = false;

    /** Current divided by capacity in the range 0..1, or -1 when this row has no rendered capacity pair. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float BarPercent = -1.0f;

    /** True when the category style requests the primary/high-emphasis row treatment. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    bool bEmphasizeRow = false;

    /** True when the category permits a tooltip breakdown of this stat's downstream contributions. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    bool bEnableContributionDrilldown = false;

    /** Canonical formatting rules copied from the stat definition. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FMythicStatNumberPresentation NumberPresentation;

    /** Indicates which numeric direction should be presented as beneficial. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    EMythicStatComparisonDirection ComparisonDirection = EMythicStatComparisonDirection::HigherIsBetter;

    /** Ascending authored order of this row within its category. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    int32 SortOrder = 0;

    /** Gameplay Ability System attribute addressed by this row. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FGameplayAttribute Attribute;
};

/** One headline card computed by its authored summary calculation. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatSummaryLine {
    GENERATED_BODY()

    /** Stable gameplay tag identity of the authored headline summary calculation. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FGameplayTag SummaryId;

    /** Localized player-facing summary-card heading. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Label;

    /** Display-ready result of the summary calculation. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Value;

    /** Localized explanation of what the summary measures. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Description;

    /** Optional soft icon supplied by the authored summary definition. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    TSoftObjectPtr<UTexture2D> Icon;

    /** Unformatted numeric result of the summary calculation. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float RawValue = 0.0f;
};

/** Project setting for optional data-driven headline summaries; canonical stat identity stays in Stat Definitions. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mythic Stat Sheet"))
class MYTHIC_API UMythicStatDisplaySettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override {
        return FName(TEXT("Game"));
    }

    /** Optional data-driven library that defines the stat sheet's headline summary cards. */
    UPROPERTY(EditAnywhere, Config, Category = "Stat Sheet")
    TSoftObjectPtr<UMythicStatSummaryLibrary> SummaryLibrary;
};

/** One line of a primary stat's tooltip: what it feeds, and how much it is feeding it right now. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatContributionLine {
    GENERATED_BODY()

    /** Localized name of a downstream stat affected by the inspected primary stat. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Label;

    /** Display-ready amount currently contributed to the downstream stat. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Value;

    /** Normalized share of the source stat that contributes to this downstream result. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float Fraction = 0.0f;

    /** True when diminishing returns reduced this contribution below its undiminished amount. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    bool bDiminished = false;
};

namespace MythicStatDisplay {
    /** Formats a final/base stat value using its canonical StatDefinition presentation. */
    MYTHIC_API FText FormatValue(float Value, const FMythicStatNumberPresentation& Presentation);

    /** Formats a signed current-minus-base delta. */
    MYTHIC_API FText FormatBonus(float Delta, const FMythicStatNumberPresentation& Presentation);

    /** Format-only overloads for summary calculations that do not represent a canonical stat. */
    MYTHIC_API FText FormatValue(float Value, EMythicStatFormat Format);
    MYTHIC_API FText FormatBonus(float Delta, EMythicStatFormat Format);

    /** Derives modifier presentation from a canonical stat, never from an attribute-name heuristic. */
    MYTHIC_API FMythicStatNumberPresentation ResolveModifierPresentation(
        const UMythicStatDefinition& Definition,
        TEnumAsByte<EGameplayModOp::Type> ModifierOp);

    /** Identity used when folding item-local modifier contributions; distinct from the final stat's baseline. */
    MYTHIC_API float GetModifierContributionIdentity(
        TEnumAsByte<EGameplayModOp::Type> ModifierOp,
        float FinalStatNeutralValue);

    /** Exact visibility rule for WhenModifiedOrNonNeutral. */
    MYTHIC_API bool ShouldRender(const UMythicStatDefinition& Definition, float BaseValue, float CurrentValue);

    /**
     * Finds the one already-resident Stat Definition for an attribute without loading or inventing semantics.
     * Systems that own a game-instance registry should query that registry directly.
     */
    MYTHIC_API const UMythicStatDefinition* FindResidentDefinition(const FGameplayAttribute& Attribute);

    /** Non-shipping diagnostic only. Shipping presentation defers instead of inventing a stat identity. */
    MYTHIC_API FText GetUnknownStatDiagnostic();
}
