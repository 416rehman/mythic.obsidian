// Copyright Stellar Games. All Rights Reserved.

#include "MythicGameplayCue_DamageNumber.h"
#include "UI/MythicDamageNumberSubsystem.h"

UMythicGameplayCue_DamageNumber::UMythicGameplayCue_DamageNumber() {}

bool UMythicGameplayCue_DamageNumber::OnExecute_Implementation(AActor *Target, const FGameplayCueParameters &Parameters) const {
    if (!Target) {
        return false;
    }

    UWorld *World = Target->GetWorld();
    if (!World) {
        return false;
    }

    UMythicDamageNumberSubsystem *DamageNumberSubsystem = World->GetSubsystem<UMythicDamageNumberSubsystem>();
    if (!DamageNumberSubsystem) {
        return false;
    }

    // Determine spawn location - prefer hit result, fall back to target location
    FVector SpawnLocation = Target->GetActorLocation();
    if (Parameters.EffectContext.GetHitResult()) {
        SpawnLocation = Parameters.EffectContext.GetHitResult()->ImpactPoint;
    }
    SpawnLocation += WorldOffset;

    // Add the damage number to the pool
    DamageNumberSubsystem->AddDamageNumber(
        SpawnLocation,
        Parameters.RawMagnitude,
        Parameters.EffectContext,
        bIsHeal
        );

    return true;
}
