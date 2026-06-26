// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GameFramework/Character.h"
#include "MythicRegistryInterface.h"
#include "Itemization/Inventory/InventorySlotDefinition.h"
#include "MythicCharacter.generated.h"

class AMythicCharacter;
class UGameplayEffect;
// OnBeginFalling Delegate
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FOnMovementModeChangeSignature, AMythicCharacter, OnMythicMovementModeChange, EMovementMode,
                                                    PrevMovementMode,
                                                    uint8, PreviousCustomMode);


UCLASS(Abstract)
class MYTHIC_API AMythicCharacter : public ACharacter, public IAbilitySystemInterface, public IMythicRegistryInterface {
    GENERATED_BODY()

public:
    AMythicCharacter();

    virtual void BeginPlay() override;
    virtual void PostInitializeComponents() override;
    virtual void PossessedBy(AController *NewController) override;
    virtual void OnRep_Controller() override;
    virtual void OnRep_PlayerState() override;
    virtual void InitializeASC() {};

    virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
    UPROPERTY(BlueprintAssignable)
    FOnMovementModeChangeSignature OnMythicMovementModeChange;

    // Implement IAbilitySystemInterface
    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override { return nullptr; };

    // Implement IMythicRegistryInterface
    virtual UAbilitySystemComponent* GetCachedASC() const override;
    virtual UMythicInventoryComponent* GetCachedInventory() const override;
    virtual UMythicLifeComponent* GetCachedLife() const override;

    //~ Fall damage — locomotion-driven self-damage on a hard landing. Server-authoritative; applied via the same
    //~ configurable-GE path as environmental hazards (the computed damage is passed as the spec level). Default OFF
    //~ (gameplay-affecting → zero regression until a designer enables it AND assigns FallDamageEffect).
    virtual void Landed(const FHitResult &Hit) override;

    // Pure: damage from a landing impact speed (cm/s, absolute). 0 at/below SafeSpeed; (Impact-Safe)*DamagePerSpeed
    // above, capped at MaxDamage (MaxDamage <= 0 = uncapped). Static so the curve is unit-testable headlessly.
    static float ComputeFallDamage(float ImpactSpeed, float SafeSpeed, float DamagePerSpeed, float MaxDamage);

    // LookAt Actor. Used by AnimBP to set the head of this character to look at the specified actor.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Blueprintable)
    AActor *LookAtActor;

protected:
    virtual void SetupPlayerInputComponent(UInputComponent *PlayerInputComponent) override;

    void Input_AbilityInputTagPressed(FGameplayTag InputTag);
    void Input_AbilityInputTagReleased(FGameplayTag InputTag);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Input")
    TObjectPtr<class UMythicInputConfig> InputConfig;

    // Fall-damage tuning (per character class). Disabled by default — gameplay-affecting.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Fall Damage")
    bool bEnableFallDamage = false;

    // Impact speed (cm/s) at or below which a landing is harmless.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Fall Damage", meta = (ClampMin = "0.0", EditCondition = "bEnableFallDamage"))
    float SafeFallSpeed = 1200.0f;

    // Damage per cm/s of impact speed above SafeFallSpeed.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Fall Damage", meta = (ClampMin = "0.0", EditCondition = "bEnableFallDamage"))
    float FallDamagePerSpeed = 0.05f;

    // Maximum fall damage from a single landing (<= 0 = uncapped).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Fall Damage", meta = (EditCondition = "bEnableFallDamage"))
    float MaxFallDamage = 100.0f;

    // GameplayEffect applied to self on a damaging landing (designer asset). The computed damage is the spec
    // LEVEL, so the GE's Health modifier should scale by level — mirrors the environmental-hazard GE pattern.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Fall Damage", meta = (EditCondition = "bEnableFallDamage"))
    TSubclassOf<UGameplayEffect> FallDamageEffect;

public:
    // spawn attachment mesh locally and register with leader component or attach to socket
    UFUNCTION(BlueprintCallable, Category = "Mythic|Equipment")
    void ApplyLocalEquipmentMesh(USkeletalMesh* EquipmentMesh, EInventorySlotType Slot);

    // detach and destroy client-side spawned mesh component for Slot
    UFUNCTION(BlueprintCallable, Category = "Mythic|Equipment")
    void RemoveLocalEquipmentMesh(EInventorySlotType Slot);

protected:
    // cached references for o1 retrieval
    UPROPERTY()
    UAbilitySystemComponent* CachedASC;

    UPROPERTY()
    UMythicInventoryComponent* CachedInventory;

    UPROPERTY()
    UMythicLifeComponent* CachedLife;
    // tracks client-side spawned skeletal mesh components for active equipment slots
    UPROPERTY(Transient)
    TMap<EInventorySlotType, TObjectPtr<USkeletalMeshComponent>> EquippedVisualMeshes;
};
