#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "MythicLandmarkPOI.generated.h"

class USphereComponent;
struct FHitResult;

UCLASS()
class MYTHIC_API AMythicLandmarkPOI : public AActor {
    GENERATED_BODY()

public:
    AMythicLandmarkPOI();

protected:
    virtual void BeginPlay() override;
#if WITH_EDITOR
    virtual void OnConstruction(const FTransform &Transform) override;
#endif

    UFUNCTION()
    void OnDiscoverySphereBeginOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp,
                                       int32 OtherBodyIndex, bool bFromSweep, const FHitResult &Sweep);

    // The discovery trigger. Its radius is kept in sync with DiscoveryRadius.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "POI")
    USphereComponent *DiscoverySphere;

    // Stable POI id — the fast-travel node key + the world-shared unlock/dedup key. Must be unique + match any save data.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "POI")
    int32 POIId = -1;

    // Category/identity tag stored with the POI (POI.Landmark by default; set a more specific POI.* per landmark).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "POI")
    FGameplayTag POITag;

    // Player-facing name surfaced in the discovery toast + on the map ("Sunken Temple").
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "POI")
    FText DisplayName;

    // Discovery radius (cm). A player entering this range unlocks the POI. Also the "standing at this POI" zone the
    // fast-travel source-resolve uses. Drives the sphere's radius.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "POI", meta = (ClampMin = "0.0"))
    float DiscoveryRadius = 800.0f;
};
