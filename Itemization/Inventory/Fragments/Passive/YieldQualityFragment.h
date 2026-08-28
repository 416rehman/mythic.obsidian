
#pragma once

#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "World/Gathering/MythicYieldQuality.h"
#include "Net/UnrealNetwork.h"
#include "YieldQualityFragment.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UYieldQualityFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(YieldQuality)

    /**
     * Immutable quality snapshot for this physical item. The definition supplies the default; authoritative factory
     * construction may replace it with an already-resolved outcome before publication, after which it never changes.
     */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Yield Quality")
    EMythicYieldQuality QualityTier = EMythicYieldQuality::Common;

    FGameplayTag GetQualityTag() const;

    static EMythicYieldQuality GetTierOfInstance(UMythicItemInstance *Instance);

    virtual void OnInstanced(UMythicItemInstance *Instance) override;
    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME_CONDITION(ThisClass, QualityTier, COND_InitialOrOwner);
    }
};
