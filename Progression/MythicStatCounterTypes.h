#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MythicStatCounterTypes.generated.h"

USTRUCT(BlueprintType)
struct FMythicStatCounter : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Progression|Stats")
    FGameplayTag Tag;

    UPROPERTY(BlueprintReadOnly, Category = "Progression|Stats")
    int64 Value = 0;
};

USTRUCT()
struct FMythicStatCounterArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicStatCounter> Items;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicStatCounter, FMythicStatCounterArray>(Items, DeltaParms, *this);
    }
};

template <>
struct TStructOpsTypeTraits<FMythicStatCounterArray> : public TStructOpsTypeTraitsBase2<FMythicStatCounterArray> {
    enum { WithNetDeltaSerializer = true };
};
