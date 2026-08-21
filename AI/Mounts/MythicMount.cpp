
#include "GAS/Mounts/MythicMount.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

#include "GAS/Mounts/MythicTags_Mounts.h"
#include "GAS/Mounts/MythicMountRosterComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicPlayerController.h"
#include "Mythic.h"

AMythicMount::AMythicMount() {
    PrimaryActorTick.bCanEverTick = false;

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;
    if (UCharacterMovementComponent *CMC = GetCharacterMovement()) {
        CMC->bOrientRotationToMovement = true;
        CMC->RotationRate = FRotator(0.0f, 640.0f, 0.0f);
        CMC->bConstrainToPlane = true;
        CMC->bSnapToPlaneAtStart = true;
        CMC->MaxWalkSpeed = WalkSpeed;
    }

    Stamina = MaxStamina;
}

void AMythicMount::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AMythicMount, OwnerMountId, COND_OwnerOnly);
    DOREPLIFETIME(AMythicMount, OwnerPlayerKey);
    DOREPLIFETIME_CONDITION(AMythicMount, OwnerPlayerState, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(AMythicMount, BondLevel, COND_OwnerOnly);
    DOREPLIFETIME(AMythicMount, CurrentGait);
    DOREPLIFETIME(AMythicMount, RiderPawn);
    DOREPLIFETIME_CONDITION(AMythicMount, Stamina, COND_OwnerOnly);
}

void AMythicMount::BeginPlay() {
    Super::BeginPlay();

    if (HasAuthority()) {
        Stamina = MaxStamina;
        ApplyGaitSpeed();

        if (LifeComponent) {
            LifeComponent->OnDeath.AddUniqueDynamic(this, &AMythicMount::HandleMountDeath);
        }
    }
}

void AMythicMount::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (HasAuthority() && IsRidden()) {
        ServerDismount();
    }
    StopStaminaTimer();
    Super::EndPlay(EndPlayReason);
}

void AMythicMount::ConfigureFromRecord(const FMythicMountRecord &Record, const FString &InOwnerKey, AMythicPlayerState *InOwnerPS) {
    if (!HasAuthority()) {
        return;
    }
    OwnerMountId = Record.MountId;
    OwnerPlayerKey = InOwnerKey;
    OwnerPlayerState = InOwnerPS;
    BondLevel = MythicMountStatics::BondLevelFromXP(Record.BondXP);
    Stamina = MaxStamina;

    SpeciesId = Record.SpeciesId;
    OnCreatureInitialized(SpeciesId, 0);
}

UAbilitySystemComponent *AMythicMount::ResolveRiderASC(const APawn *Pawn) {
    if (!Pawn) {
        return nullptr;
    }
    if (const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(Pawn)) {
        if (UAbilitySystemComponent *ASC = ASI->GetAbilitySystemComponent()) {
            return ASC;
        }
    }
    if (const IAbilitySystemInterface *PSI = Cast<IAbilitySystemInterface>(Pawn->GetPlayerState())) {
        return PSI->GetAbilitySystemComponent();
    }
    return nullptr;
}

EMountGateResult AMythicMount::EvaluateMountGate(const APawn *InRiderPawn) const {
    const bool bMountAlive = LifeComponent && !LifeComponent->IsDead();

    bool bRiderAlreadyMounted = IsRidden();
    bool bInCombat = false;
    if (const UAbilitySystemComponent *RiderASC = ResolveRiderASC(InRiderPawn)) {
        bRiderAlreadyMounted |= RiderASC->HasMatchingGameplayTag(TAG_GAS_State_Mounted);
        bInCombat = RiderASC->HasMatchingGameplayTag(GAS_STATE_INCOMBAT);
    }

    bool bOwnershipMatch = OwnerPlayerKey.IsEmpty();
    if (!bOwnershipMatch && InRiderPawn) {
        if (const AMythicPlayerState *RiderPS = InRiderPawn->GetPlayerState<AMythicPlayerState>()) {
            bOwnershipMatch = RiderPS->GetCanonicalPlayerKey() == OwnerPlayerKey;
        }
    }

    const float DistSq = InRiderPawn ? static_cast<float>(FVector::DistSquared(InRiderPawn->GetActorLocation(), GetActorLocation())) : TNumericLimits<float>::Max();
    return MythicMountStatics::CanMount(bMountAlive, bRiderAlreadyMounted, bInCombat, bOwnershipMatch, DistSq, MountRange * MountRange);
}

bool AMythicMount::ServerMount(APawn *InRiderPawn) {
    if (!HasAuthority() || !InRiderPawn) {
        return false;
    }

    const EMountGateResult Gate = EvaluateMountGate(InRiderPawn);
    if (Gate != EMountGateResult::Ok) {
        UE_LOG(Myth, Log, TEXT("Mount: %s refused rider %s (gate %d)"), *GetNameSafe(this), *GetNameSafe(InRiderPawn), static_cast<int32>(Gate));
        return false;
    }

    AController *RiderController = InRiderPawn->GetController();
    if (!RiderController || !RiderController->IsPlayerController()) {
        return false;
    }

    ParkedAIController = GetController();
    if (AController *AI = ParkedAIController.Get()) {
        AI->UnPossess();
    }

    RiderController->UnPossess();
    RiderController->Possess(this);

    RiderPawn = InRiderPawn;
    if (const ACharacter *RiderChar = Cast<ACharacter>(InRiderPawn)) {
        if (UCharacterMovementComponent *RiderCMC = RiderChar->GetCharacterMovement()) {
            RiderCMC->StopMovementImmediately();
            RiderCMC->DisableMovement();
        }
    }
    InRiderPawn->SetActorEnableCollision(false);
    USceneComponent *Seat = GetMesh() ? static_cast<USceneComponent *>(GetMesh()) : GetRootComponent();
    if (GetMesh() && !GetMesh()->DoesSocketExist(SeatSocketName)) {
        UE_LOG(Myth, Warning, TEXT("Mount: %s has no seat socket '%s' — rider snaps to the mesh origin (author the socket on the BP mesh)"),
               *GetNameSafe(this), *SeatSocketName.ToString());
    }
    InRiderPawn->AttachToComponent(Seat, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SeatSocketName);

    if (UAbilitySystemComponent *RiderASC = ResolveRiderASC(InRiderPawn)) {
        RiderASC->AddLooseGameplayTag(TAG_GAS_State_Mounted, 1, EGameplayTagReplicationState::TagOnly);
    }

    CurrentGait = EMythicMountGait::Walk;
    ApplyGaitSpeed();
    StartStaminaTimer();

    RideStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

    UE_LOG(Myth, Log, TEXT("Mount: %s mounted by %s"), *GetNameSafe(this), *GetNameSafe(InRiderPawn));
    OnMounted(InRiderPawn);
    return true;
}

void AMythicMount::ServerDismount_Implementation() {
    if (!HasAuthority() || !RiderPawn) {
        return;
    }

    APawn *Rider = RiderPawn;
    AController *RidingController = GetController();

    const FVector DropLocation = FindDismountLocation();

    Rider->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
    Rider->SetActorEnableCollision(true);
    float HalfHeight = 0.0f;
    if (const ACharacter *RiderChar = Cast<ACharacter>(Rider)) {
        if (const UCapsuleComponent *Capsule = RiderChar->GetCapsuleComponent()) {
            HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
        }
    }
    const FRotator DropRotation(0.0f, GetActorRotation().Yaw, 0.0f);
    Rider->SetActorLocationAndRotation(DropLocation + FVector(0.0f, 0.0f, HalfHeight + 2.0f), DropRotation,
 false, nullptr, ETeleportType::TeleportPhysics);
    if (const ACharacter *RiderChar = Cast<ACharacter>(Rider)) {
        if (UCharacterMovementComponent *RiderCMC = RiderChar->GetCharacterMovement()) {
            RiderCMC->SetMovementMode(MOVE_Walking);
        }
    }

    if (RidingController && RidingController->IsPlayerController()) {
        RidingController->UnPossess();
        RidingController->Possess(Rider);
    }

    if (LifeComponent && !LifeComponent->IsDead()) {
        if (AController *AI = ParkedAIController.Get()) {
            AI->Possess(this);
        }
        else if (!GetController()) {
            SpawnDefaultController();
        }
    }
    ParkedAIController = nullptr;

    if (UAbilitySystemComponent *RiderASC = ResolveRiderASC(Rider)) {
        RiderASC->RemoveLooseGameplayTag(TAG_GAS_State_Mounted, 1, EGameplayTagReplicationState::TagOnly);
    }

    if (RideStartTime > 0.0 && OwnerMountId.IsValid() && BondXpPerSecondRidden > 0.0f) {
        const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : RideStartTime;
        const double RideSeconds = FMath::Max(0.0, Now - RideStartTime);
        int32 GrantedXP = FMath::FloorToInt(RideSeconds * static_cast<double>(BondXpPerSecondRidden));
        if (MaxBondXpPerRide > 0) {
            GrantedXP = FMath::Min(GrantedXP, MaxBondXpPerRide);
        }
        if (GrantedXP > 0) {
            AMythicPlayerState *OwnerPS = OwnerPlayerState;
            if (!OwnerPS && RidingController) {
                OwnerPS = Cast<AMythicPlayerState>(RidingController->PlayerState);
            }
            if (UMythicMountRosterComponent *Roster = OwnerPS ? OwnerPS->GetMountRosterComponent() : nullptr) {
                Roster->ServerGrantBondXP(OwnerMountId, GrantedXP);
            }
        }
    }
    RideStartTime = 0.0;

    RiderPawn = nullptr;
    CurrentGait = EMythicMountGait::Walk;
    ApplyGaitSpeed();
    StopStaminaTimer();

    UE_LOG(Myth, Log, TEXT("Mount: %s dismounted %s"), *GetNameSafe(this), *GetNameSafe(Rider));
    OnDismounted(Rider);
}

void AMythicMount::ServerSetGait_Implementation(EMythicMountGait NewGait) {
    if (!HasAuthority() || !IsRidden()) {
        return;
    }
    if (LifeComponent && LifeComponent->IsDead()) {
        return;
    }
    if (NewGait == EMythicMountGait::Gallop && Stamina <= 0.0f) {
        NewGait = EMythicMountGait::Trot;
    }
    if (CurrentGait == NewGait) {
        return;
    }
    CurrentGait = NewGait;
    ApplyGaitSpeed();
}

void AMythicMount::StaminaTick() {
    if (!HasAuthority() || !IsRidden()) {
        StopStaminaTimer();
        return;
    }

    const bool bGallopingAndMoving = CurrentGait == EMythicMountGait::Gallop && GetVelocity().SizeSquared() > FMath::Square(25.0f);
    if (bGallopingAndMoving) {
        Stamina = MythicMountStatics::DrainStamina(Stamina, StaminaTickInterval, StaminaDrainPerSecond, MaxStamina);
        if (Stamina <= 0.0f) {
            CurrentGait = EMythicMountGait::Trot;
            ApplyGaitSpeed();
        }
    }
    else {
        Stamina = MythicMountStatics::RegenStamina(Stamina, StaminaTickInterval, StaminaRegenPerSecond, MaxStamina);
    }
}

void AMythicMount::StartStaminaTimer() {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimer(StaminaTimerHandle, this, &AMythicMount::StaminaTick,
                                          FMath::Max(0.05f, StaminaTickInterval), true);
    }
}

void AMythicMount::StopStaminaTimer() {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(StaminaTimerHandle);
    }
}

float AMythicMount::SpeedForGait(EMythicMountGait Gait) const {
    switch (Gait) {
        case EMythicMountGait::Gallop:
            return MythicMountStatics::ComputeGallopSpeed(GallopBaseSpeed, BondLevel, GetStamina01());
        case EMythicMountGait::Trot:
            return TrotSpeed;
        case EMythicMountGait::Walk:
        default:
            return WalkSpeed;
    }
}

void AMythicMount::ApplyGaitSpeed() {
    if (UCharacterMovementComponent *CMC = GetCharacterMovement()) {
        CMC->MaxWalkSpeed = SpeedForGait(CurrentGait);
    }
}

void AMythicMount::OnRep_Gait() {
    ApplyGaitSpeed();
}

void AMythicMount::HandleMountDeath(AActor *) {
    if (!HasAuthority()) {
        return;
    }
    if (IsRidden()) {
        ServerDismount();
    }
    SetLifeSpan(FMath::Max(0.1f, CorpseLifetime));
}

FVector AMythicMount::FindDismountLocation() const {
    const FVector Candidate = GetActorLocation() + GetActorRightVector() * DismountSideOffset;
    if (UNavigationSystemV1 *Nav = UNavigationSystemV1::GetCurrent(GetWorld())) {
        FNavLocation Projected;
        if (Nav->ProjectPointToNavigation(Candidate, Projected, FVector(200.0f, 200.0f, 400.0f))) {
            return Projected.Location;
        }
    }
    return Candidate;
}

void AMythicMount::OnPrimaryInteract_Implementation(AActor *Interactor) {
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(Interactor);
    if (!PC) {
        if (const APawn *InteractorPawn = Cast<APawn>(Interactor)) {
            PC = Cast<AMythicPlayerController>(InteractorPawn->GetController());
        }
    }
    if (!PC) {
        return;
    }

    if (HasAuthority()) {
        ServerMount(PC->GetPawn());
    }
    else if (PC->IsLocalController()) {
        PC->ServerInteractPrimary(this);
    }
}

bool AMythicMount::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    if (IsRidden()) {
        return false;
    }
    if (!OwnerPlayerKey.IsEmpty()) {
        const APawn *InteractorPawn = Cast<APawn>(Interactor);
        if (!InteractorPawn) {
            if (const AController *C = Cast<AController>(Interactor)) {
                InteractorPawn = C->GetPawn();
            }
        }
        const AMythicPlayerState *PS = InteractorPawn ? InteractorPawn->GetPlayerState<AMythicPlayerState>() : nullptr;
        if (!PS || PS->GetCanonicalPlayerKey() != OwnerPlayerKey) {
            return false;
        }
    }
    OutInteractionData.InputActionDataTable = MountInputActionDataTable;
    OutInteractionData.PrimaryInteractionName = RideInteractionName;
    return true;
}
