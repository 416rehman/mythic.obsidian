#include "MythicGA_AbilityWindow.h"
#include "Input/MythicCombatUtilityComponent.h"
#include "MythicAbilityIntentTargetData.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "AIController.h"
#include "AI/Party/PartySubsystem.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicAbilityIntentComponent.h"
#include "Components/Navigation/MythicNavigationComponent.h"
#include "Progression/Skills/MythicSkillComponent.h"
#include "Rewards/LootScaling.h"
#include "Settings/MythicCombatSettings.h"
#include "Settings/MythicDeveloperSettings.h"
#include "World/Harvesting/MythicHarvestClaimMembershipSubsystem.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "TimerManager.h"
#include "Misc/ScopeExit.h"

namespace {
UAbilitySystemComponent* AbilityASC(AActor* Actor) { return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor); }
AMythicPlayerState* AbilityPlayer(AActor* Actor) {
    if (auto* PS=Cast<AMythicPlayerState>(Actor)) return PS;
    const auto* Pawn=Cast<APawn>(Actor);
    return Pawn ? Pawn->GetPlayerState<AMythicPlayerState>() : nullptr;
}
bool AbilityParty(AActor* Source,AActor* Target) {
    if (Source==Target) return true;
    const auto* Owner=AbilityPlayer(Source); if (!Owner || !Source->GetWorld()) return false;
    if (const auto* Other=AbilityPlayer(Target)) {
        const auto* Membership=Source->GetWorld()->GetSubsystem<UMythicHarvestClaimMembershipSubsystem>();
        FGuid A,B;
        return Membership && Membership->TryResolveParty(Owner->GetCanonicalPlayerKey(),A) && Membership->TryResolveParty(Other->GetCanonicalPlayerKey(),B) && A==B;
    }
    if (const auto* Parties=Source->GetWorld()->GetSubsystem<UMythicPartySubsystem>()) {
        TArray<FMythicPartyMember> Members; Parties->GetPartyMembers(Owner->GetCanonicalPlayerKey(),Members);
        for (const auto& Member:Members) if (Member.NPCActor.Get()==Target) return true;
    }
    return false;
}
bool AbilityHostile(AActor* Source,AActor* Target) {
    if (!Source || !Target || Source==Target || AbilityParty(Source,Target)) return false;
    const auto* TargetPawn=Cast<APawn>(Target);
    if (const auto* AI=TargetPawn ? Cast<AAIController>(TargetPawn->GetController()) : nullptr)
        return AI->GetTeamAttitudeTowards(*Source)==ETeamAttitude::Hostile;
    if (AbilityPlayer(Source) && AbilityPlayer(Target)) return GetDefault<UMythicDeveloperSettings>()->bFriendlyFireEnabled;
    const auto* Pawn=Cast<APawn>(Source);
    const auto* AI=Pawn ? Cast<AAIController>(Pawn->GetController()) : nullptr;
    return AI && AI->GetTeamAttitudeTowards(*Target)==ETeamAttitude::Hostile;
}
bool AbilityLOS(const UWorld* World,FVector From,AActor* Target,AActor* Ignore) {
    if (!World || !Target) return false;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(AbilityLOS),false,Ignore);
    const FVector Destination=Target->GetActorLocation()+FVector(0,0,40);
    return !World->LineTraceSingleByChannel(Hit,From+FVector(0,0,40),Destination,ECC_Visibility,Params) || Hit.GetActor()==Target;
}
UMythicStatusRegistry* AbilityStatuses(const UObject* Context) {
    const auto* GI=Context && Context->GetWorld() ? Context->GetWorld()->GetGameInstance() : nullptr;
    return GI ? GI->GetSubsystem<UMythicStatusRegistry>() : nullptr;
}
FGameplayTag AbilityParameter(const TCHAR* Key) { return FGameplayTag::RequestGameplayTag(FName(Key),false); }
}

UMythicGA_AbilityWindow::UMythicGA_AbilityWindow() {
    bCancelOnInputInterrupted=true;
    NetExecutionPolicy=EGameplayAbilityNetExecutionPolicy::LocalPredicted;
    NetSecurityPolicy=EGameplayAbilityNetSecurityPolicy::ServerOnlyTermination;
}
bool UMythicGA_AbilityWindow::IsWindowAuthority() const {
    return bCommitted && !bCleaned && CurrentActorInfo && CurrentActorInfo->IsNetAuthority()
        && WindowAvatar.IsValid() && WindowAvatar.Get()==GetAvatarActorFromActorInfo() && CheckTargetIncarnation(WindowAvatar.Get());
}
bool UMythicGA_AbilityWindow::CheckTargetIncarnation(AActor* Actor,bool bRetain) const {
    if (!IsValid(Actor) || InvalidatedActors.Contains(Actor)) return false;
    if (const auto* Retained=RetainedActors.Find(Actor)) return Retained->Presentation.IsValid() && Retained->Presentation->RepresentsInstance(Retained->Instance);
    auto* Presentation=Actor->FindComponentByClass<UMythicEntityPresentationComponent>();
    if (!Presentation) return true;
    const auto Instance=Presentation->GetPresentationInstance(); if (!Instance.IsValid()) return false;
    if (!bRetain) return true;
    if (RetainedActors.Num()>=256) return false;
    FRetainedActor Record; Record.Presentation=Presentation; Record.Instance=Instance;
    auto* Mutable=const_cast<UMythicGA_AbilityWindow*>(this);
    Record.Deactivated=Presentation->OnPresentationDeactivated.AddWeakLambda(Mutable,[Mutable,Target=TWeakObjectPtr<AActor>(Actor)](UMythicEntityPresentationComponent&,const FMythicEntityPresentationInstance&) {
        Mutable->InvalidatedActors.Add(Target);
        if (Target==Mutable->WindowAvatar) { if (!Mutable->bCleaned) Mutable->EndAbility(Mutable->CurrentSpecHandle,Mutable->CurrentActorInfo,Mutable->CurrentActivationInfo,true,true); return; }
        FMythicAbilitySignal Signal; Signal.Actor=Target.Get(); Signal.Target=Target.Get(); Mutable->DispatchSignal(TEXT("Target.Invalidated"),Signal);
    });
    RetainedActors.Add(Actor,Record); return true;
}
void UMythicGA_AbilityWindow::ResolveSnapshot() {
    Numbers.Reset(); Parameters.Reset(); ActionProfiles.Reset(); ActionGroups.Reset(); ActionSource.Reset(); ActionSources.Reset(); Flags.Reset(); Classification.Reset();
    const auto* Definition=GetSkillDefinition(); const auto* Skills=GetSkillComponent();
    if (!Definition || !Skills) return;
    Flags=Definition->BehaviorFlags;
    Classification=Skills->GetEffectiveClassification(Definition);
    for (const auto& N:Definition->Numbers) {
        double Value=N.Value;
        for (int32 Index:Skills->GetActiveModifiers(Definition)) Value+=Definition->Modifiers[Index].NumberDeltas.FindRef(N.Key);
        Numbers.Add(N.Key,FMath::IsFinite(Value) ? FMath::Clamp(static_cast<float>(Value),N.Minimum,N.Maximum) : N.Value);
    }
    for (const auto& Parameter:Definition->Parameters) {
        double Value=Parameter.Value;
        for (int32 Index:Skills->GetActiveModifiers(Definition)) Value+=Definition->Modifiers[Index].ParameterDeltas.FindRef(Parameter.Name);
        Parameters.Add(Parameter.Name,FMath::IsFinite(Value) ? FMath::Clamp(static_cast<float>(Value),Parameter.Minimum,Parameter.Maximum) : Parameter.Value);
    }
    for (const auto& Action:Definition->Actions) ActionProfiles.Add(Action.ActionId,Action);
    TSet<FName> Overridden;
    for (int32 Index:Skills->GetActiveModifiers(Definition)) {
        const auto& Modifier=Definition->Modifiers[Index]; Flags.AppendTags(Modifier.BehaviorFlags);
        for (const auto& Action:Modifier.ActionOverrides) {
            // Conflicting action rewrites fail closed, independent of authored row order.
            if (Overridden.Contains(Action.ActionId)) ActionProfiles.Remove(Action.ActionId);
            else if (ActionProfiles.Contains(Action.ActionId)) ActionProfiles[Action.ActionId]=Action;
            Overridden.Add(Action.ActionId);
        }
    }
}
float UMythicGA_AbilityWindow::GetAbilityNumber(FGameplayTag Key,float Fallback) const {
    if (const float* Number=Numbers.Find(Key)) return *Number;
    return FMath::IsFinite(Fallback) ? Fallback : 0;
}
bool UMythicGA_AbilityWindow::HasAbilityFlag(FGameplayTag Tag) const { return Tag.IsValid() && Flags.HasTagExact(Tag); }
FVector UMythicGA_AbilityWindow::GetAimDirection() const {
    if (!ActivationAim.IsNearlyZero()) return ActivationAim;
    const auto* Pawn=Cast<APawn>(WindowAvatar.Get());
    return Pawn ? Pawn->GetBaseAimRotation().Vector().GetSafeNormal() : FVector::ForwardVector;
}
FVector UMythicGA_AbilityWindow::GetMovementIntentDirection(bool bBackwardWhenNeutral) const {
    return !ActivationMovement.IsNearlyZero() ? ActivationMovement : ActivationFacing*(bBackwardWhenNeutral ? -1.0 : 1.0);
}
float UMythicGA_AbilityWindow::GetWindowElapsed() const { return GetWorld() ? FMath::Max(0.0,GetWorld()->GetTimeSeconds()-StartTime) : 0; }
bool UMythicGA_AbilityWindow::ResolveIntent() {
    AActor* Avatar=WindowAvatar.Get(); if (!Avatar || !GetWorld()) return false;
    Origin=Avatar->GetActorLocation(); IntentTarget=Avatar;
    EMythicAbilityIntent Intent=InitialIntent;
    for (const auto& Override:IntentOverrides) if (HasAbilityFlag(Override.Key)) { Intent=Override.Value; break; }
    const float Range=FMath::Clamp(GetAbilityParameter(TEXT("Range"),GetAbilityNumber(AbilityParameter(TEXT("Ability.Parameter.Range")),IntentRange)),1.0f,3000.0f);
    if (Intent==EMythicAbilityIntent::Self) return true;
    if (Intent==EMythicAbilityIntent::Hostile) return ResolveHostileIntent(Range);
    if (Intent==EMythicAbilityIntent::Party) {
        FMythicAbilityQuery Query; Query.Radius=Range; Query.MaxTargets=32; Query.AngleDegrees=60; Query.bParty=Intent==EMythicAbilityIntent::Party;
        auto Found=QueryAbilityTargets(Query,Origin,GetAimDirection());
        Found.Remove(Avatar);
        Found.RemoveAll([Avatar,Range](AActor* Target) { return FVector::DistSquared(Avatar->GetActorLocation(),Target->GetActorLocation())>FMath::Square(Range); });
        if (Found.IsEmpty()) { if (Intent==EMythicAbilityIntent::Party) return true; return false; }
        IntentTarget=Found[0]; return true;
    }
    FVector Eye; FRotator Rotation; Avatar->GetActorEyesViewPoint(Eye,Rotation);
    const FVector End=Eye+GetAimDirection()*Range;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(AbilityIntent),false,Avatar); FHitResult Hit;
    const bool HitView=GetWorld()->LineTraceSingleByChannel(Hit,Eye,End,ECC_Visibility,Params);
    const FVector Point=HitView ? Hit.ImpactPoint : End;
    if (!GetWorld()->LineTraceSingleByChannel(Hit,Point+FVector(0,0,150),Point-FVector(0,0,1500),ECC_WorldStatic,Params)) return false;
    if (FVector::DistSquared(Avatar->GetActorLocation(),Hit.ImpactPoint)>FMath::Square(Range+200)) return false;
    Origin=Hit.ImpactPoint; IntentTarget.Reset(); return true;
}
bool UMythicGA_AbilityWindow::ValidateAbilityIntent_Implementation() { return true; }
bool UMythicGA_AbilityWindow::ValidateAbilityGroundPoint(FVector Point,FVector& GroundPoint) const {
    GroundPoint=FVector::ZeroVector;
    AActor* Avatar=WindowAvatar.Get();
    if (!Avatar || !GetWorld() || Point.ContainsNaN()) return false;
    const float Range=FMath::Clamp(GetAbilityParameter(TEXT("Range"),GetAbilityNumber(AbilityParameter(TEXT("Ability.Parameter.Range")),IntentRange)),1.0f,3000.0f);
    if (FVector::DistSquared(Avatar->GetActorLocation(),Point)>FMath::Square(Range+200)) return false;
    FHitResult Ground; FCollisionQueryParams Params(SCENE_QUERY_STAT(AbilityGroundValidation),false,Avatar);
    if (!GetWorld()->LineTraceSingleByChannel(Ground,Point+FVector(0,0,150),Point-FVector(0,0,350),ECC_WorldStatic,Params) || Ground.ImpactNormal.Z<0.5f) return false;
    FHitResult Obstacle;
    GroundPoint=Ground.ImpactPoint;
    return !GetWorld()->LineTraceSingleByChannel(Obstacle,Avatar->GetActorLocation(),Ground.ImpactPoint+FVector(0,0,20),ECC_Visibility,Params)
        || FVector::DistSquared(Obstacle.ImpactPoint,Ground.ImpactPoint)<FMath::Square(50.0f);
}
bool UMythicGA_AbilityWindow::CheckCooldown(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayTagContainer* Tags) const {
    const auto* Definition=Cast<UMythicSkillDefinition>(GetSourceObject(Handle,Info));
    const auto* PS=Info ? Cast<AMythicPlayerState>(Info->OwnerActor.Get()) : nullptr;
    const auto* Skills=PS ? PS->GetSkillComponent() : nullptr;
    return Skills && Definition && Skills->IsReady() && Skills->IsSkillUnlocked(Definition)
        && (!Definition->HasTimedCooldown() || (Skills->GetCooldownRemaining(Definition)<=0 && Super::CheckCooldown(Handle,Info,Tags)));
}
void UMythicGA_AbilityWindow::ApplyCooldown(FGameplayAbilitySpecHandle,const FGameplayAbilityActorInfo* Info,FGameplayAbilityActivationInfo) const {
    if (!Info || !Info->IsNetAuthority()) return;
    auto* Definition=GetSkillDefinition(); auto* Skills=GetSkillComponent(); if (!Definition || !Skills || !Definition->HasTimedCooldown()) return;
    const auto* Utility=Info->AbilitySystemComponent->GetSet<UMythicAttributeSet_Utility>();
    const float Maximum=FMath::Clamp(GetDefault<UMythicCombatSettings>()->MaxCooldownReduction,0.0f,1.0f);
    const float Reduction=Utility ? FMath::Clamp(Utility->GetCooldownReduction(),0.0f,Maximum) : 0;
    const float Delta=GetAbilityNumber(AbilityParameter(TEXT("Ability.Parameter.CooldownDelta")),0);
    Skills->StartCooldown(Definition,FMath::Clamp((Definition->CooldownSeconds+Delta)*(1-Reduction),0.1f,3600.0f));
}
void UMythicGA_AbilityWindow::ActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayAbilityActivationInfo Activation,const FGameplayEventData* Event) {
    UMythicGameplayAbility::ActivateAbility(Handle,Info,Activation,Event);
    bCleaned=false; bEnding=false; bCommitted=false; bSequenceStarted=false; bLifecycleEnded=false; ActivationId=FGuid::NewGuid();
    ActivationAim=FVector::ZeroVector;
    ActivationMovement=FVector::ZeroVector; ActivationFacing=FVector::ForwardVector;
    WindowAvatar=Info ? Info->AvatarActor.Get() : nullptr;
    StartTime=GetWorld() ? GetWorld()->GetTimeSeconds() : 0;
    ScheduledSignals=DispatchCount=CurrentGeneration=0; Visited.Reset(); LootReceipts.Reset();
    bInsideReaction=false; bHasReactionLineage=true; MovementLineage={};
    RemainingBudgets.Reset(); for (const auto& Budget:Budgets) RemainingBudgets.Add(Budget.Key,FMath::Clamp(Budget.Value,0,256));
    ResolveSnapshot();
    if (!Info || !WindowAvatar.IsValid()) { EndAbility(Handle,Info,Activation,true,true); return; }
    Origin=WindowAvatar->GetActorLocation();
    ActivationFacing=WindowAvatar->GetActorForwardVector().GetSafeNormal2D();
    if (Info->IsLocallyControlled()) {
        if (const auto* Pawn=Cast<APawn>(WindowAvatar.Get())) {
            // Player activation runs after this frame's Enhanced Input callbacks. LastMovementInput
            // would replay the prior frame on release, so only the pending human command is eligible.
            if (Info->PlayerController.IsValid()) {
                ActivationMovement=Pawn->GetPendingMovementInputVector().GetSafeNormal2D();
                if (ActivationMovement.IsNearlyZero()) if (const auto* Navigation=Info->PlayerController->FindComponentByClass<UMythicNavigationComponent>())
                    Navigation->TryGetCurrentCommandDirection(Pawn,ActivationMovement);
            }
            else if (const auto* Character=Cast<ACharacter>(Pawn)) ActivationMovement=Character->GetCharacterMovement()->GetCurrentAcceleration().GetSafeNormal2D();
        }
        if (auto* PC=Cast<AMythicPlayerController>(Info->PlayerController.Get()); bRequiresCameraAim && PC && PC->GetLocalPlayer()) {
            const float Range=FMath::Clamp(GetAbilityParameter(TEXT("Range"),GetAbilityNumber(AbilityParameter(TEXT("Ability.Parameter.Range")),IntentRange)),1.0f,3000.0f);
            if (!PC->AbilityIntentComponent || !PC->AbilityIntentComponent->TryCaptureAimDirection(Range,ActivationAim)) {
                EndAbility(Handle,Info,Activation,true,true); return;
            }
            // A lock nominates only hostile intent and must still fit the current camera policy and fresh LOS.
            // Ground/self/movement intent remains unchanged; authority independently resolves the transmitted ray.
            EMythicAbilityIntent AimIntent=InitialIntent;
            for (const auto& Override:IntentOverrides) if (HasAbilityFlag(Override.Key)) { AimIntent=Override.Value; break; }
            if (AimIntent==EMythicAbilityIntent::Hostile) if (auto* Utility=PC->FindComponentByClass<UMythicCombatUtilityComponent>()) {
                FVector LockedAim;
                if (Utility->TryGetLockedAimDirection(Range,LockedAim)) ActivationAim=LockedAim;
            }
        } else ActivationAim=GetAimDirection(); // Authority AI and isolated headless fixtures have no local camera.
    }
    auto* ASC=Info->AbilitySystemComponent.Get();
    if (!Info->IsNetAuthority()) {
        FScopedPredictionWindow Prediction(ASC,true);
        auto* Aim=new FMythicAbilityIntentTargetData();
        Aim->AimDirection=ActivationAim; Aim->MovementDirection=ActivationMovement;
        FGameplayAbilityTargetDataHandle Data; Data.Add(Aim);
        ASC->CallServerSetReplicatedTargetData(Handle,Activation.GetActivationPredictionKey(),Data,FGameplayTag(),ASC->ScopedPredictionKey);
        ArmRecast();
        if (WindupSeconds>0) { FTimerHandle Timer; GetWorld()->GetTimerManager().SetTimer(Timer,this,&ThisClass::BeginPredictedSequence,FMath::Clamp(WindupSeconds,0.01f,5.0f),false); Timers.Add(Timer); }
        else BeginPredictedSequence();
        return;
    }
    if (!Info->IsLocallyControlled() && Info->PlayerController.IsValid()) {
        AimDataListener=ASC->AbilityTargetDataSetDelegate(Handle,Activation.GetActivationPredictionKey()).AddUObject(this,&ThisClass::ReceiveAimData);
        GetWorld()->GetTimerManager().SetTimer(AimDataTimeout,FTimerDelegate::CreateWeakLambda(this,[this](){ if (!bCommitted && !bCleaned) EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,true); }),2.0f,false);
        ASC->CallReplicatedTargetDataDelegatesIfSet(Handle,Activation.GetActivationPredictionKey());
        return;
    }
    BeginAuthorityWindow();
}
void UMythicGA_AbilityWindow::ReceiveAimData(const FGameplayAbilityTargetDataHandle& Data,FGameplayTag) {
    if (bCommitted || bCleaned || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority()) return;
    auto* ASC=GetAbilitySystemComponentFromActorInfo();
    ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle,CurrentActivationInfo.GetActivationPredictionKey()).Remove(AimDataListener); AimDataListener.Reset();
    GetWorld()->GetTimerManager().ClearTimer(AimDataTimeout);
    FVector Aim=FVector::ZeroVector,MoveIntent=FVector::ZeroVector; bool bValid=false;
    if (Data.Num()==1 && Data.Get(0) && Data.Get(0)->GetScriptStruct()==FMythicAbilityIntentTargetData::StaticStruct()) {
        const auto* Intent=static_cast<const FMythicAbilityIntentTargetData*>(Data.Get(0));
        bValid=Intent->HasValidDirections(); Aim=Intent->AimDirection; MoveIntent=Intent->MovementDirection;
    }
    ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle,CurrentActivationInfo.GetActivationPredictionKey());
    if (!bValid) { EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,true); return; }
    ActivationAim=Aim.GetSafeNormal(); ActivationMovement=MoveIntent.GetSafeNormal2D(); BeginAuthorityWindow();
}
void UMythicGA_AbilityWindow::BeginPredictedSequence() {
    if (bCleaned || bEnding || !CurrentActorInfo || CurrentActorInfo->IsNetAuthority() || !CurrentActorInfo->IsLocallyControlled()) return;
    TGuardValue<bool> Prediction(bPredictingMotion,true); OnPredictedWindowStarted();
}
void UMythicGA_AbilityWindow::BeginAuthorityWindow() {
    const auto Handle=CurrentSpecHandle; const auto* Info=CurrentActorInfo; const auto Activation=CurrentActivationInfo;
    const FGuid CommittingActivation=ActivationId;
    TGuardValue<int32> NativeCallbacks(UtilityCallbackDepth,UtilityCallbackDepth+1);
    auto* Skills=GetSkillComponent(); auto* Definition=GetSkillDefinition();
    if (!Skills || !Skills->IsReady() || !Skills->OwnsGrantedSpec(Definition,Handle) || !ResolveIntent() || !CaptureActionSource()
        || !ValidateAbilityIntent() || !ValidatePreparedUtilityEscapes()) {
        if (!bCleaned && CommittingActivation==ActivationId) EndAbility(Handle,Info,Activation,true,true);
        return;
    }
    // Authoring validation can synchronously cancel, unequip or replace the avatar.
    // Check the same admitted activation again before GAS is allowed to pay its costs.
    if (bCleaned || bEnding || bCommitted || CommittingActivation!=ActivationId) return;
    if (!Info || !Info->IsNetAuthority() || !WindowAvatar.IsValid() || WindowAvatar.Get()!=GetAvatarActorFromActorInfo()
        || !CheckTargetIncarnation(WindowAvatar.Get()) || !Skills->IsReady() || !Skills->OwnsGrantedSpec(Definition,Handle)
        || !CommitAbility(Handle,Info,Activation)) { EndAbility(Handle,Info,Activation,true,true); return; }
    if (bCleaned || CommittingActivation!=ActivationId) return;
    bCommitted=true;
    Skills->RecordSkillUse(Definition);
    PublishLifecycleEvent(GAS_EVENT_SKILL_BEGIN);
    if (bCleaned || bEnding) return;
    ArmRecast();
    if (bListenForLootCredits) if (auto* Loot=GetWorld()->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>())
        LootListener=Loot->OnAbilityLootCredit.AddUObject(this,&ThisClass::HandleLootCredit);
    if (CastCue.IsValid()) PlayAbilityCue(CastCue,Origin,false,nullptr);
    FTimerHandle Timeout;
    const FGuid Serial=ActivationId;
    GetWorld()->GetTimerManager().SetTimer(Timeout,FTimerDelegate::CreateWeakLambda(this,[this,Serial](){ if (Serial==ActivationId) EndAbilityWindow(EMythicAbilityWindowEnd::NaturalExpiry); }),FMath::Clamp(Duration+WindupSeconds,0.1f,120.0f),false);
    Timers.Add(Timeout);
    if (WindupSeconds>0) {
        FTimerHandle Windup;
        GetWorld()->GetTimerManager().SetTimer(Windup,FTimerDelegate::CreateWeakLambda(this,[this,Serial](){ if (Serial==ActivationId && IsWindowAuthority() && !bEnding) BeginSequence(); }),FMath::Clamp(WindupSeconds,0.01f,5.0f),false);
        Timers.Add(Windup);
    } else BeginSequence();
}
void UMythicGA_AbilityWindow::BeginSequence() { if (IsWindowAuthority() && !bEnding && !bSequenceStarted) { bSequenceStarted=true; StartTime=GetWorld()->GetTimeSeconds(); OnWindowStarted(); } }
void UMythicGA_AbilityWindow::PublishLifecycleEvent(FGameplayTag EventTag) {
    if (!IsWindowAuthority()) return;
    auto* ASC=GetAbilitySystemComponentFromActorInfo();
    FMythicAbilityQuery Query=LifecycleTargetQuery; Query.bParty=false; Query.MaxTargets=1;
    const FVector CasterLocation=WindowAvatar->GetActorLocation();
    AActor* Focus=IntentTarget.Get();
    if (!FMath::IsFinite(Query.Radius) || Query.Radius<=0 || !FMath::IsFinite(Query.AngleDegrees)) Focus=nullptr;
    else if (!IsValidAbilityTarget(Focus,false,Query.bRequireLOS)
        || FVector::DistSquared(CasterLocation,Focus->GetActorLocation())>FMath::Square(FMath::Clamp(Query.Radius,1.0f,3000.0f))) {
        const auto Targets=QueryAbilityTargets(Query,CasterLocation,GetAimDirection());
        Focus=Targets.IsEmpty() ? nullptr:Targets[0];
    }
    FGameplayEventData Event; Event.EventTag=EventTag; Event.Instigator=WindowAvatar.Get(); Event.Target=Focus;
    Event.ContextHandle=MakeEffectContext(CurrentSpecHandle,CurrentActorInfo);
    Event.InstigatorTags=Classification; Event.InstigatorTags.AppendTags(GetAssetTags());
    if (auto* TargetASC=AbilityASC(Focus)) TargetASC->GetOwnedGameplayTags(Event.TargetTags);
    ASC->HandleGameplayEvent(EventTag,&Event);
}
void UMythicGA_AbilityWindow::ArmRecast() {
    if (!bAcceptRecast || bCleaned || bEnding) return;
    auto* Wait=UAbilityTask_WaitInputPress::WaitInputPress(this,false);
    Wait->OnPress.AddDynamic(this,&ThisClass::HandleRecast); Wait->ReadyForActivation();
}
void UMythicGA_AbilityWindow::HandleRecast(float) {
    if (IsWindowAuthority() && !bEnding) DispatchSignal(TEXT("Recast"),{});
    if (!bEnding && !bCleaned) ArmRecast();
}
bool UMythicGA_AbilityWindow::ScheduleSignal(FName Name,float Delay) {
    if (!IsWindowAuthority() || bEnding || Name.IsNone() || !FMath::IsFinite(Delay) || Delay<0 || Delay>120 || ScheduledSignals>=64) return false;
    ++ScheduledSignals;
    FTimerHandle Timer; const FGuid Serial=ActivationId;
    const FMythicAbilitySignal Deferred=CaptureSignalLineage();
    GetWorld()->GetTimerManager().SetTimer(Timer,FTimerDelegate::CreateWeakLambda(this,[this,Name,Serial,Deferred](){ if (Serial==ActivationId) DispatchSignal(Name,Deferred); }),FMath::Max(0.001f,Delay),false);
    Timers.Add(Timer); return true;
}
FMythicAbilitySignal UMythicGA_AbilityWindow::CaptureSignalLineage() const {
    FMythicAbilitySignal Signal; Signal.Generation=CurrentGeneration; Signal.bReaction=bInsideReaction; Signal.bHasReactionLineage=bHasReactionLineage;
    return Signal;
}
void UMythicGA_AbilityWindow::DispatchSignal(FName Name,FMythicAbilitySignal Signal) {
    if (!IsWindowAuthority() || !bSequenceStarted || bEnding || ++DispatchCount>512 || Signal.Generation>4) return;
    Signal.Name=Name;
    const FGuid Serial=ActivationId; const auto Previous=CaptureSignalLineage();
    CurrentGeneration=Signal.Generation; bInsideReaction=Signal.bReaction; bHasReactionLineage=Signal.bHasReactionLineage;
    ON_SCOPE_EXIT { if (Serial==ActivationId) { CurrentGeneration=Previous.Generation; bInsideReaction=Previous.bReaction; bHasReactionLineage=Previous.bHasReactionLineage; } };
    OnWindowSignal(Name,Signal);
}
bool UMythicGA_AbilityWindow::TrySpendAbilityBudget(FName Key,int32 Count) {
    if (!IsWindowAuthority() || Count<=0) return false;
    int32* Remaining=RemainingBudgets.Find(Key);
    if (!Remaining || *Remaining<Count) return false;
    *Remaining-=Count; return true;
}
bool UMythicGA_AbilityWindow::TryVisitAbilityTarget(FName Key,AActor* Target,int32 Maximum) {
    if (!IsWindowAuthority() || !CheckTargetIncarnation(Target) || Key.IsNone() || (Visited.Num()>=16 && !Visited.Contains(Key))) return false;
    auto& Set=Visited.FindOrAdd(Key);
    if (Set.Contains(Target) || Set.Num()>=FMath::Clamp(Maximum,1,32)) return false;
    Set.Add(Target); return true;
}
void UMythicGA_AbilityWindow::ClearAbilityVisitedSet(FName Key) { if (IsWindowAuthority()) Visited.Remove(Key); }
float UMythicGA_AbilityWindow::GetAbilityMaxHealth(AActor* Target) const { const auto* ASC=CheckTargetIncarnation(Target) ? AbilityASC(Target) : nullptr; const auto* Life=ASC ? ASC->GetSet<UMythicAttributeSet_Life>() : nullptr; return Life ? Life->GetMaxHealth() : 0; }
float UMythicGA_AbilityWindow::GetAbilityHealthFraction(AActor* Target) const { const auto* ASC=CheckTargetIncarnation(Target) ? AbilityASC(Target) : nullptr; const auto* Life=ASC ? ASC->GetSet<UMythicAttributeSet_Life>() : nullptr; return Life && Life->GetMaxHealth()>0 ? FMath::Clamp(Life->GetHealth()/Life->GetMaxHealth(),0.0f,1.0f) : 1; }
bool UMythicGA_AbilityWindow::IsValidAbilityTarget(AActor* Target,bool bParty,bool bRequireLOS) const {
    return IsEligibleAbilityTarget(Target,bParty,bRequireLOS,true);
}
bool UMythicGA_AbilityWindow::IsEligibleAbilityTarget(AActor* Target,bool bParty,bool bRequireLOS,bool bRetain) const {
    if (!CheckTargetIncarnation(Target,bRetain)) return false;
    AActor* Avatar=WindowAvatar.Get();
    if (!IsValid(Target) || !Avatar || Target->GetWorld()!=GetWorld() || Target->IsActorBeingDestroyed()) return false;
    const auto* ASC=AbilityASC(Target); const auto* Life=UMythicLifeComponent::FindHealthComponent(Target);
    const auto* Vitals=ASC ? ASC->GetSet<UMythicAttributeSet_Life>() : nullptr;
    if (!ASC || ASC->GetAvatarActor()!=Target || !Life || Life->IsDead() || Life->IsDowned() || !Vitals || Vitals->GetHealth()<=0) return false;
    if (!(bParty ? AbilityParty(Avatar,Target) : AbilityHostile(Avatar,Target))) return false;
    return !bRequireLOS || AbilityLOS(GetWorld(),Avatar->GetActorLocation(),Target,Avatar);
}
TArray<AActor*> UMythicGA_AbilityWindow::QueryAbilityTargets(FMythicAbilityQuery Query,FVector QueryOrigin,FVector Direction) const {
    TArray<AActor*> Result;
    if (!CurrentActorInfo || !CurrentActorInfo->IsNetAuthority() || !WindowAvatar.IsValid() || QueryOrigin.ContainsNaN() || Direction.ContainsNaN()
        || !FMath::IsFinite(Query.Radius) || Query.Radius<=0 || !FMath::IsFinite(Query.AngleDegrees) || Query.MaxTargets<=0) return Result;
    if (FVector::DistSquared(QueryOrigin,Origin)>FMath::Square(6000.0f)) return Result;
    float Radius=FMath::Clamp(Query.Radius,1.0f,3000.0f);
    int32 Cap=FMath::Clamp(Query.MaxTargets,1,32);
    // These are authored hard bounds shared by the phase and its tell. A graph may explicitly
    // resolve a build bonus before querying only when it also updates that phase's presentation.
    TArray<FOverlapResult> Found;
    FCollisionObjectQueryParams Objects(ECC_Pawn);
    GetWorld()->OverlapMultiByObjectType(Found,QueryOrigin,FQuat::Identity,Objects,FCollisionShape::MakeSphere(Radius));
    Direction=Direction.GetSafeNormal();
    const float Cosine=FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(Query.AngleDegrees,0.0f,360.0f)*0.5f));
    const auto HealthFraction=[](const AActor& Actor) {
        const auto* ASC=AbilityASC(const_cast<AActor*>(&Actor)); const auto* Vitals=ASC ? ASC->GetSet<UMythicAttributeSet_Life>() : nullptr;
        return Vitals && Vitals->GetMaxHealth()>0 ? Vitals->GetHealth()/Vitals->GetMaxHealth() : 1;
    };
    const auto Before=[this,&Query,QueryOrigin,&HealthFraction](const AActor& A,const AActor& B) {
        if (Query.bPrioritizeSelf && (&A==WindowAvatar.Get() || &B==WindowAvatar.Get())) return &A==WindowAvatar.Get();
        if (Query.bLowestHealthFirst) { const float AH=HealthFraction(A),BH=HealthFraction(B); if (AH!=BH) return AH<BH; }
        const double AD=FVector::DistSquared(A.GetActorLocation(),QueryOrigin),BD=FVector::DistSquared(B.GetActorLocation(),QueryOrigin);
        return AD==BD ? A.GetFName().LexicalLess(B.GetFName()) : AD<BD;
    };
    for (const auto& Hit:Found) {
        AActor* Target=Hit.GetActor();
        if (!IsEligibleAbilityTarget(Target,Query.bParty,false,false) || Result.Contains(Target)) continue;
        const FVector Delta=Target->GetActorLocation()-QueryOrigin;
        const FVector ArcDelta=Query.bHorizontalArc ? FVector(Delta.X,Delta.Y,0) : Delta;
        const FVector ArcDirection=Query.bHorizontalArc ? FVector(Direction.X,Direction.Y,0).GetSafeNormal() : Direction;
        if (Delta.SizeSquared()>FMath::Square(Radius) || (Query.AngleDegrees<360 && !ArcDirection.IsNearlyZero() && !ArcDelta.IsNearlyZero() && FVector::DotProduct(ArcDelta.GetSafeNormal(),ArcDirection)<Cosine)) continue;
        if (Query.bRequireLOS && !AbilityLOS(GetWorld(),QueryOrigin,Target,WindowAvatar.Get())) continue;
        const int32 Insert=Result.IndexOfByPredicate([&](AActor* Other){ return Before(*Target,*Other); });
        if (Insert!=INDEX_NONE) { Result.Insert(Target,Insert); if (Result.Num()>Cap) Result.Pop(EAllowShrinking::No); }
        else if (Result.Num()<Cap) Result.Add(Target);
    }
    // Retain only the bounded selected targets; a crowded overlap cannot consume the lifetime's identity budget.
    Result.RemoveAll([this](AActor* Target){ return !CheckTargetIncarnation(Target); });
    return Result;
}
void UMythicGA_AbilityWindow::ApplyAbilityPayload(AActor* Target,FName Row,float Scalar) {
    const auto* Payload=Payloads.Find(Row);
    if (!IsWindowAuthority() || !Payload || !IsValidAbilityTarget(Target,false,false) || !FMath::IsFinite(Scalar) || Scalar<0 || CurrentGeneration>=4
        || FVector::DistSquared(Target->GetActorLocation(),Origin)>FMath::Square(6000.0f)) return;
    if (Payload->Damage.DamageCalculationEffect && Payload->Damage.DamageApplicationEffect && Scalar>0) {
        auto Spec=MakeDamageContainerSpec(Payload->Damage); MarkSpecAsSkill(Spec);
        if (auto* Context=FMythicGameplayEffectContext::ExtractEffectContext(Spec.EffectContextHandle)) { Context->SetBonusDamageMultiplier(FMath::Clamp(Scalar,0.0f,20.0f)); Context->ReactionGeneration=CurrentGeneration+1; }
        AddTargetsToDamageContainerSpec(Spec,{},TArray<AActor*>{Target}); ApplyDamageContainerSpec(Spec);
    }
    if (Payload->Status.IsValid() && IsValidAbilityTarget(Target,false,false)) {
        FHitResult Hit; Hit.HitObjectHandle=FActorInstanceHandle(Target);
        ApplyStatus(TArray<FHitResult>{Hit},WindowAvatar.Get(),Payload->Status,FMath::Clamp(Payload->Chance,0.0f,1.0f),Payload->StatusDuration);
    }
}
FMythicAbilityHealResult UMythicGA_AbilityWindow::ApplyAbilityHeal(AActor* Target,float Amount) {
    if (!IsWindowAuthority() || !HealingEffect || !IsValidAbilityTarget(Target,true,false) || !FMath::IsFinite(Amount) || Amount<=0) return {};
    auto* TargetASC=AbilityASC(Target); auto Spec=MakeOutgoingGameplayEffectSpec(HealingEffect,GetAbilityLevel());
    if (!Spec.IsValid()) return {};
    Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_GENERIC,FMath::Min(Amount,GetAbilityMaxHealth(Target)));
    TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    const auto* Context=FMythicGameplayEffectContext::ExtractEffectContext(Spec.Data->GetContext());
    return Context ? Context->ResolvedHealing : FMythicAbilityHealResult();
}
bool UMythicGA_AbilityWindow::ApplyAbilityResource(AActor* Target,float Amount) {
    if (!IsWindowAuthority() || !StaminaEffect || !IsValidAbilityTarget(Target,true,false) || !FMath::IsFinite(Amount) || Amount<=0) return false;
    auto Spec=MakeOutgoingGameplayEffectSpec(StaminaEffect,GetAbilityLevel()); if (!Spec.IsValid()) return false;
    Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_GENERIC,FMath::Clamp(Amount,0.0f,1000.0f)); AbilityASC(Target)->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get()); return true;
}
FGuid UMythicGA_AbilityWindow::GrantOwnedShield(AActor* Target,FName Key,float Amount,float ShieldDuration) {
    if (!IsWindowAuthority() || !IsValidAbilityTarget(Target,true,false)) return {};
    auto* ASC=Cast<UMythicAbilitySystemComponent>(AbilityASC(Target)); if (!ASC) return {};
    Amount=FMath::Min(Amount,GetAbilityMaxHealth(Target));
    const FGuid Handle=ASC->GrantOwnedShield(this,ActivationId,Key,Amount,ShieldDuration);
    if (Handle.IsValid()) {
        Shields.Add({ASC,Handle});
        if (!ShieldListeners.Contains(ASC)) ShieldListeners.Add(ASC,ASC->OnOwnedShieldAbsorbed.AddUObject(this,&ThisClass::HandleShieldAbsorption));
    }
    return Handle;
}
float UMythicGA_AbilityWindow::GetOwnedShieldRemaining(FGuid Handle) const { for (const auto& Shield:Shields) if (Shield.Handle==Handle && Shield.ASC.IsValid()) return Shield.ASC->GetOwnedShieldRemaining(Handle); return 0; }
void UMythicGA_AbilityWindow::RemoveOwnedShield(FGuid Handle) { if (IsWindowAuthority()) for (const auto& Shield:Shields) if (Shield.Handle==Handle && Shield.ASC.IsValid()) { Shield.ASC->RemoveOwnedShield(Handle); break; } }
bool UMythicGA_AbilityWindow::HasAbilityStatus(AActor* Target,FGameplayTag Status) const {
    if (!CheckTargetIncarnation(Target)) return false;
    const auto* Registry=AbilityStatuses(this); const auto* Definition=Registry ? Registry->FindStatus(Status) : nullptr;
    const auto* ASC=AbilityASC(Target); return ASC && ASC->HasMatchingGameplayTag(Definition ? Definition->GrantedStateTag : Status);
}
int32 UMythicGA_AbilityWindow::GetAbilityConditionMask(AActor* Target) const {
    return (HasAbilityStatus(Target,AbilityParameter(TEXT("Status.Type.Burn"))) ? 1:0)
        | (HasAbilityStatus(Target,AbilityParameter(TEXT("Status.Type.Bleed"))) ? 2:0)
        | (HasAbilityStatus(Target,AbilityParameter(TEXT("Status.Type.Poison"))) ? 4:0);
}
int32 UMythicGA_AbilityWindow::CountAbilityConditions(AActor* Target) const { const int32 Mask=GetAbilityConditionMask(Target); return (Mask&1)+((Mask>>1)&1)+((Mask>>2)&1); }
void UMythicGA_AbilityWindow::CleanseAbilityStatuses(AActor* Target,FGameplayTagContainer Allowlist) {
    if (!IsWindowAuthority() || !IsValidAbilityTarget(Target,true,false)) return;
    auto* Registry=AbilityStatuses(this); auto* ASC=AbilityASC(Target); if (!Registry || !ASC) return;
    TSet<TSubclassOf<UGameplayEffect>> Classes;
    for (const auto& Tag:Allowlist) if (const auto* Definition=Registry->FindStatus(Tag))
        if (Definition->EffectToApply && (Tag==AbilityParameter(TEXT("Status.Type.Burn")) || Tag==AbilityParameter(TEXT("Status.Type.Bleed")) || Tag==AbilityParameter(TEXT("Status.Type.Poison")))) Classes.Add(Definition->EffectToApply);
    const auto Handles=ASC->GetActiveEffects(FGameplayEffectQuery());
    for (const auto& Handle:Handles) if (const auto* Active=ASC->GetActiveGameplayEffect(Handle)) if (Classes.Contains(Active->Spec.Def->GetClass())) ASC->RemoveActiveGameplayEffect(Handle);
}
bool UMythicGA_AbilityWindow::ListenToActor(AActor* Actor,FGameplayTag Event,FName Name) {
    if (!CheckTargetIncarnation(Actor)) return false;
    auto* ASC=AbilityASC(Actor);
    if (!IsWindowAuthority() || !ASC || !Event.IsValid() || Name.IsNone() || Listeners.Num()>=32) return false;
    for (const auto& Existing:Listeners) if (!Existing.bStatus && Existing.ASC==ASC && Existing.Tag==Event) return false;
    const FDelegateHandle Handle=ASC->GenericGameplayEventCallbacks.FindOrAdd(Event).AddUObject(this,&ThisClass::HandleActorEvent,TWeakObjectPtr<AActor>(Actor),Name);
    Listeners.Add({ASC,Event,Handle,false}); return true;
}
bool UMythicGA_AbilityWindow::ListenToStatus(AActor* Actor,FGameplayTag Status,FName Name) {
    if (!CheckTargetIncarnation(Actor)) return false;
    auto* ASC=AbilityASC(Actor); auto* Registry=AbilityStatuses(this); const auto* Definition=Registry ? Registry->FindStatus(Status) : nullptr;
    const FGameplayTag Tag=Definition ? Definition->GrantedStateTag : Status;
    if (!IsWindowAuthority() || !ASC || !Tag.IsValid() || Name.IsNone() || Listeners.Num()>=32) return false;
    const FGuid Serial=ActivationId;
    const FDelegateHandle Handle=ASC->RegisterGameplayTagEvent(Tag,EGameplayTagEventType::NewOrRemoved).AddWeakLambda(this,[this,Name,Actor=TWeakObjectPtr<AActor>(Actor),Serial](const FGameplayTag,int32 Count) {
        if (Serial!=ActivationId || !CheckTargetIncarnation(Actor.Get())) return;
        // A raw tag observation has no incoming effect context. Use Event.ApplyStatus for offensive reactions.
        FMythicAbilitySignal Signal; Signal.bReaction=true; Signal.bHasReactionLineage=false; Signal.Actor=Actor.Get(); Signal.Target=Actor.Get(); Signal.Magnitude=Count; Signal.Location=Actor->GetActorLocation(); DispatchSignal(Name,Signal);
    });
    Listeners.Add({ASC,Tag,Handle,true}); return true;
}
void UMythicGA_AbilityWindow::HandleActorEvent(const FGameplayEventData* Event,TWeakObjectPtr<AActor> Actor,FName Name) {
    if (!Event || !CheckTargetIncarnation(Actor.Get()) || !IsWindowAuthority()) return;
    FMythicAbilitySignal Signal; Signal.Actor=Actor.Get(); Signal.bReaction=true;
    const auto* Context=FMythicGameplayEffectContext::ExtractEffectContext(Event->ContextHandle);
    Signal.bHasReactionLineage=Context!=nullptr;
    auto* SourceASC=Event->ContextHandle.GetOriginalInstigatorAbilitySystemComponent();
    Signal.Source=SourceASC ? SourceASC->GetAvatarActor() : const_cast<AActor*>(Event->Instigator.Get());
    Signal.Target=const_cast<AActor*>(Event->Target.Get()); Signal.Magnitude=Event->EventMagnitude;
    if ((Signal.Source && !CheckTargetIncarnation(Signal.Source)) || (Signal.Target && !CheckTargetIncarnation(Signal.Target))) return;
    if (Event->EventTag==GAS_EVENT_DMG_RECEIVED || Event->EventTag==GAS_EVENT_DEATH) Signal.Target=Actor.Get();
    if (Event->EventTag==GAS_EVENT_KILL && (Signal.Source!=WindowAvatar.Get() || !AbilityHostile(Signal.Source,Signal.Target))) return;
    if (Context) { Signal.Origin=Context->DamageOrigin; Signal.ActivationId=Context->SourceActivationId; Signal.Generation=Context->ReactionGeneration; Signal.bCritical=Context->IsCriticalHit(); }
    Signal.bDirectWeapon=Signal.Origin==EMythicDamageOrigin::DirectWeapon && Signal.Generation==0 && Signal.Source==Actor.Get() && AbilityHostile(Signal.Source,Signal.Target);
    Signal.Location=Signal.Target ? Signal.Target->GetActorLocation() : Actor->GetActorLocation();
    TGuardValue<FGameplayEffectContextHandle> Scoped(CurrentOutgoingContext,Event->EventTag==GAS_EVENT_DMG_PRE ? Event->ContextHandle : FGameplayEffectContextHandle());
    DispatchSignal(Name,Signal);
}
void UMythicGA_AbilityWindow::HandleShieldAbsorption(FGuid Handle,float Amount,const FGameplayEffectContextHandle& Context) {
    const auto* Owned=Shields.FindByPredicate([Handle](const auto& S){ return S.Handle==Handle; });
    auto* SourceASC=Context.GetOriginalInstigatorAbilitySystemComponent();
    const auto* Mythic=FMythicGameplayEffectContext::ExtractEffectContext(Context);
    AActor* Target=Owned && Owned->ASC.IsValid() ? Owned->ASC->GetAvatarActor() : nullptr;
    AActor* Source=SourceASC ? SourceASC->GetAvatarActor() : nullptr;
    if (!Owned || !Mythic || Mythic->DamageOrigin==EMythicDamageOrigin::Periodic || !AbilityHostile(Source,Target)) return;
    FMythicAbilitySignal Signal; Signal.bReaction=true; Signal.Actor=Target; Signal.Source=Source; Signal.Target=Target; Signal.ShieldHandle=Handle; Signal.Magnitude=Amount;
    Signal.Origin=Mythic->DamageOrigin; Signal.Generation=Mythic->ReactionGeneration; Signal.bDirectWeapon=Mythic->DamageOrigin==EMythicDamageOrigin::DirectWeapon && Mythic->ReactionGeneration==0;
    DispatchSignal(TEXT("Shield.Absorbed"),Signal);
}
bool UMythicGA_AbilityWindow::ModifyAbilityOutgoingHit(float Multiplier) {
    auto* Context=FMythicGameplayEffectContext::ExtractEffectContext(CurrentOutgoingContext);
    if (!IsWindowAuthority() || !Context || Context->DamageOrigin!=EMythicDamageOrigin::DirectWeapon || Context->ReactionGeneration>0 || !FMath::IsFinite(Multiplier)) return false;
    Context->SetBonusDamageMultiplier(Context->GetBonusDamageMultiplier()*FMath::Clamp(Multiplier,0.0f,2.0f)); return true;
}
void UMythicGA_AbilityWindow::HandleLootCredit(const FMythicLootCreditContext& Credit,FLootTierBonus& Bonus) {
    const auto* PS=AbilityPlayer(WindowAvatar.Get());
    if (!IsWindowAuthority() || !PS || Credit.CreditedTo.Get()!=PS->GetPlayerController() || !Credit.IsGenuineEnemyDeath() || !CheckTargetIncarnation(Credit.Victim.Get()) || !AbilityHostile(WindowAvatar.Get(),Credit.Victim.Get())) return;
    TGuardValue<const FMythicLootCreditContext*> ScopedCredit(CurrentLootCredit,&Credit);
    TGuardValue<FLootTierBonus*> ScopedBonus(CurrentLootBonus,&Bonus);
    FMythicAbilitySignal Signal; Signal.bReaction=true; Signal.bHasReactionLineage=false; Signal.Receipt=Credit.Receipt; Signal.Target=Credit.Victim.Get(); Signal.Actor=Credit.Victim.Get(); Signal.Source=WindowAvatar.Get();
    DispatchSignal(TEXT("Loot.Credit"),Signal);
}
bool UMythicGA_AbilityWindow::TryModifyAbilityLootCredit(FGuid Receipt,float Scale,FName Budget) {
    if (!IsWindowAuthority() || !CurrentLootCredit || !CurrentLootBonus || Receipt!=CurrentLootCredit->Receipt || LootReceipts.Contains(Receipt)
        || LootReceipts.Num()>=3 || !FMath::IsFinite(Scale) || Scale<1 || !TrySpendAbilityBudget(Budget,1)) return false;
    LootReceipts.Add(Receipt); CurrentLootBonus->AbilityDropScale=FMath::Clamp(CurrentLootBonus->AbilityDropScale*Scale,1.0f,1.25f); return true;
}
FActiveGameplayEffectHandle UMythicGA_AbilityWindow::ApplyTrackedAbilityEffect(AActor* Target,TSubclassOf<UGameplayEffect> Effect,float EffectDuration,FName RemovedSignal) {
    if (!IsWindowAuthority() || !Effect || Effects.Num()>=32 || !(IsValidAbilityTarget(Target,true,false) || IsValidAbilityTarget(Target,false,false)) || !FMath::IsFinite(EffectDuration) || EffectDuration<=0 || EffectDuration>120) return {};
    auto* ASC=AbilityASC(Target); auto Spec=MakeOutgoingGameplayEffectSpec(Effect,GetAbilityLevel()); if (!Spec.IsValid()) return {};
    Spec.Data->SetDuration(EffectDuration,true); const auto Handle=ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    if (Handle.IsValid()) {
        Effects.Add({ASC,Handle}); const FGuid Serial=ActivationId; const auto Lineage=CaptureSignalLineage();
        if (!RemovedSignal.IsNone()) if (auto* Removed=ASC->OnGameplayEffectRemoved_InfoDelegate(Handle)) Removed->AddWeakLambda(this,[this,Target=TWeakObjectPtr<AActor>(Target),RemovedSignal,Serial,Lineage](const FGameplayEffectRemovalInfo&) {
            if (Serial==ActivationId && Target.IsValid()) { auto Signal=Lineage; Signal.Actor=Target.Get(); Signal.Target=Target.Get(); DispatchSignal(RemovedSignal,Signal); }
        });
    }
    return Handle;
}
void UMythicGA_AbilityWindow::PlayAbilityCue(FGameplayTag Tag,FVector Location,bool bPersistent,AActor* Target) {
    if (!IsWindowAuthority() || !Tag.IsValid() || Location.ContainsNaN()) return;
    auto* ASC=Cast<UMythicAbilitySystemComponent>(Target ? AbilityASC(Target) : GetAbilitySystemComponentFromActorInfo()); if (!ASC) return;
    FGameplayCueParameters Params; Params.Location=Location; Params.Normal=GetAimDirection(); Params.Instigator=WindowAvatar.Get(); Params.EffectCauser=Target ? Target : WindowAvatar.Get(); Params.SourceObject=GetSkillDefinition();
    Params.EffectContext=MakeEffectContext(CurrentSpecHandle,CurrentActorInfo);
    if (bPersistent) {
        if (Cues.Num()>=16 || Cues.ContainsByPredicate([ASC,Tag](const auto& Cue){ return Cue.ASC==ASC && Cue.Tag==Tag; })) return;
        const int32 Id=ASC->AddAbilityGameplayCue(Tag,Params);
        if (Id!=INDEX_NONE) Cues.Add({ASC,Tag,Id});
    } else ASC->ExecuteGameplayCueMulticast(Tag,Params);
}
bool UMythicGA_AbilityWindow::StartAbilityMovement(FVector Destination,float Seconds,FName Signal) { return StartAbilityPathMovement(TArray<FVector>{Destination},Seconds,Signal); }
bool UMythicGA_AbilityWindow::StartAbilityPathMovement(const TArray<FVector>& Points,float Seconds,FName Signal) {
    if (!(IsWindowAuthority() || bPredictingMotion) || bEnding || MovementTask || Points.IsEmpty() || Points.Num()>32 || !FMath::IsFinite(Seconds) || Seconds<=0 || Seconds>3) return false;
    auto* Character=Cast<ACharacter>(WindowAvatar.Get()); if (!Character) return false;
    float Length=0; FVector Previous=Character->GetActorLocation();
    for (FVector Point:Points) { if (Point.ContainsNaN()) return false; Length+=FVector::Dist(Previous,Point); Previous=Point; }
    if (Length<=1 || Length>2500) return false;
    MovementPath=Points; MovementPoint=0; PathSpeed=Length/Seconds; MovementSignal=Signal; PreviousMovementLocation=Character->GetActorLocation();
    MovementLineage=CaptureSignalLineage();
    if (IsWindowAuthority()) if (auto* ASC=Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo())) ASC->OnAbilityMovementStarted.Broadcast(this,PreviousMovementLocation);
    return StartNextMovementSegment();
}
bool UMythicGA_AbilityWindow::StartNextMovementSegment() {
    auto* Character=Cast<ACharacter>(WindowAvatar.Get()); if (!Character || !MovementPath.IsValidIndex(MovementPoint)) return false;
    const FVector Destination=MovementPath[MovementPoint];
    const float Seconds=FMath::Clamp(FVector::Dist(Character->GetActorLocation(),Destination)/FMath::Max(PathSpeed,1.0f),0.01f,3.0f);
    MovementTask=UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(this,TEXT("AbilityPath"),Destination,Seconds,false,MOVE_Walking,false,nullptr,ERootMotionFinishVelocityMode::ClampVelocity,FVector::ZeroVector,0);
    if (!MovementTask) return false;
    MovementTask->OnTimedOut.AddDynamic(this,&ThisClass::HandleMovementFinished);
    MovementTask->OnTimedOutAndDestinationReached.AddDynamic(this,&ThisClass::HandleMovementFinished);
    MovementTask->ReadyForActivation();
    if (IsWindowAuthority()) GetWorld()->GetTimerManager().SetTimer(MovementSampleTimer,this,&ThisClass::SampleMovement,0.03f,true);
    return true;
}
void UMythicGA_AbilityWindow::SampleMovement() {
    if (!IsWindowAuthority() || !MovementTask) return;
    auto Signal=MovementLineage; Signal.Actor=WindowAvatar.Get(); Signal.Location=WindowAvatar->GetActorLocation(); Signal.PreviousLocation=PreviousMovementLocation;
    if (PendingUtilityMovement.IsSet()) PendingUtilityMovement->ActualEnd=Signal.Location;
    PreviousMovementLocation=Signal.Location; DispatchSignal(TEXT("Movement.Sample"),Signal);
}
void UMythicGA_AbilityWindow::HandleMovementFinished() {
    if (!MovementTask) return;
    SampleMovement(); MovementTask=nullptr;
    GetWorld()->GetTimerManager().ClearTimer(MovementSampleTimer);
    if (!IsWindowAuthority() || bEnding) return;
    const bool Reached=FVector::DistSquared(WindowAvatar->GetActorLocation(),MovementPath[MovementPoint])<FMath::Square(80.0f);
    if (Reached && ++MovementPoint<MovementPath.Num() && StartNextMovementSegment()) return;
    if (auto* ASC=Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo())) ASC->OnAbilityMovementEnded.Broadcast(this,WindowAvatar->GetActorLocation());
    auto Signal=MovementLineage; Signal.Actor=WindowAvatar.Get(); Signal.Location=WindowAvatar->GetActorLocation(); DispatchSignal(MovementSignal,Signal);
}
void UMythicGA_AbilityWindow::EndAbilityWindow(EMythicAbilityWindowEnd Reason) {
    if (!IsWindowAuthority() || bEnding) return;
    bEnding=true; OnWindowEnding(Reason);
    EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,Reason==EMythicAbilityWindowEnd::Cancelled || Reason==EMythicAbilityWindowEnd::AvatarLost || Reason==EMythicAbilityWindowEnd::Invalidated);
}
void UMythicGA_AbilityWindow::RemoveAbilityCue(FGameplayTag Tag,AActor* Target) {
    if (!IsWindowAuthority() || !Tag.IsValid()) return;
    auto* ASC=Target ? AbilityASC(Target) : GetAbilitySystemComponentFromActorInfo();
    const int32 Index=Cues.IndexOfByPredicate([ASC,Tag](const FCue& Cue) { return Cue.ASC==ASC && Cue.Tag==Tag; });
    if (Index==INDEX_NONE) return;
    const FCue Removed=Cues[Index]; Cues.RemoveAt(Index);
    if (Removed.ASC.IsValid()) Removed.ASC->RemoveAbilityGameplayCue(Removed.ReplicationId);
}
void UMythicGA_AbilityWindow::CleanupWindow() {
    if (bCleaned) return; bCleaned=true;
    if (PendingUtilityMovement.IsSet()) FinishUtilityMovement(WindowAvatar.IsValid() && WindowAvatar.Get()==GetAvatarActorFromActorInfo()
        && CheckTargetIncarnation(WindowAvatar.Get()) ? EMythicAbilityMovementOutcome::Cancelled : EMythicAbilityMovementOutcome::AvatarLost);
    UAbilitySystemComponent* OwnerASC=CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
    ActionGroups.Reset(); ActionSource.Reset(); ActionSources.Reset();
    StatusReservations.Reset();
    PreparedUtilityEscapes.Reset();
    SuccessfulUtilityRemovals.Reset(); ActivePoseRemnants.Reset();
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(AimDataTimeout);
    if (auto* ASC=OwnerASC; ASC && AimDataListener.IsValid()) {
        ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle,CurrentActivationInfo.GetActivationPredictionKey()).Remove(AimDataListener); AimDataListener.Reset();
        ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle,CurrentActivationInfo.GetActivationPredictionKey());
    }
    if (GetWorld()) {
        for (auto& Timer:Timers) GetWorld()->GetTimerManager().ClearTimer(Timer);
        GetWorld()->GetTimerManager().ClearTimer(MovementSampleTimer);
        if (auto* GI=GetWorld()->GetGameInstance()) if (auto* Loot=GI->GetSubsystem<UMythicLootManagerSubsystem>()) Loot->OnAbilityLootCredit.Remove(LootListener);
    }
    for (const auto& Listener:Listeners) if (auto* ASC=Listener.ASC.Get()) {
        if (Listener.bStatus) ASC->RegisterGameplayTagEvent(Listener.Tag,EGameplayTagEventType::NewOrRemoved).Remove(Listener.Handle);
        else if (auto* Delegate=ASC->GenericGameplayEventCallbacks.Find(Listener.Tag)) Delegate->Remove(Listener.Handle);
    }
    for (const auto& Listener:ShieldListeners) if (auto* ASC=Listener.Key.Get()) ASC->OnOwnedShieldAbsorbed.Remove(Listener.Value);
    for (const auto& Shield:Shields) if (auto* ASC=Shield.ASC.Get()) ASC->RemoveOwnedShield(Shield.Handle);
    for (const auto& Effect:Effects) if (auto* ASC=Effect.ASC.Get()) ASC->RemoveActiveGameplayEffect(Effect.Handle);
    for (const auto& Cue:Cues) if (auto* ASC=Cue.ASC.Get()) ASC->RemoveAbilityGameplayCue(Cue.ReplicationId);
    for (const auto& Pair:RetainedActors) if (auto* Presentation=Pair.Value.Presentation.Get()) Presentation->OnPresentationDeactivated.Remove(Pair.Value.Deactivated);
    RetainedActors.Reset(); InvalidatedActors.Reset();
    if (MovementTask && WindowAvatar.IsValid()) if (auto* ASC=Cast<UMythicAbilitySystemComponent>(OwnerASC)) ASC->OnAbilityMovementEnded.Broadcast(this,WindowAvatar->GetActorLocation());
    MovementTask=nullptr; Timers.Reset(); Listeners.Reset(); Shields.Reset(); ShieldListeners.Reset(); Effects.Reset(); Cues.Reset();
    CurrentOutgoingContext={}; CurrentLootCredit=nullptr; CurrentLootBonus=nullptr;
}
void UMythicGA_AbilityWindow::EndAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayAbilityActivationInfo Activation,bool Replicate,bool Cancelled) {
    if (bCleaned) return;
    if (!bEnding) { bEnding=true; if (IsWindowAuthority()) OnWindowEnding(Cancelled ? EMythicAbilityWindowEnd::Cancelled : EMythicAbilityWindowEnd::Completed); }
    if (bCleaned) return;
    if (!Cancelled && IsWindowAuthority() && !bLifecycleEnded) {
        bLifecycleEnded=true; PublishLifecycleEvent(GAS_EVENT_SKILL_END);
        if (bCleaned) return;
    }
    if (Cancelled || PendingUtilityMovement.IsSet()) {
        if (auto* ASC=Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo()))
            for (const auto& Remnant:ActivePoseRemnants) ASC->PublishAbilityPoseRemnant(Remnant,true);
    }
    CleanupWindow();
    UMythicGameplayAbility::EndAbility(Handle,Info,Activation,Replicate,Cancelled);
    if (auto* Skills=GetSkillComponent()) Skills->OnSkillsChanged.Broadcast();
}
