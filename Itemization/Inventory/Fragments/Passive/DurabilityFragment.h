
#pragma once

#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Net/UnrealNetwork.h"
#include "DurabilityFragment.generated.h"

UENUM(BlueprintType)
enum class EMythicItemDurabilityBeat : uint8 {
    LowWarning,
    Broken,
    Repaired,
};

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

USTRUCT(BlueprintType)
struct FDurabilityRuntimeReplicatedData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, SaveGame)
    int32 Current = 0;

    UPROPERTY(BlueprintReadOnly, SaveGame)
    bool bBroken = false;
};

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
