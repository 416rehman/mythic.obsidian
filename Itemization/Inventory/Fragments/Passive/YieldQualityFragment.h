
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

    /** This definition's quality tier (P1 shared enum). The def IS the tier — this never changes at runtime. */
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
