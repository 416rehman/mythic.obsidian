#include "MythicWeaponDefenseAbility.h"
#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Itemization/Inventory/Fragments/Actionable/WeaponBlockFragment.h"
#include "Itemization/Inventory/Fragments/Actionable/MythicWeaponActionContract.h"
#include "GAS/MythicTags_GAS.h"
#include "TimerManager.h"
UMythicWeaponDefenseAbility::UMythicWeaponDefenseAbility() {
    ActivationGroup=EMythicAbilityActivationGroup::Exclusive_Replaceable;
    bCancelOnInputInterrupted=true;
    ActivationBlockedTags.AddTag(GAS_STATE_DEAD); ActivationBlockedTags.AddTag(GAS_STATE_DYING);
    ActivationBlockedTags.AddTag(GAS_DEBUFF_STUNNED); ActivationBlockedTags.AddTag(GAS_DEBUFF_FROZEN);
}
const UWeaponBlockFragment* UMythicWeaponDefenseAbility::Source(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info) const {
    const auto* ASC=Info?Info->AbilitySystemComponent.Get():nullptr;
    const auto* Spec=ASC?ASC->FindAbilitySpecFromHandle(Handle):nullptr;
    return Spec && Spec->Ability && Spec->Ability->GetClass()==StaticClass()?Cast<UWeaponBlockFragment>(Spec->SourceObject.Get()):nullptr;
}
bool UMythicWeaponDefenseAbility::CanActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
    const FGameplayTagContainer* SourceTags,const FGameplayTagContainer* TargetTags,FGameplayTagContainer* FailureTags) const {
    return CallbackDepth==0 && !IsActive() && MythicWeaponAction::IsEquippedSource(Source(Handle,Info),Info)
        && Super::CanActivateAbility(Handle,Info,SourceTags,TargetTags,FailureTags);
}
void UMythicWeaponDefenseAbility::ActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
    FGameplayAbilityActivationInfo Activation,const FGameplayEventData* Event) {
    Super::ActivateAbility(Handle,Info,Activation,Event);
    TGuardValue<int32> Guard(CallbackDepth,CallbackDepth+1); bEnding=false;
    const auto* Fragment=Source(Handle,Info);
    if (!MythicWeaponAction::IsEquippedSource(Fragment,Info)) {EndAbility(Handle,Info,Activation,true,true); return;}
    auto* ASC=Info->AbilitySystemComponent.Get();
    if (Info->IsNetAuthority()) {
        auto Context=ASC->MakeEffectContext(); Context.AddSourceObject(Fragment);
        const auto Spec=ASC->MakeOutgoingSpec(Fragment->GuardEffect,1,Context);
        if (!Spec.IsValid()) {EndAbility(Handle,Info,Activation,true,true); return;}
        const auto Applied=ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        // An application delegate can end/unequip. Reconcile the returned handle after that callback unwinds.
        if (bEnding || !IsActive()) {if (Applied.IsValid()) ASC->RemoveActiveGameplayEffect(Applied); return;}
        GuardHandle=Applied;
        if (!GuardHandle.IsValid()) {EndAbility(Handle,Info,Activation,true,true); return;}
        ASC->NotifyAbilityCommit(this);
        if (bEnding) return;
    }
    ReleaseTask=UAbilityTask_WaitInputRelease::WaitInputRelease(this,false);
    ReleaseTask->OnRelease.AddDynamic(this,&ThisClass::Released); ReleaseTask->ReadyForActivation();
    if (!bEnding) GetWorld()->GetTimerManager().SetTimer(SourceTimer,this,&ThisClass::ValidateSource,0.1f,true);
}
void UMythicWeaponDefenseAbility::Released(float) {EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,false);}
void UMythicWeaponDefenseAbility::ValidateSource() {
    const auto* ASC=GetAbilitySystemComponentFromActorInfo();
    if (!CurrentActorInfo || !ASC || !MythicWeaponAction::IsEquippedSource(Source(CurrentSpecHandle,CurrentActorInfo),CurrentActorInfo)
        || ASC->HasMatchingGameplayTag(GAS_STATE_DEAD) || ASC->HasMatchingGameplayTag(GAS_STATE_DYING)
        || ASC->HasMatchingGameplayTag(GAS_DEBUFF_STUNNED) || ASC->HasMatchingGameplayTag(GAS_DEBUFF_FROZEN)
        || (CurrentActorInfo->IsNetAuthority() && !ASC->GetActiveGameplayEffect(GuardHandle)))
        EndAbility(CurrentSpecHandle,CurrentActorInfo,CurrentActivationInfo,true,true);
}
void UMythicWeaponDefenseAbility::EndAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
    FGameplayAbilityActivationInfo Activation,bool bReplicate,bool bCancelled) {
    if (bEnding || !IsActive()) return;
    TGuardValue<int32> Guard(CallbackDepth,CallbackDepth+1); bEnding=true;
    GetWorld()->GetTimerManager().ClearTimer(SourceTimer);
    if (auto* Task=ReleaseTask.Get()) {ReleaseTask=nullptr; Task->EndTask();}
    const auto Effect=GuardHandle; GuardHandle={};
    if (Info && Info->IsNetAuthority() && Effect.IsValid()) Info->AbilitySystemComponent->RemoveActiveGameplayEffect(Effect);
    Super::EndAbility(Handle,Info,Activation,bReplicate,bCancelled);
}
