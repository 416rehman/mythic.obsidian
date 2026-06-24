#pragma once

#include "CoreMinimal.h"
#include "SavedObjective.generated.h"

/**
 * One persisted objective: the definition asset (soft path) + this player's progress toward it. Mirrors
 * FSerializedProficiencyData. The UObjectiveTracker serializes/restores these itself (it also re-subscribes the
 * restored objectives' trigger tags on load), so there is no separate helper here.
 */
USTRUCT(BlueprintType)
struct FSerializedObjectiveData {
    GENERATED_BODY()

    // Reference to the UObjectiveDefinition data asset.
    UPROPERTY(BlueprintReadWrite)
    FSoftObjectPath ObjectiveAsset;

    // Occurrences accumulated so far (clamped to the definition's RequiredCount on completion).
    UPROPERTY(BlueprintReadWrite)
    int32 CurrentCount = 0;

    // Whether the objective was already completed (rewards granted) — restored complete so it neither re-advances
    // nor re-grants.
    UPROPERTY(BlueprintReadWrite)
    bool bCompleted = false;
};
