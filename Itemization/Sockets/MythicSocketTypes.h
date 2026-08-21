#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/MythicTags_Inventory.h"
#include "MythicSocketTypes.generated.h"

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
