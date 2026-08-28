
#pragma once

#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Net/UnrealNetwork.h"
#include "DurabilityFragment.generated.h"

/** Owner-facing durability presentation beat emitted after one authoritative item durability transition. */
UENUM(BlueprintType)
enum class EMythicItemDurabilityBeat : uint8 {
    LowWarning,
    Broken,
    Repaired,
};

/** Definition-authored maximum durability and owner-facing low-durability warning threshold. */
USTRUCT(BlueprintType)
struct FDurabilityConfig {
    GENERATED_BODY()

    /** Definition-owned maximum durability; Blueprint may read it, nonpositive means generic items do not wear, and units are durability points. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxDurability = 100;

    // Fraction of MaxDurability at or below which a one-shot "durability low" callout floats over the owning player
    // (re-armed once repaired back above it). A designer-tunable FEEDBACK threshold — it changes nothing mechanical,
    // only when the player is warned. <= 0 disables the warning. Sourced from the item definition (config, copied from
    // the fragment template on load), never mutated at runtime, so it is intentionally NOT SaveGame.
    /** Definition-owned presentation threshold in [0,1]; zero disables warnings and units are a maximum-durability fraction. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LowDurabilityWarnFraction = 0.25f;
};

/** Authority-owned durability state replicated and persisted for one live item instance. */
USTRUCT(BlueprintType)
struct FDurabilityRuntimeReplicatedData {
    GENERATED_BODY()

    /** Authority-owned current durability replicated to the owning client; Blueprint reads only and units are points. */
    UPROPERTY(BlueprintReadOnly, SaveGame)
    int32 Current = 0;

    /** Authority-owned broken latch replicated to the owning client; Blueprint reads only and cannot repair the item. */
    UPROPERTY(BlueprintReadOnly, SaveGame)
    bool bBroken = false;
};

/** Instanced durability fragment that owns authoritative wear, breakage, repair, and replicated presentation state. */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UDurabilityFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Durability)

    /**
     * Immutable definition-authored durability configuration replicated for presentation; Blueprint cannot mutate it,
     * and save/load rehydrates it from the current Item Definition so balance changes cannot leave stale snapshots.
     */
    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, meta=(ShowOnlyInnerProperties))
    FDurabilityConfig DurabilityConfig = FDurabilityConfig();

    /** Authority-owned mutable durability state replicated to the owning client and persisted independently of config. */
    UPROPERTY(Replicated, BlueprintReadOnly, SaveGame)
    FDurabilityRuntimeReplicatedData DurabilityRuntimeReplicatedData = FDurabilityRuntimeReplicatedData();

    /** Returns owner-visible current durability without side effects; units are whole durability points. */
    UFUNCTION(BlueprintPure, Category = "Durability")
    int32 GetCurrentDurability() const { return DurabilityRuntimeReplicatedData.Current; }

    /** Returns current definition-authored maximum durability without side effects; units are whole durability points. */
    UFUNCTION(BlueprintPure, Category = "Durability")
    int32 GetMaxDurability() const { return DurabilityConfig.MaxDurability; }

    /** Returns the authority-replicated broken latch without side effects; true blocks tool and weapon use. */
    UFUNCTION(BlueprintPure, Category = "Durability")
    bool IsBroken() const { return DurabilityRuntimeReplicatedData.bBroken; }

    /** Authority-only wear mutation; positive Amount is clamped at zero and latches broken, while invalid calls do nothing. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Durability")
    void ServerApplyWear(int32 Amount);

    /**
     * Authority-only saturating receipt cost. Returns true once a valid positive semantic wear interval has been
     * consumed, including recovery against an already-broken item; the caller may then commit its exactly-once receipt.
     */
    bool ServerConsumeReceiptWear(int64 Amount);

    /** Authority-only repair mutation; positive Amount is clamped to maximum and clears broken above zero. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Durability")
    void ServerRepair(int32 Amount);

    /** Authority-only full repair mutation; restores the current maximum or does nothing when durability is disabled. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Durability")
    void ServerRepairFull() { ServerRepair(GetMaxDurability()); }

    virtual void OnInstanced(UMythicItemInstance *Instance) override;

    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        REP_FRAGMENT_DATA(Durability)
    }

private:
    bool bLowWarningFired = false;

    void NotifyDurabilityBeat(EMythicItemDurabilityBeat Beat) const;

    void NotifyAffixesOfBrokenState(bool bBroken) const;
};
