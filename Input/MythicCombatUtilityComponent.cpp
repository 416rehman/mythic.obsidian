#include "Input/MythicCombatUtilityComponent.h"
#include "Input/MythicControlsSettings.h"
#include "Player/MythicAbilityIntentComponent.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicSessionPartySubsystem.h"
#include "Interaction/Attention/MythicEntityAttentionSubsystem.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"
#include "World/Harvesting/MythicHarvestClaimMembershipSubsystem.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "AI/Party/PartySubsystem.h"
#include "AIController.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "MassEntityView.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "SceneView.h"
#include "CoreGlobals.h"
#include "Core/CameraOperation.h"
#include "Core/CameraRigAsset.h"
#include "Core/CameraSystemEvaluator.h"
#include "Core/RootCameraNode.h"
#include "Core/CameraRigEvaluationInfo.h"
#include "Core/CameraEvaluationContext.h"
#include "Core/CameraNodeEvaluatorHierarchy.h"
#include "GameFramework/IGameplayCameraSystemHost.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Widgets/SViewport.h"
#include "UI/Settings/MythicUserSettings.h"

namespace {
bool ValidPolicy(const UMythicCombatUtilityComponent& C) {
    return FMath::IsFinite(C.LockAcquireRange) && C.LockAcquireRange>=100 && C.LockAcquireRange<=3000
        && FMath::IsFinite(C.LockMaintainRange) && C.LockMaintainRange>=C.LockAcquireRange && C.LockMaintainRange<=4000
        && FMath::IsFinite(C.LockAcquireDegrees) && C.LockAcquireDegrees>0 && C.LockAcquireDegrees<=30
        && FMath::IsFinite(C.LockCursorRadius) && C.LockCursorRadius>0 && C.LockCursorRadius<=500
        && FMath::IsFinite(C.LockLineOfSightGrace) && C.LockLineOfSightGrace>=0 && C.LockLineOfSightGrace<=1
        && FMath::IsFinite(C.PingRange) && C.PingRange>=100 && C.PingRange<=10000
        && FMath::IsFinite(C.PingLifetime) && C.PingLifetime>=1 && C.PingLifetime<=8;
}
bool IsLocalOnScreen(const AMythicPlayerController* PC,const FVector& Point) {
    const auto* LP=PC ? PC->GetLocalPlayer() : nullptr;
    if (!LP || !LP->ViewportClient || !LP->ViewportClient->Viewport) return false;
    FSceneViewProjectionData Projection;
    if (!LP->GetProjectionData(LP->ViewportClient->Viewport,Projection)) return false;
    FVector2D Pixel;
    if (!PC->ProjectWorldLocationToScreen(Point,Pixel,false) || Pixel.ContainsNaN()) return false;
    const auto Rect=Projection.GetConstrainedViewRect();
    return Pixel.X>=Rect.Min.X && Pixel.X<Rect.Max.X && Pixel.Y>=Rect.Min.Y && Pixel.Y<Rect.Max.Y;
}
}

UMythicCombatUtilityComponent::UMythicCombatUtilityComponent() {
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bStartWithTickEnabled=false;
    PrimaryComponentTick.TickInterval=0.1f;
}
AMythicPlayerController* UMythicCombatUtilityComponent::Controller() const { return Cast<AMythicPlayerController>(GetOwner()); }
double UMythicCombatUtilityComponent::ServerTime() const {
    const auto* World=GetWorld();
    const auto* State=World ? World->GetGameState() : nullptr;
    return State ? State->GetServerWorldTimeSeconds() : World ? World->GetTimeSeconds() : 0;
}
void UMythicCombatUtilityComponent::BeginPlay() {
    Super::BeginPlay();
    if (!Controller() || !GetWorld()) return;
    const auto* Policy=GetDefault<UMythicControlsSettings>();
    LockAcquireRange=Policy->LockRange; LockMaintainRange=Policy->LockBreakRange; LockAcquireDegrees=Policy->LockAngleDegrees;
    LockCursorRadius=Policy->LockCursorRadius; LockLineOfSightGrace=Policy->LockOcclusionGrace;
    PingRange=Policy->PingRange; PingLifetime=Policy->PingLifetime;
    SenderToken=FGuid::NewGuid(); LastPingTokenTime=ServerTime();
    BoundRegistry=GetWorld()->GetSubsystem<UMythicEntityPresentationRegistry>();
    if (BoundRegistry.IsValid()) RegistryHandle=BoundRegistry->OnPresentationUnregistered.AddUObject(this,&ThisClass::HandleUnregistered);
    if (Controller()->HasAuthority()) {
        BoundMembership=GetWorld()->GetSubsystem<UMythicHarvestClaimMembershipSubsystem>();
        if (BoundMembership.IsValid()) MembershipHandle=BoundMembership->OnMembershipChanged.AddUObject(this,&ThisClass::HandleMembershipChanged);
    }
    if (Controller()->IsLocalController()) OwnerHandle=Controller()->OnLocalGameplayOwnerChanged.AddUObject(this,&ThisClass::ResetLocalUtilityState);
}
void UMythicCombatUtilityComponent::EndPlay(const EEndPlayReason::Type Reason) {
    bStopping=true;
    if (auto* PC=Controller()) {
        if (PC->HasAuthority()) AuthorityEndUtilitySession();
        PC->OnLocalGameplayOwnerChanged.Remove(OwnerHandle);
    }
    if (BoundRegistry.IsValid()) BoundRegistry->OnPresentationUnregistered.Remove(RegistryHandle);
    if (BoundMembership.IsValid()) BoundMembership->OnMembershipChanged.Remove(MembershipHandle);
    ApplyLocalLock({}); LocalPings.Reset(); Recipients.Reset();
    OnUtilityProjectionChanged.Broadcast(); // Clears passive HUD projection before component teardown.
    Super::EndPlay(Reason);
}
bool UMythicCombatUtilityComponent::HasLiveOwner() const {
    const auto* PC=Controller(); const APawn* Pawn=PC ? PC->GetPawn() : nullptr;
    const auto* Life=UMythicLifeComponent::FindHealthComponent(Pawn);
    return PC && IsValid(Pawn) && !Pawn->IsActorBeingDestroyed() && Pawn->GetWorld()==GetWorld()
        && Life && Life->IsInitialized() && !Life->IsDead() && !Life->IsDowned()
        && FMath::IsFinite(Life->GetHealth()) && Life->GetHealth()>0;
}
UMythicEntityPresentationComponent* UMythicCombatUtilityComponent::ResolvePublic(const FMythicEntityPresentationInstance& Instance) const {
    auto* Registry=GetWorld() ? GetWorld()->GetSubsystem<UMythicEntityPresentationRegistry>() : nullptr;
    auto* Component=Registry ? Registry->ResolvePresentationComponent(Instance) : nullptr;
    auto* Actor=Component ? Component->GetOwner() : nullptr;
    return IsValid(Component) && Component->RepresentsInstance(Instance) && IsValid(Actor) && !Actor->IsActorBeingDestroyed()
        && Actor->GetWorld()==GetWorld() && !Actor->IsHidden() ? Component : nullptr;
}
bool UMythicCombatUtilityComponent::ResolveAuthoritySubject(const FMythicEntityPresentationInstance& Instance,AActor*& Actor) const {
    Actor=nullptr;
    if (!Controller() || !Controller()->HasAuthority()) return false;
    auto* Component=ResolvePublic(Instance); if (!Component) return false;
    auto* Registry=GetWorld()->GetSubsystem<UMythicEntityPresentationRegistry>();
    FMythicEntityId Identity;
    if (!Registry || !Registry->ResolveAuthorityEntity(Instance,Identity) || Identity!=Component->GetAuthorityEntityId()) return false;
    auto* Candidate=Component->GetOwner();
    if (Identity.GetDomain()==EMythicEntityDomain::LivingWorld) {
        auto* NPC=Cast<AMythicNPCCharacter>(Candidate);
        auto* Brain=NPC ? NPC->FindComponentByClass<UMythicCognitiveBrainComponent>() : nullptr;
        auto* Mass=GetWorld()->GetSubsystem<UMassEntitySubsystem>();
        auto* GI=GetWorld()->GetGameInstance();
        auto* Living=GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
        if (!NPC || !Brain || !Mass || !Living) return false;
        const auto Entity=Brain->GetSourceEntity(); auto& Manager=Mass->GetMutableEntityManager();
        if (!Entity.IsSet() || !Manager.IsEntityValid(Entity) || Living->FindEmbodiedActor(Entity)!=NPC) return false;
        const auto* Fragment=Manager.GetFragmentDataPtr<FMythicIdentityFragment>(Entity);
        const FMassEntityView View(Manager,Entity);
        if (!Fragment || Fragment->EntityId!=Identity || !View.HasTag<FMythicHydratedTag>() || !View.HasTag<FMythicCognitiveTag>()) return false;
    }
    Actor=Candidate; return true;
}
bool UMythicCombatUtilityComponent::HasSight(const AMythicPlayerController* Viewer,AActor* Subject,const FVector& Point,bool bRequireLocalView) const {
    APawn* Pawn=Viewer ? Viewer->GetPawn() : nullptr;
    if (!Pawn || !GetWorld() || Point.ContainsNaN() || Viewer->GetWorld()!=GetWorld()) return false;
    FVector Eye; FRotator Rotation; Pawn->GetActorEyesViewPoint(Eye,Rotation);
    if (Eye.ContainsNaN()) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(MythicUtilitySight),false,Pawn); FHitResult Hit;
    const bool bBlocked=GetWorld()->LineTraceSingleByChannel(Hit,Eye,Point,ECC_Visibility,Params);
    if (bBlocked && Hit.GetActor()!=Subject && FVector::DistSquared(Hit.ImpactPoint,Point)>FMath::Square(25.0f)) return false;
    if (bRequireLocalView && Viewer->IsLocalController()) {
        FVector Camera; FRotator View; Viewer->GetPlayerViewPoint(Camera,View);
        if (Camera.ContainsNaN() || !IsLocalOnScreen(Viewer,Point)) return false;
        if (GetWorld()->LineTraceSingleByChannel(Hit,Camera,Point,ECC_Visibility,Params)
            && Hit.GetActor()!=Subject && FVector::DistSquared(Hit.ImpactPoint,Point)>FMath::Square(25.0f)) return false;
    }
    return true;
}
bool UMythicCombatUtilityComponent::IsAuthorityHostile(AActor* Target) const {
    const auto* PC=Controller(); APawn* Source=PC ? PC->GetPawn() : nullptr;
    if (!PC || !PC->HasAuthority() || !Source || Target==Source) return false;
    auto* ASC=UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
    auto* Life=UMythicLifeComponent::FindHealthComponent(Target);
    const auto* Vitals=ASC ? ASC->GetSet<UMythicAttributeSet_Life>() : nullptr;
    if (!ASC || ASC->GetAvatarActor()!=Target || !Life || !Life->IsInitialized() || Life->IsDead() || Life->IsDowned()
        || !Vitals || !FMath::IsFinite(Vitals->GetHealth()) || Vitals->GetHealth()<=0) return false;
    const auto* OwnerPS=PC->GetPlayerState<AMythicPlayerState>();
    if (const auto* TargetPawn=Cast<APawn>(Target)) {
        if (const auto* OtherPS=TargetPawn->GetPlayerState<AMythicPlayerState>()) {
            FGuid A,B;
            const auto* Members=GetWorld()->GetSubsystem<UMythicHarvestClaimMembershipSubsystem>();
            if (OwnerPS && Members && Members->TryResolveParty(OwnerPS->GetCanonicalPlayerKey(),A)
                && Members->TryResolveParty(OtherPS->GetCanonicalPlayerKey(),B) && A==B) return false;
            return GetDefault<UMythicDeveloperSettings>()->bFriendlyFireEnabled;
        }
        if (OwnerPS) if (const auto* Parties=GetWorld()->GetSubsystem<UMythicPartySubsystem>()) {
            TArray<FMythicPartyMember> Companions; Parties->GetPartyMembers(OwnerPS->GetCanonicalPlayerKey(),Companions);
            for (const auto& Member:Companions) if (Member.NPCActor.Get()==Target) return false;
        }
        const auto* AI=Cast<AAIController>(TargetPawn->GetController());
        return AI && AI->GetTeamAttitudeTowards(*Source)==ETeamAttitude::Hostile;
    }
    return false;
}
bool UMythicCombatUtilityComponent::CaptureAim(float Range,FVector& Eye,FVector& Direction,bool& bWorldCursor,FVector& Point) const {
    Eye=Direction=Point=FVector::ZeroVector; bWorldCursor=false;
    auto* PC=Controller(); auto* Intent=PC ? PC->AbilityIntentComponent.Get() : nullptr;
    if (!PC || !PC->IsLocalController() || PC->IsGameplayAbilityInputBlocked() || !HasLiveOwner() || !Intent) return false;
    FVector2D Pixel; bool Hardware=false;
    if (!Intent->GetReticleViewportPixel(Pixel,Hardware)) return false;
    FVector Camera,Ray;
    if (!PC->DeprojectScreenPositionToWorld(Pixel.X,Pixel.Y,Camera,Ray) || Camera.ContainsNaN() || Ray.ContainsNaN()) return false;
    FRotator Rotation; PC->GetPawn()->GetActorEyesViewPoint(Eye,Rotation);
    Ray=Ray.GetSafeNormal(); if (Ray.IsNearlyZero() || Eye.ContainsNaN() || FVector::DistSquared(Camera,Eye)>FMath::Square(10000.0f)) return false;
    const FVector End=Camera+Ray*FMath::Min(20000.0,Range+FVector::Distance(Camera,Eye));
    FCollisionQueryParams Params(SCENE_QUERY_STAT(MythicUtilityAim),false,PC->GetPawn()); FHitResult Hit;
    Point=GetWorld()->LineTraceSingleByChannel(Hit,Camera,End,ECC_Visibility,Params) ? Hit.ImpactPoint : End;
    const FVector Delta=Point-Eye;
    if (Point.ContainsNaN() || Delta.IsNearlyZero() || FVector::DotProduct(Delta,Ray)<=0) return false;
    Direction=Delta.GetSafeNormal();
    bWorldCursor=Intent->GetSelectedCameraMode().AbilityAimPolicy==EMythicAbilityAimPolicy::WorldCursor;
    return true;
}
bool UMythicCombatUtilityComponent::FitsAim(AActor* Actor,const FVector& Eye,const FVector& Direction,bool bWorldCursor,const FVector& Point,double& Score) const {
    Score=DBL_MAX;
    if (!Actor) return false;
    const FVector Target=Actor->GetTargetLocation(Controller()->GetPawn());
    if (Target.ContainsNaN()) return false;
    if (bWorldCursor) {
        // Raw pixels from the same local viewport: this is not a world-centimeter radius.
        // Authority nominations are validated separately below, without pretending to have a client viewport.
        const auto* PC=Controller(); auto* Intent=PC ? PC->AbilityIntentComponent.Get() : nullptr;
        FVector2D Cursor,Projected; bool Hardware=false;
        if (!PC || !PC->IsLocalController() || !Intent || !Intent->GetReticleViewportPixel(Cursor,Hardware)
            || !IsLocalOnScreen(PC,Target) || !PC->ProjectWorldLocationToScreen(Target,Projected,false)
            || Cursor.ContainsNaN() || Projected.ContainsNaN()) return false;
        Score=FVector2D::DistSquared(Projected,Cursor);
        return Score<=FMath::Square(LockCursorRadius);
    }
    const FVector Delta=Target-Eye;
    const double Dot=FVector::DotProduct(Delta.GetSafeNormal(),Direction);
    Score=1.0-Dot;
    return !Delta.IsNearlyZero() && Dot>=FMath::Cos(FMath::DegreesToRadians(LockAcquireDegrees));
}
bool UMythicCombatUtilityComponent::ToggleTargetLock() {
    auto* PC=Controller();
    if (!PC || !PC->IsLocalController() || PC->IsGameplayAbilityInputBlocked() || !ValidPolicy(*this) || LocalLockRequest==MAX_uint32) return false;
    if (LocalLock.IsValid() || bLockRequestPending) { ClearTargetLock(); return true; }
    FVector Eye,Direction,Point; bool WorldCursor=false;
    if (!CaptureAim(LockAcquireRange,Eye,Direction,WorldCursor,Point)) return false;
    struct FLocalCandidate { FMythicEntityPresentationInstance Instance; TWeakObjectPtr<AActor> Actor; double Score; double Distance; FString Tie; };
    TArray<FLocalCandidate> Ranked; TSet<AActor*> Seen; TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(MythicLocalLockCandidates),false,PC->GetPawn());
    GetWorld()->OverlapMultiByObjectType(Overlaps,PC->GetPawn()->GetActorLocation(),FQuat::Identity,FCollisionObjectQueryParams(ECC_Pawn),FCollisionShape::MakeSphere(LockAcquireRange),Params);
    const auto Better=[](const FLocalCandidate& A,const FLocalCandidate& B){ return A.Score!=B.Score ? A.Score<B.Score : A.Distance!=B.Distance ? A.Distance<B.Distance : A.Tie<B.Tie; };
    for (const auto& Hit:Overlaps) {
        auto* Actor=Hit.GetActor(); if (!IsValid(Actor) || Actor==PC->GetPawn() || Seen.Contains(Actor)) continue; Seen.Add(Actor);
        auto* Presentation=Actor->FindComponentByClass<UMythicEntityPresentationComponent>();
        const auto Instance=Presentation ? Presentation->GetPresentationInstance() : FMythicEntityPresentationInstance();
        double Score; const double Distance=FVector::DistSquared(PC->GetPawn()->GetActorLocation(),Actor->GetActorLocation());
        if (!Instance.IsValid() || ResolvePublic(Instance)!=Presentation || Distance>FMath::Square(LockAcquireRange)
            || !IsLocalOnScreen(PC,Actor->GetTargetLocation(PC->GetPawn())) || !FitsAim(Actor,Eye,Direction,WorldCursor,Point,Score)) continue;
        Ranked.Add({Instance,Actor,Score,Distance,Instance.ToDebugString()}); Ranked.Sort(Better);
        if (Ranked.Num()>32) Ranked.Pop(EAllowShrinking::No);
    }
    TArray<FMythicEntityPresentationInstance> Visible;
    for (const auto& Candidate:Ranked) if (auto* Actor=Candidate.Actor.Get(); Actor && HasSight(PC,Actor,Actor->GetTargetLocation(PC->GetPawn()))) Visible.Add(Candidate.Instance);
    if (Visible.IsEmpty()) return false;
    ++LocalLockRequest; bLockRequestPending=true;
    ServerRequestLock(LocalLockRequest,true,Direction,WorldCursor,Point,Visible); return true;
}
void UMythicCombatUtilityComponent::ClearTargetLock() {
    auto* PC=Controller(); if (!PC || !PC->IsLocalController()) return;
    ApplyLocalLock({}); bLockRequestPending=false;
    if (!bStopping && LocalLockRequest!=MAX_uint32) ServerRequestLock(++LocalLockRequest,false,FVector::ForwardVector,false,FVector::ZeroVector,{});
}
void UMythicCombatUtilityComponent::ResetLocalUtilityState() { ClearTargetLock(); }
void UMythicCombatUtilityComponent::ServerRequestLock_Implementation(uint32 RequestId,bool bAcquire,FVector_NetQuantizeNormal Direction,bool bWorldCursor,FVector_NetQuantize Point,const TArray<FMythicEntityPresentationInstance>& VisibleCandidates) {
    auto* PC=Controller();
    if (!PC || !PC->HasAuthority() || RequestId==0 || RequestId<=AuthorityLockRequest) return;
    AuthorityLockRequest=RequestId; AuthorityLock.Reset();
    if (!bAcquire || !ValidPolicy(*this) || !HasLiveOwner() || !IsRecipientAllowed(PC)
        || VisibleCandidates.IsEmpty() || VisibleCandidates.Num()>32) { ClientSetLock(RequestId,{}); RefreshTick(); return; }
    const double Now=ServerTime();
    if (Now-LastLockRequestTime<0.15 || Direction.ContainsNaN() || Point.ContainsNaN() || !FMath::IsNearlyEqual(Direction.SizeSquared(),1.0,0.03)) {
        ClientSetLock(RequestId,{}); RefreshTick(); return;
    }
    LastLockRequestTime=Now;
    FVector Eye; FRotator Rotation; PC->GetPawn()->GetActorEyesViewPoint(Eye,Rotation);
    const FVector ClaimedDelta=FVector(Point)-Eye;
    // CaptureAim bounds camera-to-eye by 10000 and its ray by 20000. Only nomination uses this
    // finite envelope; the target still needs the much smaller acquisition range and real LOS.
    if (Eye.ContainsNaN() || ClaimedDelta.IsNearlyZero() || ClaimedDelta.SizeSquared()>FMath::Square(30000.0)
        || FVector::DotProduct(ClaimedDelta.GetSafeNormal(),FVector(Direction))<0.99) {
        ClientSetLock(RequestId,{}); RefreshTick(); return;
    }
    struct FCandidate { FMythicEntityPresentationInstance Instance; TWeakObjectPtr<AActor> Actor; double Score; double Distance; FString Tie; };
    TArray<FCandidate> Candidates; TSet<FMythicEntityPresentationInstance> Seen;
    const auto Better=[](const FCandidate& A,const FCandidate& B){ return A.Score!=B.Score ? A.Score<B.Score : A.Distance!=B.Distance ? A.Distance<B.Distance : A.Tie<B.Tie; };
    for (const auto& Instance:VisibleCandidates) {
        if (Seen.Contains(Instance)) continue; Seen.Add(Instance);
        AActor* Actor=nullptr; double Score;
        if (!ResolveAuthoritySubject(Instance,Actor) || !IsAuthorityHostile(Actor)) continue;
        // Preserve the local pixel-ranked nomination order in WorldCursor mode. It is presentation assistance,
        // not hit authority: server range/hostility/incarnation/LOS remain mandatory and casts revalidate normally.
        if (bWorldCursor) Score=static_cast<double>(Seen.Num());
        else if (!FitsAim(Actor,Eye,Direction,false,Point,Score)) continue;
        const double Distance=FVector::DistSquared(PC->GetPawn()->GetActorLocation(),Actor->GetActorLocation());
        if (Distance>FMath::Square(LockAcquireRange)) continue;
        FCandidate Candidate{Instance,Actor,Score,Distance,Instance.ToDebugString()};
        Candidates.Add(MoveTemp(Candidate)); Candidates.Sort(Better);
        if (Candidates.Num()>32) Candidates.Pop(EAllowShrinking::No);
    }
    // Cheap candidate ordering precedes the bounded LOS budget, so the far on-reticle candidate wins over a near off-reticle one.
    for (int32 Index=0;Index<FMath::Min(Candidates.Num(),16);++Index) {
        const auto& Candidate=Candidates[Index]; auto* Actor=Candidate.Actor.Get();
        if (Actor && HasSight(PC,Actor,Actor->GetTargetLocation(PC->GetPawn()),false)) { AuthorityLock=Candidate.Instance; break; }
    }
    AuthorityAvatar=PC->GetPawn();
    const auto* Presentation=PC->GetPawn()->FindComponentByClass<UMythicEntityPresentationComponent>();
    AuthorityAvatarInstance=Presentation ? Presentation->GetPresentationInstance() : FMythicEntityPresentationInstance();
    LockLastVisible=Now; ClientSetLock(RequestId,AuthorityLock); RefreshTick();
}
void UMythicCombatUtilityComponent::ClientSetLock_Implementation(uint32 RequestId,FMythicEntityPresentationInstance Instance) {
    if (RequestId!=LocalLockRequest || bStopping) return;
    bLockRequestPending=false;
    // A reliable owner RPC can beat actor replication. Retain the opaque answer without rendering or assisting
    // until the exact local registration resolves; authority owns the maintain/expiry decision.
    ApplyLocalLock(Instance);
}
void UMythicCombatUtilityComponent::ApplyLocalLock(FMythicEntityPresentationInstance Instance) {
    const bool Changed=LocalLock!=Instance; LocalLock=Instance;
    if (Changed) SuppressLockYawAssist();
    UpdateLocalLock(); RefreshTick();
    if (Changed) OnUtilityProjectionChanged.Broadcast();
}
void UMythicCombatUtilityComponent::UpdateLocalLock() {
    auto* PC=Controller(); auto* Presentation=ResolvePublic(LocalLock);
    const bool Visible=PC && Presentation && HasLiveOwner() && HasSight(PC,Presentation->GetOwner(),Presentation->GetOwner()->GetTargetLocation(PC->GetPawn()));
    if (const auto* LP=PC ? PC->GetLocalPlayer() : nullptr) if (auto* Attention=LP->GetSubsystem<UMythicEntityAttentionSubsystem>()) {
        if (Visible) { Attention->SetHardTarget(LocalLock); PublishedAttentionLock=LocalLock; }
        else if (PublishedAttentionLock.IsValid()) { Attention->ClearHardTargetIfMatches(PublishedAttentionLock); PublishedAttentionLock.Reset(); }
    }
    if (bLocalLockVisible!=Visible) { bLocalLockVisible=Visible; OnUtilityProjectionChanged.Broadcast(); }
}
FMythicEntityPresentationInstance UMythicCombatUtilityComponent::GetVisibleLockedInstance() const {
    // Projection consumes the bounded 10Hz visibility sample. Gameplay nomination below always retraces.
    return bLocalLockVisible && ResolvePublic(LocalLock) && HasLiveOwner() ? LocalLock : FMythicEntityPresentationInstance();
}
bool UMythicCombatUtilityComponent::TryGetLockedAimDirection(float Range,FVector& Direction) {
    Direction=FVector::ZeroVector;
    if (!ValidPolicy(*this) || !FMath::IsFinite(Range) || Range<=0 || !GetVisibleLockedInstance().IsValid()) return false;
    auto* Presentation=ResolvePublic(LocalLock); auto* PC=Controller();
    FVector Eye,Aim,Point; bool WorldCursor=false; double Score;
    if (!Presentation || !PC || !HasSight(PC,Presentation->GetOwner(),Presentation->GetOwner()->GetTargetLocation(PC->GetPawn()))
        || !CaptureAim(Range,Eye,Aim,WorldCursor,Point)
        || FVector::DistSquared(PC->GetPawn()->GetActorLocation(),Presentation->GetOwner()->GetActorLocation())>FMath::Square(Range)
        || !FitsAim(Presentation->GetOwner(),Eye,Aim,WorldCursor,Point,Score)) return false;
    Direction=(Presentation->GetOwner()->GetTargetLocation(PC->GetPawn())-Eye).GetSafeNormal();
    return !Direction.IsNearlyZero();
}
void UMythicCombatUtilityComponent::SuppressLockYawAssist() {
    LockYawSuppressedUntil=FPlatformTime::Seconds()+0.5;
}
void UMythicCombatUtilityComponent::NotifyManualLook(FVector2D Value) {
    if (!Value.ContainsNaN() && !Value.IsNearlyZero()) SuppressLockYawAssist();
}
void UMythicCombatUtilityComponent::AdvanceLockYawAssist(float DeltaSeconds) {
    auto* PC=Controller();
    const auto* Settings=UMythicUserSettings::Get();
    auto* Intent=PC ? PC->AbilityIntentComponent.Get() : nullptr;
    if (bStopping || !PC || !PC->IsLocalController() || !Intent || !Settings || !Settings->GetLockYawAssist()
        || PC->IsGameplayAbilityInputBlocked() || PC->IsLookInputIgnored() || !HasLiveOwner()) {
        SuppressLockYawAssist(); return;
    }
    const FMythicCameraMode Mode=Intent->GetSelectedCameraMode();
    if (LastLockYawRig.Get()!=Mode.CameraRig.Get()) { LastLockYawRig=Mode.CameraRig; SuppressLockYawAssist(); }
    if (!Mode.CameraRig || !Mode.bAllowLockYawAssist || Mode.AbilityAimPolicy!=EMythicAbilityAimPolicy::ViewCenter
        || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds<=0 || LastLockYawFrame==GFrameCounter) return;
    const auto* LP=PC->GetLocalPlayer();
    const auto User=LP ? LP->GetSlateUser() : nullptr;
    const auto Viewport=LP && LP->ViewportClient ? LP->ViewportClient->GetGameViewportWidget() : nullptr;
    if (!FSlateApplication::IsInitialized() || !User || !Viewport
        || (!Viewport->HasUserFocus(User->GetUserIndex()).IsSet() && !Viewport->HasUserFocusedDescendants(User->GetUserIndex()))) {
        SuppressLockYawAssist(); return;
    }
    if (FPlatformTime::Seconds()<LockYawSuppressedUntil) return;
    // Marker grace is insufficient. Reuse fresh canonical local nomination gates, with the acquisition range.
    FVector AdmissibleDirection;
    if (!TryGetLockedAimDirection(LockAcquireRange,AdmissibleDirection)) return;
    auto* Presentation=ResolvePublic(LocalLock);
    if (!Presentation) return;
    const auto& Vitality=Presentation->GetPublicVitalitySnapshot();
    if (!Vitality.IsCurrentFor(LocalLock) || Vitality.HealthFractionQuantized==0) return;
    const FVector Target=Presentation->GetOwner()->GetTargetLocation(PC->GetPawn());
    if (Target.ContainsNaN()) return;

    using namespace UE::Cameras;
    auto* Host=IGameplayCameraSystemHost::FindActiveHost(PC);
    const auto Evaluator=Host ? Host->GetCameraSystemEvaluator() : nullptr;
    if (!Evaluator || !Evaluator->GetRootNodeEvaluator()) return;
    FCameraRigEvaluationInfo Info;
    Evaluator->GetRootNodeEvaluator()->GetActiveCameraRigInfo(Info);
    if (!Info.RootEvaluator || !Info.EvaluationContext || !Info.EvaluationContext->IsActive()
        || Info.EvaluationContext->GetPlayerController()!=PC || PC->GetWorld()!=GetWorld()
        || Info.CameraRig.Get()!=Mode.CameraRig.Get()) return;
    const auto& Evaluated=Evaluator->GetEvaluatedResult();
    if (!Evaluated.bIsValid) return;
    const FVector Camera=Evaluated.CameraPose.GetLocation();
    const FRotator Rotation=Evaluated.CameraPose.GetRotation();
    FVector ToTarget=Target-Camera; ToTarget.Z=0;
    if (Camera.ContainsNaN() || Rotation.ContainsNaN() || ToTarget.IsNearlyZero()) return;
    const double Error=FMath::FindDeltaAngleDegrees(Rotation.Yaw,ToTarget.Rotation().Yaw);
    // At most 90 degrees/second, and no abrupt catch-up after a stalled frame.
    const double MaximumStep=90.0*FMath::Min(static_cast<double>(DeltaSeconds),0.1);
    const double Step=FMath::Clamp(Error,-MaximumStep,MaximumStep);
    if (!FMath::IsFinite(Step) || FMath::IsNearlyZero(Step,0.001)) return;
    FCameraOperationParams Params; Params.Evaluator=Evaluator.Get(); Params.EvaluationContext=Info.EvaluationContext;
    FYawPitchCameraOperation Operation; Operation.Yaw=FConsumableDouble::Delta(Step);
    // Operate on this exact active rig only. A global Main-layer operation could spill into blending rigs.
    FCameraNodeEvaluatorHierarchy Hierarchy(Info.RootEvaluator);
    LastLockYawFrame=GFrameCounter;
    Hierarchy.CallExecuteOperation(Params,Operation);
}

bool UMythicCombatUtilityComponent::TryCapturePingIntent(FMythicEntityPresentationInstance& Subject,FVector& Point) {
    Subject.Reset(); Point=FVector::ZeroVector;
    FVector Eye,Direction; bool WorldCursor=false;
    if (!ValidPolicy(*this) || !CaptureAim(PingRange,Eye,Direction,WorldCursor,Point)) return false;
    auto* PC=Controller();
    const FVector Delta=Point-Eye;
    if (Delta.SizeSquared()>FMath::Square(PingRange)) Point=Eye+Delta.GetSafeNormal()*PingRange;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(MythicPingSubject),false,PC->GetPawn()); FHitResult Hit;
    if (GetWorld()->LineTraceSingleByChannel(Hit,Eye,Point+Direction*25,ECC_Visibility,Params)) {
        Point=Hit.ImpactPoint;
        if (auto* Actor=Hit.GetActor()) if (auto* Presentation=Actor->FindComponentByClass<UMythicEntityPresentationComponent>()) {
            const auto Instance=Presentation->GetPresentationInstance(); if (ResolvePublic(Instance)==Presentation) Subject=Instance;
        }
    }
    return HasSight(PC,nullptr,Point) && FVector::DistSquared(PC->GetPawn()->GetActorLocation(),Point)<=FMath::Square(PingRange);
}
bool UMythicCombatUtilityComponent::SubmitPing(EMythicPingIntent Intent,FMythicEntityPresentationInstance Subject,FVector Point) {
    auto* PC=Controller();
    if (!PC || !PC->IsLocalController() || PC->IsGameplayAbilityInputBlocked() || !HasLiveOwner() || !ValidPolicy(*this)
        || Point.ContainsNaN() || static_cast<uint8>(Intent)>static_cast<uint8>(EMythicPingIntent::Acknowledge)) return false;
    ServerSubmitPing(Intent,Subject,Point); return true;
}
bool UMythicCombatUtilityComponent::IsRecipientAllowed(AMythicPlayerController* Recipient) const {
    auto* PC=Controller(); auto* Session=GetWorld()->GetSubsystem<UMythicSessionPartySubsystem>();
    if (!PC || !Session || !Session->IsReadyController(PC) || !Session->IsReadyController(Recipient)) return false;
    if (Recipient==PC) return true;
    const auto* SourcePS=PC->GetPlayerState<AMythicPlayerState>(); const auto* TargetPS=Recipient->GetPlayerState<AMythicPlayerState>();
    FGuid SourceParty,TargetParty;
    const auto* Membership=GetWorld()->GetSubsystem<UMythicHarvestClaimMembershipSubsystem>();
    return SourcePS && TargetPS && Membership && Membership->TryResolveParty(SourcePS->GetCanonicalPlayerKey(),SourceParty)
        && Membership->TryResolveParty(TargetPS->GetCanonicalPlayerKey(),TargetParty) && SourceParty==TargetParty;
}
void UMythicCombatUtilityComponent::ServerSubmitPing_Implementation(EMythicPingIntent Intent,FMythicEntityPresentationInstance Subject,FVector_NetQuantize Point) {
    auto* PC=Controller();
    if (!PC || !PC->HasAuthority() || !HasLiveOwner() || !ValidPolicy(*this) || bStopping || bSubmittingPing || Point.ContainsNaN()
        || static_cast<uint8>(Intent)>static_cast<uint8>(EMythicPingIntent::Acknowledge) || !IsRecipientAllowed(PC)) return;
    TGuardValue<bool> SubmissionGuard(bSubmittingPing,true);
    const double Now=ServerTime(); PingTokens=FMath::Min(2.0,PingTokens+FMath::Max(0.0,Now-LastPingTokenTime)*2.0); LastPingTokenTime=Now;
    if (PingTokens<1) return;
    --PingTokens; // Rejected spatial requests still spend the anti-spam token, never a gameplay resource.
    if (FVector::DistSquared(PC->GetPawn()->GetActorLocation(),Point)>FMath::Square(PingRange)) return;
    AActor* Actor=nullptr;
    if (Subject.IsValid()) {
        if (!ResolveAuthoritySubject(Subject,Actor) || FVector::DistSquared(Actor->GetActorLocation(),Point)>FMath::Square(300.0f)) return;
        Point=Actor->GetTargetLocation(PC->GetPawn());
    }
    if (!HasSight(PC,Actor,Point,false) || FVector::DistSquared(PC->GetPawn()->GetActorLocation(),Point)>FMath::Square(PingRange)) return;
    TArray<AMythicPlayerController*> Controllers;
    GetWorld()->GetSubsystem<UMythicSessionPartySubsystem>()->GetReadyControllers(Controllers);
    TArray<TWeakObjectPtr<UMythicCombatUtilityComponent>> EligibleRecipients;
    for (auto* RecipientPC:Controllers) if (IsRecipientAllowed(RecipientPC)) {
        if (auto* Component=RecipientPC->FindComponentByClass<UMythicCombatUtilityComponent>()) EligibleRecipients.Add(Component);
    }
    // A capacity failure preserves the previous accepted ping instead of silently omitting part of a party.
    if (EligibleRecipients.IsEmpty() || EligibleRecipients.Num()>32) return;
    RevokeAuthorityPing();
    if (bStopping || !HasLiveOwner() || !IsRecipientAllowed(PC)) return;
    AuthorityAvatar=PC->GetPawn();
    const auto* Presentation=PC->GetPawn()->FindComponentByClass<UMythicEntityPresentationComponent>();
    AuthorityAvatarInstance=Presentation ? Presentation->GetPresentationInstance() : FMythicEntityPresentationInstance();
    AuthorityPing.SenderToken=SenderToken; AuthorityPing.PingId=FGuid::NewGuid(); AuthorityPing.Subject=Subject;
    AuthorityPing.Location=Point; AuthorityPing.Intent=Intent; AuthorityPing.ExpiryServerTime=Now+PingLifetime;
    for (auto Component:EligibleRecipients) { FRecipient Entry; Entry.Component=Component; Recipients.Add(Entry); }
    RefreshAuthorityPing(true); RefreshTick();
}
void UMythicCombatUtilityComponent::RevokeAuthorityPing() {
    const FMythicPartyPing Previous=AuthorityPing; AuthorityPing={};
    TArray<FRecipient> PreviousRecipients=MoveTemp(Recipients); Recipients.Reset();
    for (const auto& Recipient:PreviousRecipients) if (auto* Component=Recipient.Component.Get()) Component->ClientRemovePing(Previous.SenderToken,Previous.PingId);
    RefreshTick();
}
void UMythicCombatUtilityComponent::RefreshAuthorityPing(bool bForce) {
    if (!AuthorityPing.PingId.IsValid()) return;
    const double Now=ServerTime();
    if (Now>=AuthorityPing.ExpiryServerTime || !HasLiveOwner() || !IsRecipientAllowed(Controller())) { RevokeAuthorityPing(); return; }
    if (!bForce && Now<NextPingRefresh) return;
    NextPingRefresh=Now+0.5;
    AActor* Subject=nullptr;
    if (AuthorityPing.Subject.IsValid()) {
        if (ResolveAuthoritySubject(AuthorityPing.Subject,Subject) && HasSight(Controller(),Subject,Subject->GetTargetLocation(Controller()->GetPawn()),false)
            && FVector::DistSquared(Controller()->GetPawn()->GetActorLocation(),Subject->GetActorLocation())<=FMath::Square(PingRange))
            AuthorityPing.Location=Subject->GetTargetLocation(Controller()->GetPawn());
        else { AuthorityPing.Subject.Reset(); Subject=nullptr; }
    }
    const FGuid Current=AuthorityPing.PingId;
    // Never retain an array reference across an owning-client RPC: listen-server UI callbacks can reenter.
    for (int32 Index=Recipients.Num()-1;Index>=0;--Index) {
        if (AuthorityPing.PingId!=Current || !Recipients.IsValidIndex(Index)) return;
        auto* Component=Recipients[Index].Component.Get(); auto* PC=Component ? Component->Controller() : nullptr;
        if (!Component || !IsRecipientAllowed(PC)) {
            const auto Old=Recipients[Index].Last; Recipients.RemoveAtSwap(Index);
            if (Component) Component->ClientRemovePing(Old.SenderToken,Old.PingId);
            continue;
        }
        FMythicPartyPing Message=AuthorityPing;
        if (Subject && (!HasSight(PC,Subject,Message.Location,false) || !Subject->IsNetRelevantFor(PC,PC->GetPawn(),PC->GetPawn()->GetActorLocation()))) Message.Subject.Reset();
        const auto Last=Recipients[Index].Last;
        if (!bForce && Last.PingId==Message.PingId && Last.Subject==Message.Subject && FVector(Last.Location).Equals(Message.Location,5.0)) continue;
        Recipients[Index].Last=Message;
        Component->ClientReceivePing(Message);
    }
}
void UMythicCombatUtilityComponent::ClientReceivePing_Implementation(FMythicPartyPing Ping) {
    const auto* PC=Controller();
    if (bStopping || !PC || !PC->IsLocalController() || !Ping.SenderToken.IsValid() || !Ping.PingId.IsValid()
        || Ping.Location.ContainsNaN() || !FMath::IsFinite(Ping.ExpiryServerTime) || Ping.ExpiryServerTime<=ServerTime()
        || Ping.ExpiryServerTime>ServerTime()+9 || static_cast<uint8>(Ping.Intent)>static_cast<uint8>(EMythicPingIntent::Acknowledge)) return;
    PruneLocalPings();
    if (auto* Existing=LocalPings.FindByPredicate([&](const FMythicPartyPing& Value){ return Value.SenderToken==Ping.SenderToken; })) *Existing=Ping;
    else if (LocalPings.Num()<32) LocalPings.Add(Ping);
    else return;
    RefreshTick(); OnUtilityProjectionChanged.Broadcast();
}
void UMythicCombatUtilityComponent::ClientRemovePing_Implementation(FGuid Sender,FGuid Id) {
    if (LocalPings.RemoveAll([&](const FMythicPartyPing& Value){ return Value.SenderToken==Sender && Value.PingId==Id; })>0) OnUtilityProjectionChanged.Broadcast();
    RefreshTick();
}
TArray<FMythicPartyPing> UMythicCombatUtilityComponent::GetActivePings() const {
    TArray<FMythicPartyPing> Result;
    const double Now=ServerTime(); const auto* PC=Controller();
    const auto* LP=PC ? PC->GetLocalPlayer() : nullptr;
    const auto* Attention=LP ? LP->GetSubsystem<UMythicEntityAttentionSubsystem>() : nullptr;
    for (auto Ping:LocalPings) if (Ping.ExpiryServerTime>Now) {
        auto* Subject=ResolvePublic(Ping.Subject);
        FMythicEntityAttentionObservation Observation;
        // Unknown/deferred local visibility exposes only the communicated point. UI reads do not create traces.
        if (!Subject || !Attention || !Attention->FindObservation(Ping.Subject,Observation)
            || !Observation.bOnScreen || !Observation.bHasLineOfSight) Ping.Subject.Reset();
        Result.Add(MoveTemp(Ping));
    }
    return Result;
}
void UMythicCombatUtilityComponent::PruneLocalPings() {
    const double Now=ServerTime();
    if (LocalPings.RemoveAll([Now](const FMythicPartyPing& Ping){ return Ping.ExpiryServerTime<=Now; })>0) OnUtilityProjectionChanged.Broadcast();
}
void UMythicCombatUtilityComponent::HandleMembershipChanged() { RefreshAuthorityPing(true); }
void UMythicCombatUtilityComponent::HandleUnregistered(const FMythicEntityPresentationInstance& Instance,UMythicEntityPresentationComponent*) {
    if (LocalLock==Instance) ClearTargetLock();
    if (AuthorityLock==Instance) { AuthorityLock.Reset(); ClientSetLock(AuthorityLockRequest,{}); }
    if (AuthorityPing.Subject==Instance) { AuthorityPing.Subject.Reset(); RefreshAuthorityPing(true); }
    bool Changed=false; for (auto& Ping:LocalPings) if (Ping.Subject==Instance) { Ping.Subject.Reset(); Changed=true; }
    if (Changed) OnUtilityProjectionChanged.Broadcast();
    if (Instance==AuthorityAvatarInstance) AuthorityEndUtilitySession();
    RefreshTick();
}
void UMythicCombatUtilityComponent::AuthorityEndUtilitySession() {
    if (!Controller() || !Controller()->HasAuthority()) return;
    AuthorityLock.Reset(); AuthorityAvatar.Reset(); AuthorityAvatarInstance.Reset();
    RevokeAuthorityPing(); if (!bStopping) ClientSetLock(AuthorityLockRequest,{}); RefreshTick();
}
void UMythicCombatUtilityComponent::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* Tick) {
    Super::TickComponent(DeltaTime,TickType,Tick);
    auto* PC=Controller(); if (!PC || bStopping) return;
    if (PC->HasAuthority() && (AuthorityLock.IsValid() || AuthorityPing.PingId.IsValid())) {
        if (!HasLiveOwner() || AuthorityAvatar.Get()!=PC->GetPawn() || !ResolvePublic(AuthorityAvatarInstance)) AuthorityEndUtilitySession();
        else {
            AActor* Target=nullptr;
            if (AuthorityLock.IsValid()) {
                bool Keep=ResolveAuthoritySubject(AuthorityLock,Target) && IsAuthorityHostile(Target)
                    && FVector::DistSquared(PC->GetPawn()->GetActorLocation(),Target->GetActorLocation())<=FMath::Square(LockMaintainRange);
                if (Keep && HasSight(PC,Target,Target->GetTargetLocation(PC->GetPawn()),false)) LockLastVisible=ServerTime();
                else if (Keep) Keep=ServerTime()-LockLastVisible<=LockLineOfSightGrace;
                if (!Keep) { AuthorityLock.Reset(); ClientSetLock(AuthorityLockRequest,{}); }
            }
            RefreshAuthorityPing(false);
        }
    }
    if (PC->IsLocalController()) { UpdateLocalLock(); PruneLocalPings(); }
    RefreshTick();
}
void UMythicCombatUtilityComponent::RefreshTick() {
    SetComponentTickEnabled(!bStopping && (LocalLock.IsValid() || AuthorityLock.IsValid() || !LocalPings.IsEmpty() || AuthorityPing.PingId.IsValid()));
}
