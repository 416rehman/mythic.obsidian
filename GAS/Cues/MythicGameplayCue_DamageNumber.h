// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "MythicGameplayCue_DamageNumber.generated.h"

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
