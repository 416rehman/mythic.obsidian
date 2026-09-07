#include "MythicTraversalComponent.h"
#include "AbilitySystemGlobals.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/Navigation/MythicNavigationComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/Abilities/MythicGA_SystemDash.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "Input/MythicTags_Input.h"
#include "Input/MythicControlsSettings.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicAbilityIntentComponent.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythicTraversal,Log,All);

namespace {
bool SweepCapsule(ACharacter* C,const FVector& From,const FVector& To,FHitResult& Hit) {
    auto* Capsule=C ? C->GetCapsuleComponent() : nullptr;
    auto* Move=C ? C->GetCharacterMovement() : nullptr;
    if (!Capsule || !Move || Move->UpdatedComponent!=Capsule || !Capsule->IsRegistered()
        || Capsule->GetCollisionEnabled()==ECollisionEnabled::NoCollision || From.ContainsNaN() || To.ContainsNaN()) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SystemTraversal),false,C);
    FCollisionResponseParams Response;
    Capsule->InitSweepCollisionParams(Query,Response);
    C->GetWorld()->SweepSingleByChannel(Hit,From,To,Capsule->GetComponentQuat(),Capsule->GetCollisionObjectType(),
        FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),Capsule->GetScaledCapsuleHalfHeight()),Query,Response);
    return !Hit.bBlockingHit && !Hit.bStartPenetrating;
}
}
UMythicTraversalComponent::UMythicTraversalComponent() {
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.bStartWithTickEnabled=false;
    static ConstructorHelpers::FObjectFinder<UAnimMontage> Low(TEXT("/Game/AdvancedLocomotionV4/Parkour/Anims/Mantle_1m_Montage_Default"));
    static ConstructorHelpers::FObjectFinder<UAnimMontage> High(TEXT("/Game/AdvancedLocomotionV4/Parkour/Anims/Mantle_2m_Montage_Default"));
    LowMantleMontage=Low.Object; HighMantleMontage=High.Object;
}
void UMythicTraversalComponent::InitializeAbilitySystem(UMythicAbilitySystemComponent* InASC) {
    if (auto* Character=Cast<ACharacter>(GetOwner())) if (auto* Move=Character->GetCharacterMovement()) Move->GetNavAgentPropertiesRef().bCanCrouch=true;
    if (ASC!=InASC) {
        CancelTraversal();
        if (ASC && GetOwner()->HasAuthority() && DashHandle.IsValid()) ASC->ClearAbility(DashHandle);
        DashHandle={}; ASC=InASC;
    }
    if (!ASC || !GetOwner()->HasAuthority() || ASC->GetAvatarActor()!=GetOwner()) return;
    if (const auto* Spec=ASC->FindAbilitySpecFromHandle(DashHandle); Spec && Spec->SourceObject.Get()==this) return;
    FGameplayAbilitySpec Spec(UMythicGA_SystemDash::StaticClass(),1,INDEX_NONE,this);
    Spec.GetDynamicSpecSourceTags().AddTag(INPUT_ACTION_DODGE);
    DashHandle=ASC->GiveAbility(Spec);
}
bool UMythicTraversalComponent::OwnsDashSpec(FGameplayAbilitySpecHandle Handle) const {
    if (!ASC || ASC->GetAvatarActor()!=GetOwner()) return false;
    const auto* Spec=ASC->FindAbilitySpecFromHandle(Handle);
    return Spec && Spec->SourceObject.Get()==this && Spec->Ability && Spec->Ability->GetClass()==UMythicGA_SystemDash::StaticClass();
}
bool UMythicTraversalComponent::IsMovementAllowed(const ACharacter* Character) {
    if (!IsValid(Character) || !Character->GetController() || !Character->GetCharacterMovement()) return false;
    auto* ASC=UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Character);
    const auto* Life=Character->FindComponentByClass<UMythicLifeComponent>();
    const auto* Move=Character->GetCharacterMovement();
    return ASC && ASC->GetAvatarActor()==Character && Life && Life->IsInitialized()
        && (Move->IsMovingOnGround() || Move->IsFalling())
        && !ASC->HasMatchingGameplayTag(GAS_STATE_DEAD) && !ASC->HasMatchingGameplayTag(GAS_STATE_DYING)
        && !ASC->HasMatchingGameplayTag(GAS_DEBUFF_STUNNED) && !ASC->HasMatchingGameplayTag(GAS_DEBUFF_FROZEN);
}
bool UMythicTraversalComponent::CaptureDirections(ACharacter* Character,FVector& Aim,FVector& Movement) {
    Aim=Movement=FVector::ZeroVector;
    if (!Character) return false;
    Movement=Character->GetPendingMovementInputVector().GetSafeNormal2D();
    if (auto* PC=Cast<AMythicPlayerController>(Character->GetController())) {
        if (Movement.IsNearlyZero()) if (auto* Navigation=PC->FindComponentByClass<UMythicNavigationComponent>())
            Navigation->TryGetCurrentCommandDirection(Character,Movement);
        if (PC->GetLocalPlayer()) return PC->AbilityIntentComponent && PC->AbilityIntentComponent->TryCaptureAimDirection(1400,Aim);
    }
    Aim=Character->GetBaseAimRotation().Vector().GetSafeNormal();
    return !Aim.ContainsNaN() && !Aim.IsNearlyZero();
}
bool UMythicTraversalComponent::HasValidDashSettings() {
    const auto* S=GetDefault<UMythicControlsSettings>();
    return S && FMath::IsFinite(S->DashDistance) && S->DashDistance>=40 && S->DashDistance<=600
        && FMath::IsFinite(S->DashDuration) && S->DashDuration>=0.1f && S->DashDuration<=0.6f
        && FMath::IsFinite(S->DashStaminaCost) && S->DashStaminaCost>=0 && S->DashStaminaCost<=100
        && FMath::IsFinite(S->DashCooldown) && S->DashCooldown>=0.2f && S->DashCooldown<=5;
}
bool UMythicTraversalComponent::BuildDashDestination(ACharacter* Character,FVector Direction,FVector& Destination) {
    Destination=FVector::ZeroVector;
    if (!HasValidDashSettings() || !IsMovementAllowed(Character) || Direction.ContainsNaN() || Direction.GetSafeNormal2D().IsNearlyZero()) return false;
    Direction=Direction.GetSafeNormal2D(); const FVector Start=Character->GetActorLocation();
    FHitResult Hit;
    const float RequestedDistance=GetDefault<UMythicControlsSettings>()->DashDistance;
    const bool bClear=SweepCapsule(Character,Start,Start+Direction*RequestedDistance,Hit);
    UE_LOG(LogMythicTraversal,VeryVerbose,TEXT("Dash sweep actor=%s authority=%d start=%s direction=%s clear=%d penetrating=%d hitTime=%f component=%s normal=%s"),
        *Character->GetPathName(),Character->HasAuthority(),*Start.ToString(),*Direction.ToString(),bClear,Hit.bStartPenetrating,Hit.Time,
        *GetPathNameSafe(Hit.GetComponent()),*Hit.ImpactNormal.ToString());
    if (Hit.bStartPenetrating) return false;
    const float Distance=bClear ? RequestedDistance : FMath::Max(0.0f,RequestedDistance*Hit.Time-2.0f);
    if (Distance<40) return false;
    Destination=Start+Direction*Distance;
    return SweepCapsule(Character,Start,Destination,Hit);
}
bool UMythicTraversalComponent::BuildMantleRoute(ACharacter* Character,FVector Direction,FMythicMantleRoute& Route) {
    Route={};
    if (!IsMovementAllowed(Character) || Direction.ContainsNaN()) return false;
    Direction=Direction.GetSafeNormal2D(); if (Direction.IsNearlyZero()) return false;
    auto* Capsule=Character->GetCapsuleComponent(); auto* Move=Character->GetCharacterMovement();
    if (!Capsule || Move->UpdatedComponent!=Capsule) return false;
    const FVector Start=Character->GetActorLocation(); const float Half=Capsule->GetScaledCapsuleHalfHeight();
    const float Radius=Capsule->GetScaledCapsuleRadius();
    FHitResult Wall;
    if (SweepCapsule(Character,Start,Start+Direction*150,Wall) || Wall.bStartPenetrating || Move->IsWalkable(Wall)) return false;
    // The wall's collision-safe contact is the only forward nomination. No client endpoint crosses this API.
    const FVector Probe=Wall.ImpactPoint+Direction*(Radius+8);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(SystemMantleTop),false,Character);
    FCollisionResponseParams Response; Capsule->InitSweepCollisionParams(Query,Response);
    FHitResult Top;
    const FVector TraceTop(Probe.X,Probe.Y,Start.Z-Half+202);
    const FVector TraceBottom(Probe.X,Probe.Y,Start.Z-Half+38);
    Character->GetWorld()->LineTraceSingleByChannel(Top,TraceTop,TraceBottom,Capsule->GetCollisionObjectType(),Query,Response);
    const float Height=Top.ImpactPoint.Z-(Start.Z-Half);
    if (!Top.bBlockingHit || Top.bStartPenetrating || !Move->IsWalkable(Top) || Height<40 || Height>200 || !Top.GetComponent()) return false;
    const FVector End=Top.ImpactPoint+FVector(0,0,Half+2.5f);
    const float Reach=FVector::Dist2D(Start,End);
    // A capsule close to the wall can have a safe landing below 100cm away.
    // The complete capsule route below owns clearance; only bound forward reach.
    if (!FMath::IsFinite(Reach) || Reach<=UE_KINDA_SMALL_NUMBER || Reach>150
        || FVector::DotProduct(End-Start,Direction)<=0) return false;
    // Up, over, then settle: every leg and the complete end capsule are admitted now and swept by CMC in motion.
    Route.Points.Add(FVector(Start.X,Start.Y,End.Z+5));
    Route.Points.Add(End+FVector(0,0,5)); Route.Points.Add(End);
    FVector From=Start;
    for (const FVector& To:Route.Points) { FHitResult Hit; if (!SweepCapsule(Character,From,To,Hit)) {Route={}; return false;} From=To; }
    FHitResult Fit; if (!SweepCapsule(Character,End,End,Fit)) {Route={}; return false;}
    Route.Height=Height; Route.Support=Top.GetComponent(); Route.SupportTransform=Top.GetComponent()->GetComponentTransform();
    return true;
}
void UMythicTraversalComponent::Dash() {
    auto* Character=Cast<ACharacter>(GetOwner());
    if (!Character || !Character->IsLocallyControlled() || !ASC || !ASC->CanAcceptAbilityInput()) return;
    ASC->AbilityInputTagPressed(INPUT_ACTION_DODGE);
}
void UMythicTraversalComponent::ReleaseDash() { if (ASC) ASC->AbilityInputTagReleased(INPUT_ACTION_DODGE); }
void UMythicTraversalComponent::Jump() {
    auto* Character=Cast<ACharacter>(GetOwner());
    if (!Character || !Character->IsLocallyControlled() || bJumpHeld || !IsMovementAllowed(Character)) return;
    bJumpHeld=true;
    if (IsMantling()) return;
    FVector Aim, Movement; FMythicMantleRoute Route;
    const bool bHasAim=CaptureDirections(Character,Aim,Movement);
    // Mantle follows a current movement command first. This is local traversal
    // nomination only: combat aim and the server's full route validation stay intact.
    FVector Direction=Movement.ContainsNaN() ? FVector::ZeroVector : Movement.GetSafeNormal2D();
    if (Direction.IsNearlyZero() && bHasAim) Direction=Aim.GetSafeNormal2D();
    if (!Direction.IsNearlyZero() && BuildMantleRoute(Character,Direction,Route)) {
        PendingRequest=++NextRequest; if (!PendingRequest) PendingRequest=++NextRequest;
        ServerRequestMantle(PendingRequest,Direction);
    } else Character->Jump();
}
void UMythicTraversalComponent::ReleaseJump() {
    bJumpHeld=false; if (auto* Character=Cast<ACharacter>(GetOwner())) Character->StopJumping();
}
void UMythicTraversalComponent::CancelTraversal() {
    const uint32 Request=ActiveRequest ? ActiveRequest : PendingRequest;
    ReleaseJump(); PendingRequest=0;
    if (Request) ServerCancelMantle(Request);
    if (ASC) ASC->AbilityInputTagCanceled(INPUT_ACTION_DODGE);
    if (GetOwner() && GetOwner()->HasAuthority()) FinishMantle(true);
}
void UMythicTraversalComponent::ServerRequestMantle_Implementation(uint32 Request,FVector_NetQuantizeNormal Direction) {
    auto* Character=Cast<ACharacter>(GetOwner());
    const FVector Aim(Direction);
    if (!Request || static_cast<int32>(Request-LastServerRequest)<=0) return;
    LastServerRequest=Request;
    FMythicMantleRoute Route;
    const bool bDirectionValid=!Aim.ContainsNaN() && FMath::IsNearlyZero(Aim.Z,0.001f) && FMath::IsNearlyEqual(Aim.SizeSquared(),1.0,0.02f);
    if (bEnding || IsMantling() || !bDirectionValid || !BuildMantleRoute(Character,Aim,Route)
        || !ASC || ASC->IsActivationGroupBlocked(EMythicAbilityActivationGroup::Exclusive_Replaceable)) {ClientMantleRejected(Request); return;}
    const bool bHigh=Route.Height>110;
    const auto* Montage=bHigh?HighMantleMontage.Get():LowMantleMontage.Get();
    const float Duration=bHigh?HighMantleSeconds:LowMantleSeconds;
    if (!Montage || !FMath::IsFinite(Duration) || Duration<0.2f || Duration>2 || Montage->GetPlayLength()<=0) {ClientMantleRejected(Request); return;}
    BeginMantle(Request,MoveTemp(Route));
}
void UMythicTraversalComponent::ClientMantleRejected_Implementation(uint32 Request) {
    if (PendingRequest!=Request) return;
    PendingRequest=0;
    // Rejected authority geometry falls back only while the same fresh jump press still owns the action.
    if (auto* Character=Cast<ACharacter>(GetOwner()); bJumpHeld && IsMovementAllowed(Character)) Character->Jump();
}
void UMythicTraversalComponent::ServerCancelMantle_Implementation(uint32 Request) {
    if (Request && Request==ActiveRequest) FinishMantle(true);
}
void UMythicTraversalComponent::BeginMantle(uint32 Request,FMythicMantleRoute&& Route) {
    auto* Character=CastChecked<ACharacter>(GetOwner());
    ActiveRequest=Request; ActiveRoute=MoveTemp(Route); Leg=0;
    MantleDuration=ActiveRoute.Height>110?HighMantleSeconds:LowMantleSeconds;
    MantleEnd=GetWorld()->GetTimeSeconds()+MantleDuration+0.25;
    if (auto* Navigation=Character->GetController()->FindComponentByClass<UMythicNavigationComponent>()) Navigation->CancelDestinationCommand();
    Character->StopJumping();
    // Falling supports full XYZ override root motion and retains genuine gravity/landing once the source ends.
    auto* Move=Character->GetCharacterMovement();
    PreviousMovementMode=static_cast<uint8>(Move->MovementMode.GetValue()); PreviousCustomMovementMode=Move->CustomMovementMode;
    bOwnsMantleMode=true;
    Character->MovementModeChangedDelegate.AddDynamic(this,&ThisClass::OnMantleMovementModeChanged);
    { TGuardValue<bool> Changing(bSettingMantleMode,true); Move->SetMovementMode(MOVE_Falling); }
    if (ActiveRequest!=Request || bEnding) return;
    if (!StartMantleLeg()) {FinishMantle(true); return;}
    SetComponentTickEnabled(true);
    MulticastMantlePresentation(Request,true,ActiveRoute.Height>110,MantleDuration);
}
void UMythicTraversalComponent::OnMantleMovementModeChanged(ACharacter*,EMovementMode,uint8) {
    if (ActiveRequest && !bSettingMantleMode) { bOwnsMantleMode=false; FinishMantle(true); }
}
bool UMythicTraversalComponent::StartMantleLeg() {
    auto* Character=Cast<ACharacter>(GetOwner()); auto* Move=Character?Character->GetCharacterMovement():nullptr;
    if (!Move || !ActiveRoute.Points.IsValidIndex(Leg)) return false;
    if (MotionId) {Move->RemoveRootMotionSourceByID(MotionId); MotionId=0;}
    const FVector Start=Character->GetActorLocation(), End=ActiveRoute.Points[Leg];
    FHitResult Hit; if (!SweepCapsule(Character,Start,End,Hit)) return false;
    const float Seconds=MantleDuration*(Leg==0?0.45f:(Leg==1?0.45f:0.1f));
    auto Source=MakeShared<FRootMotionSource_MoveToForce>();
    Source->InstanceName=FName(*FString::Printf(TEXT("SystemMantle_%u_%d"),ActiveRequest,Leg));
    Source->AccumulateMode=ERootMotionAccumulateMode::Override; Source->Priority=1000;
    Source->StartLocation=Start; Source->TargetLocation=End; Source->Duration=Seconds; Source->bRestrictSpeedToExpected=true;
    Source->FinishVelocityParams.Mode=ERootMotionFinishVelocityMode::ClampVelocity;
    Source->FinishVelocityParams.ClampVelocity=Move->GetMaxSpeed();
    const uint32 Request=ActiveRequest;
    const uint16 Applied=Move->ApplyRootMotionSource(Source);
    if (!Request || ActiveRequest!=Request || bEnding) { if (Applied) Move->RemoveRootMotionSourceByID(Applied); return false; }
    MotionId=Applied; LegEnd=GetWorld()->GetTimeSeconds()+Seconds;
    return MotionId!=0;
}
bool UMythicTraversalComponent::ValidateActiveMantle() const {
    const auto* Character=Cast<ACharacter>(GetOwner());
    return ActiveRequest && IsMovementAllowed(Character) && ASC && ASC->GetAvatarActor()==Character
        && ActiveRoute.Support.IsValid() && ActiveRoute.Support->IsRegistered()
        && ActiveRoute.Support->GetComponentTransform().Equals(ActiveRoute.SupportTransform,0.1f)
        && Character->GetCharacterMovement()->UpdatedComponent==Character->GetCapsuleComponent();
}
void UMythicTraversalComponent::TickComponent(float Delta,ELevelTick TickType,FActorComponentTickFunction* Tick) {
    Super::TickComponent(Delta,TickType,Tick);
    if (!GetOwner()->HasAuthority() || !ActiveRequest) return;
    const double Now=GetWorld()->GetTimeSeconds();
    if (!ValidateActiveMantle() || Now>MantleEnd) {FinishMantle(true); return;}
    if (Now+0.001<LegEnd) return;
    auto* Character=CastChecked<ACharacter>(GetOwner());
    if (FVector::DistSquared(Character->GetActorLocation(),ActiveRoute.Points[Leg])>FMath::Square(12.0f)) {FinishMantle(true); return;}
    ++Leg;
    if (Leg==ActiveRoute.Points.Num()) {FinishMantle(false); return;}
    if (!StartMantleLeg()) FinishMantle(true);
}
void UMythicTraversalComponent::FinishMantle(bool bCancelled) {
    if (!ActiveRequest || bEnding) return;
    TGuardValue<bool> Ending(bEnding,true); const uint32 Finished=ActiveRequest; ActiveRequest=0;
    SetComponentTickEnabled(false);
    if (auto* Character=Cast<ACharacter>(GetOwner())) if (auto* Move=Character->GetCharacterMovement()) {
        Character->MovementModeChangedDelegate.RemoveDynamic(this,&ThisClass::OnMantleMovementModeChanged);
        if (MotionId) Move->RemoveRootMotionSourceByID(MotionId);
        if (bCancelled && bOwnsMantleMode && Move->MovementMode==MOVE_Falling) {
            const auto Previous=static_cast<EMovementMode>(PreviousMovementMode);
            if (Previous==MOVE_Walking || Previous==MOVE_NavWalking) {
                FFindFloorResult Floor; Move->FindFloor(Character->GetActorLocation(),Floor,false);
                if (Floor.IsWalkableFloor() && Floor.FloorDist<=UCharacterMovementComponent::MAX_FLOOR_DIST)
                    Move->SetMovementMode(Previous,PreviousCustomMovementMode);
            } else Move->SetMovementMode(Previous,PreviousCustomMovementMode);
        }
        bOwnsMantleMode=false;
        // Airborne cancellation never fabricates ground; external movement-mode changes retain ownership.
    }
    MotionId=0; ActiveRoute={};
    if (GetOwner()->HasAuthority()) MulticastMantlePresentation(Finished,false,false,0);
}
void UMythicTraversalComponent::MulticastMantlePresentation_Implementation(uint32 Request,bool bStart,bool bHigh,float Seconds) {
    auto* Character=Cast<ACharacter>(GetOwner());
    if (!Character || GetNetMode()==NM_DedicatedServer) return;
    auto* Anim=Character->GetMesh()?Character->GetMesh()->GetAnimInstance():nullptr;
    if (bStart) {
        VisualRequest=Request; if (PendingRequest==Request) PendingRequest=0;
        if (!Character->HasAuthority()) ActiveRequest=Request;
        PlayingMontage=bHigh?HighMantleMontage:LowMantleMontage;
        if (Anim && PlayingMontage && Seconds>0) {
            MantleAnim=Anim; PreviousRootMotionMode=static_cast<uint8>(Anim->RootMotionMode.GetValue());
            // The retained montage is visual only; its extracted animation root motion cannot override the validated CMC route.
            Anim->SetRootMotionMode(ERootMotionMode::IgnoreRootMotion);
            Anim->Montage_Play(PlayingMontage,PlayingMontage->GetPlayLength()/Seconds);
        }
    } else if (VisualRequest==Request) {
        VisualRequest=0;
        if (!Character->HasAuthority()) ActiveRequest=0;
        if (auto* OwnedAnim=MantleAnim.Get()) {
            if (PlayingMontage) OwnedAnim->Montage_Stop(0.1f,PlayingMontage);
            if (OwnedAnim->RootMotionMode==ERootMotionMode::IgnoreRootMotion)
                OwnedAnim->SetRootMotionMode(static_cast<ERootMotionMode::Type>(PreviousRootMotionMode));
        }
        MantleAnim.Reset(); PlayingMontage=nullptr;
    }
}
void UMythicTraversalComponent::EndPlay(EEndPlayReason::Type Reason) {
    CancelTraversal();
    if (VisualRequest) MulticastMantlePresentation_Implementation(VisualRequest,false,false,0);
    if (ASC && GetOwner()->HasAuthority() && DashHandle.IsValid()) ASC->ClearAbility(DashHandle);
    DashHandle={}; ASC=nullptr;
    Super::EndPlay(Reason);
}
