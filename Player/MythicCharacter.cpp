#include "MythicCharacter.h"
#include "Mythic/Mythic.h"
#include "MythicPlayerController.h"
#include "MythicPlayerState.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Input/MythicInputComponent.h"
#include "Input/MythicInputConfig.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GameplayEffect.h"

AMythicCharacter::AMythicCharacter() {
    // Replication
    bReplicates = true;
}

float AMythicCharacter::ComputeFallDamage(float ImpactSpeed, float SafeSpeed, float DamagePerSpeed, float MaxDamage) {
    const float Excess = ImpactSpeed - SafeSpeed;
    if (Excess <= 0.0f || DamagePerSpeed <= 0.0f) {
        return 0.0f;
    }
    const float Damage = Excess * DamagePerSpeed;
    return MaxDamage > 0.0f ? FMath::Min(Damage, MaxDamage) : Damage;
}

void AMythicCharacter::Landed(const FHitResult &Hit) {
    Super::Landed(Hit);

    // Server-authoritative, opt-in, and inert without a configured GE. The downward impact speed is still in the
    // movement component's velocity at the moment Landed fires (it is zeroed afterward in SetPostLandedPhysics).
    if (!bEnableFallDamage || !HasAuthority() || !FallDamageEffect) {
        return;
    }
    const UCharacterMovementComponent *Move = GetCharacterMovement();
    const float ImpactSpeed = Move ? FMath::Abs(Move->Velocity.Z) : 0.0f;
    const float Damage = ComputeFallDamage(ImpactSpeed, SafeFallSpeed, FallDamagePerSpeed, MaxFallDamage);
    if (Damage <= 0.0f) {
        return;
    }

    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    if (!ASC) {
        return;
    }
    // Same pipeline as environmental hazards: configurable GE, computed magnitude carried as the spec level so the
    // damage routes through the normal attribute pipeline (mitigation, clamps, out-of-health latch).
    FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
    Ctx.AddInstigator(this, this);
    const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(FallDamageEffect, Damage, Ctx);
    if (Spec.IsValid()) {
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
}

void AMythicCharacter::BeginPlay() {
    Super::BeginPlay();

    InitializeASC();
}

void AMythicCharacter::PossessedBy(AController *NewController) {
    Super::PossessedBy(NewController);

    InitializeASC();
}

void AMythicCharacter::OnRep_Controller() {
    Super::OnRep_Controller();

    InitializeASC();
}

void AMythicCharacter::OnRep_PlayerState() {
    Super::OnRep_PlayerState();

    InitializeASC();
}

void AMythicCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) {
    OnMythicMovementModeChange.Broadcast(PrevMovementMode, PreviousCustomMode);
}

void AMythicCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent) {
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UMythicInputComponent *MythicIC = Cast<UMythicInputComponent>(PlayerInputComponent);
    if (!MythicIC) {
        UE_LOG(Myth, Error, TEXT("AMythicCharacter::SetupPlayerInputComponent: Input Component is not UMythicInputComponent. Check Project Settings."));
        return;
    }

    if (!InputConfig) {
        UE_LOG(Myth, Warning, TEXT("AMythicCharacter::SetupPlayerInputComponent: InputConfig is null."));
        return;
    }

    // Bind Ability Actions (Gas Input)
    TArray<uint32> BindHandles;
    MythicIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

    UE_LOG(Myth, Log, TEXT("SetupPlayerInputComponent: Bound %d Ability Actions"), BindHandles.Num());
}

void AMythicCharacter::Input_AbilityInputTagPressed(FGameplayTag InputTag) {
    UE_LOG(Myth, Log, TEXT("AMythicCharacter::Input_AbilityInputTagPressed: Tag=%s, Character=%s"),
           *InputTag.ToString(),
           *GetName());
    if (UMythicAbilitySystemComponent *MythicASC = Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponent())) {
        MythicASC->AbilityInputTagPressed(InputTag);
    }
    else {
        UE_LOG(Myth, Error, TEXT("  -> FAILED: No ASC found!"));
    }
}

void AMythicCharacter::Input_AbilityInputTagReleased(FGameplayTag InputTag) {
    if (UMythicAbilitySystemComponent *MythicASC = Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponent())) {
        MythicASC->AbilityInputTagReleased(InputTag);
    }
    else {
        UE_LOG(Myth, Error, TEXT("  -> FAILED: No ASC found!"));
    }
}

void AMythicCharacter::PostInitializeComponents() {
    Super::PostInitializeComponents();

    CachedInventory = FindComponentByClass<UMythicInventoryComponent>();
    CachedLife = FindComponentByClass<UMythicLifeComponent>();
}

UAbilitySystemComponent* AMythicCharacter::GetCachedASC() const {
    return GetAbilitySystemComponent();
}

UMythicInventoryComponent* AMythicCharacter::GetCachedInventory() const {
    return CachedInventory;
}

UMythicLifeComponent* AMythicCharacter::GetCachedLife() const {
    return CachedLife;
}

void AMythicCharacter::ApplyLocalEquipmentMesh(USkeletalMesh* EquipmentMesh, EInventorySlotType Slot) {
    if (!EquipmentMesh || !GetMesh()) {
        return;
    }

    // clean up any existing mesh in the same slot first
    RemoveLocalEquipmentMesh(Slot);

    // spawn the new local skeletal mesh component
    USkeletalMeshComponent* NewMeshComp = NewObject<USkeletalMeshComponent>(this);
    if (!NewMeshComp) {
        return;
    }

    NewMeshComp->RegisterComponent();
    NewMeshComp->SetSkeletalMeshAsset(EquipmentMesh);

    // handle socket attachment or leader pose alignment depending on slot type
    if (Slot == EInventorySlotType::OffHand) {
        NewMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("OffHandWeapon"));
    } else if (Slot == EInventorySlotType::MainHand) {
        NewMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("Weapon"));
    } else {
        NewMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
        NewMeshComp->SetLeaderPoseComponent(GetMesh());
    }

    // cache the spawned component for unequip cleanup
    EquippedVisualMeshes.Add(Slot, NewMeshComp);
}

void AMythicCharacter::RemoveLocalEquipmentMesh(EInventorySlotType Slot) {
    TObjectPtr<USkeletalMeshComponent> FoundMeshComp;
    if (EquippedVisualMeshes.RemoveAndCopyValue(Slot, FoundMeshComp)) {
        if (FoundMeshComp) {
            FoundMeshComp->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
            FoundMeshComp->UnregisterComponent();
            FoundMeshComp->DestroyComponent();
        }
    }
}
