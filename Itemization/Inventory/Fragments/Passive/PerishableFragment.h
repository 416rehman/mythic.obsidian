
#pragma once

#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Net/UnrealNetwork.h"
#include "PerishableFragment.generated.h"

USTRUCT(BlueprintType)
struct FPerishableConfig {
    GENERATED_BODY()

    // Effective aged seconds after which the item is fully spoiled. <= 0 disables spoilage entirely (default).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, meta = (ClampMin = "0.0"))
    double ShelfLifeSeconds = 0.0;

    // Quantized-bucket width for the stacking/merge compare (P8: same-window items stack; cross-window never merge).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, meta = (ClampMin = "1.0"))
    double BucketSeconds = 300.0;

    // Freshness fraction below which the ingredient reads as Stale (cooking potency window; UI hint).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StaleBelowFraction = 0.5f;
};

USTRUCT(BlueprintType)
struct FPerishableRuntimeReplicatedData {
    GENERATED_BODY()

    // Aged seconds already banked at the last fold (container transition).
    UPROPERTY(BlueprintReadOnly, SaveGame)
    double AgedBankedSeconds = 0.0;

    // RAW UTC seconds of the last fold (or the harvest stamp). 0 = not yet stamped (fresh instancing stamps it).
    UPROPERTY(BlueprintReadOnly, SaveGame)
    double AnchorUtcSeconds = 0.0;

    // Aging-rate multiplier of the inventory the item currently sits in (1 = normal, 0.25 = cold storage, 0 = frozen).
    UPROPERTY(BlueprintReadOnly, SaveGame)
    float CurrentPreservationMult = 1.0f;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UPerishableFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Perishable)

    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, meta = (ShowOnlyInnerProperties), SaveGame)
    FPerishableConfig PerishableConfig;

    UPROPERTY(Replicated, BlueprintReadOnly, SaveGame)
    FPerishableRuntimeReplicatedData PerishableRuntimeReplicatedData;

    static double UtcNowSeconds();

    double GetEffectiveAgedSeconds(double NowUtcSeconds) const;

    /** Remaining freshness fraction [0,1] at an explicit 'now'. 1 = fresh; shelf-life <= 0 pins at 1 (never spoils). */
    UFUNCTION(BlueprintPure, Category = "Perishable")
    float GetFreshnessFraction(double NowUtcSeconds) const;

    /** Remaining freshness fraction at the live wall clock. */
    UFUNCTION(BlueprintPure, Category = "Perishable")
    float GetFreshnessFractionNow() const { return GetFreshnessFraction(UtcNowSeconds()); }

    UFUNCTION(BlueprintPure, Category = "Perishable")
    bool IsSpoiled(double NowUtcSeconds) const { return GetFreshnessFraction(NowUtcSeconds) <= 0.0f; }

    virtual void OnInstanced(UMythicItemInstance *Instance) override;
    virtual void OnInventorySlotChanged(UMythicInventoryComponent *NewInventory, int32 NewSlot) override;
    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        REP_FRAGMENT_DATA(Perishable)
    }

private:
    static float ResolvePreservationMultiplier(const UMythicInventoryComponent *Inventory);

    bool HasMutationAuthority() const;
};
