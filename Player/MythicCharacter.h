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
class UMythicInputConfig;
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FOnMovementModeChangeSignature, AMythicCharacter, OnMythicMovementModeChange, EMovementMode,
                                                    PrevMovementMode,
                                                    uint8, PreviousCustomMode);

// ImpactSpeed in cm/s; Damage after FallDamageTaken and the fall hooks; bPrevented when nothing is applied (immune
// tag, or no damage to apply). The immune case still reports the damage it refused.
DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnMythicFallDamageResolved, float /*ImpactSpeed*/, float /*Damage*/, bool /*bPrevented*/);


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

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override { return nullptr; };

    virtual UAbilitySystemComponent* GetCachedASC() const override;
    virtual UMythicInventoryComponent* GetCachedInventory() const override;
    virtual UMythicLifeComponent* GetCachedLife() const override;

    virtual void Landed(const FHitResult &Hit) override;

    // Server only, once per landing, after the fall damage is decided and before it is applied.
    FOnMythicFallDamageResolved OnFallDamageResolved;

    static float ComputeFallDamage(float ImpactSpeed, float SafeSpeed, float DamagePerSpeed, float MaxDamage);

protected:
    // Native word on a damaging landing before the Blueprint's. Runs only for a landing that would hurt.
    virtual float ModifyFallDamage(float ImpactSpeed, float Damage) { return Damage; }

    // Last word on a damaging landing: returns the damage to apply, 0 applies nothing.
    UFUNCTION(BlueprintNativeEvent, Category = "Mythic|Fall Damage")
    float OnFallDamageComputed(float ImpactSpeed, float Damage);
    virtual float OnFallDamageComputed_Implementation(float ImpactSpeed, float Damage) { return Damage; }

public:

    // LookAt Actor. Used by AnimBP to set the head of this character to look at the specified actor.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Blueprintable)
    AActor *LookAtActor;

    // ─── Stealth → Living-World witness perception (Feature L3) ───
    // Server-authoritative sneak intent. A character that is sneaking OR crouched is perceived from a shorter range by
    // the Living-World witness pipeline (UMythicActionEventSubsystem::SubmitAction reads GetStealthPerceptionScale and
    // rides it into the action event). bIsCrouched is ACharacter's own replicated pose flag; bIsSneaking is an explicit
    // stealth intent independent of the crouch pose (e.g. a slow walk). Toggle via the server RPC; getters are pure.
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Mythic|Stealth")
    void ServerSetSneaking(bool bNewSneaking);

    UFUNCTION(BlueprintPure, Category = "Mythic|Stealth")
    bool IsSneaking() const { return bIsSneaking; }

    // True while stealthed (explicitly sneaking OR crouched). Drives the witness perception scale.
    UFUNCTION(BlueprintPure, Category = "Mythic|Stealth")
    bool IsStealthed() const { return bIsSneaking || bIsCrouched; }

    float GetStealthPerceptionScale() const { return IsStealthed() ? StealthPerceptionScale : 1.0f; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    /**
     * The input config this pawn binds its abilities through. The HUD needs it to show the right key on a slot:
     * the config already maps input tag -> InputAction, so nothing has to hardcode a key or an asset path.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Input")
    const UMythicInputConfig *GetInputConfig() const { return InputConfig; }

protected:
    virtual void SetupPlayerInputComponent(UInputComponent *PlayerInputComponent) override;

    void Input_AbilityInputTagPressed(FGameplayTag InputTag);
    void Input_AbilityInputTagReleased(FGameplayTag InputTag);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Input")
    TObjectPtr<class UMythicInputConfig> InputConfig;

    // Replicated sneak intent (server-authoritative; toggled via ServerSetSneaking). See the stealth API above.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Mythic|Stealth")
    bool bIsSneaking = false;

    // Perception range multiplier applied while stealthed (crouched or sneaking). (0,1]; lower = harder to perceive.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Stealth", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float StealthPerceptionScale = 0.4f;

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

    // Applied to self on a damaging landing. Instant, Life.Damage += SetByCaller.Generic: Landed sets that magnitude
    // to the resolved damage, so the landing runs the whole damage pipeline instead of writing Health directly.
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
    UPROPERTY()
    UAbilitySystemComponent* CachedASC;

    UPROPERTY()
    UMythicInventoryComponent* CachedInventory;

    UPROPERTY()
    UMythicLifeComponent* CachedLife;
    UPROPERTY(Transient)
    TMap<EInventorySlotType, TObjectPtr<USkeletalMeshComponent>> EquippedVisualMeshes;
};
