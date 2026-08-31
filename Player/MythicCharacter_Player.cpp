// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicCharacter_Player.h"

#include "MythicPlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "Interaction/ContextActions/MythicContextActionDefinition.h"
#include "Interaction/ContextActions/MythicTags_ContextActions.h"
#include "Settings/MythicDeveloperSettings.h"
#include "MythicPlayerController.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationTags.h"

AMythicCharacter_Player::AMythicCharacter_Player() {
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    LifeComponent = CreateDefaultSubobject<UMythicLifeComponent>(TEXT("LifeComponent"));
    EntityPresentationComponent =
        CreateDefaultSubobject<UMythicEntityPresentationComponent>(
            TEXT("EntityPresentationComponent"));

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
}


void AMythicCharacter_Player::InitializeASC() {
    auto OwningController = GetController();
    auto ASCInterface = Cast<IAbilitySystemInterface>(OwningController);
    if (ASCInterface) {
        this->ASC_Ref = ASCInterface->GetAbilitySystemComponent();
        if (ASC_Ref && ASC_Ref->HasBeenInitialized()) {
            ASC_Ref->InitAbilityActorInfo(GetPlayerState<AMythicPlayerState>(), this);
            if (LifeComponent && !LifeComponent->IsInitialized()) {
                LifeComponent->InitializeWithAbilitySystem(ASC_Ref);
            }
            if (HasAuthority() && EntityPresentationComponent) {
                EntityPresentationComponent->AuthorityBindAbilitySystem(ASC_Ref);
            }
        }
        return;
    }

    ASCInterface = Cast<IAbilitySystemInterface>(GetPlayerState());
    if (ASCInterface) {
        this->ASC_Ref = ASCInterface->GetAbilitySystemComponent();
        if (ASC_Ref && ASC_Ref->HasBeenInitialized()) {
            ASC_Ref->InitAbilityActorInfo(GetPlayerState<AMythicPlayerState>(), this);
            if (LifeComponent && !LifeComponent->IsInitialized()) {
                LifeComponent->InitializeWithAbilitySystem(ASC_Ref);
            }
            if (HasAuthority() && EntityPresentationComponent) {
                EntityPresentationComponent->AuthorityBindAbilitySystem(ASC_Ref);
            }
        }
    }
}

void AMythicCharacter_Player::BeginPlay() {
    Super::BeginPlay();
    if (EntityPresentationComponent) {
        EntityPresentationComponent->SetPresentationAnchor(
            GetRootComponent(), FVector(0.0f, 0.0f, 125.0f));
    }
    if (HasAuthority() && LifeComponent && !bBoundLifePresentation) {
        LifeComponent->OnDowned.AddDynamic(
            this, &ThisClass::HandlePlayerDowned);
        LifeComponent->OnRevived.AddDynamic(
            this, &ThisClass::HandlePlayerRevived);
        LifeComponent->OnDeath.AddDynamic(
            this, &ThisClass::HandlePlayerDeath);
        bBoundLifePresentation = true;
    }
    BindPersistentEntityIdentity();
    TryActivateEntityPresentation();
}

void AMythicCharacter_Player::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
    UnbindPersistentEntityIdentity();
    if (LifeComponent && bBoundLifePresentation) {
        LifeComponent->OnDowned.RemoveDynamic(
            this, &ThisClass::HandlePlayerDowned);
        LifeComponent->OnRevived.RemoveDynamic(
            this, &ThisClass::HandlePlayerRevived);
        LifeComponent->OnDeath.RemoveDynamic(
            this, &ThisClass::HandlePlayerDeath);
        bBoundLifePresentation = false;
    }
    if (HasAuthority() && EntityPresentationComponent) {
        EntityPresentationComponent->AuthorityDeactivateEmbodiment();
    }
    Super::EndPlay(EndPlayReason);
}

void AMythicCharacter_Player::PossessedBy(AController *NewController) {
    Super::PossessedBy(NewController);
    BindPersistentEntityIdentity();
    TryActivateEntityPresentation();
}

void AMythicCharacter_Player::OnRep_PlayerState() {
    Super::OnRep_PlayerState();
    if (EntityPresentationComponent) {
        EntityPresentationComponent->SetPresentationAnchor(
            GetRootComponent(), FVector(0.0f, 0.0f, 125.0f));
    }
}

UAbilitySystemComponent *AMythicCharacter_Player::GetAbilitySystemComponent() const {
    return this->ASC_Ref;
}

void AMythicCharacter_Player::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMythicCharacter_Player, ASC_Ref);
}

void AMythicCharacter_Player::OnPrimaryInteract_Implementation(AActor *Interactor) {
    AMythicPlayerController *ReviverPC = Cast<AMythicPlayerController>(Interactor);
    if (!ReviverPC) {
        return;
    }
    if (HasAuthority()) {
        if (!LifeComponent) {
            return;
        }
        bool bReviverDowned = false;
        if (const APawn *ReviverPawn = ReviverPC->GetPawn()) {
            if (const UMythicLifeComponent *ReviverLife = UMythicLifeComponent::FindHealthComponent(ReviverPawn)) {
                bReviverDowned = ReviverLife->IsDowned();
            }
        }
        if (UMythicLifeComponent::CanReviveTarget(LifeComponent->IsDowned(), bReviverDowned)) {
            LifeComponent->ServerBeginReviveChannel(ReviverPC->GetPawn());
        }
        return;
    }
    if (ReviverPC->IsLocalController()) {
        ReviverPC->ServerInteractPrimary(this);
    }
}

void AMythicCharacter_Player::OnSecondaryInteract_Implementation(AActor *Interactor) {
}

USceneComponent *AMythicCharacter_Player::GetWidgetAttachmentComponent_Implementation() const {
    return GetRootComponent();
}

bool AMythicCharacter_Player::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    if (!LifeComponent || !LifeComponent->IsDowned()) {
        return false;
    }
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = ReviveInteractionName;
    return true;
}

void AMythicCharacter_Player::OnFocused_Implementation(AActor *Interactor) {
}

void AMythicCharacter_Player::OnUnfocused_Implementation(AActor *Interactor) {
}

UMythicEntityPresentationComponent *
AMythicCharacter_Player::GetEntityPresentationComponent_Implementation() const {
    return EntityPresentationComponent;
}

void AMythicCharacter_Player::GatherContextActions_Implementation(
    AController *RequestingController, AActor *Subject,
    TArray<FMythicContextActionOffer> &OutOffers) const {
    if (!HasAuthority() || Subject != this || !ReviveContextActionDefinition
        || !LifeComponent || !LifeComponent->IsDowned()) {
        return;
    }

    FMythicContextActionOffer Offer;
    Offer.Definition = ReviveContextActionDefinition;
    Offer.SourceRevision = static_cast<int64>(ContextActionRevision);
    FGameplayTag FailureReason;
    Offer.Availability = ValidateReviveContextAction(
        RequestingController, Subject,
        ReviveContextActionDefinition->ActionTag,
        Offer.SourceRevision, FailureReason)
        ? EMythicContextActionAvailability::Available
        : EMythicContextActionAvailability::UnavailableWithReason;
    Offer.UnavailableReasonTag = FailureReason;
    OutOffers.Add(MoveTemp(Offer));
}

bool AMythicCharacter_Player::CanExecuteContextAction_Implementation(
    AController *RequestingController, AActor *Subject,
    const FGameplayTag ActionTag, const int64 ObservedOfferRevision,
    FGameplayTag &OutFailureReason) const {
    return ValidateReviveContextAction(
        RequestingController, Subject, ActionTag,
        ObservedOfferRevision, OutFailureReason);
}

bool AMythicCharacter_Player::ExecuteContextAction_Implementation(
    AController *RequestingController, AActor *Subject,
    const FGameplayTag ActionTag, const int64 ObservedOfferRevision,
    FGameplayTag &OutFailureReason) {
    if (!ValidateReviveContextAction(
            RequestingController, Subject, ActionTag,
            ObservedOfferRevision, OutFailureReason)) {
        return false;
    }

    APawn *ReviverPawn = RequestingController
        ? RequestingController->GetPawn() : nullptr;
    if (!ReviverPawn || !LifeComponent) {
        OutFailureReason = CONTEXT_ACTION_REASON_INVALID_TARGET;
        return false;
    }
    LifeComponent->ServerBeginReviveChannel(ReviverPawn);
    OutFailureReason = FGameplayTag();
    return true;
}

void AMythicCharacter_Player::BindPersistentEntityIdentity() {
    if (!HasAuthority()) {
        return;
    }
    AMythicPlayerState *MythicPS =
        GetPlayerState<AMythicPlayerState>();
    if (BoundIdentityPlayerState.Get() == MythicPS) {
        return;
    }
    UnbindPersistentEntityIdentity();
    BoundIdentityPlayerState = MythicPS;
    if (!MythicPS) {
        return;
    }
    PersistentEntityIdentityReadyHandle =
        MythicPS->OnPersistentEntityIdentityReady().AddUObject(
            this, &ThisClass::HandlePersistentEntityIdentityReady);
}

void AMythicCharacter_Player::UnbindPersistentEntityIdentity() {
    if (AMythicPlayerState *MythicPS = BoundIdentityPlayerState.Get();
        MythicPS && PersistentEntityIdentityReadyHandle.IsValid()) {
        MythicPS->OnPersistentEntityIdentityReady().Remove(
            PersistentEntityIdentityReadyHandle);
    }
    PersistentEntityIdentityReadyHandle.Reset();
    BoundIdentityPlayerState.Reset();
}

void AMythicCharacter_Player::TryActivateEntityPresentation() {
    if (!HasAuthority() || !EntityPresentationComponent) {
        return;
    }
    AMythicPlayerState *MythicPS =
        GetPlayerState<AMythicPlayerState>();
    if (!MythicPS) {
        return;
    }
    const FMythicEntityId &EntityId = MythicPS->GetPersistentEntityId();
    if (!EntityId.IsValid()
        || EntityId.GetDomain() != EMythicEntityDomain::PlayerCharacter) {
        return;
    }
    if (EntityPresentationComponent->GetAuthorityEntityId() == EntityId
        && EntityPresentationComponent->GetPublicIdentitySnapshot().IsActive()) {
        EntityPresentationComponent->AuthorityBindAbilitySystem(
            GetAbilitySystemComponent());
        return;
    }

    FMythicPublicIdentitySnapshot SafeIdentity;
    SafeIdentity.PublicKindTag =
        MythicEntityPresentationTags::EntityKindPlayer;
    if (!EntityPresentationComponent->AuthorityPrepareEmbodiment(
            EntityId, SafeIdentity)) {
        return;
    }
    EntityPresentationComponent->AuthorityBindAbilitySystem(
        GetAbilitySystemComponent());
    if (!EntityPresentationComponent->AuthorityActivateEmbodiment()) {
        EntityPresentationComponent->AuthorityDeactivateEmbodiment();
        return;
    }

    if (LifeComponent && LifeComponent->IsDead()) {
        EntityPresentationComponent->SetObservableFact(
            MythicEntityPresentationTags::ObservableSlotLifeState,
            MythicEntityPresentationTags::ObservableLifeDead,
            FMythicPresentationHandle());
    } else if (LifeComponent && LifeComponent->IsDowned()) {
        EntityPresentationComponent->SetObservableFact(
            MythicEntityPresentationTags::ObservableSlotLifeState,
            MythicEntityPresentationTags::ObservableLifeDowned,
            FMythicPresentationHandle());
    }
}

void AMythicCharacter_Player::AdvanceContextActionRevision() {
    ++ContextActionRevision;
    if (ContextActionRevision == 0) {
        ContextActionRevision = 1;
    }
}

bool AMythicCharacter_Player::ValidateReviveContextAction(
    AController *RequestingController, AActor *Subject,
    const FGameplayTag ActionTag, const int64 ObservedOfferRevision,
    FGameplayTag &OutFailureReason) const {
    OutFailureReason = FGameplayTag();
    if (!HasAuthority() || Subject != this || !ReviveContextActionDefinition
        || !ReviveContextActionDefinition->ActionTag.IsValid()
        || ActionTag != ReviveContextActionDefinition->ActionTag) {
        OutFailureReason = CONTEXT_ACTION_REASON_INVALID_TARGET;
        return false;
    }
    if (ObservedOfferRevision < 0
        || ObservedOfferRevision > static_cast<int64>(MAX_uint32)
        || static_cast<uint32>(ObservedOfferRevision)
               != ContextActionRevision) {
        OutFailureReason = CONTEXT_ACTION_REASON_STALE;
        return false;
    }
    if (!LifeComponent || !LifeComponent->IsDowned()) {
        OutFailureReason = CONTEXT_ACTION_REASON_UNAVAILABLE;
        return false;
    }

    APawn *ReviverPawn = RequestingController
        ? RequestingController->GetPawn() : nullptr;
    if (!ReviverPawn || ReviverPawn == this) {
        OutFailureReason = CONTEXT_ACTION_REASON_INVALID_TARGET;
        return false;
    }
    const UMythicLifeComponent *ReviverLife =
        UMythicLifeComponent::FindHealthComponent(ReviverPawn);
    if (ReviverLife && ReviverLife->IsDowned()) {
        OutFailureReason = CONTEXT_ACTION_REASON_UNAVAILABLE;
        return false;
    }
    return true;
}

void AMythicCharacter_Player::HandlePlayerDowned(AActor *DownedActor) {
    if (!HasAuthority() || DownedActor != this) {
        return;
    }
    AdvanceContextActionRevision();
    if (EntityPresentationComponent) {
        EntityPresentationComponent->SetObservableFact(
            MythicEntityPresentationTags::ObservableSlotLifeState,
            MythicEntityPresentationTags::ObservableLifeDowned,
            FMythicPresentationHandle());
    }
}

void AMythicCharacter_Player::HandlePlayerRevived(AActor *RevivedActor) {
    if (!HasAuthority() || RevivedActor != this) {
        return;
    }
    AdvanceContextActionRevision();
    if (EntityPresentationComponent) {
        EntityPresentationComponent->ClearObservableFact(
            MythicEntityPresentationTags::ObservableSlotLifeState);
    }
}

void AMythicCharacter_Player::HandlePlayerDeath(AActor *DeadActor) {
    if (!HasAuthority() || DeadActor != this) {
        return;
    }
    AdvanceContextActionRevision();
    if (EntityPresentationComponent) {
        EntityPresentationComponent->SetObservableFact(
            MythicEntityPresentationTags::ObservableSlotLifeState,
            MythicEntityPresentationTags::ObservableLifeDead,
            FMythicPresentationHandle());
    }
}

void AMythicCharacter_Player::HandlePersistentEntityIdentityReady(
    const FMythicEntityId &EntityId) {
    const AMythicPlayerState *MythicPS =
        BoundIdentityPlayerState.Get();
    if (!MythicPS || MythicPS->GetPersistentEntityId() != EntityId) {
        return;
    }
    TryActivateEntityPresentation();
}
