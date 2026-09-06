#include "MythicInventoryComponent.h"
#include "MythicItemInstance.h"
#include "Fragments/Actionable/ConsumableEffectFragment.h"
#include "Fragments/Actionable/ConsumableActionFragment.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "Player/MythicPlayerController.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectHash.h"

namespace {
TSet<const AActor*> ActiveUseOwners;
TSet<const AActor*> FrozenUseOwners;
TSet<const UMythicItemInstance*> FrozenUseItems;

bool IsLiveUseRecipient(AActor* Owner, UAbilitySystemComponent* ASC, AActor* Avatar, UMythicLifeComponent* Life) {
    if (!IsValid(Owner) || !Owner->HasAuthority() || !IsValid(ASC) || !IsValid(Avatar) || !IsValid(Life)
        || ASC->GetAvatarActor()!=Avatar || UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner)!=ASC
        || UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Avatar)!=ASC
        || UMythicLifeComponent::FindHealthComponent(Avatar)!=Life || !Life->IsInitialized()
        || Life->IsDead() || Life->IsDowned()) return false;
    if (const auto* PC=Cast<APlayerController>(Owner)) {
        if (PC->GetPawn()!=Avatar || Cast<APawn>(Avatar)->GetController()!=PC) return false;
    }
    return FMath::IsFinite(Life->GetHealth()) && Life->GetHealth()>0
        && FMath::IsFinite(Life->GetMaxHealth()) && Life->GetMaxHealth()>0;
}
}
bool UMythicInventoryComponent::IsOwnerUseInProgress(const AActor* Owner) { return ActiveUseOwners.Contains(Owner); }
bool UMythicInventoryComponent::IsOwnerMutationFrozen(const AActor* Owner) { return FrozenUseOwners.Contains(Owner); }
bool UMythicInventoryComponent::IsItemMutationFrozen(const UMythicItemInstance* Item) {
    return Item && (FrozenUseItems.Contains(Item) || IsOwnerMutationFrozen(Item->GetInventoryOwner()));
}

void UMythicInventoryComponent::SettleReservedConsumableUse(UMythicItemInstance* Item, int32 SlotIndex, int32 ExpectedQuantity) {
    // Admission and the source/owner lease guarantee this exact pointer layout. There is no fallible detach here.
    check(Item && IsItemMutationFrozen(Item) && IsOwnerMutationFrozen(GetOwner()));
    check(Slots.IsValidIndex(SlotIndex) && Slots.Items[SlotIndex].SlottedItemInstance==Item);
    check(!Slots.Items[SlotIndex].AppliesEquipmentEffects() && Item->GetStacks()==ExpectedQuantity && ExpectedQuantity>0);
    if (ExpectedQuantity>1) {
        Item->Quantity=ExpectedQuantity-1;
        NotifyItemInstanceUpdated(SlotIndex,false);
        return;
    }
    // No callback observes quantity zero in an occupied slot, or a source back-pointer naming an empty slot.
    Slots.Items[SlotIndex].SlottedItemInstance=nullptr;
    Item->OwningInventory=nullptr; Item->SlotIndex=INDEX_NONE; Item->Quantity=0;
    // Passive-storage items have no worn effects to reconcile. Clear any exact input spec owned by the effect
    // fragment before OnInventorySlotChanged clears its cached ASC. Every callback still sees both reservations.
    if (auto* Effect=const_cast<UConsumableEffectFragment*>(Item->GetFragment<UConsumableEffectFragment>())) Effect->OnItemDeactivated(Item);
    for (UItemFragment* Fragment:Item->ItemFragments) if (IsValid(Fragment)) Fragment->OnInventorySlotChanged(nullptr,INDEX_NONE);
    // Item/fragment ownership may use actor or component registered lists. Retire only this item's own subtree.
    TArray<UObject*> OwnedObjects;
    GetObjectsWithOuter(Item,OwnedObjects,EGetObjectsFlags::IncludeNestedObjects,RF_NoFlags);
    OwnedObjects.Add(Item);
    AActor* Owner=GetOwner();
    for (UObject* Object:OwnedObjects) {
        auto* Replicated=Cast<UMythicReplicatedObject>(Object);
        if (!Replicated || Replicated->GetOwningActor()!=Owner) continue;
        if (Owner->IsActorComponentReplicatedSubObjectRegistered(this,Replicated)) Owner->RemoveActorComponentReplicatedSubObject(this,Replicated);
        if (Owner->IsReplicatedSubObjectRegistered(Replicated)) Owner->RemoveReplicatedSubObject(Replicated);
        Replicated->MarkAsGarbage();
    }
    Item->ItemFragments.Empty();
    Slots.MarkItemDirty(Slots.Items[SlotIndex]);
    HandleSlotsChanged(TArrayView<int32>(&SlotIndex,1),Slots.Num());
}

EMythicInventoryActionResult UMythicInventoryComponent::ExecuteConsumableUse(const FMythicInventorySourceLocator& Source,int32& OutQuantityConsumed) {
    OutQuantityConsumed=0;
    AActor* Owner=GetOwner();
    if (!IsInGameThread() || !IsValid(Owner) || !Owner->HasAuthority() || ActiveUseOwners.Contains(Owner)) return EMythicInventoryActionResult::CommitFailed;
    UMythicItemInstance* Item=nullptr;
    const auto Resolved=ResolveSourceLocator(Source,Item);
    if (Resolved!=EMythicInventoryActionResult::Succeeded) return Resolved;
    if (!CanUseItemInSlot(Source.SlotIndex) || IsItemMutationFrozen(Item) || Item->GetOwningActor()!=Owner) return EMythicInventoryActionResult::NotUsable;
    auto* Definition=Item->GetItemDefinition();
    const auto* Effect=Item->GetFragment<UConsumableEffectFragment>();
    auto* Action=const_cast<UConsumableActionFragment*>(Item->GetFragment<UConsumableActionFragment>());
    if (!Definition || (!Effect && !Action) || (Effect && Action)) return EMythicInventoryActionResult::NotUsable;
    if (static_cast<uint8>(Source.UsePurpose)>static_cast<uint8>(EMythicItemUsePurpose::Utility)) return EMythicInventoryActionResult::InvalidRequest;
    auto* ASC=UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
    AActor* Avatar=ASC ? ASC->GetAvatarActor():nullptr;
    auto* Life=UMythicLifeComponent::FindHealthComponent(Avatar);
    if (!IsLiveUseRecipient(Owner,ASC,Avatar,Life)) return EMythicInventoryActionResult::NotUsable;
    // Preserve referenced objects through arbitrary synchronous GAS callbacks; the owner reservation rejects saves.
    TStrongObjectPtr<UMythicInventoryComponent> KeepInventory(this);
    TStrongObjectPtr<UMythicItemInstance> KeepItem(Item);
    TStrongObjectPtr<UItemDefinition> KeepDefinition(Definition);
    TStrongObjectPtr<UAbilitySystemComponent> KeepASC(ASC);
    TStrongObjectPtr<AActor> KeepOwner(Owner), KeepAvatar(Avatar);
    TStrongObjectPtr<UMythicLifeComponent> KeepLife(Life);
    TArray<FGameplayEffectSpecHandle,TInlineAllocator<4>> Specs;
    if (Effect) {
        const auto* Authored=UItemDefinition::GetFragment<UConsumableEffectFragment>(Definition);
        if (!Authored || Item->CountFragmentsOfClass(UConsumableEffectFragment::StaticClass())!=1
            || Effect->GetOwningItemInstance()!=Item || !Effect->CanBeStackedWith(Authored)) return EMythicInventoryActionResult::NotUsable;
        const auto& Config=Effect->ConsumableEffectConfig;
        if (Config.Effects.IsEmpty() || Config.Effects.Num()>32) return EMythicInventoryActionResult::NotUsable;
        if (Source.UsePurpose==EMythicItemUsePurpose::Healing && !Config.bHealing) return EMythicInventoryActionResult::NotUsable;
        if (Source.UsePurpose==EMythicItemUsePurpose::Utility && !Config.bUtility) return EMythicInventoryActionResult::NotUsable;
        if ((Source.UsePurpose==EMythicItemUsePurpose::Healing || (Config.bHealing && !Config.bUtility)) && Life->GetHealth()>=Life->GetMaxHealth()) return EMythicInventoryActionResult::NotUsable;
        for (const auto& Entry:Config.Effects) {
            if (!Entry.GameplayEffectClass || !FMath::IsFinite(Entry.EffectLevel) || Entry.EffectLevel<=0) return EMythicInventoryActionResult::NotUsable;
            auto Context=ASC->MakeEffectContext(); Context.AddSourceObject(Effect);
            auto Spec=ASC->MakeOutgoingSpec(Entry.GameplayEffectClass,Entry.EffectLevel,Context);
            if (!Spec.IsValid()) return EMythicInventoryActionResult::CommitFailed;
            for (const auto& Magnitude:Entry.SetByCallerMagnitudes) {
                if (!Magnitude.Key.IsValid() || !FMath::IsFinite(Magnitude.Value)) return EMythicInventoryActionResult::NotUsable;
                Spec.Data->SetSetByCallerMagnitude(Magnitude.Key,Magnitude.Value);
            }
            Specs.Add(Spec);
        }
    } else {
        const auto* Authored=UItemDefinition::GetFragment<UConsumableActionFragment>(Definition);
        if (Source.UsePurpose!=EMythicItemUsePurpose::Inventory || !Authored
            || Item->CountFragmentsOfClass(UConsumableActionFragment::StaticClass())!=1
            || Action->GetOwningItemInstance()!=Item || !Action->CanBeStackedWith(Authored)) return EMythicInventoryActionResult::NotUsable;
    }
    ActiveUseOwners.Add(Owner); FrozenUseOwners.Add(Owner); FrozenUseItems.Add(Item);
    ON_SCOPE_EXIT { FrozenUseItems.Remove(Item); FrozenUseOwners.Remove(Owner); ActiveUseOwners.Remove(Owner); };
    bool bAccepted=false, bConsumeOne=Effect!=nullptr;
    if (Effect) {
        for (const auto& Spec:Specs) {
            if (!IsLiveUseRecipient(Owner,ASC,Avatar,Life)) break;
            bAccepted |= ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()).WasSuccessfullyApplied();
        }
    } else if (IsLiveUseRecipient(Owner,ASC,Avatar,Life)) {
        bAccepted=Action->ExecuteReservedInventoryAction(Item,bConsumeOne);
    }
    if (!bAccepted) return EMythicInventoryActionResult::CommitFailed;
    if (bConsumeOne) {
        SettleReservedConsumableUse(Item,Source.SlotIndex,Source.ExpectedQuantity);
        OutQuantityConsumed=1;
        if (auto* PC=Cast<AMythicPlayerController>(Owner); IsValid(PC)) PC->NotifyItemUsed(Definition,1);
    }
    return EMythicInventoryActionResult::Succeeded;
}
