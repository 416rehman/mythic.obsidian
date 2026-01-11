// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "MythicCharacter.generated.h"

class AMythicCharacter;
// OnBeginFalling Delegate
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FOnMovementModeChangeSignature, AMythicCharacter, OnMythicMovementModeChange, EMovementMode,
                                                    PrevMovementMode,
                                                    uint8, PreviousCustomMode);


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

    virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
    UPROPERTY(BlueprintAssignable)
    FOnMovementModeChangeSignature OnMythicMovementModeChange;

    // Implement IAbilitySystemInterface
    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override { return nullptr; };

    // LookAt Actor. Used by AnimBP to set the head of this character to look at the specified actor.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Blueprintable)
    AActor *LookAtActor;

protected:
    virtual void SetupPlayerInputComponent(UInputComponent *PlayerInputComponent) override;

    void Input_AbilityInputTagPressed(FGameplayTag InputTag);
    void Input_AbilityInputTagReleased(FGameplayTag InputTag);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Input")
    TObjectPtr<class UMythicInputConfig> InputConfig;
};
