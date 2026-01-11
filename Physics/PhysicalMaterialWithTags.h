#pragma once

#include "GameplayTagContainer.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include "PhysicalMaterialWithTags.generated.h"

class UObject;

/**
 * UPhysicalMaterialWithTags
 *
 * A physical material that has gameplay tags associated with it
 */
UCLASS()
class UPhysicalMaterialWithTags : public UPhysicalMaterial {
    GENERATED_BODY()

public:
    UPhysicalMaterialWithTags(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

    // A container of gameplay tags that game code can use to reason about this physical material
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=PhysicalProperties)
    FGameplayTagContainer Tags;
};
