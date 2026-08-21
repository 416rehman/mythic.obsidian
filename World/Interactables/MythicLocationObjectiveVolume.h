
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
    TSet<TWeakObjectPtr<AController>> FiredControllers;
};
