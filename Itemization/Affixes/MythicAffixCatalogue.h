#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MythicAffixTierTypes.h"
#include "MythicAffixCatalogue.generated.h"

USTRUCT(BlueprintType)
struct FMythicAffixCatalogueEntry {
    GENERATED_BODY()

    // Stable id the item-type rules name. Renaming it orphans every rule that references it, which
    // IsDataValid reports rather than silently dropping the affix. Ids compare case-insensitively, so
    // "Affix_Power" and "affix_power" are the same id.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affixes")
    FName AffixId;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affixes")
    FMythicTieredAffixDef Def;
};

USTRUCT(BlueprintType)
struct FMythicItemTypeAffixRule {
    GENERATED_BODY()

    // An Itemization.Type.* tag. Every rule the item type matches contributes: a deeper rule ADDS to the
    // broader one rather than replacing it.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affixes")
    FGameplayTag ItemType;

    // Guaranteed on every item of this type. Naming an id here rolls it - Applicability filters the random
    // pool only, never a core affix.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affixes")
    TArray<FName> CoreAffixIds;

    // Empty means every entry whose Applicability matches the item, so a broad type needs no list. That
    // meaning survives a narrower rule: a child naming two ids adds them to the parent's open pool.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affixes")
    TArray<FName> RandomAffixIds;
};

/** Pure lookup rules, kept off the asset so tests exercise them without loading content. */
struct MYTHIC_API FMythicAffixCatalogueMath {
    static int32 TagDepth(const FGameplayTag &Tag) {
        TArray<FGameplayTag> Parents;
        Tag.ParseParentTags(Parents);
        return Parents.Num() + 1;
    }

    /**
     * Every rule the item type matches, deepest first, ties to the earlier index. The chain merges rather
     * than overrides: a Weapon.Sword rule authored only to narrow the random pool must not wipe the core
     * stats the broad Weapon rule guarantees.
     */
    static void ResolveRuleChain(TConstArrayView<FMythicItemTypeAffixRule> Rules, const FGameplayTag &ItemType,
                                 TArray<int32> &OutIndices) {
        OutIndices.Reset();
        if (!ItemType.IsValid()) {
            return;
        }
        for (int32 i = 0; i < Rules.Num(); ++i) {
            const FGameplayTag &RuleTag = Rules[i].ItemType;
            if (RuleTag.IsValid() && ItemType.MatchesTag(RuleTag)) {
                OutIndices.Add(i);
            }
        }
        OutIndices.StableSort([Rules](const int32 A, const int32 B) {
            return TagDepth(Rules[A].ItemType) > TagDepth(Rules[B].ItemType);
        });
    }

    /** The deepest matching rule, or INDEX_NONE. Equal depth resolves to the earlier rule. */
    static int32 ResolveRuleIndex(TConstArrayView<FMythicItemTypeAffixRule> Rules, const FGameplayTag &ItemType) {
        TArray<int32> Chain;
        ResolveRuleChain(Rules, ItemType, Chain);
        return (Chain.Num() > 0) ? Chain[0] : INDEX_NONE;
    }

    static int32 FindEntryIndex(TConstArrayView<FMythicAffixCatalogueEntry> Entries, FName AffixId) {
        if (AffixId.IsNone()) {
            return INDEX_NONE;
        }
        for (int32 i = 0; i < Entries.Num(); ++i) {
            if (Entries[i].AffixId == AffixId) {
                return i;
            }
        }
        return INDEX_NONE;
    }
};

/**
 * Which affixes exist, and which of them each item type may roll.
 *
 * Entries own the tiered ladders; rules point item types at them by id. One "Increased Attack Speed"
 * ladder is authored once and claimed by every weapon type, instead of each pool asset carrying its own
 * drifting copy of the same numbers.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicAffixCatalogue : public UDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affixes")
    TArray<FMythicAffixCatalogueEntry> Entries;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affixes")
    TArray<FMythicItemTypeAffixRule> RulesByItemType;

    const FMythicAffixCatalogueEntry *FindEntry(FName AffixId) const;

    /** The deepest rule the item type matches. Callers wanting the whole inherited chain use ResolveRuleChain. */
    const FMythicItemTypeAffixRule *ResolveRule(const FGameplayTag &ItemType) const;

    /**
     * Appends the guaranteed defs for this item type, merged across every rule it matches. Returns the number
     * appended. Never clears Out. TypeProbe is carried for symmetry and future gating, not used as a filter:
     * an id named in CoreAffixIds rolls.
     */
    int32 BuildCoreDefs(const FGameplayTag &ItemType, const FGameplayTagContainer &TypeProbe,
                        TArray<FMythicTieredAffixDef> &Out) const;

    /**
     * Appends the rollable defs for this item type, filtered by the item's own probe. Returns the number
     * appended. Never clears Out, and never appends one entry twice.
     *
     * RandomAffixIds is unioned across every rule the type matches. An EMPTY list on ANY rule in that chain
     * means "everything applicable", so those entries are appended BESIDE the named ones - a Sword rule
     * naming two ids widens the open pool a Weapon rule left open, it does not shrink it to two. An item
     * type matching no rule at all takes the same open pool, so unruled content rolls the generic list
     * rather than nothing.
     *
     * Overlap with the core half is de-duplicated at the roll site against what actually rolled, because a
     * fragment's own CoreAffixes beat the catalogue's and this list cannot know which won.
     */
    int32 BuildRandomDefs(const FGameplayTag &ItemType, const FGameplayTagContainer &TypeProbe,
                          TArray<FMythicTieredAffixDef> &Out) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};
