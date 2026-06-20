// Per-player objective/quest tracker — subscribes to GAS kill events, advances active objectives, grants rewards.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "ObjectiveTracker.generated.h"

class UObjectiveDefinition;
class UAbilitySystemComponent;
struct FGameplayEventData;

/** Per-player runtime progress toward one objective. Mirrors the faction-standing entry (small replicated struct
 *  in a TArray — no TMap). */
USTRUCT(BlueprintType)
struct FObjectiveProgress {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    TObjectPtr<UObjectiveDefinition> Definition = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    int32 CurrentCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    bool bCompleted = false;
};

/**
 * Hosted on AMythicPlayerController. On the server it binds to the player's ASC GAS.Event.Kill callback (the same
 * attributed event combat already emits on the killer's ASC), advances every active matching objective, and on
 * reaching the required count grants the objective's rewards via FRewardsToGive::Give and latches it complete. The
 * objective list replicates COND_OwnerOnly so only the owning client sees its own quest progress (for UI).
 *
 * Scope: one trigger family (GAS.Event.Kill), reward-on-complete, owner-only replication. Multi-step chains,
 * branching, prerequisites, NPC objective-giver hooks, the tracker UI, and save persistence are follow-ups.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UObjectiveTracker : public UActorComponent {
    GENERATED_BODY()

public:
    UObjectiveTracker();

    // SERVER: assign an objective to this player (authority-gated; mirrors FactionStanding's server gate).
    // Idempotent — a no-op if this player already has the objective (active or completed), so a quest-giver NPC
    // can be talked to repeatedly without re-adding or resetting the quest.
    UFUNCTION(BlueprintCallable, Category = "Objectives")
    void ServerAddObjective(UObjectiveDefinition *Definition);

    // True if this player already tracks the given objective (active or completed). Used to gate re-offers.
    UFUNCTION(BlueprintPure, Category = "Objectives")
    bool HasObjective(const UObjectiveDefinition *Definition) const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // SERVER: GAS gameplay-event callback bound to GAS.Event.Kill on the owning player's ASC.
    void HandleGameplayEvent(const FGameplayEventData *Payload);

    // Active + completed objectives for this player. Owner-only so quest progress stays private to its owner.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Objectives")
    TArray<FObjectiveProgress> ActiveObjectives;

private:
    // The ASC we bound on (the PlayerState's ASC, reused across pawns).
    UPROPERTY()
    UAbilitySystemComponent *BoundASC = nullptr;

    // SERVER: subscribe HandleGameplayEvent to a given trigger tag's gameplay event on BoundASC (idempotent).
    void EnsureSubscribedToTag(const FGameplayTag &Tag);

    // One bound handle per DISTINCT objective TriggerEventTag, so objectives keyed to any emitted GAS.Event.*
    // (Kill / Death / Dmg.Delivered / Heal.* …) advance — not just kills. EndPlay unbinds each.
    TMap<FGameplayTag, FDelegateHandle> BoundEventHandles;
};
