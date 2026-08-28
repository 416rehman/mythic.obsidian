#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MythicSocketTypes.generated.h"

namespace MythicSocketSerialization {
MYTHIC_API extern const FGuid ReplicatedSocketArrayMagic;
static constexpr int32 ReplicatedSocketArrayVersion = 1;
static constexpr int32 MaxSocketsPerItem = 8;
static constexpr int32 MaxAffixesPerSocket = 64;
}

USTRUCT(BlueprintType)
struct FMythicSocketSlot {
    GENERATED_BODY()

    /** Optional gem-type restriction for THIS socket (empty = universal). See FMythicSocketMath::IsGemCompatible. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Socket", meta = (Categories = "Itemization.Gem"))
    FGameplayTag SocketColor;

    /** The gem-type currently slotted (invalid when the socket is empty). Drives runeword sequence matching. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Socket")
    FGameplayTag SocketedGemType;

    /** COPY of the socketed gem's granted affixes (applied to the wearer while equipped). Empty when the socket is empty. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Socket")
    TArray<FRolledAffix> SocketedAffixes;

    /** True when a gem is slotted here. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Socket")
    bool bFilled = false;
};

/**
 * One authoritative socket in the host-owned outer Fast Array.
 *
 * SocketedAffixSnapshots is deliberately an ordinary bounded array. Nesting a second Fast Array here would make
 * delta ownership and save restoration ambiguous; changing any contained snapshot marks this outer item dirty.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicReplicatedSocketItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    /** Stable host-local identity used to rekey copied gem grants and preserve socket provenance across saves. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Socket")
    FGuid SocketGuid;

    /** Optional compatibility restriction for this socket; an empty tag accepts every valid gem type. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Socket", meta = (Categories = "Itemization.Gem"))
    FGameplayTag SocketColor;

    /** Gem-type identity currently occupying the socket; invalid when bFilled is false. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Socket", meta = (Categories = "Itemization.Gem"))
    FGameplayTag SocketedGemType;

    /** Identity of the physical gem whose immutable grants were copied into this host socket. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Socket")
    FGuid SourceGemItemGuid;

    /** Immutable host-owned copies of the inserted gem's grants; these snapshots drive equipped gameplay effects. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Socket")
    TArray<FRolledAffix> SocketedAffixSnapshots;

    /** Authoritative occupancy flag; when false, gem identity and copied snapshots must be empty. */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Socket")
    bool bFilled = false;

    bool NetSerialize(FArchive &Ar, UPackageMap *Map, bool &bOutSuccess);
};

template <>
struct TStructOpsTypeTraits<FMythicReplicatedSocketItem>
    : TStructOpsTypeTraitsBase2<FMythicReplicatedSocketItem> {
    enum { WithNetSerializer = true };
};

/** Host-owned delta container. SaveGame serialization never persists Fast Array replication bookkeeping. */
USTRUCT()
struct MYTHIC_API FMythicReplicatedSocketArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    TArray<FMythicReplicatedSocketItem> Items;

    /** Non-owning runtime callback target; deliberately outside reflection, replication, duplication, and persistence. */
    TWeakObjectPtr<UObject> Owner;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicReplicatedSocketItem,
                                                              FMythicReplicatedSocketArray>(
            Items, DeltaParms, *this);
    }

    bool Serialize(FArchive &Ar);
    void PostReplicatedAdd(const TArrayView<int32> &Added, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &Changed, int32 FinalSize);
    void PreReplicatedRemove(const TArrayView<int32> &Removed, int32 FinalSize);
    void SetOwner(UObject *InOwner) { Owner = InOwner; }
};

template <>
struct TStructOpsTypeTraits<FMythicReplicatedSocketArray>
    : TStructOpsTypeTraitsBase2<FMythicReplicatedSocketArray> {
    enum { WithNetDeltaSerializer = true, WithSerializer = true };
    static constexpr EPropertyObjectReferenceType WithSerializerObjectReferences =
        EPropertyObjectReferenceType::None;
};

USTRUCT(BlueprintType)
struct FMythicSocketCountRule {
    GENERATED_BODY()

    /** Item-type this rule applies to; matched hierarchically against the item's type (e.g. a Weapon rule matches a Sword). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sockets", meta = (Categories = "Itemization.Type"))
    FGameplayTag ItemTypeParent;

    /** Max sockets per rarity index (Common..Mythic). Author non-decreasing to keep RollSocketCount monotonic in rarity. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sockets")
    TArray<int32> MaxByRarity;
};

USTRUCT(BlueprintType)
struct FMythicSocketCountTable {
    GENERATED_BODY()

    /** Item-type-specific rarity ceilings; the most specific matching parent tag wins. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sockets")
    TArray<FMythicSocketCountRule> Rules;

    /** Absolute ceiling on sockets regardless of rarity/level. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sockets")
    int32 HardCap = 6;

    /** Item levels required to UNLOCK each additional socket (item-level gate). 1 = level never binds below the cap. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Sockets")
    int32 ItemLevelsPerSocket = 1;
};

struct FMythicSocketMath {
    static int32 FindRuleIndex(const FGameplayTag &ItemType, const FMythicSocketCountTable &Table) {
        int32 Best = INDEX_NONE;
        int32 BestDepth = -1;
        for (int32 i = 0; i < Table.Rules.Num(); ++i) {
            const FGameplayTag &Parent = Table.Rules[i].ItemTypeParent;
            if (!Parent.IsValid() || !ItemType.MatchesTag(Parent)) {
                continue;
            }
            const int32 Depth = Parent.ToString().Len();
            if (Depth > BestDepth) {
                BestDepth = Depth;
                Best = i;
            }
        }
        return Best;
    }

    static int32 RollSocketCount(const FGameplayTag &ItemType, int32 ItemLevel, int32 RarityIndex,
                                 const FMythicSocketCountTable &Table, float Roll01) {
        const int32 RuleIdx = FindRuleIndex(ItemType, Table);
        if (RuleIdx == INDEX_NONE) {
            return 0;
        }
        const FMythicSocketCountRule &Rule = Table.Rules[RuleIdx];
        if (!Rule.MaxByRarity.IsValidIndex(RarityIndex)) {
            return 0;
        }
        const int32 HardCap = FMath::Max(0, Table.HardCap);
        const int32 RarityCap = FMath::Clamp(Rule.MaxByRarity[RarityIndex], 0, HardCap);

        const int32 Pace = FMath::Max(1, Table.ItemLevelsPerSocket);
        const int32 LevelCap = FMath::Clamp(FMath::Max(0, ItemLevel) / Pace, 0, HardCap);

        const int32 EffectiveCap = FMath::Min(RarityCap, LevelCap);
        if (EffectiveCap <= 0) {
            return 0;
        }
        const float Clamped01 = FMath::Clamp(Roll01, 0.0f, 1.0f);
        const int32 Count = FMath::RoundToInt(Clamped01 * static_cast<float>(EffectiveCap));
        return FMath::Clamp(Count, 0, EffectiveCap);
    }

    static bool IsGemCompatible(const FGameplayTag &GemType, const FGameplayTag &SocketColor) {
        if (!SocketColor.IsValid()) {
            return true;
        }
        return GemType.IsValid() && GemType.MatchesTag(SocketColor);
    }

    static FMythicSocketCountTable DefaultSocketCountTable() {
        FMythicSocketCountTable Table;
        Table.HardCap = 6;
        Table.ItemLevelsPerSocket = 1;

        auto AddRule = [&Table](const FGameplayTag &Parent, TArray<int32> Max) {
            FMythicSocketCountRule Rule;
            Rule.ItemTypeParent = Parent;
            Rule.MaxByRarity = MoveTemp(Max);
            Table.Rules.Add(MoveTemp(Rule));
        };
        AddRule(ITEMIZATION_TYPE_EQUIPMENT_WEAPON, {0, 1, 2, 3, 3});
        AddRule(ITEMIZATION_TYPE_EQUIPMENT_GEAR, {0, 1, 2, 3, 3});
        AddRule(ITEMIZATION_TYPE_EQUIPMENT_ACCESSORY, {0, 0, 1, 2, 2});
        return Table;
    }
};
