#pragma once
#include "ActiveGameplayEffectHandle.h"
#include "AttributeSet.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Itemization/Affixes/MythicAffixTierTypes.h"
#include "Net/UnrealNetwork.h"
#include "AffixesFragment.generated.h"

class UMythicAffixCatalogue;


USTRUCT(BlueprintType, Blueprintable)
struct FRolledAffix : public FRolledAttributeSpec {
    GENERATED_BODY()

    // Whether the affix can be refined. By default, the Forge's "Refine" system will lock these after it has refined 1 of the affixes.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess=true))
    bool bIsLocked = false;

    // C1 — roll quality for tooltips. When this affix was rolled from a TIERED pool, TierIndex is the chosen tier's
    // index into its def's ladder (0 = strongest rung authored first is up to the designer) and TierLabel is that
    // tier's designer label ("Superior" / "T2"). -1 / empty for a legacy flat roll (no tier concept). Replicated with
    // the affix (part of the default per-property struct replication) so the owning client's tooltip can show it.
    UPROPERTY(BlueprintReadOnly)
    int32 TierIndex = -1;

    UPROPERTY(BlueprintReadOnly)
    FText TierLabel;

    FRolledAffix(FGameplayAttribute Attribute, int ItemLvl, FRollDefinition &RollDef, bool IsLocked = true) : FRolledAttributeSpec(
        Attribute, ItemLvl, RollDef) {
        bIsLocked = IsLocked;
    }

    FRolledAffix() : FRolledAttributeSpec() {
        bIsLocked = false;
    }

    bool Serialize(FArchive &Ar) {
        if (!Ar.IsSaveGame()) {
            return false;
        }
        FRolledAttributeSpec::Serialize(Ar);
        Ar << bIsLocked;
        Ar << TierIndex;
        FString TierLabelStr = Ar.IsSaving() ? TierLabel.ToString() : FString();
        Ar << TierLabelStr;
        if (Ar.IsLoading()) {
            TierLabel = FText::FromString(TierLabelStr);
        }
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FRolledAffix> : TStructOpsTypeTraitsBase2<FRolledAffix> {
    enum {
        WithSerializer = true
    };
};

USTRUCT(BlueprintType)
struct FAffixesRuntimeReplicatedData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, SaveGame)
    TArray<FRolledAffix> RolledCoreAffixes = TArray<FRolledAffix>();

    UPROPERTY(BlueprintReadWrite, SaveGame)
    TArray<FRolledAffix> RolledAffixes = TArray<FRolledAffix>();

    UPROPERTY(SaveGame)
    bool bEquipEventEmitted = false;

    // Server-only runtime wiring. NotReplicated so the default per-property struct replication doesn't push a
    // server ASC object reference to the owning client on every equip/unequip (it's read only on the server, where
    // affixes are activated/rerolled). The replicated struct then carries only the rolled affix arrays.
    UPROPERTY(NotReplicated, BlueprintReadOnly)
    UAbilitySystemComponent *ASC = nullptr;

    // Permanent. Once set, CanApplyCraftOp refuses every craft op on this item. Replicates with the struct so the
    // owning client's tooltip can say so, and persists by the default tagged-property path like bEquipEventEmitted.
    UPROPERTY(BlueprintReadOnly, SaveGame)
    bool bCorrupted = false;
};

USTRUCT(BlueprintType)
struct FAffixesRuntimeClientOnlyData {
    GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FAffixesRuntimeServerOnlyData {
    GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FAffixesConfig {
    GENERATED_BODY()
};

USTRUCT(BlueprintType)
struct FAffixesBuildData {
    GENERATED_BODY()

    // Core Stats are GUARANTEED to be rolled when this item is instanced
    UPROPERTY(EditDefaultsOnly)
    TMap<FGameplayAttribute, FRollDefinition> CoreAffixes = TMap<FGameplayAttribute, FRollDefinition>();

    // Affixes from this pool will be applied to this item when it is instanced.
    // The amount of affixes on the final item is determined by the "Rarity" of the item.
    // Common = 1; Rare = 2; Epic = 3; Legendary = 4; Mythic = 5;
    UPROPERTY(EditDefaultsOnly)
    TMap<FGameplayAttribute, FRollDefinition> AffixPoolMap = TMap<FGameplayAttribute, FRollDefinition>();

    // The catalogue THIS item rolls from, replacing the shared one in Mythic Loot Settings. Set it when an item
    // needs a bespoke set; leave it null and the item takes the shared catalogue matched on its item type.
    // It answers BOTH halves, so a bespoke item can never end up half-overridden.
    UPROPERTY(EditDefaultsOnly)
    TObjectPtr<UMythicAffixCatalogue> AffixCatalogueOverride = nullptr;
};


UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UAffixesFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Affixes)

    /** Designer friendly configuration data that defines this fragment. */
    /** REPLICATED and fields should be BlueprintReadOnly */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties), SaveGame)
    FAffixesConfig AffixesConfig = FAffixesConfig();

    /** This is used in the OnInstanced method to calculate/fill the rest of the data. */
    /** This should not be replicated or blueprint accessible and safely discarded after being used in the OnInstanced method. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
    FAffixesBuildData AffixesBuildData = FAffixesBuildData();

    /** Contains the runtime state of the fragment (replicated to client) */
    /** REPLICATED */
    UPROPERTY(Replicated, BlueprintReadOnly, SaveGame)
    FAffixesRuntimeReplicatedData AffixesRuntimeReplicatedData = FAffixesRuntimeReplicatedData();

    /** Contains the runtime client side state of the fragment for use in methods like OnActiveItemClient */
    /** Shouldn't be accessed on server side methods like OnActiveItem. */
    UPROPERTY(BlueprintReadOnly)
    FAffixesRuntimeClientOnlyData AffixesRuntimeClientOnlyData = FAffixesRuntimeClientOnlyData();

    /** Contains the runtime server-only state of the fragment for use in methods like OnActiveItem */
    /** Shouldn't be accessed on client side methods like OnActiveItemClient. */
    UPROPERTY(BlueprintReadOnly)
    FAffixesRuntimeServerOnlyData AffixesRuntimeServerOnlyData = FAffixesRuntimeServerOnlyData();

    void RollAffixes(int ItemLevel, int Qty);

    void RollAffixesTiered(int ItemLevel, int TotalCount, const FGameplayTagContainer &TypeProbe,
                           TConstArrayView<FMythicTieredAffixDef> Defs);

    void RollCoreAffixes(int ItemLevel);

    // Core affixes are GUARANTEED: every def rolls, none compete for a weighted slot, and Applicability does not
    // filter them - a designer naming a core def means it rolls. Each still picks a tier from its own ladder, and
    // each rolls LOCKED so a Refine reroll cannot take the item's core stats. TypeProbe is carried for symmetry
    // with the random roller, not used as a filter.
    void RollCoreAffixesTiered(int ItemLevel, const FGameplayTagContainer &TypeProbe,
                               TConstArrayView<FMythicTieredAffixDef> Defs);

    static void ApplyAffixes(UAbilitySystemComponent *ASC, TArray<FRolledAffix> &InRolledAffixes);
    static void RemoveAffixes(UAbilitySystemComponent *ASC, TArray<FRolledAffix> &InRolledAffixes);

    static bool ComputeReversedModValue(TEnumAsByte<EGameplayModOp::Type> Modifier, float Value, float &OutReversed);

    // SERVER: re-roll the value of every UNLOCKED random affix (locked affixes AND core affixes — which roll
    // locked — keep their values). Authority-gated (mirrors UDurabilityFragment::ServerApplyWear). If the item is
    // currently active on an ASC, the live GAS modifiers are reversed (with the OLD values) and re-applied (with
    // the NEW values) so the change takes effect immediately and reversibly. ItemLevel scales the roll range.
    // The "Refine/Forge" gameplay verb: re-roll the stats you don't like, lock the ones you do.
    UFUNCTION(BlueprintCallable, Category = "Affixes")
    void RerollUnlockedAffixes(int32 ItemLevel);

    // SERVER: lock/unlock a random affix by index so RerollUnlockedAffixes preserves it. Authority-gated.
    UFUNCTION(BlueprintCallable, Category = "Affixes")
    void SetAffixLocked(int32 AffixIndex, bool bLocked);

    // C4 (crafting) — is this item permanently corrupted (blocks all crafting ops)? Read on server + owning client.
    UFUNCTION(BlueprintPure, Category = "Affixes")
    bool IsCorrupted() const { return AffixesRuntimeReplicatedData.bCorrupted; }

    /** Whether a craft op may run, and why not when it may not. Every craft verb asks this before touching state. */
    UFUNCTION(BlueprintPure, Category = "Affixes")
    bool CanApplyCraftOp(FText &OutReason) const;

    /**
     * SERVER: seals this item against every future craft op, permanently. The risk half of crafting - what makes
     * a good roll worth stopping on. Refused when item corruption is switched off in settings.
     */
    UFUNCTION(BlueprintCallable, Category = "Affixes")
    void ServerCorruptItem();


    static bool IsAffixRolled(const FGameplayAttribute &Affix, TArray<FRolledAffix> &InRolledAffixes);

    static bool ShouldApplyAffixes(bool bBroken);

    void OnDurabilityBrokenStateChanged(bool bBroken);

    static bool ShouldEmitArmorEquipEvent(bool bIsCanonicalAffixesFragment, bool bItemHasWeaponFragment, bool bAlreadyEmitted);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        REP_FRAGMENT_DATA(Affixes)
    }

#if WITH_EDITOR
    virtual bool IsValidFragment(FText &OutErrorMessage) const override;
#endif
    virtual void OnInstanced(UMythicItemInstance *Instance) override;
    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) override;

    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;
};

inline void UAffixesFragment::RollAffixesTiered(int ItemLevel, int TotalCount, const FGameplayTagContainer &TypeProbe,
                                                TConstArrayView<FMythicTieredAffixDef> Defs) {
    if (Defs.Num() == 0 || TotalCount <= 0) {
        return;
    }

    TArray<int32> EligibleDefs;
    EligibleDefs.Reserve(Defs.Num());
    for (int32 i = 0; i < Defs.Num(); ++i) {
        const FMythicTieredAffixDef &Def = Defs[i];
        if (!Def.Attribute.IsValid()) {
            continue;
        }
        if (!Def.Applicability.IsEmpty() && !Def.Applicability.Matches(TypeProbe)) {
            continue;
        }
        if (FMythicAffixTierMath::SumEligibleTierWeight(ItemLevel, Def.Tiers) <= 0.0f) {
            continue;
        }
        EligibleDefs.Add(i);
    }
    if (EligibleDefs.Num() == 0) {
        return;
    }

    const FMythicAffixBudget Budget = FMythicAffixTierMath::ComputeAffixBudget(TotalCount);
    int32 PrefixAdded = 0;
    int32 SuffixAdded = 0;

    const int32 TotalCap = Budget.PrefixCap + Budget.SuffixCap;
    for (int32 Guard = 0; (PrefixAdded + SuffixAdded) < TotalCap && Guard < 256; ++Guard) {
        TArray<float, TInlineAllocator<16>> Weights;
        TArray<int32, TInlineAllocator<16>> Candidates;
        for (const int32 DefIdx : EligibleDefs) {
            const FMythicTieredAffixDef &Def = Defs[DefIdx];
            // Core rolls first and owns its attributes: rolling one again here would apply the stat twice, print
            // it twice on the tooltip, and let a Refine reroll only half of it.
            if (IsAffixRolled(Def.Attribute, this->AffixesRuntimeReplicatedData.RolledAffixes)
                || IsAffixRolled(Def.Attribute, this->AffixesRuntimeReplicatedData.RolledCoreAffixes)) {
                continue;
            }
            if (!FMythicAffixTierMath::BudgetAllows(Def.Group, PrefixAdded, SuffixAdded, Budget)) {
                continue;
            }
            Weights.Add(FMythicAffixTierMath::SumEligibleTierWeight(ItemLevel, Def.Tiers));
            Candidates.Add(DefIdx);
        }
        if (Candidates.Num() == 0) {
            break;
        }

        const int32 Picked = FMythicAffixTierMath::WeightedPickDef(Weights, FMath::FRand());
        if (Picked < 0) {
            break;
        }
        const FMythicTieredAffixDef &Def = Defs[Candidates[Picked]];

        const int32 TierIdx = FMythicAffixTierMath::SelectTierIndex(ItemLevel, Def.Tiers, FMath::FRand());
        if (!Def.Tiers.IsValidIndex(TierIdx)) {
            continue;
        }
        const FMythicAffixTier &Tier = Def.Tiers[TierIdx];

        FRollDefinition RollDef;
        RollDef.Min = Tier.Min;
        RollDef.Max = Tier.Max;
        RollDef.Modifier = Def.ModOp;
        RollDef.LevelScaling = Tier.LevelScaling;
        RollDef.bWholeNumber = Def.bWholeNumber;

        FRolledAffix NewAffix(Def.Attribute, ItemLevel, RollDef, false);
        // The tier roll overwrites the ctor's value, so the whole-number snap must re-apply here.
        NewAffix.Value = FMythicAffixTierMath::RollValueInTier(Tier, ItemLevel, FMath::FRand());
        if (RollDef.bWholeNumber) {
            NewAffix.Value = FMath::RoundToFloat(NewAffix.Value);
        }
        if ((RollDef.Modifier == EGameplayModOp::Multiplicitive || RollDef.Modifier == EGameplayModOp::Division)
            && FMath::IsNearlyZero(NewAffix.Value)) {
            NewAffix.Value = KINDA_SMALL_NUMBER;
        }
        NewAffix.TierIndex = TierIdx;
        NewAffix.TierLabel = Tier.TierLabel;

        this->AffixesRuntimeReplicatedData.RolledAffixes.Add(NewAffix);
        if (Def.Group == EMythicAffixGroup::Prefix) {
            ++PrefixAdded;
        }
        else {
            ++SuffixAdded;
        }
    }
}
