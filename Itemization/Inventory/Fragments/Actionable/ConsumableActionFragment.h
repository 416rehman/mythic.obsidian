// 

#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/Fragments/ActionableItemFragment.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "UObject/Object.h"
#include "ConsumableActionFragment.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct FConsumableActionRuntimeServerOnlyData {
    GENERATED_BODY()
};

USTRUCT(Blueprintable, BlueprintType)
struct FConsumableActionRuntimeClientOnlyData {
    GENERATED_BODY()
};

USTRUCT(Blueprintable, BlueprintType)
struct FConsumableActionRuntimeReplicatedData {
    GENERATED_BODY()

    // Cache the ASC for the player
    UPROPERTY(BlueprintReadOnly)
    UAbilitySystemComponent *ASC = nullptr;
};

USTRUCT(Blueprintable, BlueprintType)
struct FConsumableActionConfig {
    GENERATED_BODY()

    // The tag to add to the player ASC when the item is consumed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
    FGameplayTagContainer TagsToAdd = FGameplayTagContainer();

    // The tag to remove from the player ASC when the item is consumed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags")
    FGameplayTagContainer TagsToRemove = FGameplayTagContainer();

    // The Gameplay ability to grant to the player ASC when the item is consumed
    // To activate the ability on grant, consider using a Passive ability
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities")
    TSoftClassPtr<UGameplayAbility> GameplayAbility = nullptr;

    // Should we remove the ability instead of granting it
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Abilities")
    bool RemoveAbility = false;

    // The gameplay event to trigger when the item is consumed
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay Events")
    FGameplayTag EventTag = FGameplayTag();
};

USTRUCT(Blueprintable, BlueprintType)
struct FConsumableActionBuildData {
    GENERATED_BODY()
};

/** OnAction does one or more of the following:
 * Grant Ability (Optionally activate it)
 * Remove Ability
 * Trigger Gameplay Event
 * Assign Tags
 * Remove Tags
* */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UConsumableActionFragment : public UActionableItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(ConsumableAction)

    /** Designer friendly configuration data that defines this fragment. */
    /** REPLICATED and fields should be BlueprintReadOnly */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
    FConsumableActionConfig ConsumableActionConfig = FConsumableActionConfig();

    /** This is used in the OnInstanced method to calculate/fill the rest of the data. */
    /** This should not be replicated or blueprint accessible and safely discarded after being used in the OnInstanced method. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
    FConsumableActionBuildData ConsumableActionBuildData = FConsumableActionBuildData();

    /** Contains the runtime state of the fragment (replicated to client) */
    /** REPLICATED */
    UPROPERTY(Replicated, BlueprintReadOnly)
    FConsumableActionRuntimeReplicatedData ConsumableActionRuntimeReplicatedData = FConsumableActionRuntimeReplicatedData();

    /** Contains the runtime client side state of the fragment for use in methods like OnActiveItemClient */
    /** Shouldn't be accessed on server side methods like OnActiveItem. */
    UPROPERTY(BlueprintReadOnly)
    FConsumableActionRuntimeClientOnlyData ConsumableActionRuntimeClientOnlyData = FConsumableActionRuntimeClientOnlyData();

    /** Contains the runtime server-only state of the fragment for use in methods like OnActiveItem */
    /** Shouldn't be accessed on client side methods like OnActiveItemClient. */
    UPROPERTY(BlueprintReadOnly)
    FConsumableActionRuntimeServerOnlyData ConsumableActionRuntimeServerOnlyData = FConsumableActionRuntimeServerOnlyData();

    virtual void OnClientActionEnd(UMythicItemInstance *ItemInstance) override;

    virtual void OnInstanced(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnInventorySlotChanged(UMythicInventoryComponent* Inventory, int32 SlotIndex) override;

    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;
    //~

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        REP_FRAGMENT_DATA(ConsumableAction)
    }

    //~ GiveAbility only works on the server
    UFUNCTION(Server, Reliable)
    void ServerHandleGrantAbility(UMythicItemInstance *ItemInstance);
    //~

    //~ RemoveAbility only works on the server
    UFUNCTION(Server, Reliable)
    void ServerHandleInHandRemoveAbility(UMythicItemInstance *ItemInstance);
    //~

    //~ ServerHandleTags only works on the server
    UFUNCTION(Server, Reliable)
    void ServerHandleTags(UMythicItemInstance *ItemInstance);
    //~
};
