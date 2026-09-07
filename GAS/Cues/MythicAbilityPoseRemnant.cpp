#include "MythicAbilityPoseRemnant.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Player/MythicCharacter.h"
#include "TimerManager.h"
#include "World/Entity/MythicEntityPresentationComponent.h"

namespace {
constexpr int32 MaxLeases=16,MaxMeshes=16,MaxLeaseBones=2048,MaxWorldBones=16384,MaxRetired=64;

USkeletalMeshComponent* EvaluatedPoseSource(USkeletalMeshComponent* Visible) {
    if (!IsValid(Visible) || !Visible->IsRegistered() || !Visible->GetSkeletalMeshAsset()) return nullptr;
    auto* Source=Visible;
    if (auto* Leader=Visible->LeaderPoseComponent.Get()) Source=Cast<USkeletalMeshComponent>(Leader);
    if (!IsValid(Source) || !Source->IsRegistered() || !Source->GetSkeletalMeshAsset()) return nullptr;
    const auto& SourceSkeleton=Source->GetSkeletalMeshAsset()->GetRefSkeleton();
    if (Source->GetBoneSpaceTransformsView().Num()!=SourceSkeleton.GetNum() || SourceSkeleton.GetNum()==0) return nullptr;
    const auto& TargetSkeleton=Visible->GetSkeletalMeshAsset()->GetRefSkeleton();
    if (TargetSkeleton.GetNum()>MaxLeaseBones) return nullptr;
    // CopyPose's different-mesh path otherwise fills unmatched bones from reference pose.
    for (int32 Index=0;Index<TargetSkeleton.GetNum();++Index) {
        const int32 SourceIndex=SourceSkeleton.FindBoneIndex(TargetSkeleton.GetBoneName(Index));
        if (SourceIndex==INDEX_NONE) return nullptr;
        const int32 Parent=TargetSkeleton.GetParentIndex(Index),SourceParent=SourceSkeleton.GetParentIndex(SourceIndex);
        if ((Parent==INDEX_NONE)!=(SourceParent==INDEX_NONE)
            || (Parent!=INDEX_NONE && TargetSkeleton.GetBoneName(Parent)!=SourceSkeleton.GetBoneName(SourceParent))) return nullptr;
    }
    return Source;
}
}

bool UMythicAbilityPoseRemnantSubsystem::ShouldCreateSubsystem(UObject* Outer) const {
    const auto* World=Cast<UWorld>(Outer);
    return Super::ShouldCreateSubsystem(Outer) && World && World->IsGameWorld();
}

void UMythicAbilityPoseRemnantSubsystem::Receive(const FMythicAbilityPoseRemnantEvent& Event,bool bCancel) {
    auto* World=GetWorld();
    if (!World || World->GetNetMode()==NM_DedicatedServer || !Event.HasValidKey()) return;
    Expire();
    const double Now=World->GetTimeSeconds();
    if (bCancel) {
        if (const auto* Lease=Leases.Find(Event.ReceiptId); Lease && Lease->Event.SameKey(Event)) Remove(Event.ReceiptId);
        if (!Retired.Contains(Event.ReceiptId) && Retired.Num()<MaxRetired) Retired.Add(Event.ReceiptId,{Event,Now+4});
        ScheduleExpiry(); return;
    }
    if (Leases.Contains(Event.ReceiptId) || Retired.Contains(Event.ReceiptId) || Leases.Num()>=MaxLeases
        || Retired.Num()>=MaxRetired || Event.Departure.ContainsNaN() || !FMath::IsFinite(Event.ServerTime)
        || !FMath::IsFinite(Event.Duration) || Event.Duration<=0 || Event.Duration>3
        || !FMath::IsFinite(Event.CapsuleHalfHeight) || Event.CapsuleHalfHeight<=0 || Event.CapsuleHalfHeight>500
        || !IsValid(Event.Style) || !IsValid(Event.Style->PoseMaterial) || !FMath::IsFinite(Event.Style->Opacity)
        || Event.Style->Opacity<=0 || Event.Style->Opacity>1 || !FMath::IsFinite(Event.Style->Tint.R)
        || !FMath::IsFinite(Event.Style->Tint.G) || !FMath::IsFinite(Event.Style->Tint.B) || !FMath::IsFinite(Event.Style->Tint.A)) return;
    const auto* GameState=World->GetGameState();
    if (!GameState && World->GetNetMode()==NM_Client) return;
    const double ServerNow=GameState ? GameState->GetServerWorldTimeSeconds() : Now;
    if (Event.ServerTime>ServerNow+0.25) return;
    const float Remaining=Event.Duration-static_cast<float>(FMath::Max(0.0,ServerNow-Event.ServerTime));
    if (Remaining<=0.01f) return;
    auto* Character=Cast<AMythicCharacter>(Event.Avatar);
    auto* Presentation=Character ? Character->FindComponentByClass<UMythicEntityPresentationComponent>() : nullptr;
    if (!IsValid(Character) || Character->GetWorld()!=World || !Presentation || Presentation->GetPresentationInstance()!=Event.Instance) return;
    auto* Body=Character->GetMesh();
    if (!EvaluatedPoseSource(Body)) return;
    TArray<USkeletalMeshComponent*> VisibleMeshes; VisibleMeshes.Reserve(MaxMeshes);
    Character->GatherAbilityPoseMeshes(VisibleMeshes);
    if (VisibleMeshes.Num()>MaxMeshes) return;
    int32 RequiredBones=0;
    for (auto* Visible:VisibleMeshes) {
        if (!EvaluatedPoseSource(Visible)) return;
        RequiredBones+=Visible->GetSkeletalMeshAsset()->GetRefSkeleton().GetNum();
    }
    if (RequiredBones>MaxLeaseBones || TotalBones+RequiredBones>MaxWorldBones) return;
    FMythicAbilityPoseRemnantLease Lease;
    Lease.Event=Event; Lease.LocalExpiry=Now+Remaining; Lease.Presentation=Presentation;
    const FTransform Current=Character->GetActorTransform();
    for (auto* Visible:VisibleMeshes) {
        auto* Source=EvaluatedPoseSource(Visible);
        const int32 Bones=Visible->GetSkeletalMeshAsset()->GetRefSkeleton().GetNum();
        auto* Pose=NewObject<UPoseableMeshComponent>(this);
        Pose->SetVisibility(false); Pose->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Pose->SetGenerateOverlapEvents(false); Pose->SetCanEverAffectNavigation(false);
        Pose->SetCastShadow(false); Pose->SetComponentTickEnabled(false);
        Pose->SetSkinnedAssetAndUpdate(Visible->GetSkeletalMeshAsset(),true);
        Pose->SetWorldTransform(Visible->GetComponentTransform().GetRelativeTransform(Current)*Event.Departure);
        Pose->RegisterComponentWithWorld(World);
        Pose->CopyPoseFromSkeletalComponent(Source); Pose->RefreshBoneTransforms();
        auto* Material=UMaterialInstanceDynamic::Create(Event.Style->PoseMaterial,Pose);
        Material->SetVectorParameterValue(TEXT("Tint"),Event.Style->Tint);
        Material->SetScalarParameterValue(TEXT("Opacity"),Event.Style->Opacity);
        Material->SetScalarParameterValue(TEXT("LeaseStartTime"),Now);
        Material->SetScalarParameterValue(TEXT("LeaseDuration"),Remaining);
        for (int32 Index=0;Index<Pose->GetNumMaterials();++Index) Pose->SetMaterial(Index,Material);
        Pose->SetComponentTickEnabled(false); Pose->SetVisibility(true);
        Lease.Components.Add(Pose); Lease.BoneCount+=Bones;
    }
    if (Lease.Components.IsEmpty()) return;
    if (Event.Style->AccentSystem) {
        const FVector Feet=Event.Departure.GetLocation()-Event.Departure.GetUnitAxis(EAxis::Z)*Event.CapsuleHalfHeight;
        auto* Accent=UNiagaraFunctionLibrary::SpawnSystemAtLocation(World,Event.Style->AccentSystem,Feet,
            Event.Departure.Rotator(),FVector::OneVector,false,false,ENCPoolMethod::None,false);
        if (Accent) {
            Accent->SetVariableFloat(TEXT("User.VisibleSeconds"),Remaining);
            Accent->SetVariableLinearColor(TEXT("User.Tint"),Event.Style->Tint);
            Accent->Activate(true); Lease.Components.Add(Accent);
        }
    }
    const FGuid Receipt=Event.ReceiptId;
    Lease.Deactivated=Presentation->OnPresentationDeactivated.AddWeakLambda(this,[this,Receipt](auto&,const auto&){ Remove(Receipt); ScheduleExpiry(); });
    TotalBones+=Lease.BoneCount; Leases.Add(Receipt,MoveTemp(Lease)); ScheduleExpiry();
}

void UMythicAbilityPoseRemnantSubsystem::Remove(FGuid ReceiptId) {
    FMythicAbilityPoseRemnantLease Lease;
    if (!Leases.RemoveAndCopyValue(ReceiptId,Lease)) return;
    TotalBones-=Lease.BoneCount;
    if (auto* Presentation=Lease.Presentation.Get()) Presentation->OnPresentationDeactivated.Remove(Lease.Deactivated);
    for (USceneComponent* Component:Lease.Components) if (IsValid(Component)) Component->DestroyComponent();
    if (Retired.Num()<MaxRetired) Retired.Add(ReceiptId,{Lease.Event,GetWorld()->GetTimeSeconds()+4});
}

void UMythicAbilityPoseRemnantSubsystem::Expire() {
    const double Now=GetWorld()->GetTimeSeconds();
    TArray<FGuid,TInlineAllocator<MaxLeases>> Expired;
    for (const auto& Pair:Leases) if (Pair.Value.LocalExpiry<=Now || !Pair.Value.Presentation.IsValid()
        || Pair.Value.Presentation->GetPresentationInstance()!=Pair.Value.Event.Instance) Expired.Add(Pair.Key);
    for (const auto& Id:Expired) Remove(Id);
    for (auto It=Retired.CreateIterator();It;++It) if (It->Value.LocalExpiry<=Now) It.RemoveCurrent();
    ScheduleExpiry();
}

void UMythicAbilityPoseRemnantSubsystem::ScheduleExpiry() {
    if (!GetWorld()) return;
    auto& Timers=GetWorld()->GetTimerManager(); Timers.ClearTimer(ExpiryTimer);
    if (Leases.IsEmpty() && Retired.IsEmpty()) return;
    double Next=MAX_dbl;
    for (const auto& Pair:Leases) Next=FMath::Min(Next,Pair.Value.LocalExpiry);
    for (const auto& Pair:Retired) Next=FMath::Min(Next,Pair.Value.LocalExpiry);
    Timers.SetTimer(ExpiryTimer,this,&ThisClass::Expire,FMath::Max(0.01,Next-GetWorld()->GetTimeSeconds()),false);
}

void UMythicAbilityPoseRemnantSubsystem::Deinitialize() {
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(ExpiryTimer);
    TArray<FGuid> Ids; Leases.GenerateKeyArray(Ids);
    for (const auto& Id:Ids) Remove(Id);
    Retired.Reset(); Super::Deinitialize();
}
