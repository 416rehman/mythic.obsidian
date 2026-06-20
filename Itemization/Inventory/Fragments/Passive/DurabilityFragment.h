//

#pragma once

#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Net/UnrealNetwork.h"
#include "DurabilityFragment.generated.h"

/** Designer-authored config: the item's maximum durability. MaxDurability <= 0 disables durability. */
USTRUCT(BlueprintType)
struct FDurabilityConfig {
    GENERATED_BODY()

    // Maximum durability. Each landed hit costs 1; the item breaks at 0. <= 0 means "no durability".
    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame)
    int32 MaxDurability = 100;
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
    //~

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        REP_FRAGMENT_DATA(Durability)
    }
};
