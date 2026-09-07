#include "MythicCharacter.h"
#include "Mythic/Mythic.h"
#include "MythicPlayerState.h"
#include "MythicAbilityIntentComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Input/MythicInputComponent.h"
#include "Input/MythicInputConfig.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/Effects/MythicCrowdControl.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "GameplayEffect.h"

namespace {
const FName ModularBodyTag(TEXT("Mythic.Character.Body"));
const FName OwnerHeadTag(TEXT("Mythic.Character.OwnerHead"));
// A pawn with no Defense set takes plain fall damage; reading the absent attribute would answer 0 and make it immune.
float ResolveFallDamageTaken(const UAbilitySystemComponent &ASC) {
    const FGameplayAttribute Attribute = UMythicAttributeSet_Defense::GetFallDamageTakenAttribute();
    return ASC.HasAttributeSetForAttribute(Attribute) ? FMath::Max(0.0f, ASC.GetNumericAttribute(Attribute)) : 1.0f;
}
}

AMythicCharacter::AMythicCharacter() {
    bReplicates = true;
}

void AMythicCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMythicCharacter, bIsSneaking);
}

void AMythicCharacter::ServerSetSneaking_Implementation(bool bNewSneaking) {
    bIsSneaking = bNewSneaking;
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

    if (!HasAuthority()) {
        return;
    }
    UAbilitySystemComponent *ASC = GetAbilitySystemComponent();
    const UCharacterMovementComponent *Move = GetCharacterMovement();
    const float ImpactSpeed = Move ? FMath::Abs(Move->Velocity.Z) : 0.0f;

    float Damage = 0.0f;
    if (ASC && bEnableFallDamage && FallDamageEffect) {
        Damage = ComputeFallDamage(ImpactSpeed, SafeFallSpeed, FallDamagePerSpeed, MaxFallDamage) * ResolveFallDamageTaken(*ASC);
        if (Damage > 0.0f) {
            Damage = OnFallDamageComputed(ImpactSpeed, ModifyFallDamage(ImpactSpeed, Damage));
        }
    }
    const bool bPrevented = !ASC || ASC->HasMatchingGameplayTag(GAS_IMMUNE_FALLDAMAGE) || Damage <= 0.0f;
    OnFallDamageResolved.Broadcast(ImpactSpeed, Damage, bPrevented);
    if (bPrevented) {
        return;
    }

    FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
    Ctx.AddInstigator(this, this);
    if (FMythicGameplayEffectContext *MythicCtx = FMythicGameplayEffectContext::ExtractEffectContext(Ctx)) {
        MythicCtx->AddHitTag(GAS_HIT_FALL);
    }
    const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(FallDamageEffect, 1.0f, Ctx);
    if (Spec.IsValid()) {
        Spec.Data->SetSetByCallerMagnitude(GAS_SETBYCALLER_GENERIC, Damage);
        ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
    }
}

void AMythicCharacter::BeginPlay() {
    Super::BeginPlay();

    InitializeASC();
    InitializeModularBodyPresentation();
}

void AMythicCharacter::PossessedBy(AController *NewController) {
    Super::PossessedBy(NewController);

    InitializeASC();
    RefreshModularBodyVisibility();
}

void AMythicCharacter::UnPossessed() {
    Super::UnPossessed();
    RefreshModularBodyVisibility();
}

void AMythicCharacter::OnRep_Controller() {
    Super::OnRep_Controller();

    InitializeASC();
    RefreshModularBodyVisibility();
}

void AMythicCharacter::OnRep_PlayerState() {
    Super::OnRep_PlayerState();

    InitializeASC();
    RefreshModularBodyVisibility();
}

void AMythicCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) {
    Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
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

    if (BoundAbilityInputComponent.IsValid()) BoundAbilityInputComponent->RemoveBinds(AbilityInputBindHandles);
    if (auto* ASC=Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponent())) ASC->ClearAbilityInput();
    BoundAbilityInputComponent=MythicIC;
    if (const auto* PC=Cast<AMythicPlayerController>(GetController()); !PC || !PC->ControlsComponent)
        MythicIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, &ThisClass::Input_AbilityInputTagCanceled, AbilityInputBindHandles);

    UE_LOG(Myth, Log, TEXT("SetupPlayerInputComponent: Bound %d Ability Actions"), AbilityInputBindHandles.Num());
}

void AMythicCharacter::Input_AbilityInputTagCanceled(FGameplayTag InputTag) {
    if (auto* ASC=Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponent())) ASC->AbilityInputTagCanceled(InputTag);
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

    RemoveLocalEquipmentMesh(Slot);

    USkeletalMeshComponent* NewMeshComp = NewObject<USkeletalMeshComponent>(this);
    if (!NewMeshComp) {
        return;
    }

    NewMeshComp->RegisterComponent();
    NewMeshComp->SetSkeletalMeshAsset(EquipmentMesh);

    if (Slot == EInventorySlotType::OffHand) {
        NewMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("OffHandWeapon"));
    } else if (Slot == EInventorySlotType::MainHand) {
        NewMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("Weapon"));
    } else {
        NewMeshComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
        NewMeshComp->SetLeaderPoseComponent(GetMesh());
    }

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

void AMythicCharacter::InitializeModularBodyPresentation() {
    if (!GetMesh()) return;
    TInlineComponentArray<USkeletalMeshComponent*> Parts(this);
    bool bHasBody=false;
    for (auto* Part:Parts) {
        if (Part==GetMesh() || !Part->ComponentHasTag(ModularBodyTag) || !Part->GetSkeletalMeshAsset()) continue;
        bHasBody=true;
        Part->bEnableUpdateRateOptimizations=false;
        // Retarget once on the body source, then copy that pose into its modular children.
        Part->AddTickPrerequisiteComponent(GetMesh());
        if (auto* PoseParent=Cast<USkeletalMeshComponent>(Part->GetAttachParent()))
            Part->AddTickPrerequisiteComponent(PoseParent);
        Part->SetTickGroup(TG_PostPhysics);
        Part->SetComponentTickEnabled(GetNetMode()!=NM_DedicatedServer);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        Part->SetCanEverAffectNavigation(false);
    }
    if (bHasBody) {
        // Copy Pose can read the driver's cached URO pose before its interpolation.
        // Keep this player-only assembly on one evaluated pose, including observer clients.
        GetMesh()->bEnableUpdateRateOptimizations=false;
        GetMesh()->SetHiddenInGame(true,false);
        GetMesh()->SetVisibility(false,false);
        GetMesh()->SetCastShadow(false);
        GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    }
    RefreshModularBodyVisibility();
}

void AMythicCharacter::RefreshModularBodyVisibility() {
    const auto* PC=Cast<APlayerController>(GetController());
    const auto* Intent=PC && PC->IsLocalController() ? PC->FindComponentByClass<UMythicAbilityIntentComponent>() : nullptr;
    const bool bHideHead=Intent && Intent->GetSelectedCameraMode().bHideOwnerHead;
    TInlineComponentArray<USkeletalMeshComponent*> Parts(this);
    for (auto* Part:Parts) if (Part->ComponentHasTag(ModularBodyTag))
        Part->SetOwnerNoSee(bHideHead && Part->ComponentHasTag(OwnerHeadTag));
}

void AMythicCharacter::GatherAbilityPoseMeshes(TArray<USkeletalMeshComponent*>& Out) const {
    Out.Reset();
    if (GetMesh() && GetMesh()->IsVisible() && !GetMesh()->bHiddenInGame) Out.Add(GetMesh());
    TInlineComponentArray<USkeletalMeshComponent*> Parts(this);
    Parts.Sort([](const USkeletalMeshComponent& A,const USkeletalMeshComponent& B) { return A.GetFName().LexicalLess(B.GetFName()); });
    for (auto* Part:Parts) if (Part->ComponentHasTag(ModularBodyTag) && Part->IsVisible() && !Part->bHiddenInGame) Out.AddUnique(Part);
    for (const auto& Pair:EquippedVisualMeshes) if (IsValid(Pair.Value) && Pair.Value->IsVisible() && !Pair.Value->bHiddenInGame) Out.AddUnique(Pair.Value);
}
