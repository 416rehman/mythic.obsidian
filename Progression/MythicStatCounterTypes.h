#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MythicStatCounterTypes.generated.h"

/**
 * Stat. is two namespaces sharing one root. Ledger counters are cumulative events this component accumulates and
 * replicates. The stat sheet's identities - Stat.Attribute.* (a live GAS attribute), Stat.Category.* (a sheet
 * grouping) and Stat.Summary.* (a headline computed on read) - have no write path at all, so a ledger row carrying
 * one of those tags would be a stale shadow of a value the sheet computes elsewhere.
 */
struct FMythicStatCounterTag {
    static const TArray<FString> &StatSheetIdentityPrefixes() {
        static const TArray<FString> Prefixes = {
            TEXT("Stat.Attribute."),
            TEXT("Stat.Category."),
            TEXT("Stat.Summary."),
        };
        return Prefixes;
    }

    static bool IsStatSheetIdentity(const FString &TagName) {
        for (const FString &Prefix : StatSheetIdentityPrefixes()) {
            if (TagName.StartsWith(Prefix)) {
                return true;
            }
        }
        return false;
    }

    /** True only for tags the ledger may accumulate: under Stat., and not one of the stat sheet's identities. */
    static bool IsLedgerCounter(const FGameplayTag &Tag) {
        if (!Tag.IsValid()) {
            return false;
        }
        const FString Name = Tag.ToString();
        return Name.StartsWith(TEXT("Stat.")) && !IsStatSheetIdentity(Name);
    }
};

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
