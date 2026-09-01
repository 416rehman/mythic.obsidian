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
 * has no write path, and it is recomputed on read. Combat-critical summaries use native calculations shared with
 * gameplay; presentation-only summaries may use Blueprint subclasses. Calculations run on the class default object
 * and therefore must remain
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

    /** Native optional range projection; scalar summaries leave this false and use Calculate. */
    virtual bool CalculateRange(const UAbilitySystemComponent *ASC,
                                float &OutMinimum,
                                float &OutMaximum) const {
        OutMinimum = 0.0f;
        OutMaximum = 0.0f;
        return false;
    }
};

/**
 * Canonical character-effective basic-attack damage summary. It consumes the same weapon range, primary-stat
 * contribution, and equipped weapon-class bonus as the damage execution, so tuning moves combat and the sheet
 * together.
 */
UCLASS(NotBlueprintable)
class MYTHIC_API UMythicBasicAttackDamageSummaryCalculation final
    : public UMythicStatSummaryCalculation {
    GENERATED_BODY()

public:
    virtual float Calculate_Implementation(const UAbilitySystemComponent *ASC) const override;
    virtual bool CalculateRange(const UAbilitySystemComponent *ASC,
                                float &OutMinimum,
                                float &OutMaximum) const override;
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

    /** Pure native or Blueprint calculation class that defines how this headline value is derived from GAS. */
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

    /** Resolves an optional minimum-to-maximum display band from the authored calculation class. */
    bool ComputeRange(const UAbilitySystemComponent *ASC,
                      float &OutMinimum,
                      float &OutMaximum) const;
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
