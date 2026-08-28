#pragma once

#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Net/UnrealNetwork.h"
#include "HarvestToolFragment.generated.h"

class UItemDefinition;
class UMythicHarvestToolTypeDefinition;

/** Immutable definition-authored harvesting capability copied onto one exact live item instance. */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UHarvestToolFragment : public UItemFragment {
    GENERATED_BODY()

public:
    /**
     * Fragment-owned direct family asset compared by exact identity on authority; clients may read it for prompts,
     * null fails validation, and tags/names/strings never substitute for this reference.
     */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Harvest Tool")
    TObjectPtr<UMythicHarvestToolTypeDefinition> ToolType = nullptr;

    /**
     * Fragment-owned tier read from the exact active server item; clients may display it, negative values fail
     * validation, and units are integer tool tiers independent of item rarity.
     */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Harvest Tool", meta = (ClampMin = "0"))
    int32 ToolTier = 0;

    /**
     * Fragment-owned base work read from the exact active server item; clients may project it, nonfinite/nonpositive
     * or unquantizable values fail validation, and units are continuous harvest-work units per accepted cycle.
     */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Harvest Tool", meta = (ClampMin = "0.0001"))
    float BaseWork = 1.0f;

    /**
     * Fragment-owned wear applied by authority exactly once after accepted work is known; clients may display it,
     * negative values fail validation, rejection/miss applies none, and units are whole durability points per hit.
     */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Harvest Tool", meta = (ClampMin = "0"))
    int32 DurabilityWearPerAcceptedHit = 1;

    /**
     * Fragment-owned authoritative node budget for one server-issued attack cycle; clients may display it, values
     * below one fail validation, and units are distinct stable harvest nodes per cycle.
     */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Harvest Tool", meta = (ClampMin = "1"))
    int32 MaxNodesPerCycle = 1;

    /** Returns the harvest fragment on this exact live item, or null; native-only and side-effect free. */
    static const UHarvestToolFragment *FindOnItem(UMythicItemInstance *ItemInstance);

    /** Returns the harvest fragment on this immutable definition, or null; native-only and side-effect free. */
    static const UHarvestToolFragment *FindOnDefinition(const UItemDefinition *ItemDefinition);

#if WITH_EDITOR
    virtual bool IsValidFragment(FText &OutErrorMessage) const override;
#endif

    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME_CONDITION(ThisClass, ToolType, COND_InitialOrOwner);
        DOREPLIFETIME_CONDITION(ThisClass, ToolTier, COND_InitialOrOwner);
        DOREPLIFETIME_CONDITION(ThisClass, BaseWork, COND_InitialOrOwner);
        DOREPLIFETIME_CONDITION(ThisClass, DurabilityWearPerAcceptedHit, COND_InitialOrOwner);
        DOREPLIFETIME_CONDITION(ThisClass, MaxNodesPerCycle, COND_InitialOrOwner);
    }
};
