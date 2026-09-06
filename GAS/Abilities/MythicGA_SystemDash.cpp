#include "MythicGA_SystemDash.h"
#include "MythicAbilityIntentTargetData.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Input/MythicTraversalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "NativeGameplayTags.h"
#include "Input/MythicControlsSettings.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogMythicSystemDash,Log,All);

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_SystemDashCooldown,"Cooldown.System.Dash");
UMythicGE_SystemDashCooldown::UMythicGE_SystemDashCooldown() {
    DurationPolicy=EGameplayEffectDurationType::HasDuration;
    DurationMagnitude=FGameplayEffectModifierMagnitude(FScalableFloat(GetDefault<UMythicControlsSettings>()->DashCooldown));
    GrantedBuffTags.AddTag(TAG_SystemDashCooldown);
}
UMythicGA_SystemDash::UMythicGA_SystemDash() {
    ActivationGroup=EMythicAbilityActivationGroup::Exclusive_Replaceable;
    bCancelOnInputInterrupted=true;
    Movement=EMythicSkillMovement::Dash;
    MovementDistance=GetDefault<UMythicControlsSettings>()->DashDistance;
    MovementDuration=GetDefault<UMythicControlsSettings>()->DashDuration;
    CooldownGameplayEffectClass=UMythicGE_SystemDashCooldown::StaticClass();
    ActivationBlockedTags.AddTag(GAS_STATE_DEAD); ActivationBlockedTags.AddTag(GAS_STATE_DYING);
    ActivationBlockedTags.AddTag(GAS_DEBUFF_STUNNED); ActivationBlockedTags.AddTag(GAS_DEBUFF_FROZEN);
}
bool UMythicGA_SystemDash::CheckCost(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayTagContainer* Failure) const {
    const auto* Avatar=Info ? Info->AvatarActor.Get() : nullptr;
    const auto* Life=Avatar ? Avatar->FindComponentByClass<UMythicLifeComponent>() : nullptr;
    const bool bCan=UMythicTraversalComponent::HasValidDashSettings() && Life && Life->IsInitialized()
        && Life->CanSpendStamina(GetDefault<UMythicControlsSettings>()->DashStaminaCost);
    if (!bCan && Failure) Failure->AddTag(UAbilitySystemGlobals::Get().ActivateFailCostTag);
    return bCan;
}
bool UMythicGA_SystemDash::CanActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
    const FGameplayTagContainer* Source,const FGameplayTagContainer* Target,FGameplayTagContainer* Failure) const {
    const auto* Avatar=Info ? Cast<ACharacter>(Info->AvatarActor.Get()) : nullptr;
    const auto* Traversal=Avatar ? Avatar->FindComponentByClass<UMythicTraversalComponent>() : nullptr;
    const bool bTraversalReady=CallbackDepth==0 && !IsActive() && Traversal && Traversal->OwnsDashSpec(Handle) && !Traversal->IsMantling()
        && UMythicTraversalComponent::IsMovementAllowed(Avatar);
    const bool bAccepted=bTraversalReady
        && UMythicGameplayAbility::CanActivateAbility(Handle,Info,Source,Target,Failure);
    UE_LOG(LogMythicSystemDash,VeryVerbose,TEXT("CanActivate object=%s cdo=%d authority=%d active=%d depth=%d traversal=%d accepted=%d failure=%s"),
        *GetPathName(),HasAnyFlags(RF_ClassDefaultObject),Info && Info->IsNetAuthority(),IsActive(),CallbackDepth,bTraversalReady,bAccepted,
        Failure ? *Failure->ToString():TEXT("none"));
    return bAccepted;
}
void UMythicGA_SystemDash::ActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
    FGameplayAbilityActivationInfo Activation,const FGameplayEventData* Event) {
    UMythicGameplayAbility::ActivateAbility(Handle,Info,Activation,Event);
    UE_LOG(LogMythicSystemDash,VeryVerbose,TEXT("Activate object=%s authority=%d local=%d"),*GetPathName(),Info && Info->IsNetAuthority(),Info && Info->IsLocallyControlled());
    bEnding=false; bMovementStarted=false; DashAvatar=Info ? Info->AvatarActor.Get() : nullptr;
    if (!Info || !DashAvatar.IsValid()) { EndAbility(Handle,Info,Activation,true,true); return; }
    auto* ASC=Info->AbilitySystemComponent.Get();
    if (Info->IsLocallyControlled()) {
        FVector Aim, Move;
        if (!UMythicTraversalComponent::CaptureDirections(Cast<ACharacter>(DashAvatar.Get()),Aim,Move)) {
            UE_LOG(LogMythicSystemDash,VeryVerbose,TEXT("Rejected local aim capture object=%s"),*GetPathName());
            EndAbility(Handle,Info,Activation,true,true); return;
        }
        if (!Info->IsNetAuthority()) {
            FScopedPredictionWindow Prediction(ASC,true);
            auto* Intent=new FMythicAbilityIntentTargetData(); Intent->AimDirection=Aim; Intent->MovementDirection=Move;
            FGameplayAbilityTargetDataHandle Data; Data.Add(Intent);
            ASC->CallServerSetReplicatedTargetData(Handle,Activation.GetActivationPredictionKey(),Data,FGameplayTag(),ASC->ScopedPredictionKey);
        }
        StartDash(Aim,Move); return;
    }
    if (Info->IsNetAuthority() && Info->PlayerController.IsValid()) {
        IntentHandle=ASC->AbilityTargetDataSetDelegate(Handle,Activation.GetActivationPredictionKey()).AddUObject(this,&ThisClass::ReceiveIntent);
        GetWorld()->GetTimerManager().SetTimer(IntentTimeout,FTimerDelegate::CreateWeakLambda(this,[this](){Blocked();}),2.0f,false);
        ASC->CallReplicatedTargetDataDelegatesIfSet(Handle,Activation.GetActivationPredictionKey()); return;
    }
    EndAbility(Handle,Info,Activation,true,true);
}
void UMythicGA_SystemDash::ReceiveIntent(const FGameplayAbilityTargetDataHandle& Data,FGameplayTag) {
    if (bEnding || !IsActive() || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority()) return;
    auto* ASC=GetAbilitySystemComponentFromActorInfo();
    ASC->AbilityTargetDataSetDelegate(CurrentSpecHandle,CurrentActivationInfo.GetActivationPredictionKey()).Remove(IntentHandle); IntentHandle.Reset();
    GetWorld()->GetTimerManager().ClearTimer(IntentTimeout);
    FVector Aim=FVector::ZeroVector, Move=FVector::ZeroVector; bool bValid=false;
    if (Data.Num()==1 && Data.Get(0) && Data.Get(0)->GetScriptStruct()==FMythicAbilityIntentTargetData::StaticStruct()) {
        const auto* Intent=static_cast<const FMythicAbilityIntentTargetData*>(Data.Get(0));
        bValid=Intent->HasValidDirections(); Aim=Intent->AimDirection; Move=Intent->MovementDirection;
    }
    ASC->ConsumeClientReplicatedTargetData(CurrentSpecHandle,CurrentActivationInfo.GetActivationPredictionKey());
    if (!bValid) { Blocked(); return; }
    StartDash(Aim,Move);
}
void UMythicGA_SystemDash::StartDash(FVector Aim,FVector Move) {
    TGuardValue<int32> Guard(CallbackDepth,CallbackDepth+1);
    auto* Avatar=Cast<ACharacter>(DashAvatar.Get());
    const auto Handle=CurrentSpecHandle; const auto Activation=CurrentActivationInfo; const auto* Info=CurrentActorInfo;
    const auto* Settings=GetDefault<UMythicControlsSettings>();
    const float Cost=Settings->DashStaminaCost, Duration=Settings->DashDuration, Cooldown=Settings->DashCooldown;
    FVector Direction=Move.GetSafeNormal2D();
    if (Direction.IsNearlyZero()) Direction=Aim.GetSafeNormal2D();
    if (Direction.IsNearlyZero() && Avatar) Direction=Avatar->GetActorForwardVector().GetSafeNormal2D();
    FVector Destination=FVector::ZeroVector;
    const bool bAvatarReady=Info && !bEnding && Avatar && Avatar==GetAvatarActorFromActorInfo()
        && Info->MovementComponent.Get()==Avatar->GetCharacterMovement();
    const bool bRouteReady=bAvatarReady && UMythicTraversalComponent::BuildDashDestination(Avatar,Direction,Destination);
    const bool bCostReady=bRouteReady && CheckCost(Handle,Info);
    const bool bCooldownReady=bCostReady && CheckCooldown(Handle,Info);
    UE_LOG(LogMythicSystemDash,VeryVerbose,TEXT("Start object=%s authority=%d avatar=%d route=%d cost=%d cooldown=%d aim=%s move=%s destination=%s"),
        *GetPathName(),Info && Info->IsNetAuthority(),bAvatarReady,bRouteReady,bCostReady,bCooldownReady,*Aim.ToString(),*Move.ToString(),*Destination.ToString());
    if (!bCooldownReady) { Blocked(); return; }
    auto* ASC=Cast<UMythicAbilitySystemComponent>(Info->AbilitySystemComponent.Get());
    if (Info->IsNetAuthority()) {
        auto* Life=Avatar->FindComponentByClass<UMythicLifeComponent>();
        // Admit the exact removable cooldown first. Failure cannot spend stamina or yield a free-reuse dash.
        auto CooldownSpec=ASC->MakeOutgoingSpec(UMythicGE_SystemDashCooldown::StaticClass(),1,ASC->MakeEffectContext());
        if (!Life || !CooldownSpec.IsValid()) { Blocked(); return; }
        CooldownSpec.Data->SetDuration(Cooldown,true);
        const auto CooldownHandle=ASC->ApplyGameplayEffectSpecToSelf(*CooldownSpec.Data.Get());
        if (bEnding || !IsActive() || !CooldownHandle.IsValid() || Avatar!=GetAvatarActorFromActorInfo()) {
            if (CooldownHandle.IsValid()) ASC->RemoveActiveGameplayEffect(CooldownHandle);
            Blocked(); return;
        }
        // Attribute callbacks may end this activation; the scope prevents same-instance reentry.
        if (!Life->TrySpendStamina(Cost)) { ASC->RemoveActiveGameplayEffect(CooldownHandle); Blocked(); return; }
        ASC->NotifyAbilityCommit(this);
        if (bEnding || !IsActive() || Avatar!=GetAvatarActorFromActorInfo()) return;
    }
    auto* NewMotion=UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(this,TEXT("SystemDash"),Destination,Duration,
        false,MOVE_Walking,true,nullptr,ERootMotionFinishVelocityMode::ClampVelocity,FVector::ZeroVector,Avatar->GetCharacterMovement()->GetMaxSpeed());
    if (bEnding || !IsActive()) { if (NewMotion) NewMotion->EndTask(); return; }
    Motion=NewMotion;
    if (!Motion) { Blocked(); return; }
    Motion->OnTimedOut.AddDynamic(this,&ThisClass::Blocked);
    Motion->OnTimedOutAndDestinationReached.AddDynamic(this,&ThisClass::Reached);
    if (Info->IsNetAuthority()) {
        bMovementStarted=true; ASC->OnAbilityMovementStarted.Broadcast(this,Avatar->GetActorLocation());
        if (bEnding || !Motion) return;
    }
    Motion->ReadyForActivation();
    if (!bEnding) GetWorld()->GetTimerManager().SetTimer(MotionGuard,this,&ThisClass::ValidateMotion,0.025f,true);
}
void UMythicGA_SystemDash::ValidateMotion() {
    auto* Avatar=Cast<ACharacter>(DashAvatar.Get());
    if (!Avatar || Avatar!=GetAvatarActorFromActorInfo() || !UMythicTraversalComponent::IsMovementAllowed(Avatar)) Blocked();
}
void UMythicGA_SystemDash::Reached() { EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,false); }
void UMythicGA_SystemDash::Blocked() { EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,true); }
void UMythicGA_SystemDash::EndAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
    FGameplayAbilityActivationInfo Activation,bool bReplicate,bool bCancelled) {
    if (bEnding || !IsActive()) return;
    TGuardValue<int32> Guard(CallbackDepth,CallbackDepth+1); bEnding=true;
    GetWorld()->GetTimerManager().ClearTimer(IntentTimeout);
    GetWorld()->GetTimerManager().ClearTimer(MotionGuard);
    auto* ASC=Info ? Cast<UMythicAbilitySystemComponent>(Info->AbilitySystemComponent.Get()) : nullptr;
    if (ASC) {
        ASC->AbilityTargetDataSetDelegate(Handle,Activation.GetActivationPredictionKey()).Remove(IntentHandle); IntentHandle.Reset();
        if (Info->IsNetAuthority()) ASC->ConsumeClientReplicatedTargetData(Handle,Activation.GetActivationPredictionKey());
    }
    if (auto* Task=Motion.Get()) { Motion=nullptr; Task->EndTask(); }
    if (bMovementStarted && ASC && Info->IsNetAuthority()) {
        bMovementStarted=false;
        ASC->OnAbilityMovementEnded.Broadcast(this,DashAvatar.IsValid()?DashAvatar->GetActorLocation():FVector::ZeroVector);
    }
    DashAvatar.Reset();
    UMythicGameplayAbility::EndAbility(Handle,Info,Activation,bReplicate,bCancelled);
}
