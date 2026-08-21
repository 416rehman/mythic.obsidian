#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SavedWorldFlags.generated.h"

USTRUCT()
struct FSerializedPOIUnlock {
    GENERATED_BODY()

    UPROPERTY()
    int32 POIId = INDEX_NONE;

    UPROPERTY()
    FVector Anchor = FVector::ZeroVector;

    UPROPERTY()
    FGameplayTag POITag;

    UPROPERTY()
    FText DisplayName;

    UPROPERTY()
    float Radius = 0.0f;
};
