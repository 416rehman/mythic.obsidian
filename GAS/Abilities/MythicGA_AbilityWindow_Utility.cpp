#include "MythicGA_AbilityWindow.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Player/MythicCharacter.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "TimerManager.h"

namespace {
bool ReadUtilityParameter(const FMythicAbilityActionProfile* Profile,FName Name,float& Value) {
    Value=0;
    if (!Profile || Profile->Work!=EMythicAbilityActionWork::Utility || Name.IsNone()) return false;
    const auto* Parameter=Profile->Parameters.FindByPredicate([Name](const auto& Row){ return Row.Name==Name; });
    if (!Parameter || !FMath::IsFinite(Parameter->Value) || Parameter->Value<Parameter->Minimum || Parameter->Value>Parameter->Maximum) return false;
    Value=Parameter->Value; return true;
}
}

bool UMythicGA_AbilityWindow::CanActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
    const FGameplayTagContainer* SourceTags,const FGameplayTagContainer* TargetTags,FGameplayTagContainer* RelevantTags) const {
    // GAS may ask the CDO. Consult the actual equipped instance while a native utility
    // callback is on its stack, so a nested activation cannot overwrite that Blueprint frame.
    const auto* Spec=Info && Info->AbilitySystemComponent.IsValid() ? Info->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle) : nullptr;
    const auto* Instance=Spec ? Cast<UMythicGA_AbilityWindow>(Spec->GetPrimaryInstance()) : nullptr;
    return UtilityCallbackDepth==0 && (!Instance || Instance->UtilityCallbackDepth==0)
        && Super::CanActivateAbility(Handle,Info,SourceTags,TargetTags,RelevantTags);
}

bool UMythicGA_AbilityWindow::TryGetResolvedUtilityParameter(FName ActionId,FName Name,float& Value) const {
    Value=0;
    return !bCleaned && !bEnding && CurrentActorInfo && CurrentActorInfo->IsNetAuthority()
        && WindowAvatar.IsValid() && CheckTargetIncarnation(WindowAvatar.Get())
        && ReadUtilityParameter(ActionProfiles.Find(ActionId),Name,Value);
}

bool UMythicGA_AbilityWindow::TryGetUtilityActionParameter(FGuid ActionHandle,FName Name,float& Value) const {
    Value=0; const auto* Found=ActionGroups.Find(ActionHandle);
    return IsWindowAuthority() && !bEnding && Found && (*Found)->Activation==ActivationId
        && ReadUtilityParameter(&(*Found)->Profile,Name,Value);
}

bool UMythicGA_AbilityWindow::TryReadUtilityActionSourceAttribute(FGuid ActionHandle,FGameplayAttribute Attribute,float& Value) const {
    Value=0; const auto* Found=ActionGroups.Find(ActionHandle);
    if (!IsWindowAuthority() || bEnding || !Found || (*Found)->Activation!=ActivationId
        || (*Found)->Profile.Work!=EMythicAbilityActionWork::Utility || !(*Found)->Profile.SourceAttributes.Contains(Attribute)) return false;
    const auto* Context=FMythicGameplayEffectContext::ExtractEffectContext((*Found)->Context);
    const auto Source=Context && Context->ActionSnapshot ? Context->ActionSnapshot->Source : nullptr;
    if (!Source) return false;
    FGameplayTagContainer Tags=Source->StateTags; Tags.AppendTags((*Found)->Profile.Facets);
    Tags.AddTag(GAS_ABILITY_TYPE_SKILL);
    return Source->EvaluateStat(Attribute,Tags,nullptr,Value);
}

FGuid UMythicGA_AbilityWindow::ReserveSelfStatusGroup(const TArray<FGameplayTag>& Priority) {
    if (bCleaned || bEnding || bCommitted || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority()
        || !WindowAvatar.IsValid() || WindowAvatar.Get()!=GetAvatarActorFromActorInfo()
        || !CheckTargetIncarnation(WindowAvatar.Get()) || StatusReservations.Num()>=4 || Priority.IsEmpty() || Priority.Num()>8) return {};
    const auto* GI=GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const auto* Registry=GI ? GI->GetSubsystem<UMythicStatusRegistry>() : nullptr;
    auto* ASC=Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (!Registry || !ASC) return {};
    TArray<const UMythicStatusEffectDefinition*> Definitions;
    for (const auto Tag:Priority) {
        const auto* Definition=Registry->FindStatus(Tag);
        if (!Definition) return {}; // Bad authoring cannot silently turn into a broader cleanse.
        Definitions.AddUnique(Definition);
    }
    auto Lease=FMythicStatusGroupReservation::Reserve(ASC,Definitions);
    if (!Lease) return {};
    const FGuid Id=FGuid::NewGuid(); StatusReservations.Add(Id,MoveTemp(Lease)); return Id;
}

FMythicStatusGroupRemoval UMythicGA_AbilityWindow::ConsumeSelfStatusGroup(FGuid Reservation) {
    if (!IsWindowAuthority() || bEnding) return {};
    TGuardValue<int32> NativeCallbacks(UtilityCallbackDepth,UtilityCallbackDepth+1);
    TSharedPtr<FMythicStatusGroupReservation> Lease;
    if (!StatusReservations.RemoveAndCopyValue(Reservation,Lease) || !Lease) return {};
    const FGuid Serial=ActivationId;
    auto Result=Lease->Consume();
    if (Result.bRemoved) Result.ActivationId=Serial;
    return Result;
}

void UMythicGA_AbilityWindow::ReleaseStatusGroupReservation(FGuid Reservation) { StatusReservations.Remove(Reservation); }

FMythicStatusGroupRemoval UMythicGA_AbilityWindow::ConsumeUtilityStatusGroup(FGuid ActionHandle,FGuid Reservation) {
    if (!IsWindowAuthority() || bEnding) return {};
    const auto* Found=ActionGroups.Find(ActionHandle);
    const auto RemoveStatus=FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Effect.RemoveStatus"),false);
    if (!Found || (*Found)->Activation!=ActivationId || (*Found)->Profile.Work!=EMythicAbilityActionWork::Utility
        || !RemoveStatus.IsValid() || !(*Found)->Profile.Facets.HasTagExact(RemoveStatus)) return {};
    const auto Group=*Found;
    ActionGroups.Remove(ActionHandle); // Claim before GAS removal can reenter this action.
    const auto* GameState=GetWorld()->GetGameState();
    const double RemovedAt=GameState ? GameState->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    auto Receipt=ConsumeSelfStatusGroup(Reservation);
    if (Receipt.bRemoved) {
        Receipt.ActionId=Group->Profile.ActionId; Receipt.ActionGroupId=ActionHandle; Receipt.Facets=Group->Profile.Facets;
        float Maximum=0;
        if (IsWindowAuthority() && !bEnding && Receipt.ActivationId==ActivationId && SuccessfulUtilityRemovals.Num()<4
            && ReadUtilityParameter(&Group->Profile,TEXT("RemnantMaximum"),Maximum) && Maximum>0 && Maximum<=3)
            SuccessfulUtilityRemovals.Add(Receipt.ReceiptId,{ActivationId,RemovedAt,FMath::Min(Maximum,Receipt.RemainingSeconds)});
    }
    return Receipt;
}

FGuid UMythicGA_AbilityWindow::PrepareUtilityEscape(FName ActionId,FVector Direction) {
    if (bCleaned || bEnding || bCommitted || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority()
        || PreparedUtilityEscapes.Num()>=4 || Direction.ContainsNaN()) return {};
    const auto* Profile=ActionProfiles.Find(ActionId);
    const auto* Source=ActionSources.Find(ActionId);
    const auto MovementFacet=FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Effect.Movement"),false);
    float Distance=0,Transit=0;
    if (!Source || !Profile || !MovementFacet.IsValid() || !Profile->Facets.HasTagExact(MovementFacet)
        || !TryGetResolvedUtilityParameter(ActionId,TEXT("Distance"),Distance)
        || !TryGetResolvedUtilityParameter(ActionId,TEXT("Transit"),Transit) || Transit<0.05f || Transit>1.0f) return {};
    Direction=Direction.GetSafeNormal2D(); TArray<FVector> Points;
    if (!ValidateAbilityEscapePath(Direction,Distance,Points)) return {};
    const auto* Character=CastChecked<ACharacter>(WindowAvatar.Get());
    FPreparedUtilityEscape Prepared;
    Prepared.ActionId=ActionId; Prepared.Activation=ActivationId; Prepared.Avatar=WindowAvatar; Prepared.Source=*Source;
    Prepared.Start=Character->GetActorLocation(); Prepared.End=Points.Last(); Prepared.Direction=Direction;
    Prepared.Departure=Character->GetActorTransform();
    Prepared.Distance=Distance; Prepared.Transit=Transit;
    Character->GetCapsuleComponent()->GetScaledCapsuleSize(Prepared.Radius,Prepared.HalfHeight);
    const FGuid Id=FGuid::NewGuid(); PreparedUtilityEscapes.Add(Id,MoveTemp(Prepared)); return Id;
}

bool UMythicGA_AbilityWindow::ValidatePreparedUtilityEscape(FGuid Preparation) const {
    const auto* Prepared=PreparedUtilityEscapes.Find(Preparation);
    if (!Prepared || Prepared->Activation!=ActivationId || Prepared->Avatar!=WindowAvatar || !WindowAvatar.IsValid()
        || !CheckTargetIncarnation(WindowAvatar.Get()) || !WindowAvatar->GetActorLocation().Equals(Prepared->Start,1.0)) return false;
    const auto* Source=ActionSources.Find(Prepared->ActionId);
    const auto* Character=Cast<ACharacter>(WindowAvatar.Get());
    if (!Source || *Source!=Prepared->Source || !Character) return false;
    float Radius=0,HalfHeight=0; Character->GetCapsuleComponent()->GetScaledCapsuleSize(Radius,HalfHeight);
    if (!FMath::IsNearlyEqual(Radius,Prepared->Radius) || !FMath::IsNearlyEqual(HalfHeight,Prepared->HalfHeight)) return false;
    TArray<FVector> Current;
    return ValidateAbilityEscapePath(Prepared->Direction,Prepared->Distance,Current) && Current.Last().Equals(Prepared->End,1.0);
}

bool UMythicGA_AbilityWindow::PresentUtilityStatusRemnant(FGuid RemovalReceiptId,FGuid EscapePreparation,UMythicAbilityPoseRemnantStyle* Style) {
    if (!IsWindowAuthority() || bEnding || !IsValid(Style) || !Style->PoseMaterial || ActivePoseRemnants.Num()>=4
        || !ValidatePreparedUtilityEscape(EscapePreparation)) return false;
    const auto* Success=SuccessfulUtilityRemovals.Find(RemovalReceiptId);
    const auto* Prepared=PreparedUtilityEscapes.Find(EscapePreparation);
    auto* ASC=Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    auto* Presentation=WindowAvatar->FindComponentByClass<UMythicEntityPresentationComponent>();
    if (!Success || Success->Activation!=ActivationId || Success->Duration<=0 || !Prepared || !ASC
        || !Presentation || !Presentation->GetPresentationInstance().IsValid()) return false;
    TGuardValue<int32> NativeCallbacks(UtilityCallbackDepth,UtilityCallbackDepth+1);
    FMythicAbilityPoseRemnantEvent Event;
    Event.ActivationId=ActivationId; Event.ReceiptId=RemovalReceiptId; Event.Instance=Presentation->GetPresentationInstance();
    Event.Avatar=WindowAvatar.Get(); Event.Style=Style; Event.Departure=Prepared->Departure;
    Event.ServerTime=Success->ServerTime; Event.Duration=Success->Duration;
    Event.CapsuleHalfHeight=Prepared->HalfHeight;
    SuccessfulUtilityRemovals.Remove(RemovalReceiptId);
    ActivePoseRemnants.Add(Event); // Own cancellation before the local multicast can execute presentation callbacks.
    ASC->PublishAbilityPoseRemnant(Event,false); return true;
}

bool UMythicGA_AbilityWindow::PlayPreparedUtilityDepartureCue(FGuid Preparation,FGameplayTag Cue) {
    if (!IsWindowAuthority() || bEnding || !Cue.IsValid() || !ValidatePreparedUtilityEscape(Preparation)) return false;
    auto* Prepared=PreparedUtilityEscapes.Find(Preparation);
    auto* Character=Cast<AMythicCharacter>(WindowAvatar.Get());
    if (!Prepared || Prepared->Activation!=ActivationId || Prepared->Avatar!=WindowAvatar || Prepared->bDeparturePresented || !Character) return false;
    TGuardValue<int32> NativeCallbacks(UtilityCallbackDepth,UtilityCallbackDepth+1);
    FGameplayCueParameters Params;
    Params.Location=Prepared->Departure.GetLocation()-Prepared->Departure.GetUnitAxis(EAxis::Z)*Prepared->HalfHeight;
    Params.Normal=Prepared->Direction; Params.Instigator=WindowAvatar.Get(); Params.EffectCauser=WindowAvatar.Get();
    Params.SourceObject=GetSkillDefinition(); Params.EffectContext=MakeEffectContext(CurrentSpecHandle,CurrentActorInfo);
    Prepared->bDeparturePresented=true;
    Character->PublishAbilityCue(Cue,Params); return true;
}

bool UMythicGA_AbilityWindow::ValidatePreparedUtilityEscapes() const {
    for (const auto& Prepared:PreparedUtilityEscapes) if (!ValidatePreparedUtilityEscape(Prepared.Key)) return false;
    return true;
}

bool UMythicGA_AbilityWindow::StartPreparedUtilityEscape(FGuid ActionHandle,FGuid Preparation) {
    if (!IsWindowAuthority() || bEnding || MovementTask || PendingUtilityMovement.IsSet()) return false;
    TGuardValue<int32> NativeCallbacks(UtilityCallbackDepth,UtilityCallbackDepth+1);
    const auto* Found=ActionGroups.Find(ActionHandle);
    const auto* Prepared=PreparedUtilityEscapes.Find(Preparation);
    if (!Found || !Prepared || (*Found)->Activation!=ActivationId || (*Found)->Profile.Work!=EMythicAbilityActionWork::Utility
        || (*Found)->Profile.ActionId!=Prepared->ActionId) return false;
    const auto Group=*Found; const FPreparedUtilityEscape Request=*Prepared;
    const bool bStillAdmitted=ValidatePreparedUtilityEscape(Preparation);
    ActionGroups.Remove(ActionHandle); PreparedUtilityEscapes.Remove(Preparation);
    FMythicAbilityMovementReceipt Receipt;
    Receipt.ActionId=Request.ActionId; Receipt.ActionGroupId=ActionHandle; Receipt.ReceiptId=FGuid::NewGuid();
    Receipt.ActivationId=ActivationId; Receipt.Facets=Group->Profile.Facets; Receipt.Start=Request.Start; Receipt.AdmittedEnd=Request.End;
    Receipt.ActualEnd=WindowAvatar->GetActorLocation(); PendingUtilityMovement=Receipt; bUtilityMovementStarted=false;
    if (!bStillAdmitted) { FinishUtilityMovement(EMythicAbilityMovementOutcome::StartFailed); return false; }
    PreviousMovementLocation=Receipt.Start; MovementLineage=CaptureSignalLineage();
    MovementTask=UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(this,TEXT("UtilityEscape"),Request.End,Request.Transit,
        false,MOVE_Walking,false,nullptr,ERootMotionFinishVelocityMode::ClampVelocity,FVector::ZeroVector,0);
    if (!MovementTask) { FinishUtilityMovement(EMythicAbilityMovementOutcome::StartFailed); return false; }
    MovementTask->OnTimedOut.AddDynamic(this,&ThisClass::HandleUtilityMovementTimedOut);
    MovementTask->OnTimedOutAndDestinationReached.AddDynamic(this,&ThisClass::HandleUtilityMovementReached);
    const FGuid Serial=ActivationId;
    bUtilityMovementStarted=true;
    if (auto* ASC=Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo())) ASC->OnAbilityMovementStarted.Broadcast(this,Receipt.Start);
    if (Serial!=ActivationId || !IsWindowAuthority() || bEnding || !MovementTask || !PendingUtilityMovement.IsSet()) return false;
    MovementTask->ReadyForActivation();
    if (Serial==ActivationId && IsWindowAuthority() && MovementTask && PendingUtilityMovement.IsSet())
        GetWorld()->GetTimerManager().SetTimer(MovementSampleTimer,this,&ThisClass::SampleMovement,0.03f,true);
    return true;
}

void UMythicGA_AbilityWindow::HandleUtilityMovementReached() {
    const bool Reached=PendingUtilityMovement.IsSet() && WindowAvatar.IsValid()
        && WindowAvatar->GetActorLocation().Equals(PendingUtilityMovement->AdmittedEnd,20.0);
    FinishUtilityMovement(Reached ? EMythicAbilityMovementOutcome::Reached : EMythicAbilityMovementOutcome::Blocked);
}
void UMythicGA_AbilityWindow::HandleUtilityMovementTimedOut() { FinishUtilityMovement(EMythicAbilityMovementOutcome::Blocked); }

void UMythicGA_AbilityWindow::FinishUtilityMovement(EMythicAbilityMovementOutcome Outcome) {
    if (!PendingUtilityMovement.IsSet()) return;
    TGuardValue<int32> NativeCallbacks(UtilityCallbackDepth,UtilityCallbackDepth+1);
    auto Receipt=PendingUtilityMovement.GetValue(); PendingUtilityMovement.Reset(); // Claim before all callbacks.
    const bool bStarted=bUtilityMovementStarted; bUtilityMovementStarted=false;
    auto* Task=MovementTask.Get(); MovementTask=nullptr;
    if (Task) { Task->OnTimedOut.RemoveAll(this); Task->OnTimedOutAndDestinationReached.RemoveAll(this); Task->EndTask(); }
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(MovementSampleTimer);
    if (WindowAvatar.IsValid()) Receipt.ActualEnd=WindowAvatar->GetActorLocation();
    Receipt.Outcome=Outcome;
    if (bStarted) if (auto* ASC=Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo())) ASC->OnAbilityMovementEnded.Broadcast(this,Receipt.ActualEnd);
    if (Receipt.ActivationId==ActivationId) {
        OnUtilityMovementResolved.Broadcast(Receipt);
        OnUtilityMovementFinished(Receipt);
    }
}

bool UMythicGA_AbilityWindow::ValidateAbilityEscapePath(FVector Direction,float Distance,TArray<FVector>& Points) const {
    Points.Reset();
    auto* Character=Cast<ACharacter>(WindowAvatar.Get());
    if (!Character || Character->GetWorld()!=GetWorld() || !CheckTargetIncarnation(Character,false)) return false;
    FName StopReason;
    FHitResult StoppingHit;
    return QueryAbilityEscapePath(Character,Direction,Distance,Points,StopReason,StoppingHit);
}

void UMythicGA_AbilityWindow::InspectAbilityEscapePath(ACharacter* Character,FVector Direction,float Distance,
    bool& bAdmitted,TArray<FVector>& Points,FName& StopReason,FHitResult& StoppingHit) {
    // A bool return with output parameters becomes tuple-or-None in Python,
    // discarding the obstruction on failure. Keep admission an explicit output.
    bAdmitted=QueryAbilityEscapePath(Character,Direction,Distance,Points,StopReason,StoppingHit);
}

bool UMythicGA_AbilityWindow::QueryAbilityEscapePath(ACharacter* Character,FVector Direction,float Distance,
    TArray<FVector>& Points,FName& StopReason,FHitResult& StoppingHit) {
    Points.Reset();
    StopReason=TEXT("Complete");
    StoppingHit=FHitResult();
    const UWorld* World=IsValid(Character) ? Character->GetWorld() : nullptr;
    const auto* Move=Character ? Character->GetCharacterMovement() : nullptr;
    const auto* Capsule=Character ? Character->GetCapsuleComponent() : nullptr;
    if (!World || !Move || !Capsule) { StopReason=TEXT("InvalidCharacter"); return false; }
    if (Move->UpdatedComponent!=Capsule || !Capsule->IsRegistered() || !Capsule->IsQueryCollisionEnabled()) {
        StopReason=TEXT("UnavailableMovementCapsule"); return false;
    }
    if (!Move->IsMovingOnGround()) { StopReason=TEXT("NotGrounded"); return false; }
    if (Direction.ContainsNaN()) { StopReason=TEXT("InvalidDirection"); return false; }
    if (!FMath::IsFinite(Distance) || Distance<40 || Distance>600) { StopReason=TEXT("InvalidDistance"); return false; }
    Direction.Z=0; Direction=Direction.GetSafeNormal();
    if (Direction.IsNearlyZero()) { StopReason=TEXT("InvalidDirection"); return false; }
    const FVector Start=Character->GetActorLocation();
    if (Start.ContainsNaN()) { StopReason=TEXT("InvalidOrigin"); return false; }
    FVector Previous=Start;
    float Radius=0,HalfHeight=0; Capsule->GetScaledCapsuleSize(Radius,HalfHeight);
    if (!FMath::IsFinite(Radius) || !FMath::IsFinite(HalfHeight) || Radius<=0 || HalfHeight<Radius) {
        StopReason=TEXT("InvalidCapsuleSize"); return false;
    }
    const auto CapsuleShape=FCollisionShape::MakeCapsule(Radius,HalfHeight);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(AbilityGroundEscape),false,Character);
    FCollisionResponseParams Responses;
    // Admission and actual CharacterMovement must respect the same authored
    // collision policy, including component ignores and complex movement traces.
    Capsule->InitSweepCollisionParams(Query,Responses);
    const float StepHeight=FMath::Clamp(Move->MaxStepHeight,0.0f,100.0f);
    const int32 Steps=FMath::CeilToInt(Distance/40.0f);
    Points.Reserve(Steps);
    for (int32 Index=1;Index<=Steps;++Index) {
        FVector Candidate=Start+Direction*FMath::Min(Distance,Index*40.0f);
        // Probe from above the previous floor so even a shallow uphill does not
        // begin inside the ground. Use the capsule's floor distance, not the hit
        // point's Z: contact can be off-center on a slope.
        Candidate.Z=Previous.Z+StepHeight;
        FFindFloorResult Floor; Move->ComputeFloorDist(Candidate,2*StepHeight+6,2*StepHeight+6,Floor,Radius);
        if (!Floor.IsWalkableFloor() || Floor.HitResult.bStartPenetrating) {
            StopReason=Floor.HitResult.bStartPenetrating ? TEXT("FloorPenetration") : TEXT("NoWalkableFloor");
            StoppingHit=Floor.HitResult; break;
        }
        Candidate.Z-=Floor.bLineTrace ? Floor.LineDist : Floor.FloorDist;
        Candidate.Z+=2.0f;
        if (!FMath::IsFinite(Candidate.Z) || FMath::Abs(Candidate.Z-Previous.Z)>StepHeight+2.0f) {
            StopReason=TEXT("FloorStepExceeded"); StoppingHit=Floor.HitResult; break;
        }
        FHitResult Hit;
        if (World->SweepSingleByChannel(Hit,Previous,Candidate,Capsule->GetComponentQuat(),Capsule->GetCollisionObjectType(),CapsuleShape,Query,Responses)) {
            StopReason=TEXT("CorridorBlocked"); StoppingHit=Hit; break;
        }
        Points.Add(Candidate); Previous=Candidate;
    }
    if (Points.IsEmpty() || FVector::DistSquared2D(Start,Points.Last())<FMath::Square(40.0f)) {
        if (StopReason==TEXT("Complete")) StopReason=TEXT("InsufficientGroundDistance");
        Points.Reset(); return false;
    }
    return true;
}
