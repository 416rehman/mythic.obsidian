

#include "ConsumableActionFragment.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Abilities/MythicAbilityCost_ItemStack.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Mythic.h"
#include "System/MythicAssetManager.h"

void UConsumableActionFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);
}

void UConsumableActionFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);
}

void UConsumableActionFragment::OnInventorySlotChanged(UMythicInventoryComponent *Inventory, int32 SlotIndex) {
    Super::OnInventorySlotChanged(Inventory, SlotIndex);
    auto CachedASC = &this->ConsumableActionRuntimeReplicatedData.ASC;
    if (Inventory) {
        *CachedASC = Cast<UMythicAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Inventory->GetOwner()));
    }
    else {
        *CachedASC = nullptr;
    }
}

void UConsumableActionFragment::OnInstanced(UMythicItemInstance *ItemInstance) {
    Super::OnInstanced(ItemInstance);

    this->ConsumableActionRuntimeReplicatedData.ASC = Cast<UMythicAbilitySystemComponent>(
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ItemInstance->GetInventoryOwner()));

    UMythicAssetManager::LoadAsync(this, this->ConsumableActionConfig.GameplayAbility,
                                   [](TSubclassOf<UGameplayAbility>) {});
}

void UConsumableActionFragment::OnClientActionEnd(UMythicItemInstance *ItemInstance) {
    // Release is not a second request. The inventory/controller or exact granted GAS source owns use.
}

void UConsumableActionFragment::ServerHandleAction_Implementation(UMythicItemInstance *ItemInstance) {
    if (!IsValid(ItemInstance) || GetOwningItemInstance()!=ItemInstance || !ItemInstance->ContainsFragment(this)) return;
    UMythicInventoryComponent* Inventory=ItemInstance->GetInventoryComponent();
    if (!Inventory) return;
    FMythicInventorySourceLocator Source;
    Source.Inventory=Inventory; Source.SlotIndex=ItemInstance->GetSlot();
    Source.ExpectedItemGuid=ItemInstance->GetItemInstanceGuid(); Source.ExpectedQuantity=ItemInstance->GetStacks();
    int32 Consumed=0; Inventory->TryUseItemInSlot(Source,Consumed);
}

bool UConsumableActionFragment::ExecuteReservedInventoryAction(UMythicItemInstance* ItemInstance, bool& bConsumeOne) {
    bConsumeOne=false;
    if (!ItemInstance || GetOwningItemInstance()!=ItemInstance || !ItemInstance->ContainsFragment(this)
        || !UMythicInventoryComponent::IsItemMutationFrozen(ItemInstance)) return false;
    auto* Owner=ItemInstance->GetInventoryOwner();
    auto* ASC=Owner ? Cast<UMythicAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner)):nullptr;
    if (!Owner || !Owner->HasAuthority() || !ASC) return false;
    // Prepare every fallible immutable reference before tags, events or ability grants can commit.
    UClass* AbilityClass=ConsumableActionConfig.GameplayAbility.IsNull() ? nullptr : ConsumableActionConfig.GameplayAbility.LoadSynchronous();
    if (!ConsumableActionConfig.GameplayAbility.IsNull() && !AbilityClass) return false;
    bool bAccepted=false, bRetainForGrantedAbility=false;
    if (AbilityClass) {
        if (ConsumableActionConfig.RemoveAbility) {
            if (auto* Spec=ASC->FindAbilitySpecFromClass(AbilityClass)) {
                const auto Handle=Spec->Handle;
                ASC->ClearAbility(Handle); bAccepted=true;
            }
        } else {
            bRetainForGrantedAbility=HandleGrantAbility(ASC,ItemInstance);
            bAccepted=bRetainForGrantedAbility;
        }
    }
    if (!ConsumableActionConfig.TagsToAdd.IsEmpty()) {
        ASC->AddLooseGameplayTags(ConsumableActionConfig.TagsToAdd,1,EGameplayTagReplicationState::TagAndCountToAll);
        bAccepted=true;
    }
    if (!ConsumableActionConfig.TagsToRemove.IsEmpty() && ASC->HasAnyMatchingGameplayTags(ConsumableActionConfig.TagsToRemove)) {
        ASC->RemoveLooseGameplayTags(ConsumableActionConfig.TagsToRemove,1,EGameplayTagReplicationState::TagAndCountToAll);
        bAccepted=true;
    }
    if (ConsumableActionConfig.EventTag.IsValid()) {
        FGameplayEventData Payload; Payload.EventTag=ConsumableActionConfig.EventTag;
        Payload.Instigator=Owner; Payload.OptionalObject=this;
        bAccepted |= ASC->HandleGameplayEvent(Payload.EventTag,&Payload)>0;
    }
    bConsumeOne=bAccepted && !bRetainForGrantedAbility;
    return bAccepted;
}

bool UConsumableActionFragment::HandleGrantAbility(UMythicAbilitySystemComponent *ASC, UMythicItemInstance *ItemInstance) {
    const auto AbilityClass=ConsumableActionConfig.GameplayAbility.LoadSynchronous();
    if (!ASC || !ItemInstance || !AbilityClass) return false;
    // A preparation item intentionally survives use. Repeated accepted requests must not duplicate its exact-source spec.
    for (const FGameplayAbilitySpec& Spec:ASC->GetActivatableAbilities()) {
        if (!Spec.PendingRemove && !Spec.RemoveAfterActivation && Spec.Ability
            && Spec.Ability->GetClass()==AbilityClass && Spec.SourceObject.Get()==this) return true;
    }
    auto AbilityHandle = GrantItemAbility(ASC, ItemInstance, AbilityClass);

    if (!AbilityHandle.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UConsumableActionFragment::HandleGrantAbility: Failed to grant ability."));
        return false;
    }


    UE_LOG(Myth, Warning, TEXT("UConsumableFragment::GrantAbility: Granted Ability"));
    return true;
}

void UConsumableActionFragment::ExecuteGenericAction(UMythicItemInstance *ItemInstance) {
    ServerHandleAction(ItemInstance);
}

void UConsumableActionFragment::HandleInHandRemoveAbility(UMythicAbilitySystemComponent *ASC) {
    UClass *AbilityClass = this->ConsumableActionConfig.GameplayAbility.Get();
    if (!AbilityClass) {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::HandleInHandRemoveAbility: Ability class not loaded."));
        return;
    }

    auto Spec = ASC->FindAbilitySpecFromClass(AbilityClass);
    if (Spec && Spec->Handle.IsValid()) {
        ASC->ClearAbility(Spec->Handle);
        UE_LOG(Myth, Warning, TEXT("UConsumableFragment::OnInactiveItem: Removed Granted Ability"));
    }
    else {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::OnInactiveItem: Failed to remove Granted Ability"));
    }
}

void UConsumableActionFragment::HandleTags(UMythicItemInstance *ItemInstance) {
    auto InventoryOwner = ItemInstance->GetInventoryOwner();
    if (InventoryOwner->GetLocalRole() != ROLE_Authority) {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::HandleTags: Not authority"));
        return;
    }

    auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InventoryOwner);
    if (ASC) {
        if (ConsumableActionConfig.TagsToAdd.IsValid()) {
            UE_LOG(Myth, Warning, TEXT("UConsumableFragment::HandleTags: Adding Tag %s to ASC %s"), *ConsumableActionConfig.TagsToAdd.ToString(),
                   *ASC->GetName());
            ASC->AddLooseGameplayTags(ConsumableActionConfig.TagsToAdd, 1, EGameplayTagReplicationState::TagAndCountToAll);
        }

        if (ConsumableActionConfig.TagsToRemove.IsValid()) {
            ASC->RemoveLooseGameplayTags(ConsumableActionConfig.TagsToRemove, 1, EGameplayTagReplicationState::TagAndCountToAll);
        }
    }
    else {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::HandleTags: OwningASC is null"));
    }
}

bool UConsumableActionFragment::CanBeStackedWith(const UItemFragment *Other) const {
    auto OtherConsumable = Cast<UConsumableActionFragment>(Other);
    if (!OtherConsumable) {
        return false;
    }

    return Super::CanBeStackedWith(Other) &&
        this->ConsumableActionConfig.TagsToAdd == OtherConsumable->ConsumableActionConfig.TagsToAdd &&
        this->ConsumableActionConfig.TagsToRemove == OtherConsumable->ConsumableActionConfig.TagsToRemove &&
        this->ConsumableActionConfig.GameplayAbility == OtherConsumable->ConsumableActionConfig.GameplayAbility &&
        this->ConsumableActionConfig.RemoveAbility == OtherConsumable->ConsumableActionConfig.RemoveAbility &&
        this->ConsumableActionConfig.EventTag == OtherConsumable->ConsumableActionConfig.EventTag;
}
