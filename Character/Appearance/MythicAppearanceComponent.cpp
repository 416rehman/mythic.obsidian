#include "MythicAppearanceComponent.h"
#include "MythicAppearanceRenderer.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicCharacter.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Subsystem/SaveSystem/MythicSaveGameSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

UMythicAppearanceComponent::UMythicAppearanceComponent() {
    SetIsReplicatedByDefault(true); PrimaryComponentTick.bCanEverTick=false;
}
void UMythicAppearanceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicAppearanceComponent,OwnerState,COND_OwnerOnly);
}
void UMythicAppearanceComponent::BeginPlay() {
    Super::BeginPlay(); Catalog=UMythicAppearanceCatalog::Get();
    if (Catalog) { TArray<FText> Errors; bCatalogReady=Catalog->ValidateCatalog(Errors); }
    if (auto* State=Cast<APlayerState>(GetOwner())) State->OnPawnSet.AddDynamic(this,&ThisClass::HandlePawnSet);
    BindInventories();
    if (GetOwner()->HasAuthority()) {
        OwnerState.Session=FGuid::NewGuid();
        if (Catalog) { OwnerState.Recipe=Catalog->DefaultRecipe; bHasPersonalRecipe=true; }
        MarkEquipmentDirty();
    }
}
bool UMythicAppearanceComponent::IsReady() const {
    const auto* State=Cast<AMythicPlayerState>(GetOwner());
    return bCatalogReady && OwnerState.Session.IsValid() && RestoreDepth==0 && State && State->GetPawn() && State->GetPlayerController()
        && (!GetOwner()->HasAuthority() || BoundPawn.Get()==State->GetPawn());
}
TArray<UMythicInventoryComponent*> UMythicAppearanceComponent::Inventories() const {
    const auto* State=Cast<AMythicPlayerState>(GetOwner()); return State?State->GetAllInventoryComponents():TArray<UMythicInventoryComponent*>{};
}
void UMythicAppearanceComponent::UnbindInventories() {
    for (auto Weak:BoundInventories) if (auto* Inv=Weak.Get()) {
        Inv->OnSlotUpdated.RemoveDynamic(this,&ThisClass::HandleSlotChanged);
        Inv->OnInventorySizeChanged.RemoveDynamic(this,&ThisClass::HandleSizeChanged);
    }
    BoundInventories.Reset();
}
void UMythicAppearanceComponent::BindInventories() {
    const auto Current=Inventories();
    bool bSame=Current.Num()==BoundInventories.Num();
    for (int32 I=0;bSame && I<Current.Num();++I) bSame=Current[I]==BoundInventories[I].Get();
    if (bSame) return;
    UnbindInventories();
    for (auto* Inv:Current) if (Inv) {
        Inv->OnSlotUpdated.AddDynamic(this,&ThisClass::HandleSlotChanged);
        Inv->OnInventorySizeChanged.AddDynamic(this,&ThisClass::HandleSizeChanged);
        BoundInventories.Add(Inv);
    }
}
void UMythicAppearanceComponent::HandleSlotChanged(int32 Slot) { MarkEquipmentDirty(); }
void UMythicAppearanceComponent::HandleSizeChanged(int32 NewSize,int32 OldSize) { MarkEquipmentDirty(); }
void UMythicAppearanceComponent::HandlePawnSet(APlayerState* State,APawn* NewPawn,APawn* OldPawn) {
    if (GetOwner()->HasAuthority() && NewPawn!=OldPawn) {
        OwnerState.Session.Invalidate(); PendingSaves.Reset(); FailedSaves.Reset(); Replays.Reset(); ReplayOrder.Reset();
        BoundPawn.Reset(); GetOwner()->ForceNetUpdate();
    }
    MarkEquipmentDirty(); OnRep_State();
}
void UMythicAppearanceComponent::MarkEquipmentDirty() {
    if (!GetWorld()) return;
    bDirty=true;
    if (RestoreDepth || GetWorld()->GetTimerManager().IsTimerActive(RebuildTimer)) return;
    // Inventory removal signals arrive before array erasure; only consume the settled transaction.
    RebuildTimer=GetWorld()->GetTimerManager().SetTimerForNextTick(this,&ThisClass::RebuildPublic);
}
void UMythicAppearanceComponent::GetEquipment(TArray<FMythicAppearanceEquippedItem>& Out) const {
    Out.Reset();
    for (auto* Inv:Inventories()) if (Inv) for (const auto& Slot:Inv->GetAllSlots()) {
        auto* Item=Slot.SlottedItemInstance.Get();
        if (Item && Slot.AppliesEquipmentEffects() && Slot.SlotDefinition) Out.Add({Item->GetItemDefinition(),Slot.SlotDefinition->SlotType,Item->GetAppearanceData(),Item->GetItemInstanceGuid()});
    }
}
TArray<UMythicItemInstance*> UMythicAppearanceComponent::GetEditableItems() const {
    TArray<UMythicItemInstance*> Out;
    if (!Catalog) return Out;
    for (auto* Inv:Inventories()) if (Inv) for (const auto& Slot:Inv->GetAllSlots()) {
        if (auto* Item=Slot.SlottedItemInstance.Get(); Item && Slot.AppliesEquipmentEffects() && Catalog->FindGear(Item->GetItemDefinition())) Out.Add(Item);
    }
    return Out;
}
UMythicItemInstance* UMythicAppearanceComponent::FindEquippedItem(FGuid Guid) const {
    UMythicItemInstance* Result=nullptr;
    for (auto* Item:GetEditableItems()) if (Item->GetItemInstanceGuid()==Guid) {
        if (Result) return nullptr; // Ambiguous physical identity never authorizes an edit.
        Result=Item;
    }
    return Result;
}
void UMythicAppearanceComponent::RebuildPublic() {
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RebuildTimer);
    if (RestoreDepth) return;
    if (!GetOwner()->HasAuthority()) { bDirty=false; OnRep_State(); return; }
    if (!Catalog) { Catalog=UMythicAppearanceCatalog::Get(); if (Catalog) { TArray<FText> Errors; bCatalogReady=Catalog->ValidateCatalog(Errors); } }
    if (!bCatalogReady) return;
    BindInventories();
    auto* State=Cast<AMythicPlayerState>(GetOwner());
    auto* Pawn=State?State->GetPawn():nullptr;
    if (!Pawn) return;
    if (BoundPawn.Get()!=Pawn) {
        BoundPawn=Pawn; PublicIncarnation=FGuid::NewGuid();
        OwnerState.Session=FGuid::NewGuid(); Replays.Reset(); ReplayOrder.Reset(); PendingSaves.Reset(); FailedSaves.Reset();
        EquipmentSignature.Reset();
    }
    TArray<FString> Entries;
    for (auto* Inv:Inventories()) if (Inv) for (const auto& Slot:Inv->GetAllSlots()) {
        if (auto* Item=Slot.SlottedItemInstance.Get(); Item && Slot.AppliesEquipmentEffects()) Entries.Add(FString::Printf(TEXT("%s:%d:%s:%s:%d"),
            *Inv->GetName(),Slot.EntryIndex,*Item->GetItemInstanceGuid().ToString(),*GetPathNameSafe(Item->GetItemDefinition()),Item->GetAppearanceData().Revision));
    }
    Entries.Sort(); const FString Signature=FString::Join(Entries,TEXT("|"));
    const bool bEquipmentChanged=Signature!=EquipmentSignature;
    if (bEquipmentChanged && !bCommittedRevision && OwnerState.Revision<MAX_int32) ++OwnerState.Revision;
    EquipmentSignature=Signature; bCommittedRevision=false; bDirty=false;
    TArray<FMythicAppearanceEquippedItem> Equipment; GetEquipment(Equipment);
    FMythicAppearancePublicState Public;
    bool bResolved=Catalog->ResolveOutfit(OwnerState.Recipe,Equipment,Public);
    if (!bResolved) bResolved=Catalog->ResolveOutfit(OwnerState.Recipe,{},Public);
    if (bResolved) {
        Public.Incarnation=PublicIncarnation; Public.Revision=OwnerState.Revision;
        if (auto* Renderer=Pawn->FindComponentByClass<UMythicAppearanceRenderer>()) Renderer->Publish(Public);
    }
    GetOwner()->ForceNetUpdate(); OnRep_State();
}
void UMythicAppearanceComponent::OnRep_State() { BindInventories(); OnAppearanceChanged.Broadcast(); }

void UMythicAppearanceComponent::SendResult(FGuid Request,EMythicAppearanceResult Result,const FMythicAppearanceSaveStamp* Accepted) {
    FMythicAppearanceReceipt Receipt; Receipt.RequestId=Request; Receipt.Session=Accepted?Accepted->Session:OwnerState.Session;
    Receipt.Revision=Accepted?Accepted->Revision:OwnerState.Revision; Receipt.Result=Result;
    if (!Replays.Contains(Request)) ReplayOrder.Add(Request);
    Replays.Add(Request,Receipt);
    while (ReplayOrder.Num()>32) { FailedSaves.Remove(ReplayOrder[0]); Replays.Remove(ReplayOrder[0]); ReplayOrder.RemoveAt(0); }
    ClientAppearanceResult(Receipt);
}
void UMythicAppearanceComponent::ClientAppearanceResult_Implementation(FMythicAppearanceReceipt Receipt) {
    if (Receipt.Session!=LastReceipt.Session || Receipt.Revision>=LastReceipt.Revision) LastReceipt=Receipt;
    OnAppearanceResult.Broadcast(Receipt);
}
void UMythicAppearanceComponent::ServerApplyAppearance_Implementation(FMythicAppearanceEditRequest Request) {
    if (const auto* Cached=Replays.Find(Request.RequestId)) { ClientAppearanceResult(*Cached); return; }
    if (!Request.IsBounded()) { SendResult(Request.RequestId,EMythicAppearanceResult::Invalid); return; }
    if (!IsReady()) { SendResult(Request.RequestId,EMythicAppearanceResult::NotReady); return; }
    if (Request.Session!=OwnerState.Session || Request.ExpectedRevision!=OwnerState.Revision) { SendResult(Request.RequestId,EMythicAppearanceResult::Stale); return; }
    const double Now=FPlatformTime::Seconds();
    if (Now-LastRequestTime<0.15 || PendingSaves.Num()>=4) { SendResult(Request.RequestId,EMythicAppearanceResult::Busy); return; }
    LastRequestTime=Now;
    auto* State=Cast<AMythicPlayerState>(GetOwner()); auto* PC=State?State->GetPlayerController():nullptr;
    if (!PC || UMythicInventoryComponent::IsOwnerMutationFrozen(PC) || UMythicInventoryComponent::IsOwnerUseInProgress(PC)) { SendResult(Request.RequestId,EMythicAppearanceResult::Busy); return; }
    if (bDirty) RebuildPublic();
    if (Request.ExpectedRevision!=OwnerState.Revision || Request.Session!=OwnerState.Session) { SendResult(Request.RequestId,EMythicAppearanceResult::Stale); return; }
    if (Request.bChangePersonal && !Catalog->ValidatePersonalChange(OwnerState.Recipe,Request.Personal)) { SendResult(Request.RequestId,EMythicAppearanceResult::Invalid); return; }
    struct FStaged { TWeakObjectPtr<UMythicItemInstance> Item; FMythicItemAppearanceData Data; };
    TArray<FStaged> Staged;
    for (const auto& Edit:Request.Items) {
        auto* Item=FindEquippedItem(Edit.ItemGuid);
        if (!Item || UMythicInventoryComponent::IsItemMutationFrozen(Item)) { SendResult(Request.RequestId,EMythicAppearanceResult::NotOwned); return; }
        if (Item->GetAppearanceData().Revision!=Edit.ExpectedRevision) { SendResult(Request.RequestId,EMythicAppearanceResult::Stale); return; }
        const auto* Adapter=Catalog->FindGear(Item->GetItemDefinition());
        auto Next=Item->GetAppearanceData();
        if (!Adapter || !Catalog->ValidateItemChanges(*Adapter,Edit.Changes) || !FMythicItemAppearanceData::NormalizeColors(Next.Colors) || Next.Revision==MAX_int32) { SendResult(Request.RequestId,EMythicAppearanceResult::Invalid); return; }
        for (const auto& Color:Edit.Changes) {
            Next.Colors.RemoveAll([&](const auto& C){return C.Channel==Color.Channel;});
            if (!Color.Palette.IsNone()) Next.Colors.Add(Color);
        }
        if (!FMythicItemAppearanceData::NormalizeColors(Next.Colors)) { SendResult(Request.RequestId,EMythicAppearanceResult::Invalid); return; }
        if (!Next.HasSameColors(Item->GetAppearanceData())) { ++Next.Revision; Staged.Add({Item,MoveTemp(Next)}); }
    }
    if (OwnerState.Revision==MAX_int32) { SendResult(Request.RequestId,EMythicAppearanceResult::NotReady); return; }
    // Complete validation precedes assignment. Notify only after every physical item has its new data.
    if (Request.bChangePersonal) OwnerState.Recipe=Request.Personal;
    for (auto& Change:Staged) Change.Item->AppearanceData=MoveTemp(Change.Data);
    ++OwnerState.Revision; bCommittedRevision=true;
    TArray<FMythicAppearanceItemStamp> Stamps;
    for (const auto& Edit:Request.Items) if (auto* Item=FindEquippedItem(Edit.ItemGuid)) Stamps.Add({Edit.ItemGuid,Item->GetAppearanceData().Revision});
    auto Wanted=CaptureSaveStamp(); Wanted.Items=Stamps;
    const auto StillCurrent=[&](){return Wanted.Session==OwnerState.Session && Wanted.Revision==OwnerState.Revision && !RestoreDepth;};
    for (const auto& Change:Staged) {
        if (!StillCurrent()) break;
        if (auto* Item=Change.Item.Get()) if (auto* Inv=Item->GetInventoryComponent()) Inv->NotifyItemInstanceUpdated(Item->GetSlot(),false);
    }
    if (!StillCurrent()) {
        ClientAppearanceResult({Request.RequestId,Wanted.Session,Wanted.Revision,EMythicAppearanceResult::Superseded}); return;
    }
    RebuildPublic();
    if (!StillCurrent()) { ClientAppearanceResult({Request.RequestId,Wanted.Session,Wanted.Revision,EMythicAppearanceResult::Superseded}); return; }
    SendResult(Request.RequestId,EMythicAppearanceResult::Applied);
    if (StillCurrent()) RequestSave(Request.RequestId,Wanted);
}
FMythicAppearanceSaveStamp UMythicAppearanceComponent::CaptureSaveStamp() const {
    FMythicAppearanceSaveStamp Stamp; Stamp.Session=OwnerState.Session; Stamp.Revision=OwnerState.Revision; Stamp.Recipe=OwnerState.Recipe;
    if (const auto* State=Cast<AMythicPlayerState>(GetOwner())) Stamp.Route=State->GetPersistentCharacterId();
    return Stamp;
}
void UMythicAppearanceComponent::RequestSave(FGuid Request,const FMythicAppearanceSaveStamp& Wanted) {
    auto* State=Cast<AMythicPlayerState>(GetOwner()); auto* PC=State?State->GetPlayerController():nullptr;
    auto* Save=GetWorld()->GetGameInstance()?GetWorld()->GetGameInstance()->GetSubsystem<UMythicSaveGameSubsystem>():nullptr;
    FailedSaves.Add(Request,Wanted);
    if (!State || !PC || !Save) { SendResult(Request,EMythicAppearanceResult::SaveFailed,&Wanted); return; }
    if (Wanted.Session!=OwnerState.Session || Wanted.Route!=State->GetPersistentCharacterId()) return;
    PendingSaves.Add(Request,Wanted.Items); // Save admission may call completion synchronously.
    FGuid Operation;
    const TWeakObjectPtr<UMythicAppearanceComponent> Weak(this);
    const bool bAccepted=Save->RequestDurableCharacterSave(PC,Wanted.Route,
        FMythicDurableCharacterSaveComplete::CreateLambda([Weak,Request,Wanted](const FMythicDurableCharacterSaveResult& Result) {
            if (auto* Self=Weak.Get()) Self->SaveCompleted(Request,Wanted,Result);
        }),Operation);
    if (!bAccepted && PendingSaves.Contains(Request)) { PendingSaves.Remove(Request); SendResult(Request,EMythicAppearanceResult::SaveFailed,&Wanted); }
}
void UMythicAppearanceComponent::SaveCompleted(FGuid Request,FMythicAppearanceSaveStamp Wanted,const FMythicDurableCharacterSaveResult& Result) {
    if (!PendingSaves.Remove(Request) || Wanted.Session!=OwnerState.Session) return;
    if (!Result.bSuccess) { SendResult(Request,EMythicAppearanceResult::SaveFailed,&Wanted); return; }
    const auto& Captured=Result.CapturedAppearance;
    bool bCovered=Result.RoutingCharacterId==Wanted.Route && Captured.Route==Wanted.Route && Captured.Session==Wanted.Session && Captured.Revision>=Wanted.Revision && Captured.Recipe==Wanted.Recipe;
    for (const auto& Item:Wanted.Items) bCovered &= Captured.Items.Contains(Item);
    FailedSaves.Remove(Request); SendResult(Request,bCovered?EMythicAppearanceResult::Saved:EMythicAppearanceResult::Superseded,&Wanted);
}
void UMythicAppearanceComponent::ServerRetryAppearanceSave_Implementation(FGuid RequestId,FGuid Session,int32 Revision) {
    const auto* Previous=Replays.Find(RequestId);
    if (!IsReady() || Session!=OwnerState.Session || !Previous || Revision!=Previous->Revision || Previous->Session!=Session || Previous->Result!=EMythicAppearanceResult::SaveFailed || PendingSaves.Contains(RequestId) || PendingSaves.Num()>=4) return;
    const double Now=FPlatformTime::Seconds();
    if (Now-LastRequestTime<0.15) return;
    LastRequestTime=Now;
    if (const auto* Failed=FailedSaves.Find(RequestId)) { const auto Wanted=*Failed; RequestSave(RequestId,Wanted); }
}
void UMythicAppearanceComponent::BeginRestore() {
    if (!GetOwner()->HasAuthority()) return;
    ++RestoreDepth; PendingSaves.Reset(); FailedSaves.Reset(); Replays.Reset(); ReplayOrder.Reset();
    OwnerState.Session.Invalidate();
}
void UMythicAppearanceComponent::FinishRestore(const FMythicAppearanceRecipe* Recipe,bool bSuccess) {
    if (!GetOwner()->HasAuthority() || RestoreDepth<=0) return;
    if (--RestoreDepth) return;
    if (bSuccess) {
        if (Recipe) { OwnerState.Recipe=*Recipe; bHasPersonalRecipe=true; }
        else if (Catalog) { OwnerState.Recipe=Catalog->DefaultRecipe; bHasPersonalRecipe=true; }
        else { OwnerState.Recipe={}; bHasPersonalRecipe=false; }
    }
    OwnerState.Session=FGuid::NewGuid(); OwnerState.Revision=0; PublicIncarnation=FGuid::NewGuid();
    EquipmentSignature.Reset(); MarkEquipmentDirty();
}
FText UMythicAppearanceComponent::DescribeResult(EMythicAppearanceResult Result) {
    switch (Result) {
    case EMythicAppearanceResult::Applied: return NSLOCTEXT("Appearance","Applied","Applied · Saving…");
    case EMythicAppearanceResult::Saved: return NSLOCTEXT("Appearance","Saved","Saved");
    case EMythicAppearanceResult::SaveFailed: return NSLOCTEXT("Appearance","SaveFailed","Applied. Save failed; retry saving.");
    case EMythicAppearanceResult::Superseded: return NSLOCTEXT("Appearance","Superseded","Appearance or item ownership changed before this save completed.");
    case EMythicAppearanceResult::Stale: return NSLOCTEXT("Appearance","Stale","Your character or equipment changed. Review the current appearance before applying.");
    case EMythicAppearanceResult::NotOwned: return NSLOCTEXT("Appearance","NotOwned","That item is no longer equipped by this character.");
    case EMythicAppearanceResult::Busy: return NSLOCTEXT("Appearance","Busy","Finish the current item action, then try again.");
    case EMythicAppearanceResult::Invalid: return NSLOCTEXT("Appearance","Invalid","This appearance choice is unavailable or incompatible.");
    default: return NSLOCTEXT("Appearance","NotReady","Appearance is still loading.");
    }
}
void UMythicAppearanceComponent::EndPlay(const EEndPlayReason::Type Reason) {
    if (auto* State=Cast<APlayerState>(GetOwner())) State->OnPawnSet.RemoveDynamic(this,&ThisClass::HandlePawnSet);
    UnbindInventories(); if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RebuildTimer);
    PendingSaves.Reset(); OwnerState.Session.Invalidate(); Super::EndPlay(Reason);
}
