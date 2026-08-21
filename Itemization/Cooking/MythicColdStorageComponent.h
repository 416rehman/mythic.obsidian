
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MythicColdStorageComponent.generated.h"

UCLASS(Blueprintable, ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicColdStorageComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicColdStorageComponent() {
        PrimaryComponentTick.bCanEverTick = false;
    }

    /** Aging-rate multiplier applied to perishables stored on this actor: 1 = normal (default), 0.25 = cold, 0 = frozen. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cold Storage", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PreservationMultiplier = 1.0f;

    UFUNCTION(BlueprintPure, Category = "Cold Storage")
    float GetPreservationMultiplier() const { return FMath::Max(0.0f, PreservationMultiplier); }
};
