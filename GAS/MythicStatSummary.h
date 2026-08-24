#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/MythicDataAsset.h"
#include "Templates/SubclassOf.h"
#include "UI/ViewModels/MythicStatDisplay.h"
#include "MythicStatSummary.generated.h"

class UAbilitySystemComponent;
class UTexture2D;

/**
 * A summarized value — one headline number computed from the stats that feed it, the way Diablo's Damage / Toughness
 * / Recovery answer "how strong am I" without reading forty rows. It is a calculation result: nothing writes it, it
 * has no write path, and it is recomputed on read. A designer authors one Blueprint subclass per summary and
 * overrides Calculate; C++ owns only this base, and the override runs on the class default object, so it must stay
 * pure — read the ASC, return a number, touch no member state.
 */
UCLASS(Blueprintable, Abstract)
class MYTHIC_API UMythicStatSummaryCalculation : public UObject {
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Mythic|Stats")
    float Calculate(const UAbilitySystemComponent *ASC) const;
    virtual float Calculate_Implementation(const UAbilitySystemComponent *ASC) const { return 0.0f; }
};

/** One row of the summary catalogue: what it is called, how it reads, and the calculation behind it. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicStatSummaryDefinition : public UMythicDataAsset {
    GENERATED_BODY()

public:
    // Stable identity (Stat.Summary.*). The panel keys its card pool on this so a reorder re-texts rather than rebuilds.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary", meta = (Categories = "Stat.Summary"))
    FGameplayTag SummaryId;

    // The one word players say aloud when they compare gear. Keep it a word, not a sentence.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary")
    FText Label;

    // What the number means, shown on the card's tooltip. Supports the project's rich-text markup.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary")
    TSoftObjectPtr<UTexture2D> Icon;

    // The Blueprint that computes this summary. Its Calculate override is the whole definition of the number.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary")
    TSubclassOf<UMythicStatSummaryCalculation> CalculationClass;

    // How the card reads its number — through the sheet's one formatter, so "1,240" and "38%" cannot grow a
    // second formatting path.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary")
    EMythicStatFormat Format = EMythicStatFormat::Integer;

    // This summary's value for a given ability system, computed on read. Zero when no calculation is authored, so a
    // half-authored row reads as a plain zero rather than crashing or showing a stale number.
    UFUNCTION(BlueprintPure, Category = "Summary")
    float Compute(const UAbilitySystemComponent *ASC) const;
};

/** Every summary the panel may show, in display order. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicStatSummaryLibrary : public UMythicDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary")
    TArray<TObjectPtr<UMythicStatSummaryDefinition>> Summaries;
};
