#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicAchievementCondition.h"
#include "Rewards/RewardBase.h"
#include "MythicAchievementDefinition.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class MYTHIC_API UMythicAchievementDefinition : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    // Stable identity of this achievement AND the earned-tag emitted on unlock (mirrored into the narrative ledger).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement")
    FGameplayTag AchievementTag;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement")
    TSoftObjectPtr<UTexture2D> Icon;

    // The unlock predicate (tag gate + stat thresholds). All-empty = never auto-fires (see the component — an authored
    // achievement should always carry at least one stat requirement or tag clause).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement")
    FMythicAchievementCondition Condition;

    // Reward granted once on unlock (NOT re-given on save-load — the component gates on the persisted unlocked set).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement")
    FRewardsToGive Reward;

    // TRUE = an account-wide achievement (persist across characters). Account persistence is deferred (mirrors the stat
    // ledger's account counters); for now this is informational and every unlock persists per-character.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement")
    bool bAccountScope = false;

    // TRUE = hidden until unlocked (UI hint only — the unlock logic ignores it).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Achievement")
    bool bHidden = false;
};
