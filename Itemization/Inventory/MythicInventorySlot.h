// 
#pragma once

#include "CoreMinimal.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Net/UnrealNetwork.h"
#include "Mythic/Utility/MythicReplicatedObject.h"
#include "MythicInventorySlot.generated.h"

class UMythicItemInstance;
// forward declare the inventory component method GetSlotType
class UMythicInventoryComponent;

UCLASS(Blueprintable)
class MYTHIC_API UMythicInventorySlot : public UMythicReplicatedObject {
    GENERATED_BODY()

protected:
    UPROPERTY(Replicated)
    TObjectPtr<UMythicItemInstance> SlottedItemInstance;

    // Whether the slot is currently active - the item in this slot is currently active and providing its benefits
    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Slot")
    bool bIsActive = false;

    void ActivateSlot();
    void DeactivateSlot();

    // Called when the slot is activated on the client (this happens after the activation flow is completed on the server)
    void ClientActivateSlot();
    // Called when the slot is deactivated on the client (this happens after the deactivation flow is completed on the server)
    void ClientDeactivateSlot();

public:
    // Slot type - Item instances can have different functions depending on the slot they are in. For example, a helmet when put in the head slot will give a bonus to defense, but when put in the chest slot will give a bonus to health.
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Slot", meta=(Categories="Inventory.Slot"))
    FGameplayTag SlotType = INVENTORY_SLOT_DEFAULT;

    // Permit types of items that can be placed in this slot. To permit all items, leave this tag container empty. For example, a weapon slot may only permit items with the "Weapon" tag, while a chest slot may permit items with the "Armor" tag.
    UPROPERTY(Replicated, BlueprintReadWrite, Category = "Slot", meta=(Categories="Itemization.Type"))
    FGameplayTagContainer ItemTypeWhitelist = FGameplayTagContainer();

    // Accessor for the relevant inventory component
    UFUNCTION(BlueprintPure, Category = "Slot")
    UMythicInventoryComponent *GetInventory() const {
        return OwningInventory;
    }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME_CONDITION(UMythicInventorySlot, SlottedItemInstance, COND_OwnerOnly);
        DOREPLIFETIME_CONDITION(UMythicInventorySlot, bIsActive, COND_OwnerOnly);
        
        DOREPLIFETIME_CONDITION(UMythicInventorySlot, SlotType, COND_InitialOrOwner);
        DOREPLIFETIME_CONDITION(UMythicInventorySlot, ItemTypeWhitelist, COND_InitialOrOwner);
        DOREPLIFETIME_CONDITION(UMythicInventorySlot, OwningInventory, COND_InitialOrOwner);
    }

    // Name of the item (every ItemInstance has a ItemDefinition, which has a name)
    UFUNCTION(BlueprintPure, Category = "Slot")
    UMythicItemInstance *GetItemInstance() const;

    // Remove the item instance from this slot
    void Clear();

    // Set the item instance in this slot. Returns true if the item instance was changed, false if another item instance was already in this slot
    // If null is passed, equivalent to calling Clear()
    bool SetItemInstance(UMythicItemInstance *NewItemInstance);

    void Initialize(TObjectPtr<UMythicInventoryComponent> InventoryComponent, FGameplayTag newSlotType, FGameplayTagContainer newItemTypeWhitelist);

    UFUNCTION(BlueprintPure, Category = "Slot")
    bool IsActive() const {
        return bIsActive;
    }

private:
    friend class UMythicInventoryComponent;

protected:
    UPROPERTY(BlueprintReadOnly, Replicated)
    TObjectPtr<UMythicInventoryComponent> OwningInventory;

    // onrep for the type
    UFUNCTION()
    void OnRep_SlotType() const {
        UE_LOG(LogTemp, Warning, TEXT("OnRep_SlotType - SlotType changed to %s"), *SlotType.ToString());
    }
};
