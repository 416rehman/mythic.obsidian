#pragma once
#include "AbilitySystemGlobals.h"
#include "Engine/DataTable.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Logging/MessageLog.h"
#include "Mythic/Utility/MythicReplicatedObject.h"
#include "Mythic/GAS/MythicAbilitySystemComponent.h"
#if WITH_EDITOR
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "UObject/ObjectSaveContext.h"
#endif
#include "ItemFragment.generated.h"

class UMythicInventorySlot;

#define DECLARE_FRAGMENT(Name) \
/** Returns the typed fragment from a live item instance, or null when the item does not own one. */ \
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Fragment", \
          meta = (ToolTip = "Returns this typed fragment from a live item instance, or null when the item does not own one.")) \
const U##Name##Fragment* Get##Name##FragmentFromInstance(UMythicItemInstance* ItemInstance) { \
    return ItemInstance->GetFragment<U##Name##Fragment>(); \
} \
/** Returns the typed fragment from an immutable item definition, or null when the definition does not own one. */ \
UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Item Fragment", \
          meta = (ToolTip = "Returns this typed fragment from an immutable item definition, or null when the definition does not own one.")) \
static const U##Name##Fragment* Get##Name##FragmentFromDefinition(UItemDefinition* ItemDefinition) { \
    return UItemDefinition::GetFragment<U##Name##Fragment>(ItemDefinition); \
}

#define REP_FRAGMENT_DATA(Name) \
DOREPLIFETIME_CONDITION(ThisClass, Name##RuntimeReplicatedData, COND_InitialOrOwner); \
DOREPLIFETIME_CONDITION(ThisClass, Name##Config, COND_InitialOrOwner);


/** Abstract instanced base for immutable item configuration plus replicated per-instance fragment state. */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, Abstract)
class MYTHIC_API UItemFragment : public UMythicReplicatedObject {
    GENERATED_BODY()

protected:
    /** Live owning item instance assigned when the fragment is instanced; null on immutable definition templates. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item Fragment")
    UMythicItemInstance *ParentItemInstance;

public:
    virtual void OnInstanced(UMythicItemInstance *Instance) {
        SetOwnerItemInstance(Instance);
    }

    void SetOwnerItemInstance(UMythicItemInstance *Instance) {
        ParentItemInstance = Instance;
    }


    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) {}

    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) {}

    virtual void OnClientItemActivated(UMythicItemInstance *ItemInstance) {}

    virtual void OnClientItemDeactivated(UMythicItemInstance *ItemInstance) {}

    virtual void OnInventorySlotChanged(UMythicInventoryComponent *NewInventory, int32 NewSlot) {}

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME_CONDITION(UItemFragment, ParentItemInstance, COND_InitialOrOwner);
    }

    /** Returns the live item instance that owns this runtime fragment, or null on a definition template. */
    UFUNCTION(BlueprintPure, Category = "Item Fragment")
    UMythicItemInstance *GetOwningItemInstance() const { return ParentItemInstance; }

    /** Returns the inventory component that owns the live item, or null while the fragment is not in an inventory. */
    UFUNCTION(BlueprintPure, Category = "Item Fragment")
    UMythicInventoryComponent *GetOwningInventoryComponent() const;

    /** Returns the Mythic ASC associated with the fragment's owning actor, or null when no owner has one. */
    UFUNCTION(BlueprintPure, Category = "Item Fragment")
    UMythicAbilitySystemComponent *GetOwningAbilitySystemComponent() const;

    virtual bool CanBeStackedWith(const UItemFragment *Other) const { return true; }

#if WITH_EDITOR
    virtual bool IsValidFragment(FText &OutErrorMessage) const { return true; }

    void ValidateDefinition() const {
        FText ErrorMessage;
        if (!IsValidFragment(ErrorMessage)) {
            FMessageLog("AssetCheck").Error()
                                     ->AddToken(FTextToken::Create(ErrorMessage));

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

inline UMythicAbilitySystemComponent *UItemFragment::GetOwningAbilitySystemComponent() const {
    auto Owner = GetOwningActor();
    if (!Owner) {
        UE_LOG(Myth, Error, TEXT("UItemFragment::GetOwningAbilitySystemComponent: Fragment has no owning inventory component."));
        return nullptr;
    }

    auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
    return Cast<UMythicAbilitySystemComponent>(ASC);
}
