// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MythicCharacter.h"
#include "MythicCharacter_Player.generated.h"

UCLASS(Blueprintable)
class AMythicCharacter_Player : public AMythicCharacter {
    GENERATED_BODY()

    UPROPERTY(Replicated)
    class UAbilitySystemComponent *ASC_Ref;

public:
    AMythicCharacter_Player();

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
    virtual void InitializeASC() override;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> &OutLifetimeProps) const override;
};
