
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "Narrative/MythicStoryCondition.h"
#include "MythicStorylineDefinition.generated.h"

class UMythicQuestDefinition;

UCLASS(BlueprintType)
class MYTHIC_API UMythicStorylineDefinition : public UDataAsset {
    GENERATED_BODY()

public:
    // Canonical arc identity tag (telemetry / cross-references). Not required for progression.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Storyline")
    FGameplayTag ArcTag;

    // Ordered quests making up this arc. The journal starts them in order (each still gated by its own quest-level
    // UnlockConditions) and advances as each completes.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Storyline")
    TArray<TObjectPtr<UMythicQuestDefinition>> Quests;

    // Tag-driven gate deciding whether the ARC may start at all (against owned story tags ∪ world flags). Empty = ungated.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Storyline|Narrative")
    FMythicStoryCondition ArcGate;

    // Arc-completion rewards, granted once every non-optional quest in the arc is Completed. Reuses FRewardsToGive.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Storyline")
    FRewardsToGive Rewards;

    // Story tags stamped into the ledger when the arc completes — the arc's lasting consequence.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Storyline|Narrative")
    FGameplayTagContainer GrantStoryTagsOnComplete;

    // Player-facing arc header + body.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Storyline|UI")
    FText JournalTitle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Storyline|UI")
    FText JournalText;
};
