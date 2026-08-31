#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MythicHarvestToolTypeDefinition.generated.h"

class UAnimMontage;
class UNiagaraSystem;
class USoundBase;
class UTexture2D;

/**
 * Direct semantic identity and shared presentation for one harvest-tool family. Runtime eligibility compares this
 * exact asset reference; AnalyticsTaxonomyTag is descriptive telemetry and never authorization evidence.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicHarvestToolTypeDefinition : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    /**
     * Definition-owned localized family name readable on every peer; reading has no side effects, an empty value
     * fails asset validation, and this text carries no gameplay units or authorization authority.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Tool Type|Presentation")
    FText DisplayName;

    /**
     * Definition-owned optional UI icon loaded for presentation on any peer; reading is side-effect free, a null
     * asset is allowed, load failure suppresses only the icon, and the value has no gameplay units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Tool Type|Presentation")
    TSoftObjectPtr<UTexture2D> Icon;

    /**
     * Definition-owned optional contact effect selected after an accepted server result; Blueprint may present but
     * never commit it, a null/load failure is mechanically inert, and its transform units are supplied by feedback.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Tool Type|Presentation")
    TSoftObjectPtr<UNiagaraSystem> ContactEffect;

    /**
     * Definition-owned optional action sound selected after an accepted server result; playback is presentation-only,
     * a null/load failure is mechanically inert, and this reference carries no gameplay units.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Tool Type|Presentation")
    TSoftObjectPtr<USoundBase> ActionSound;

    /**
     * Definition-owned swing shared by every tool of this family, played instead of the granting item's own attack
     * montage. The tool is never wielded; only the animation is substituted. Unset falls back to that item's montage,
     * and a montage without the same authored hit-sample notifies a weapon attack requires is refused, not played.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Tool Type|Presentation")
    TSoftObjectPtr<UAnimMontage> HarvestMontage;

    /**
     * Definition-owned optional analytics classification readable on every peer; it causes no side effects, may be
     * unset, has no units, and must never be used to authorize a tool family or replace direct asset equality.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Tool Type|Analytics")
    FGameplayTag AnalyticsTaxonomyTag;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    bool AppendValidationErrors(TArray<FText> &OutErrors) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};
