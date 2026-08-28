#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/MythicDataAsset.h"
#include "MythicItemizationRuleset.generated.h"

class UMythicAffixProfile;

/**
 * Typed release/season control plane for item affix generation.
 *
 * A ruleset declares the concrete profile assets enabled by a build, season, playlist, or DLC. Membership is a real
 * asset reference that rename/move tooling can update and Asset Manager can cook as one dependency closure.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicItemizationRuleset : public UMythicDataAsset {
    GENERATED_BODY()

public:
    /** Internal, non-localized label used by release engineering, validation, and diagnostics. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName DeveloperName;

    /** Explains the season, playlist, or content cohort controlled by this ruleset; never shown to players. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MultiLine = true))
    FString DesignerPurpose;

    /** Gameplay-semantic revision used by manifests, telemetry, and tuning diagnostics. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Revision = 1;

    /** Locked semantic identity used in manifests and telemetry; cross-asset links use typed references instead. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "Itemization.Ruleset"))
    FGameplayTag RulesetTag;

    /** Concrete item-family profiles enabled by this ruleset, including unique/signature profiles as separate assets. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AssetBundles = "Runtime"))
    TArray<TSoftObjectPtr<UMythicAffixProfile>> Profiles;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};
