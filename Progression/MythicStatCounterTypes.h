#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MythicStatCounterTypes.generated.h"

/** Replicated gameplay-tag counter row used by the data-driven progression stat ledger. */
USTRUCT(BlueprintType)
struct FMythicStatCounter : public FFastArraySerializerItem {
    GENERATED_BODY()

    /** Canonical gameplay-tag identity of the tracked progression statistic. */
    UPROPERTY(BlueprintReadOnly, Category = "Progression|Stats")
    FGameplayTag Tag;

    /** Authoritative lifetime counter value replicated for this statistic. */
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
