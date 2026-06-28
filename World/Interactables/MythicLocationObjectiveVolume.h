// Mythic — location-objective trigger volume
// Drives non-combat "reach/visit X" objectives: when a player first enters, the SERVER fires GAS.Event.ReachedLocation
// on that player's ASC (TargetTags = {LocationTag}), which the per-player UObjectiveTracker advances. Server-authoritative,
// per-player one-shot (re-entering doesn't re-count), overlap-driven (no tick). A designer places one and authors an
// objective with TriggerEventTag = GAS.Event.ReachedLocation + RequiredPayloadTag = the same LocationTag.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "MythicLocationObjectiveVolume.generated.h"

class UBoxComponent;
class AController;
struct FHitResult;

UCLASS()
class MYTHIC_API AMythicLocationObjectiveVolume : public AActor {
    GENERATED_BODY()

public:
    AMythicLocationObjectiveVolume();

    // Pure: an overlap emits the reach event only on authority, for an overlapper whose ASC resolved, and not already
    // credited for this volume. Static + no engine state so the emit gate is unit-testable without a live overlap.
    static bool ShouldEmitReachEvent(bool bHasAuthority, bool bResolvedPlayerASC, bool bAlreadyFiredForPlayer);

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnVolumeBeginOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp,
                              int32 OtherBodyIndex, bool bFromSweep, const FHitResult &Sweep);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Location Objective")
    UBoxComponent *Trigger;

    // The location id this volume reports. An objective's RequiredPayloadTag must match it. Empty = inert (never fires).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Location Objective")
    FGameplayTag LocationTag;

private:
    // Players (by controller) already credited for this volume — the per-player one-shot, so re-entry doesn't re-count a
    // multi-count objective. Runtime-only (a save/load mid-objective can re-credit on re-entry — logged edge).
    TSet<TWeakObjectPtr<AController>> FiredControllers;
};
