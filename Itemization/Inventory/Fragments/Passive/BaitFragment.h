
#pragma once

#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "GameplayTagContainer.h"
#include "Net/UnrealNetwork.h"
#include "BaitFragment.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UBaitFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Bait)

    /** This definition's Bait.* identity tags (e.g. Bait.Worm). Matched (HasAll) against a catch entry's RequiredBait. */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Bait", meta = (Categories = "Bait"))
    FGameplayTagContainer BaitTags;

    virtual void OnInstanced(UMythicItemInstance *Instance) override;
    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME_CONDITION(ThisClass, BaitTags, COND_InitialOrOwner);
    }
};
