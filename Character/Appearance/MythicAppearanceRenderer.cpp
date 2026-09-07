#include "MythicAppearanceRenderer.h"
#include "MythicAppearanceCatalog.h"
#include "MythicAppearanceComponent.h"
#include "MythicAppearanceMeshComponent.h"
#include "Player/MythicCharacter.h"
#include "Player/MythicPlayerState.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Engine/AssetManager.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

namespace {
const FName BodyTag(TEXT("Mythic.Character.Body")), HeadTag(TEXT("Mythic.Character.OwnerHead"));
}

UMythicAppearanceRenderer::UMythicAppearanceRenderer() {
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bStartWithTickEnabled=false;
    PrimaryComponentTick.TickGroup=TG_PostUpdateWork;
}
void UMythicAppearanceRenderer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UMythicAppearanceRenderer,PublicState);
}
void UMythicAppearanceRenderer::BeginPlay() {
    Super::BeginPlay();
    Catalog=UMythicAppearanceCatalog::Get();
    if (auto* Character=Cast<AMythicCharacter>(GetOwner())) {
        TInlineComponentArray<USkeletalMeshComponent*> Components(Character);
        for (auto* Part:Components) if (Part!=Character->GetMesh() && Part->ComponentHasTag(BodyTag)) AuthoredBaseline.Add(Part);
    }
    RefreshBinding(); OnRep_PublicState();
}
void UMythicAppearanceRenderer::RefreshBinding() {
    if (auto* Character=Cast<AMythicCharacter>(GetOwner()); Character && Character->HasAuthority()) {
        if (auto* State=Character->GetPlayerState<AMythicPlayerState>()) {
            if (auto* Appearance=State->FindComponentByClass<UMythicAppearanceComponent>()) Appearance->MarkEquipmentDirty();
        }
    }
}
void UMythicAppearanceRenderer::Publish(const FMythicAppearancePublicState& State) {
    if (!GetOwner()->HasAuthority() || !State.Incarnation.IsValid()) return;
    if (PublicState.Incarnation==State.Incarnation && PublicState.SameLook(State)) return;
    PublicState=State; GetOwner()->ForceNetUpdate(); OnRep_PublicState();
}
void UMythicAppearanceRenderer::OnRep_PublicState() {
    if (!HasBegunPlay() || PublicState.Parts.IsEmpty()) return;
    QueueGameplayAnimation(PublicState.Frame);
    if (GetNetMode()==NM_DedicatedServer) return;
    Apply(PublicState);
}
void UMythicAppearanceRenderer::QueueGameplayAnimation(FName Frame) {
    if (PreviewDriver || !Catalog || !Driver()) return;
    PendingGameplayFrame=Frame;
    if (GameplayLoad) { GameplayLoad->CancelHandle(); GameplayLoad.Reset(); }
    const auto* Definition=Catalog->FindFrame(Frame);
    if (!Definition || Definition->GameplayAnimation.IsNull()) return;
    if (Definition->GameplayAnimation.Get()) { TryGameplayAnimation(); return; }
    GameplayLoad=UAssetManager::GetStreamableManager().RequestAsyncLoad(Definition->GameplayAnimation.ToSoftObjectPath(),
        FStreamableDelegate::CreateWeakLambda(this,[this,Frame](){ if (PendingGameplayFrame==Frame) TryGameplayAnimation(); }));
}
void UMythicAppearanceRenderer::TryGameplayAnimation() {
    if (PreviewDriver || !Catalog || !Driver()) return;
    const auto* Frame=Catalog->FindFrame(PendingGameplayFrame);
    if (!Frame || !Frame->GameplayAnimation.Get()) return;
    if (auto* Animation=Driver()->GetAnimInstance(); Animation && Animation->Montage_IsActive(nullptr)) {
        if (WaitingAnimation.Get()!=Animation) {
            if (WaitingAnimation.IsValid()) WaitingAnimation->OnMontageEnded.RemoveDynamic(this,&ThisClass::OnGameplayMontageEnded);
            WaitingAnimation=Animation;
            Animation->OnMontageEnded.AddUniqueDynamic(this,&ThisClass::OnGameplayMontageEnded);
        }
        return;
    }
    if (WaitingAnimation.IsValid()) WaitingAnimation->OnMontageEnded.RemoveDynamic(this,&ThisClass::OnGameplayMontageEnded);
    WaitingAnimation.Reset();
    if (Driver()->GetAnimClass()!=Frame->GameplayAnimation.Get()) Driver()->SetAnimInstanceClass(Frame->GameplayAnimation.Get());
    GameplayLoad.Reset();
}
void UMythicAppearanceRenderer::OnGameplayMontageEnded(UAnimMontage* Montage,bool bInterrupted) {
    // Never replace an AnimInstance while it is dispatching GAS montage delegates.
    if (GetWorld()) GameplayRetry=GetWorld()->GetTimerManager().SetTimerForNextTick(this,&ThisClass::TryGameplayAnimation);
}
USkeletalMeshComponent* UMythicAppearanceRenderer::Driver() const {
    if (PreviewDriver) return PreviewDriver;
    const auto* Character=Cast<AMythicCharacter>(GetOwner()); return Character?Character->GetMesh():nullptr;
}
void UMythicAppearanceRenderer::ApplyPreview(const FMythicAppearancePublicState& State,USkeletalMeshComponent* InDriver) {
    PreviewDriver=InDriver; Catalog=UMythicAppearanceCatalog::Get(); Apply(State);
}
void UMythicAppearanceRenderer::DestroyParts(TMap<FName,TObjectPtr<USkeletalMeshComponent>>& Parts) {
    for (auto& Pair:Parts) if (IsValid(Pair.Value)) Pair.Value->DestroyComponent();
    Parts.Reset();
}
void UMythicAppearanceRenderer::Apply(const FMythicAppearancePublicState& State,bool bAsFallback) {
    if (!Catalog) Catalog=UMythicAppearanceCatalog::Get();
    if (!Catalog || !Driver() || State.Parts.Num()>MythicAppearanceLimits::Parts) return;
    ++Generation;
    if (!bAsFallback) RequestedState=State;
    if (Load) { Load->CancelHandle(); Load.Reset(); }
    GetWorld()->GetTimerManager().ClearTimer(LoadDeadline);
    DestroyParts(CandidateParts); CandidatePoseRevisions.Reset(); SetComponentTickEnabled(false);
    bFallback=bAsFallback;
    bool bSameParts=RenderedState.Frame==State.Frame && CurrentParts.Num()==State.Parts.Num();
    for (const auto& Part:State.Parts) bSameParts &= CurrentParts.Contains(Part.Part);
    if (bSameParts) { ApplyColors(State,CurrentParts); RenderedState=State; bLoading=false; return; }
    const auto* Frame=Catalog->FindFrame(State.Frame);
    if (!Frame) return;
    TArray<FSoftObjectPath> Dependencies;
    Dependencies.AddUnique(Frame->BodyAnimation.ToSoftObjectPath());
    Dependencies.AddUnique(Frame->GameplayAnimation.ToSoftObjectPath());
    Dependencies.AddUnique(Catalog->CopyPoseAnimation.ToSoftObjectPath());
    for (const auto& Row:State.Parts) {
        const auto* Part=Catalog->FindPart(Row.Part);
        if (!Part || (!Part->Frame.IsNone() && Part->Frame!=Frame->Id) || Row.Colors.Num()>MythicAppearanceLimits::Channels) return;
        Dependencies.AddUnique(Part->Mesh.ToSoftObjectPath());
        for (const auto& M:Part->Materials) if (!M.Material.IsNull()) Dependencies.AddUnique(M.Material.ToSoftObjectPath());
    }
    Dependencies.RemoveAll([](const auto& P){return P.IsNull();});
    bLoading=true;
    const uint32 Expected=Generation;
    const TWeakObjectPtr<UMythicAppearanceRenderer> Weak(this);
    GetWorld()->GetTimerManager().SetTimer(LoadDeadline,this,&ThisClass::ShowFallback,5.0f,false);
    Load=UAssetManager::GetStreamableManager().RequestAsyncLoad(Dependencies,FStreamableDelegate::CreateLambda([Weak,Expected,State]() {
        if (auto* Self=Weak.Get(); Self && Self->Generation==Expected && IsValid(Self->GetOwner())) Self->BuildCandidate(Expected,State);
    }));
}

void UMythicAppearanceRenderer::BuildCandidate(uint32 Expected,FMythicAppearancePublicState State) {
    if (Expected!=Generation || !Catalog || !Driver()) return;
    const auto* Frame=Catalog->FindFrame(State.Frame);
    if (!Frame || !Frame->BodyAnimation.Get() || !Frame->GameplayAnimation.Get() || !Catalog->CopyPoseAnimation.Get()) { ShowFallback(); return; }
    int32 Bones=0, Materials=0;
    const FMythicAppearancePart* HeadDefinition=nullptr;
    for (const auto& Row:State.Parts) {
        const auto* Part=Catalog->FindPart(Row.Part); auto* Mesh=Part?Part->Mesh.Get():nullptr;
        if (!Mesh) { ShowFallback(); return; }
        Bones+=Mesh->GetRefSkeleton().GetNum(); Materials+=Mesh->GetMaterials().Num();
        if (Mesh->GetMaterials().Num()>MythicAppearanceLimits::MaterialSlots) { ShowFallback(); return; }
        if (Part->Region==FName("Head")) HeadDefinition=Part;
    }
    if (!HeadDefinition || Bones>MythicAppearanceLimits::Bones || Materials>MythicAppearanceLimits::Materials) { ShowFallback(); return; }
    auto Make=[&](const FMythicAppearancePart& Part,USkeletalMeshComponent* Parent,UClass* Animation) {
        auto* Component=NewObject<UMythicAppearanceMeshComponent>(GetOwner(),NAME_None,RF_Transient);
        GetOwner()->AddInstanceComponent(Component);
        Component->SetSkeletalMesh(Part.Mesh.Get());
        Component->SetupAttachment(Parent,Part.Socket);
        Component->SetRelativeTransform(FTransform::Identity);
        Component->ComponentTags.Add(BodyTag);
        if (Part.bHideOwnerHead) Component->ComponentTags.Add(HeadTag);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetGenerateOverlapEvents(false); Component->SetCanEverAffectNavigation(false);
        Component->bEnableUpdateRateOptimizations=false;
        Component->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Component->SetHiddenInGame(true,false);
        Component->SetTickGroup(TG_PostPhysics);
        Component->RegisterComponent();
        if (Animation) Component->SetAnimInstanceClass(Animation);
        Component->AddTickPrerequisiteComponent(Driver());
        if (Parent!=Driver()) Component->AddTickPrerequisiteComponent(Parent);
        CandidateParts.Add(Part.Id,Component);
        if (Animation) CandidatePoseRevisions.Add(Part.Id,Component->GetBoneTransformRevisionNumber());
        return Component;
    };
    auto* Pose=Make(*HeadDefinition,Driver(),Frame->BodyAnimation.Get()); Pose->SetForcedLOD(1);
    for (const auto& Row:State.Parts) {
        const auto* Part=Catalog->FindPart(Row.Part);
        if (Part==HeadDefinition) continue;
        Make(*Part,Part->Socket.IsNone()?Pose:Driver(),Part->Socket.IsNone()?Catalog->CopyPoseAnimation.Get():nullptr);
    }
    ApplyColors(State,CandidateParts);
    CandidateState=MoveTemp(State);
    // Promotion after the dependency-ordered animation group prevents a reference-pose flash.
    SetComponentTickEnabled(true);
}

void UMythicAppearanceRenderer::ApplyColors(const FMythicAppearancePublicState& State,const TMap<FName,TObjectPtr<USkeletalMeshComponent>>& Parts) {
    for (const auto& Row:State.Parts) {
        const auto* Found=Parts.Find(Row.Part); const auto* Definition=Catalog->FindPart(Row.Part);
        if (!Found || !Definition || !IsValid(Found->Get())) continue;
        auto* Component=Cast<UMythicAppearanceMeshComponent>(Found->Get());
        if (!Component) continue;
        TMap<int32,UMaterialInstanceDynamic*> MIDs;
        for (const auto& Binding:Definition->Materials) {
            const int32 Slot=Component->GetMaterialIndex(Binding.Slot);
            if (Slot==INDEX_NONE) continue;
            UMaterialInstanceDynamic* MID=MIDs.FindRef(Slot);
            if (!MID) {
                UMaterialInterface* Base=Binding.Material.IsNull()?Component->GetSkeletalMeshAsset()->GetMaterials()[Slot].MaterialInterface.Get():Binding.Material.Get();
                if (!Base) continue;
                MID=Component->GetPaletteMaterial(Slot,Base);
                if (!MID) continue;
                MID->ClearParameterValues(); MIDs.Add(Slot,MID);
            }
            const auto* Color=Row.Colors.FindByPredicate([&](const auto& C){return C.Channel==Binding.Channel;});
            const auto* Palette=Color?Catalog->FindPalette(Color->Palette,Binding.PaletteDomain):nullptr;
            if (!Palette) continue;
            for (const auto& P:Palette->Scalars) MID->SetScalarParameterValue(P.Parameter,P.Value);
            for (const auto& P:Palette->Vectors) MID->SetVectorParameterValue(P.Parameter,P.Value);
        }
    }
}

void UMythicAppearanceRenderer::TickComponent(float Delta,ELevelTick Type,FActorComponentTickFunction* Function) {
    Super::TickComponent(Delta,Type,Function);
    if (CandidateParts.IsEmpty()) { SetComponentTickEnabled(false); return; }
    for (const auto& Pair:CandidatePoseRevisions) {
        const auto* Part=CandidateParts.Find(Pair.Key);
        if (!Part || (*Part)->GetBoneTransformRevisionNumber()==Pair.Value) return;
    }
    CandidatePoseRevisions.Reset();
    GetWorld()->GetTimerManager().ClearTimer(LoadDeadline);
    for (const auto& Part:AuthoredBaseline) if (IsValid(Part)) { Part->SetHiddenInGame(true,false); Part->SetVisibility(false,false); Part->SetComponentTickEnabled(false); }
    DestroyParts(CurrentParts);
    CurrentParts=MoveTemp(CandidateParts); RenderedState=MoveTemp(CandidateState);
    for (auto& Pair:CurrentParts) { Pair.Value->SetHiddenInGame(false,false); Pair.Value->SetVisibility(true,false); }
    if (auto* Character=Cast<AMythicCharacter>(GetOwner())) Character->RefreshModularBodyVisibility();
    bLoading=false; Load.Reset(); SetComponentTickEnabled(false);
}

void UMythicAppearanceRenderer::ShowFallback() {
    GetWorld()->GetTimerManager().ClearTimer(LoadDeadline);
    if (bFallback) {
        ++Generation; if (Load) { Load->CancelHandle(); Load.Reset(); }
        DestroyParts(CandidateParts); DestroyParts(CurrentParts);
        for (const auto& Part:AuthoredBaseline) if (IsValid(Part)) { Part->SetHiddenInGame(false,false); Part->SetVisibility(true,false); Part->SetComponentTickEnabled(true); }
        if (auto* Character=Cast<AMythicCharacter>(GetOwner())) Character->RefreshModularBodyVisibility();
        bLoading=false; SetComponentTickEnabled(false); return;
    }
    FMythicAppearanceRecipe Recipe=Catalog->DefaultRecipe;
    if (Catalog->FindFrame(RequestedState.Frame)) Recipe.Frame=RequestedState.Frame;
    FMythicAppearancePublicState Bare;
    if (Catalog->ResolveOutfit(Recipe,{},Bare)) {
        Bare.Incarnation=RequestedState.Incarnation; Bare.Revision=RequestedState.Revision;
        Apply(Bare,true);
    }
}
void UMythicAppearanceRenderer::GetWorldParts(TArray<USkeletalMeshComponent*>& Out) const {
    for (const auto& Row:RenderedState.Parts) if (const auto* Component=CurrentParts.Find(Row.Part); Component && IsValid(Component->Get())) Out.AddUnique(Component->Get());
}
void UMythicAppearanceRenderer::EndPlay(const EEndPlayReason::Type Reason) {
    ++Generation; if (Load) { Load->CancelHandle(); Load.Reset(); }
    if (GameplayLoad) { GameplayLoad->CancelHandle(); GameplayLoad.Reset(); }
    if (WaitingAnimation.IsValid()) WaitingAnimation->OnMontageEnded.RemoveDynamic(this,&ThisClass::OnGameplayMontageEnded);
    WaitingAnimation.Reset();
    if (GetWorld()) { GetWorld()->GetTimerManager().ClearTimer(LoadDeadline); GetWorld()->GetTimerManager().ClearTimer(GameplayRetry); }
    DestroyParts(CandidateParts); DestroyParts(CurrentParts); AuthoredBaseline.Reset();
    Super::EndPlay(Reason);
}
