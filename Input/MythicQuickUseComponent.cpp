#include "MythicQuickUseComponent.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/Actionable/ConsumableEffectFragment.h"
#include "Player/MythicPlayerController.h"
#include "Net/UnrealNetwork.h"

UMythicQuickUseComponent::UMythicQuickUseComponent() { PrimaryComponentTick.bCanEverTick=false; SetIsReplicatedByDefault(true); }
void UMythicQuickUseComponent::BeginPlay() {
    Super::BeginPlay();
    if (auto* PC=Cast<APlayerController>(GetOwner())) PC->OnPossessedPawnChanged.AddDynamic(this,&ThisClass::HandlePawn);
    RefreshInventories();
}
void UMythicQuickUseComponent::EndPlay(const EEndPlayReason::Type Reason) {
    if (auto* PC=Cast<APlayerController>(GetOwner())) PC->OnPossessedPawnChanged.RemoveDynamic(this,&ThisClass::HandlePawn);
    for (auto& Weak:BoundInventories) if (auto* Inventory=Weak.Get()) { Inventory->OnSlotUpdated.RemoveAll(this); Inventory->OnInventorySizeChanged.RemoveAll(this); }
    Super::EndPlay(Reason);
}
void UMythicQuickUseComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(ThisClass,SelectedHealing,COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(ThisClass,SelectedUtility,COND_OwnerOnly);
}
void UMythicQuickUseComponent::HandlePawn(APawn*,APawn*) { RefreshInventories(); }
void UMythicQuickUseComponent::HandleSlot(int32) { RebuildEntries(); }
void UMythicQuickUseComponent::HandleSize(int32,int32) { RebuildEntries(); }
void UMythicQuickUseComponent::RefreshInventories() {
    for (auto& Weak:BoundInventories) if (auto* Inventory=Weak.Get()) { Inventory->OnSlotUpdated.RemoveAll(this); Inventory->OnInventorySizeChanged.RemoveAll(this); }
    BoundInventories.Reset();
    if (auto* PC=Cast<AMythicPlayerController>(GetOwner())) for (auto* Inventory:PC->GetAllInventoryComponents()) if (Inventory) {
        BoundInventories.Add(Inventory);
        Inventory->OnSlotUpdated.AddUniqueDynamic(this,&ThisClass::HandleSlot);
        Inventory->OnInventorySizeChanged.AddUniqueDynamic(this,&ThisClass::HandleSize);
    }
    RebuildEntries();
}
bool UMythicQuickUseComponent::Describe(UMythicItemInstance* Item,bool bHealing,FMythicQuickUseEntry& Out) {
    if (!IsValid(Item) || Item->GetStacks()<=0 || !Item->GetItemDefinition()) return false;
    const auto* Fragment=Item->GetFragment<UConsumableEffectFragment>();
    if (!Fragment || Fragment->ConsumableEffectConfig.Effects.IsEmpty()) return false;
    const auto& Config=Fragment->ConsumableEffectConfig;
    if (!(bHealing ? Config.bHealing : Config.bUtility)) return false;
    // A saved or rolled variant must still agree with the immutable definition's quick-use contract.
    const auto* Authored=UItemDefinition::GetFragment<UConsumableEffectFragment>(Item->GetItemDefinition());
    if (!Authored || !Fragment->CanBeStackedWith(Authored)) return false;
    Out.Key.Definition=FSoftObjectPath(Item->GetItemDefinition()); Out.Key.Variant=Config.QuickUseVariant;
    Out.Name=Item->GetItemDefinition()->Name; Out.Icon=Item->GetItemDefinition()->Icon2d; Out.Quantity=Item->GetStacks();
    Out.SortPriority=Authored->ConsumableEffectConfig.QuickUsePriority;
    return true;
}
void UMythicQuickUseComponent::RebuildEntries() {
    HealingEntries.Reset(); UtilityEntries.Reset();
    for (auto& Weak:BoundInventories) if (auto* Inventory=Weak.Get()) {
        const auto& Slots=Inventory->GetAllSlots();
        for (int32 Index=0;Index<Slots.Num();++Index) if (Inventory->CanUseItemInSlot(Index)) for (bool bHealing:{true,false}) {
            FMythicQuickUseEntry Entry;
            if (!Describe(Slots[Index].SlottedItemInstance,bHealing,Entry)) continue;
            auto& Entries=bHealing ? HealingEntries:UtilityEntries;
            if (auto* Existing=Entries.FindByPredicate([&](const auto& Candidate){return Candidate.Key==Entry.Key;})) Existing->Quantity+=Entry.Quantity;
            else Entries.Add(Entry);
        }
    }
    const auto Sort=[](const auto& A,const auto& B){
        if (A.SortPriority!=B.SortPriority) return A.SortPriority<B.SortPriority;
        const int32 NameOrder=A.Name.CompareTo(B.Name);
        if (NameOrder!=0) return NameOrder<0;
        const FString AP=A.Key.Definition.ToString(),BP=B.Key.Definition.ToString();
        return AP==BP ? A.Key.Variant.LexicalLess(B.Key.Variant):AP<BP;
    };
    HealingEntries.Sort(Sort); UtilityEntries.Sort(Sort);
    if (GetOwner()->HasAuthority()) {
        if (!SelectedHealing.IsValid() && !HealingEntries.IsEmpty()) SelectedHealing=HealingEntries[0].Key;
        if (!SelectedUtility.IsValid() && !UtilityEntries.IsEmpty()) SelectedUtility=UtilityEntries[0].Key;
    }
    OnEntriesChanged.Broadcast();
}
bool UMythicQuickUseComponent::Select(bool bHealing,const FMythicQuickUseKey& Key) {
    if (!GetEntries(bHealing).ContainsByPredicate([&](const auto& Entry){return Entry.Key==Key && Entry.Quantity>0;})) return false;
    ServerSelect(bHealing,Key); return true;
}
void UMythicQuickUseComponent::ServerSelect_Implementation(bool bHealing,FMythicQuickUseKey Key) {
    RefreshInventories();
    if (!GetEntries(bHealing).ContainsByPredicate([&](const auto& Entry){return Entry.Key==Key && Entry.Quantity>0;})) return;
    (bHealing ? SelectedHealing:SelectedUtility)=Key; OnEntriesChanged.Broadcast();
}
void UMythicQuickUseComponent::RestoreSelections(const FMythicQuickUseKey& Healing,const FMythicQuickUseKey& Utility) {
    if (!GetOwner()->HasAuthority()) return;
    SelectedHealing=Healing; SelectedUtility=Utility; RefreshInventories();
}
void UMythicQuickUseComponent::UseSelected(bool bHealing) {
    auto* PC=Cast<AMythicPlayerController>(GetOwner()); if (!PC || !PC->IsLocalController()) return;
    const auto Key=GetSelected(bHealing); if (!Key.IsValid()) return;
    // Scan only on use, select one exact physical stack; no fallback to a different variant.
    for (auto* Inventory:PC->GetAllInventoryComponents()) if (Inventory) {
        const auto& Slots=Inventory->GetAllSlots();
        for (int32 Index=0;Index<Slots.Num();++Index) {
            auto* Item=Slots[Index].SlottedItemInstance.Get(); FMythicQuickUseEntry Entry;
            if (!Inventory->CanUseItemInSlot(Index) || !Describe(Item,bHealing,Entry) || !(Entry.Key==Key)) continue;
            FMythicInventorySourceLocator Source;
            Source.Inventory=Inventory; Source.SlotIndex=Index; Source.ExpectedItemGuid=Item->GetItemInstanceGuid(); Source.ExpectedQuantity=Item->GetStacks();
            Source.UsePurpose=bHealing ? EMythicItemUsePurpose::Healing:EMythicItemUsePurpose::Utility;
            PC->SubmitInventoryUse(Source); return;
        }
    }
}
