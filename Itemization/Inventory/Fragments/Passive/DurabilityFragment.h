//

#pragma once

#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Net/UnrealNetwork.h"
#include "DurabilityFragment.generated.h"

/** Which durability state-transition a feedback callout is announcing (server -> owning client). */
UENUM(BlueprintType)
enum class EMythicItemDurabilityBeat : uint8 {
    LowWarning, // durability wore down to/below the low-warning threshold (still usable)
    Broken,     // durability reached 0 — the item broke (a weapon now deals no damage until repaired)
    Repaired,   // a broken item was restored to working condition
};

/** Designer-authored config: the item's maximum durability. MaxDurability <= 0 disables durability. */
USTRUCT(BlueprintType)
struct FDurabilityConfig {
    GENERATED_BODY()

    // Maximum durability. Each landed hit costs 1; the item breaks at 0. <= 0 means "no durability".
    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame)
    int32 MaxDurability = 100;

    // Fraction of MaxDurability at or below which a one-shot "durability low" callout floats over the owning player
    // (re-armed once repaired back above it). A designer-tunable FEEDBACK threshold — it changes nothing mechanical,
    // only when the player is warned. <= 0 disables the warning. Sourced from the item definition (config, copied from
    // the fragment template on load), never mutated at runtime, so it is intentionally NOT SaveGame.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LowDurabilityWarnFraction = 0.25f;
};

/** Mutable per-instance state - replicated to clients (for UI) and persisted via SaveGame. */
USTRUCT(BlueprintType)
struct FDurabilityRuntimeReplicatedData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, SaveGame)
    int32 Current = 0;

    UPROPERTY(BlueprintReadOnly, SaveGame)
    bool bBroken = false;
};

/**
 * DurabilityFragment - a passive fragment giving an item finite durability that wears with use and breaks
 * at zero. Wear is applied server-side at the damage chokepoint (UMythicGameplayAbility::ApplyDamageContainer),
 * which also makes a broken weapon deal no damage until repaired. Repair is the server-authoritative
 * ServerRepair API (callable from a repair interaction/station).
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UDurabilityFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Durability)

    /** Designer config (max durability). REPLICATED + SaveGame. */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties), SaveGame)
    FDurabilityConfig DurabilityConfig = FDurabilityConfig();

    /** Runtime state (current durability + broken latch). REPLICATED + SaveGame. */
    UPROPERTY(Replicated, BlueprintReadOnly, SaveGame)
    FDurabilityRuntimeReplicatedData DurabilityRuntimeReplicatedData = FDurabilityRuntimeReplicatedData();

    UFUNCTION(BlueprintPure, Category = "Durability")
    int32 GetCurrentDurability() const { return DurabilityRuntimeReplicatedData.Current; }

    UFUNCTION(BlueprintPure, Category = "Durability")
    int32 GetMaxDurability() const { return DurabilityConfig.MaxDurability; }

    UFUNCTION(BlueprintPure, Category = "Durability")
    bool IsBroken() const { return DurabilityRuntimeReplicatedData.bBroken; }

    // SERVER: reduce durability by Amount (>= 0); latches broken when it reaches 0.
    UFUNCTION(BlueprintCallable, Category = "Durability")
    void ServerApplyWear(int32 Amount);

    // SERVER: restore durability by Amount (>= 0), clamped to max; clears broken once it rises above 0.
    UFUNCTION(BlueprintCallable, Category = "Durability")
    void ServerRepair(int32 Amount);

    // SERVER: fully repair to max durability.
    UFUNCTION(BlueprintCallable, Category = "Durability")
    void ServerRepairFull() { ServerRepair(GetMaxDurability()); }

    //~ Overrides
    virtual void OnInstanced(UMythicItemInstance *Instance) override;

    // Two durability items only stack when their wear state matches — otherwise a merge (AddToAnySlot keeps only the
    // survivor's fragment and Destroy()s the incoming instance) would silently discard the incoming durability. Every
    // other runtime-state fragment (Affixes, Attack, Consumable, Talent) guards CanBeStackedWith; Durability was the
    // lone exception, so worn/pristine items of the same def (StackSizeMax>=2) merged and free-repaired or lost wear.
    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;
    //~

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        REP_FRAGMENT_DATA(Durability)
    }

private:
    // Server-only debounce so the low-durability warning floats ONCE per wear-down (not on every landed hit while
    // below the threshold). Re-armed by ServerRepair when durability rises back above the threshold. Not replicated /
    // not SaveGame — purely a server-side notification latch (the authoritative wear + repair both run server-side).
    bool bLowWarningFired = false;

    // Resolve the owning player and float the given durability beat over them via the standard player-controller
    // feedback RPC. Items NOT in a player inventory (container / merchant / NPC / world-drop) resolve to a null
    // controller and this cleanly no-ops. Server-side (called from the authoritative wear/repair edges).
    void NotifyDurabilityBeat(EMythicItemDurabilityBeat Beat) const;
};
