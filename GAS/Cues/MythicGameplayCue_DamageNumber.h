// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "MythicGameplayCue_DamageNumber.generated.h"

/**
 * UMythicGameplayCue_DamageNumber
 *
 * A static (non-instanced) gameplay cue that adds damage numbers to the screen-space
 * damage number subsystem. Very lightweight - no actor spawning, just adds to a pool.
 *
 * Usage: Set the gameplay cue tag to something like "GameplayCue.Damage.Number"
 * and trigger it with the magnitude set to the damage amount.
 */
UCLASS(Blueprintable, Category = "GameplayCueNotify", Meta = (DisplayName = "GCN Damage Number"))
class MYTHIC_API UMythicGameplayCue_DamageNumber : public UGameplayCueNotify_Static {
    GENERATED_BODY()

public:
    UMythicGameplayCue_DamageNumber();

protected:
    virtual bool OnExecute_Implementation(AActor *Target, const FGameplayCueParameters &Parameters) const override;

protected:
    // Offset from hit location (or actor location) to spawn the damage number
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage Number")
    FVector WorldOffset = FVector(0.0f, 0.0f, 50.0f);

    // If true, treat this cue as healing (uses heal color)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage Number")
    bool bIsHeal = false;
};
