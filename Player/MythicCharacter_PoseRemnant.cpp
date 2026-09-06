#include "MythicCharacter.h"
#include "Engine/World.h"
#include "AbilitySystemGlobals.h"
#include "GameplayCueManager.h"

void AMythicCharacter::PublishAbilityPoseRemnant(const FMythicAbilityPoseRemnantEvent& Event,bool bCancel) {
    if (HasAuthority() && Event.HasValidKey() && Event.Avatar==this) MulticastAbilityPoseRemnant(Event,bCancel);
}

void AMythicCharacter::MulticastAbilityPoseRemnant_Implementation(FMythicAbilityPoseRemnantEvent Event,bool bCancel) {
    if (Event.Avatar!=this) return;
    if (auto* World=GetWorld(); World && World->GetNetMode()!=NM_DedicatedServer)
        if (auto* Remnants=World->GetSubsystem<UMythicAbilityPoseRemnantSubsystem>()) Remnants->Receive(Event,bCancel);
}

void AMythicCharacter::PublishAbilityCue(FGameplayTag Cue,const FGameplayCueParameters& Parameters) {
    if (HasAuthority() && Cue.IsValid()) MulticastAbilityCue(Cue,Parameters);
}

void AMythicCharacter::MulticastAbilityCue_Implementation(FGameplayTag Cue,FGameplayCueParameters Parameters) {
    if (GetNetMode()!=NM_DedicatedServer && Cue.IsValid())
        UAbilitySystemGlobals::Get().GetGameplayCueManager()->HandleGameplayCue(this,Cue,EGameplayCueEvent::Executed,Parameters);
}
