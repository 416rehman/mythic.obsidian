// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "MythicCharacter.generated.h"

UCLASS(Abstract)
class AMythicCharacter : public ACharacter, public IAbilitySystemInterface {
    GENERATED_BODY()

public:
    AMythicCharacter();

    virtual void BeginPlay() override;
    virtual void PossessedBy(AController *NewController) override;
    virtual void OnRep_Controller() override;
    virtual void OnRep_PlayerState() override;
    virtual void InitializeASC() {};

    // Implement IAbilitySystemInterface
    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override { return nullptr; };

    // LookAt Actor. Used by AnimBP to set the head of this character to look at the specified actor.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Blueprintable)
    AActor *LookAtActor;
};
