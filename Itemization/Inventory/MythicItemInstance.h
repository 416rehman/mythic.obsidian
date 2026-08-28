#pragma once

#include "CoreMinimal.h"
#include "ItemDefinition.h"
#include "MythicItemFactoryTypes.h"
#include "Mythic/Utility/MythicReplicatedObject.h"
#include "Net/UnrealNetwork.h"
#include "MythicItemInstance.generated.h"

class UItemFragment;
class UMythicInventorySlot;
class UMythicInventoryComponent;
struct FCompiledAffixProfile;

UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API UMythicItemInstance : public UMythicReplicatedObject {
    GENERATED_BODY()

protected:
    /** Item definition that supplied this instance's immutable authored data. */
    UPROPERTY(ReplicatedUsing=OnRep_ItemDefinition, BlueprintReadOnly, Category = "Item", SaveGame)
    UItemDefinition *ItemDefinition;

    /** Runtime fragment copies materialized from the item definition for this instance. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
    TArray<TObjectPtr<UItemFragment>> ItemFragments;

    /** Current stack quantity, clamped by the definition's maximum stack size. */
    UPROPERTY(ReplicatedUsing=OnRep_Quantity, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"), SaveGame)
    int32 Quantity = 1;

    /** Inventory component that currently owns this item instance; null while the item is in transit or world-owned. */
    UPROPERTY(ReplicatedUsing=OnRep_OwningInventory, BlueprintReadOnly, Category = "Item")
    TObjectPtr<UMythicInventoryComponent> OwningInventory;

    /** Replicated index of this item inside its owning inventory, or -1 when it is not assigned to a slot. */
    UPROPERTY(ReplicatedUsing=OnRep_SlotIndex, BlueprintReadOnly, Category = "Item")
    int32 SlotIndex = -1;

    /** Item level used by affix, reward, and scaling calculations. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item", SaveGame)
    int32 ItemLevel = 1;

    /**
     * Stable identity for this physical item. Affix rolls and socket provenance derive from it, so it persists across
     * save, replication, transfer, and reconnect.
     */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item|Identity", SaveGame)
    FGuid ItemInstanceGuid;

    /** Dynamic gameplay tags assigned to this item instance at runtime. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item", SaveGame)
    FGameplayTagContainer ItemTags;

    // Loot-filter "junk" mark (P5): the PERSISTED, server-authoritative source of truth for whether the player has
    // manually flagged this item as junk. Travels with the instance (SaveGame persists it; replication lets the owning
    // client's slot VM read it), so the flag survives moving the item between bags/containers. A dedicated bool (not a
    // gameplay tag) keeps it OUT of the item's type probe (GetTypeProbe) so it can never affect slot whitelisting or
    // conversion-ingredient matching. Set only via ServerSetMarkedJunk (authority). Auto-junk (low rarity) is derived,
    // not stored — see MythicLootFilter::IsJunk.
    /** Persisted authority-owned manual junk mark; intentionally excluded from the item's type-tag probe. */
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
        DOREPLIFETIME_CONDITION(UMythicItemInstance, ItemInstanceGuid, COND_OwnerOnly);
        DOREPLIFETIME(UMythicItemInstance, ItemTags);
        DOREPLIFETIME(UMythicItemInstance, bMarkedJunk);
    }

    void SetStackSize(const int32 newQuantity);

    /** Returns the current stack quantity. */
    UFUNCTION(BlueprintCallable, Category = "Item")
    int32 GetStacks() const { return Quantity; }

    /** Returns the item level used by itemization scaling. */
    UFUNCTION(BlueprintCallable, Category = "Item")
    int32 GetItemLevel() const { return ItemLevel; }

    /** Returns the persistent identity of this physical item used by affix, socket, save, and replication provenance. */
    UFUNCTION(BlueprintPure, Category = "Item|Identity")
    FGuid GetItemInstanceGuid() const { return ItemInstanceGuid; }

    // Assigns a random identity to a newly-created or intentionally copied physical item. Authority only.
    bool AssignNewItemInstanceGuid();

    // Idempotent fresh-item helper used before fragments materialize affix RollGuids.
    bool EnsureNewItemInstanceGuid();

    /**
     * Builds all new-item state off the replication/inventory graph and commits only after affix generation succeeds.
     * OptionalCompiledProfile must be the exact immutable, prewarmed closure selected by the item factory.
     */
    FMythicItemInitializeResult InitializeTransactional(
        const FMythicCreateItemRequest &Request,
        const FCompiledAffixProfile *OptionalCompiledProfile);

    static int32 ClampInitialStackQuantity(int32 Requested, int32 StackSizeMax);

    /**
     * Creates an unowned current-format stack split that preserves immutable gameplay state while assigning a new
     * physical item identity and rekeying every item-owned affix/socket identity. Returns null without mutating this
     * item when the complete clone cannot be validated.
     */
    UMythicItemInstance *CloneForStackSplit(UObject *NewOuter, int32 NewQuantity) const;

    /** Returns the authored definition from which this item instance was created. */
    UFUNCTION(BlueprintCallable, Category = "Item")
    UItemDefinition *GetItemDefinition() const {
        return ItemDefinition;
    }

#if WITH_DEV_AUTOMATION_TESTS
    /** Test-only fixture seam; production item creation must use the transactional item factory. */
    void InitializeFixtureForTests(UItemDefinition *ItemDef, int32 QuantityIfStackable, int32 Level);

    /** Test-only fixture seam that clones one already-materialized fragment without invoking production generation. */
    void AddFragmentFixtureForTests(TObjectPtr<UItemFragment> Fragment);
#endif

    void OnActiveItem();

    void OnInactiveItem();

    void OnClientActiveItem();
    void OnClientInactiveItem();
    void SetInventory(UMythicInventoryComponent *NewInventory, int32 NewSlotIndex);

    int32 GetSlot() const;

    /** Returns the inventory currently containing this item, or null while it is unowned or in transit. */
    UFUNCTION(BlueprintCallable, Category = "Item")
    UMythicInventoryComponent *GetInventoryComponent() const;

    /** Returns the actor that owns the containing inventory, or null when this item is not in an inventory. */
    UFUNCTION(BlueprintCallable, Category = "Item")
    AActor *GetInventoryOwner() const;

    void AddTag(const FGameplayTag &Tag);

    void RemoveTag(const FGameplayTag &Tag);

    bool HasTag(const FGameplayTag &Tag) const;

    /** Returns the replicated manual junk mark; automatic loot-filter classification is evaluated separately. */
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

    /** Counts live runtime fragments whose class is Child Of Fragment Class; used to fail closed on authority duplicates. */
    int32 CountFragmentsOfClass(TSubclassOf<UItemFragment> FragmentClass) const;

    void ConsumeItem(int32 StackQty = 1);

    virtual void OnDestroyed() override;
};
