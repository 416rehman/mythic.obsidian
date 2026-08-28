#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/MythicDataAsset.h"
#include "Stats/MythicStatTypes.h"
#include "Templates/SubclassOf.h"
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
    /** Computes the current headline value from the supplied Ability System Component without mutating state. */
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Mythic|Stats")
    float Calculate(const UAbilitySystemComponent *ASC) const;
    virtual float Calculate_Implementation(const UAbilitySystemComponent *ASC) const { return 0.0f; }
};

/** One row of the summary catalogue: what it is called, how it reads, and the calculation behind it. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicStatSummaryDefinition : public UMythicDataAsset {
    GENERATED_BODY()

public:
    /** Stable Stat.Summary.* identity used to preserve card identity when designers reorder the library. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary", meta = (Categories = "Stat.Summary"))
    FGameplayTag SummaryId;

    /** Short player-facing heading, such as Damage, Toughness, or Recovery. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary")
    FText Label;

    /** Player-facing explanation shown in the summary card tooltip; rich-text markup is supported. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary", meta = (MultiLine = true))
    FText Description;

    /** Optional icon displayed on the summary card. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary")
    TSoftObjectPtr<UTexture2D> Icon;

    /** Pure Blueprint calculation class that defines how this headline value is derived from GAS. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary")
    TSubclassOf<UMythicStatSummaryCalculation> CalculationClass;

    // How the card reads its number — through the sheet's one formatter, so "1,240" and "38%" cannot grow a
    // second formatting path.
    /** Canonical number format shared with ordinary stat-sheet rows. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary")
    EMythicStatFormat Format = EMythicStatFormat::Integer;

    /** Computes this summary on demand; returns zero when no calculation class is authored. */
    UFUNCTION(BlueprintPure, Category = "Summary")
    float Compute(const UAbilitySystemComponent *ASC) const;
};

/** Every summary the panel may show, in display order. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicStatSummaryLibrary : public UMythicDataAsset {
    GENERATED_BODY()

public:
    /** Summary definitions in player-facing display order. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Summary")
    TArray<TObjectPtr<UMythicStatSummaryDefinition>> Summaries;
};
