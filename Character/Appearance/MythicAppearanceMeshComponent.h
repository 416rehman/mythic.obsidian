#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "MythicAppearanceMeshComponent.generated.h"

class UMaterialInstanceDynamic;

/** Own the underlying palette even while a camera temporarily replaces the slot. */
UCLASS(Transient)
class MYTHIC_API UMythicAppearanceMeshComponent : public USkeletalMeshComponent {
    GENERATED_BODY()
public:
    UMaterialInstanceDynamic* GetPaletteMaterial(int32 Slot, UMaterialInterface* Base);
private:
    // Outer alone is not a GC reference. GameplayCameras retains original materials
    // as raw pointers while occluded, so mesh lifetime must retain these instances.
    UPROPERTY(Transient) TMap<int32,TObjectPtr<UMaterialInstanceDynamic>> PaletteMaterials;
};
