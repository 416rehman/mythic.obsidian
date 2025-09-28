#pragma once
#include "AbilitySystemGlobals.h"
#include "Engine/DataTable.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Utility/MythicReplicatedObject.h"
#if WITH_EDITOR
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "UObject/ObjectSaveContext.h"
#endif
#include "ItemFragment.generated.h"

class UMythicInventorySlot;

// Use this, then add DECLARE_FRAGMENT_DATA, then add REP_FRAGMENT_DATA to GetLifetimeReplicatedProps
// NOTE: Declare in class' "public" section and class must be named U<Name>Fragment
// Macro that declares two getter functions for accessing fragments:
// 1. Get{Name}FragmentFromInstance - Gets fragment from an item instance
// 2. Get{Name}FragmentFromDefinition - Gets fragment from an item definition statically
#define DECLARE_FRAGMENT(Name) \
UFUNCTION(BlueprintCallable, BlueprintPure) \
const U##Name##Fragment* Get##Name##FragmentFromInstance(UMythicItemInstance* ItemInstance) { \
    return ItemInstance->GetFragment<U##Name##Fragment>(); \
} \
UFUNCTION(BlueprintCallable, BlueprintPure) \
static const U##Name##Fragment* Get##Name##FragmentFromDefinition(UItemDefinition* ItemDefinition) { \
    return UItemDefinition::GetFragment<U##Name##Fragment>(ItemDefinition); \
}

// Use this macro inside the "GetLifetimeReplicatedProps" if DECLARE_FRAGMENT_DATA is used.
#define REP_FRAGMENT_DATA(Name) \
DOREPLIFETIME_CONDITION(ThisClass, Name##RuntimeReplicatedData, COND_InitialOrOwner); \
DOREPLIFETIME_CONDITION(ThisClass, Name##Config, COND_InitialOrOwner);

/// Fragment Boilerplate
/// /**
//  * Fragment: This is an item fragment
//  */
// UCLASS(Blueprintable, BlueprintType)
// class MYTHIC_API U<Name>Fragment : public UItemFragment {
//     GENERATED_BODY()
//
// public:
//     DECLARE_FRAGMENT(<Name>)
//
//     /** Designer friendly configuration data that defines this fragment. */
//     /** REPLICATED and fields should be BlueprintReadOnly */
//     UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
//     F<Name>Config TalentConfig;
//
//     /** Contains the runtime state of the fragment (replicated to client) */
//     /** REPLICATED */
//     UPROPERTY(Replicated, BlueprintReadOnly)
//     F<Name>RuntimeReplicatedData TalentRuntimeReplicatedData;
//
//     //~ Overrides
//     virtual bool IsValidFragment(FText &OutErrorMessage) const override;
//     virtual void OnInstanced(UMythicItemInstance *ItemInstance) override;
//     virtual void OnItemActivated(UMythicItemInstance *ItemInstance) override;
//     virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) override;
//
//     virtual bool CanBeStackedWith(const UItemFragment *Other) const override;
//     //~
//
//     virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
//         Super::GetLifetimeReplicatedProps(OutLifetimeProps);
//         REP_FRAGMENT_DATA(<Name>)
//     }
// };


/// ItemFragments are ReplicatedUObjects, unique to each item instance, and are used to define the item's behavior.
/// These receive events such as OnInstanced, OnPutInSlot, OnRemovedFromSlot, OnActiveItem, OnInactiveItem, etc.
/// Activation pipeline (SERVER: slot -> item -> fragment -> activate)
/// Deactivation pipeline (SERVER: slot -> item -> fragment -> deactivate)
///
/// By default, properties of the ItemFragment are on the SERVER only. However, if the client is interested, REPLICATED properties can be accessed.
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, Abstract)
class MYTHIC_API UItemFragment : public UMythicReplicatedObject {
    GENERATED_BODY()

protected:
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item Fragment")
    UMythicItemInstance *ParentItemInstance;

public:
    // This is called after the item fragment has been instanced and added to an item instance.
    // Use case: Initial setup of the fragment
    virtual void OnInstanced(UMythicItemInstance *Instance) {
        ParentItemInstance = Instance;
    }

    /*
    * Fragment Activation Flow - Server Side
    * ------------------------------
    * Called when the item becomes active in a slot (server-side)
    * Examples:
    * - Grant gameplay abilities
    * - Apply attribute modifiers
    * - Start gameplay effects
    * - Modify character stats
    * - Handle inventory logic
    */
    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) {}

    // This is called when the slot this item is in is no longer the active slot in the inventory
    // Use case: Removing abilities that were granted when the item was in hand
    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) {}

    /*
    * Fragment Activation Flow - Client Side
    * ------------------------------
    * Called when the item becomes active in a slot (client-side)
    * Examples:
    * - Play animations
    * - Spawn particle effects
    * - Update UI elements
    * - Play sounds
    * - Handle visual feedback
    */
    virtual void OnClientItemActivated(UMythicItemInstance *ItemInstance) {}

    /*
    * Fragment Deactivation Flow - Client Side
    * ------------------------------
    * Called when the item becomes inactive in a slot (client-side)
    * Examples:
    * - Stop animations
    * - Remove particle effects
    * - Reset UI elements
    * - Stop sounds
    * - Clean up visual feedback
    */
    virtual void OnClientItemDeactivated(UMythicItemInstance *ItemInstance) {}

    // This is called when the inventory slot is changed. If null, the item has been dropped
    // Use case: Optionally caching the owning inventory component or other "player" related data
    virtual void OnInventorySlotChanged(UMythicInventoryComponent *NewInventory, int32 NewSlot) {}

    // replicate
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME_CONDITION(UItemFragment, ParentItemInstance, COND_InitialOrOwner);
    }

    // Get the owning item instance
    UFUNCTION(BlueprintPure, Category = "Item Fragment")
    UMythicItemInstance *GetOwningItemInstance() const { return ParentItemInstance; }

    // Get the owning inventory component's owners ability system component
    UFUNCTION(BlueprintPure, Category = "Item Fragment")
    UMythicInventoryComponent *GetOwningInventoryComponent() const;

    // Get the owning inventory component's owners ability system component
    UFUNCTION(BlueprintPure, Category = "Item Fragment")
    UAbilitySystemComponent *GetOwningAbilitySystemComponent() const;

    // Compare the runtime properties of this fragment with another fragment to determine if they can be stacked
    // i.e. a fragment that always grants +1 strength can be stacked with another fragment that always grants +1 strength
    // but a fragment that will grant a strength in range [1, 3] cannot be stacked with a fragment that will grant a strength in range [2, 4]
    virtual bool CanBeStackedWith(const UItemFragment *Other) const { return true; }

#if WITH_EDITOR
    // Validates the fragment. When an item is instanced from an ItemDefinition, the fragments from the ItemDef are validated using this.
    // If returned true, this fragment will be added to the item instance.
    virtual bool IsValidFragment(FText &OutErrorMessage) const { return true; }

    void ValidateDefinition() const {
        FText ErrorMessage;
        if (!IsValidFragment(ErrorMessage)) {
            FMessageLog("AssetCheck").Error()
                                     ->AddToken(FTextToken::Create(ErrorMessage));

            // You could also show a notification in the editor
            FNotificationInfo Info(FText::FromString("Fragment validation failed"));
            Info.ExpireDuration = 5.0f;
            Info.bUseLargeFont = false;
            Info.bFireAndForget = true;
            FSlateNotificationManager::Get().AddNotification(Info);
        }
    }

    virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override {
        Super::PostEditChangeProperty(PropertyChangedEvent);

        ValidateDefinition();
    }

    virtual void PreSave(FObjectPreSaveContext SaveContext) override {
        Super::PreSave(SaveContext);

        ValidateDefinition();
    }
#endif
};

inline UMythicInventoryComponent *UItemFragment::GetOwningInventoryComponent() const {
    auto owningItemInstance = GetOwningItemInstance();
    if (!owningItemInstance) {
        UE_LOG(Myth, Error, TEXT("UItemFragment::GetOwningInventoryComponent: Fragment has no owning item instance."));
        return nullptr;
    }

    return owningItemInstance->GetInventoryComponent();
}

inline UAbilitySystemComponent *UItemFragment::GetOwningAbilitySystemComponent() const {
    auto owningInventoryComponent = GetOwningInventoryComponent();
    if (!owningInventoryComponent) {
        UE_LOG(Myth, Error, TEXT("UItemFragment::GetOwningAbilitySystemComponent: Fragment has no owning inventory component."));
        return nullptr;
    }

    return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(owningInventoryComponent->GetOwner());
}
