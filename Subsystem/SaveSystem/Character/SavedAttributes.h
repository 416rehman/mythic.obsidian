#pragma once

#include "CoreMinimal.h"
#include "SavedAttributes.generated.h"

class UAbilitySystemComponent;

USTRUCT(BlueprintType)
struct FSerializedAttributesData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    TMap<FString, float> Attributes;

    UPROPERTY(BlueprintReadWrite)
    TArray<FSoftClassPath> GrantedAbilities;

    static void Serialize(UAbilitySystemComponent *ASC, FSerializedAttributesData &OutData);
    static void Deserialize(UAbilitySystemComponent *ASC, const FSerializedAttributesData &InData);
};
