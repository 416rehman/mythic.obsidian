#pragma once

#include "CoreMinimal.h"
#include "ItemDefinition.h"
#include "Mythic/Utility/MythicReplicatedObject.h"
#include "Net/UnrealNetwork.h"
#include "MythicItemInstance.generated.h"

class UItemFragment;
class UMythicInventorySlot;
class UMythicInventoryComponent;

UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API UMythicItemInstance : public UMythicReplicatedObject {
    GENERATED_BODY()

protected:
    // object pointer to the item definition data asset
    UPROPERTY(ReplicatedUsing=OnRep_ItemDefinition, BlueprintReadOnly, Category = "Item", SaveGame)
    UItemDefinition *ItemDefinition;

    // Item Fragments copied from the item definition
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
    TArray<TObjectPtr<UItemFragment>> ItemFragments;

    // Quantity of item (Current size of stack), with a setter to make sure its never over the max stack size
    UPROPERTY(ReplicatedUsing=OnRep_Quantity, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"), SaveGame)
    int32 Quantity = 1;

    UPROPERTY(ReplicatedUsing=OnRep_OwningInventory, BlueprintReadOnly, Category = "Item")
    TObjectPtr<UMythicInventoryComponent> OwningInventory;

    UPROPERTY(ReplicatedUsing=OnRep_SlotIndex, BlueprintReadOnly, Category = "Item")
    int32 SlotIndex = -1;

    // The level of the item
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item", SaveGame)
    int32 ItemLevel = 1;

    // Tags assigned to the item - these are dynamic and can be changed at runtime
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item", SaveGame)
    FGameplayTagContainer ItemTags;

    // Loot-filter "junk" mark (P5): the PERSISTED, server-authoritative source of truth for whether the player has
    // manually flagged this item as junk. Travels with the instance (SaveGame persists it; replication lets the owning
    // client's slot VM read it), so the flag survives moving the item between bags/containers. A dedicated bool (not a
    // gameplay tag) keeps it OUT of the item's type probe (GetTypeProbe) so it can never affect slot whitelisting or
    // conversion-ingredient matching. Set only via ServerSetMarkedJunk (authority). Auto-junk (low rarity) is derived,
    // not stored — see MythicLootFilter::IsJunk.
    UPROPERTY(ReplicatedUsing=OnRep_MarkedJunk, BlueprintReadOnly, Category = "Item", SaveGame)
    bool bMarkedJunk = false;

    UFUNCTION()
    void OnRep_Quantity();

    UFUNCTION()
    void OnRep_MarkedJunk();

    UFUNCTION()
    void OnRep_ItemDefinition();

    UFUNCTION()
    void OnRep_OwningInventory();

    UFUNCTION()
    void OnRep_SlotIndex();

public:
    virtual void Serialize(FArchive &Ar) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(UMythicItemInstance, ItemDefinition);
        DOREPLIFETIME(UMythicItemInstance, Quantity);
        DOREPLIFETIME(UMythicItemInstance, OwningInventory);
        DOREPLIFETIME(UMythicItemInstance, SlotIndex);
        DOREPLIFETIME(UMythicItemInstance, ItemFragments);
        DOREPLIFETIME(UMythicItemInstance, ItemLevel);
        DOREPLIFETIME(UMythicItemInstance, ItemTags);
        DOREPLIFETIME(UMythicItemInstance, bMarkedJunk);
    }

    void SetStackSize(const int32 newQuantity);

    // Get the quantity of the item
    UFUNCTION(BlueprintCallable, Category = "Item")
    int32 GetStacks() const { return Quantity; }

    // Get the level of the item
    UFUNCTION(BlueprintCallable, Category = "Item")
    int32 GetItemLevel() const { return ItemLevel; }

    void Initialize(UItemDefinition *ItemDef, const int32 quantityIfStackable, const int32 level);

    static int32 ClampInitialStackQuantity(int32 Requested, int32 StackSizeMax);

    // Get the item definition
    UFUNCTION(BlueprintCallable, Category = "Item")
    UItemDefinition *GetItemDefinition() const {
        return ItemDefinition;
    }

    void AddFragment(TObjectPtr<UItemFragment> Fragment);

    void OnActiveItem();

    void OnInactiveItem();

    void OnClientActiveItem();
    void OnClientInactiveItem();
    void SetInventory(UMythicInventoryComponent *NewInventory, int32 NewSlotIndex);

    int32 GetSlot() const;

    // Get Inventory Component, can be null if the item is not in an inventory
    UFUNCTION(BlueprintCallable, Category = "Item")
    UMythicInventoryComponent *GetInventoryComponent() const;

    // Get Inventory Owner, can be null if the item is not in an inventory
    UFUNCTION(BlueprintCallable, Category = "Item")
    AActor *GetInventoryOwner() const;

    void AddTag(const FGameplayTag &Tag);

    void RemoveTag(const FGameplayTag &Tag);

    bool HasTag(const FGameplayTag &Tag) const;

    // Loot-filter (P5): read the persisted manual "junk" mark. Valid on server + owning client (replicated).
    UFUNCTION(BlueprintCallable, Category = "Item")
    bool IsMarkedJunk() const { return bMarkedJunk; }

    void ServerSetMarkedJunk(bool bJunk);

    const FGameplayTagContainer &GetItemTags() const { return ItemTags; }

    void GetTypeProbe(FGameplayTagContainer &Out) const;

    void ServerApplyTransform(const FGameplayTag &NewItemType,
                              const FGameplayTagContainer &TagsToAdd,
                              const FGameplayTagContainer &TagsToRemove,
                              UItemDefinition *OptionalNewDef);

    bool isStackableWith(const UMythicItemInstance *Other) const;

    template <typename T>
    const T *GetFragment() {
        for (const UItemFragment *frag : this->ItemFragments) {
            if (auto casted = Cast<T>(frag)) {
                return casted;
            }
        }

        return nullptr;
    }

    void ConsumeItem(int32 StackQty = 1);

    virtual void OnDestroyed() override;
};
