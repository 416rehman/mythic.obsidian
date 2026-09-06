


#include "MythicNavigationComponent.h"
#include "Input/MythicControlsComponent.h"

#include "DestinationComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "NiagaraFunctionLibrary.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Mythic/Mythic.h"
#include "Player/MythicPlayerController.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/NavMovementInterface.h"


UMythicNavigationComponent::UMythicNavigationComponent() {
    PrimaryComponentTick.bCanEverTick = false;

    this->ShortPressThreshold = 0.3f;
    this->ObstacleAvoidanceDistance = 100.0f;
    this->bUseObstacleAvoidance = true;
}

void UMythicNavigationComponent::BeginPlay() {
    Super::BeginPlay();
    OwnerController = Cast<APlayerController>(GetOwner());

    if (OwnerController) {
        if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwnerController->GetLocalPlayer())) {
            SetupMappingContext(Subsystem);
            SetupBinds();
        }

        OwnerController->OnPossessedPawnChanged.AddDynamic(this, &UMythicNavigationComponent::OnPossessedPawnChanged);
        CacheAndPrepareCharacter(OwnerController->GetPawn());
    }
    else {
        UE_LOG(Myth, Warning,
               TEXT("MythicNavigationComponent is not attached to a PlayerController. Please attach it to a PlayerController to enable navigation."));
    }
}

void UMythicNavigationComponent::OnPossessedPawnChanged(APawn *OldPawn, APawn *NewPawn) {
    CancelDestinationCommand();
    if (CachedPlayerInteractionSphere) CachedPlayerInteractionSphere->OnComponentBeginOverlap.RemoveDynamic(this,&ThisClass::OnInteractionOverlapped);
    if (CachedPlayerObstacleCapsule) CachedPlayerObstacleCapsule->OnComponentBeginOverlap.RemoveDynamic(this,&ThisClass::OnObstacleOverlapped);
    CachedPlayerInteractionSphere=nullptr; CachedPlayerObstacleCapsule=nullptr;
    CacheAndPrepareCharacter(NewPawn);
}

void UMythicNavigationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    CancelDestinationCommand();
    if (CachedPlayerInteractionSphere) {
        CachedPlayerInteractionSphere->OnComponentBeginOverlap.RemoveDynamic(this, &UMythicNavigationComponent::OnInteractionOverlapped);
    }
    if (CachedPlayerObstacleCapsule) {
        CachedPlayerObstacleCapsule->OnComponentBeginOverlap.RemoveDynamic(this, &UMythicNavigationComponent::OnObstacleOverlapped);
    }
    if (OwnerController) {
        OwnerController->OnPossessedPawnChanged.RemoveDynamic(this, &UMythicNavigationComponent::OnPossessedPawnChanged);
        if (UEnhancedInputComponent *EIC = Cast<UEnhancedInputComponent>(OwnerController->InputComponent)) {
            for (uint32 Handle : BindHandles) {
                EIC->RemoveBindingByHandle(Handle);
            }
        }
    }
    BindHandles.Reset();

    Super::EndPlay(EndPlayReason);
}

void UMythicNavigationComponent::CacheAndPrepareCharacter(APawn *owningPawn) {
    this->CachedPlayerCharacter = Cast<ACharacter>(owningPawn);
    if (CachedPlayerCharacter) {

        if (this->NavigationType == ENavigationType::DestinationBased) {
            if (this->bUseObstacleAvoidance) { this->CachePlayerObstacleCapsule(); }
            this->CachePlayerInteractionSphere();
        }
    }
}

void UMythicNavigationComponent::CachePlayerInteractionSphere() {
    if (this->CachedPlayerInteractionSphere) {
        this->CachedPlayerInteractionSphere->OnComponentBeginOverlap.RemoveDynamic(this, &UMythicNavigationComponent::OnInteractionOverlapped);
    }

    auto SphereComponents = this->CachedPlayerCharacter->GetComponentsByTag(USphereComponent::StaticClass(), "InteractionSphere");
    if (SphereComponents.Num() > 0) {
        this->CachedPlayerInteractionSphere = Cast<USphereComponent>(SphereComponents[0]);
    }

    if (this->CachedPlayerInteractionSphere) {
        this->CachedPlayerInteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &UMythicNavigationComponent::OnInteractionOverlapped);
        RefreshInteractionEvents();
    }
    else {
        UE_LOG(Myth, Warning,
               TEXT(
                   "No Sphere Component with tag 'InteractionSphere' found on the character. Please add a sphere component with the tag 'InteractionSphere' to the character to enable interaction."
               ));
    }
}

void UMythicNavigationComponent::CachePlayerObstacleCapsule() {
    if (this->CachedPlayerObstacleCapsule) {
        this->CachedPlayerObstacleCapsule->OnComponentBeginOverlap.RemoveDynamic(this, &UMythicNavigationComponent::OnObstacleOverlapped);
    }

    auto CapsuleComponents = this->CachedPlayerCharacter->GetComponentsByTag(UCapsuleComponent::StaticClass(), "NavigationCapsule");
    if (CapsuleComponents.Num() > 0) {
        this->CachedPlayerObstacleCapsule = Cast<UCapsuleComponent>(CapsuleComponents[0]);
        UE_LOG(Myth, Warning, TEXT("NavigationCapsule found"));
        this->CachedPlayerObstacleCapsule->OnComponentBeginOverlap.AddDynamic(this, &UMythicNavigationComponent::OnObstacleOverlapped);
    }
    else {
        UE_LOG(Myth, Warning,
               TEXT(
                   "No Capsule Component with tag 'NavigationCapsule' found on the character. Adding a capsule component with the tag 'NavigationCapsule' to the character to enable collision avoidance."
               ));
        AddNavigationCapsuleToChar();
    }

    this->CachedPlayerObstacleCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void UMythicNavigationComponent::AddNavigationCapsuleToChar() {
    if (this->CachedPlayerObstacleCapsule) {
        this->CachedPlayerObstacleCapsule->DestroyComponent();
    }

    this->CachedPlayerObstacleCapsule = NewObject<UCapsuleComponent>(CachedPlayerCharacter);
    this->CachedPlayerObstacleCapsule->ComponentTags.Add("NavigationCapsule");
    this->CachedPlayerObstacleCapsule->SetupAttachment(CachedPlayerCharacter->GetMesh());

    this->CachedPlayerObstacleCapsule->SetCapsuleHalfHeight(this->ObstacleAvoidanceDistance);
    this->CachedPlayerObstacleCapsule->SetCapsuleRadius(40.f);

    this->CachedPlayerObstacleCapsule->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
    this->CachedPlayerObstacleCapsule->SetRelativeLocation(FVector(0, this->ObstacleAvoidanceDistance, 50.0f));

    this->CachedPlayerObstacleCapsule->SetCollisionProfileName("OverlapAll");
    this->CachedPlayerObstacleCapsule->SetGenerateOverlapEvents(true);
    this->CachedPlayerObstacleCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    this->CachedPlayerObstacleCapsule->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
    this->CachedPlayerObstacleCapsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

    this->CachedPlayerObstacleCapsule->OnComponentBeginOverlap.AddDynamic(this, &UMythicNavigationComponent::OnObstacleOverlapped);

    this->CachedPlayerObstacleCapsule->RegisterComponent();
}

bool UMythicNavigationComponent::HandleInteraction(AActor *overlappedActor) {
    return false;
}

void UMythicNavigationComponent::OnInteractionOverlapped(UPrimitiveComponent *PrimitiveComponent, AActor *Actor,
                                                         UPrimitiveComponent *PrimitiveComponent1, int I, bool bArg, const FHitResult &HitResult) {
    if (this->targetActor == Actor) {
        this->targetActor = nullptr;

        if (HandleInteraction(Actor)) {
            UE_LOG(Myth, Warning, TEXT("Interaction Handled"));
        }
        else {
        }
    }
}


void UMythicNavigationComponent::SetupMappingContext(UEnhancedInputLocalPlayerSubsystem *Subsystem) {
    switch (this->NavigationType) {
    case ENavigationType::Controlled:
        Subsystem->RemoveMappingContext(this->DestinationBasedMappingContext);
        Subsystem->AddMappingContext(this->ControlledMappingContext, 0);
        break;
    default:
        Subsystem->RemoveMappingContext(this->ControlledMappingContext);
        Subsystem->AddMappingContext(this->DestinationBasedMappingContext, 0);
        break;
    }
}

void UMythicNavigationComponent::SetupBinds() {
    if (UEnhancedInputComponent *EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(OwnerController->InputComponent)) {
        for (uint32 Handle : BindHandles) {
            EnhancedInputComponent->RemoveBindingByHandle(Handle);
        }
        BindHandles.Reset();

        switch (NavigationType) {
        case ENavigationType::Controlled:
            BindHandles.Add(EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Started, this,
                                                               &UMythicNavigationComponent::OnControlledInputStarted).GetHandle());
            BindHandles.Add(EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this,
                                                               &UMythicNavigationComponent::OnMoveForwardTriggered).GetHandle());
            BindHandles.Add(EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Started, this,
                                                               &UMythicNavigationComponent::OnControlledInputStarted).GetHandle());
            BindHandles.Add(EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Triggered, this,
                                                               &UMythicNavigationComponent::OnMoveRightTriggered).GetHandle());

            break;

        default:
            BindHandles.Add(EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this,
                                                               &UMythicNavigationComponent::OnInputStarted).GetHandle());
            BindHandles.Add(EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this,
                                                               &UMythicNavigationComponent::OnSetDestinationTriggered).GetHandle());
            BindHandles.Add(EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this,
                                                               &UMythicNavigationComponent::OnSetDestinationReleased).GetHandle());
            BindHandles.Add(EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this,
                                                               &UMythicNavigationComponent::OnDestinationInputCanceled).GetHandle());

            BindHandles.Add(EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this,
                                                               &UMythicNavigationComponent::OnTouchStarted).GetHandle());
            BindHandles.Add(EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this,
                                                               &UMythicNavigationComponent::OnTouchTriggered).GetHandle());
            BindHandles.Add(EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this,
                                                               &UMythicNavigationComponent::OnTouchReleased).GetHandle());
            BindHandles.Add(EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this,
                                                               &UMythicNavigationComponent::OnDestinationInputCanceled).GetHandle());

            break;
        }
    }
}

void UMythicNavigationComponent::OnObstacleOverlapped(UPrimitiveComponent *PrimitiveComponent, AActor *Actor,
                                                      UPrimitiveComponent *PrimitiveComponent1, int I, bool bArg, const FHitResult &HitResult) {
    if (NavigationType != ENavigationType::DestinationBased || !CanAcceptNavigationInput()) {
        return;
    }
    FVector CurrentDirection;
    if (!(bDestinationPressActive && DestinationPressPawn.Get()==OwnerController->GetPawn())
        && !TryGetCurrentCommandDirection(OwnerController->GetPawn(),CurrentDirection)) return;
    if (Actor != CachedPlayerCharacter) {
        UE_LOG(Myth, Warning, TEXT("Obstacle %s"), *Actor->GetName());
        this->OwnerController->StopMovement();
        IssueDestinationCommand(CachedDestination);

        this->AvoidingObstacleSince = GetWorld()->GetTimeSeconds();
    }
}

void UMythicNavigationComponent::RefreshInteractionEvents() {
    if (this->CachedPlayerInteractionSphere) {
        this->CachedPlayerInteractionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        this->CachedPlayerInteractionSphere->SetHiddenInGame(true);

        this->CachedPlayerInteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        this->CachedPlayerInteractionSphere->SetHiddenInGame(false);
    }
}


void UMythicNavigationComponent::OnInputStarted() { BeginDestinationPress(false); }
void UMythicNavigationComponent::OnTouchStarted() { BeginDestinationPress(true); }
void UMythicNavigationComponent::BeginDestinationPress(bool bTouch) {
    CancelDestinationCommand();
    if (!CanAcceptNavigationInput()) return;
    bDestinationPressActive=true; DestinationPressPawn=OwnerController->GetPawn(); bIsTouch=bTouch;
    this->OwnerController->StopMovement();
    if (!CacheLocationUnderCursor()) { CancelDestinationCommand(); return; }

    this->HandleMoveToDestinationActor();

    if (this->bUseObstacleAvoidance) {
        this->AvoidingObstacleSince = 0.0f;
        if (this->CachedPlayerObstacleCapsule) {
            this->CachedPlayerObstacleCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        }
    }
}

bool UMythicNavigationComponent::CacheLocationUnderCursor() {
    FHitResult Hit;
    bool bHitSuccessful = false;
    if (this->bIsTouch) {
        bHitSuccessful = this->OwnerController->GetHitResultUnderFinger(
            ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit);
    }
    else {
        bHitSuccessful = this->OwnerController->GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
    }

    if (bHitSuccessful && !Hit.Location.ContainsNaN()) {
        this->CachedDestination = Hit.Location;
        return true;
    }
    return false;
}

void UMythicNavigationComponent::OnSetDestinationTriggered() {
    if (!CanAcceptNavigationInput()) { CancelDestinationCommand(); return; }
    if (!bDestinationPressActive || DestinationPressPawn.Get()!=OwnerController->GetPawn()) return;
    this->FollowTime += GetWorld()->GetDeltaSeconds();

    if (this->targetActor) {
        UE_LOG(Myth, Warning, TEXT("Skipping Set Destination Triggered. Target Actor: %s"), *this->targetActor->GetName());
        return;
    }

    if (this->bUseObstacleAvoidance) {
        if (this->AvoidingObstacleSince > 0.f) {
            if (this->GetWorld()->TimeSeconds - this->AvoidingObstacleSince > 0.50f) {
                UE_LOG(Myth, Warning, TEXT("Resetting Avoiding Obstacle Since"));
                this->AvoidingObstacleSince = 0.0f;
                this->OwnerController->StopMovement();
            }
            else {
                UE_LOG(Myth, Warning, TEXT("Avoiding Obstacle"));
                return;
            }
        }
    }

    if (!CacheLocationUnderCursor()) { CancelDestinationCommand(); return; }

    APawn *ControlledPawn = this->OwnerController->GetPawn();
    if (ControlledPawn != nullptr) {
        FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
        ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
    }
}

void UMythicNavigationComponent::OnSetDestinationReleased() {
    if (!bDestinationPressActive || !OwnerController || DestinationPressPawn.Get()!=OwnerController->GetPawn()) return;
    bDestinationPressActive=false; DestinationPressPawn.Reset();
    if (!CanAcceptNavigationInput()) { CancelDestinationCommand(); return; }
    if (this->FollowTime <= ShortPressThreshold && this->targetActor == nullptr) {
        if (!CacheLocationUnderCursor()) { CancelDestinationCommand(); return; }
        IssueDestinationCommand(CachedDestination);
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator,
                                                       FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
    }
    else {
        if (this->bStopOnRelease && this->targetActor == nullptr) {
            ClearDestinationCommand(true);
            this->OwnerController->StopMovement();
        }

        if (this->bUseObstacleAvoidance) {
            if (this->CachedPlayerObstacleCapsule) {
                this->CachedPlayerObstacleCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            }
        }
    }

    FollowTime = 0.f;
}

void UMythicNavigationComponent::OnTouchTriggered() {
    OnSetDestinationTriggered();
}

void UMythicNavigationComponent::OnTouchReleased() {
    OnSetDestinationReleased();
    bIsTouch = false;
}

bool UMythicNavigationComponent::HandleMoveToDestinationActor() {
    this->targetActor = nullptr;
    bool success = false;

    FHitResult Hit;
    bool bHitSuccessful = bIsTouch
        ? OwnerController->GetHitResultUnderFinger(ETouchIndex::Touch1,ECC_Visibility,true,Hit)
        : OwnerController->GetHitResultUnderCursor(ECC_Visibility,true,Hit);
    if (bHitSuccessful && Hit.GetActor() && Hit.GetActor() != CachedPlayerCharacter && Hit.GetActor()->FindComponentByClass<UDestinationComponent>()) {
        this->targetActor = Hit.GetActor();

        this->CachedDestination = Hit.ImpactPoint;
        IssueDestinationCommand(CachedDestination);
        UE_LOG(Myth, Warning, TEXT("Target Actor: %s"), *this->targetActor->GetName());
        success = true;
    }
    this->RefreshInteractionEvents();
    return success;
}

void UMythicNavigationComponent::OnControlledInputStarted() { CancelDestinationCommand(); }

void UMythicNavigationComponent::OnMoveForwardTriggered(const FInputActionValue &Value) {
    if (CanAcceptNavigationInput() && OwnerController->PlayerCameraManager) {
        auto forward = UKismetMathLibrary::ProjectVectorOnToPlane(OwnerController->PlayerCameraManager->GetActorForwardVector(), FVector::UpVector);
        forward = forward.GetSafeNormal();
        CachedPlayerCharacter->AddMovementInput(forward, Value.Get<float>(), false);
    }
}

void UMythicNavigationComponent::OnMoveRightTriggered(const FInputActionValue &Value) {
    if (CanAcceptNavigationInput() && OwnerController->PlayerCameraManager) {
        auto right = UKismetMathLibrary::ProjectVectorOnToPlane(OwnerController->PlayerCameraManager->GetActorRightVector(), FVector::UpVector);
        right = right.GetSafeNormal();
        CachedPlayerCharacter->AddMovementInput(right, Value.Get<float>(), false);
    }
}

bool UMythicNavigationComponent::SetNavigationType(ENavigationType NewType) {
    CancelDestinationCommand();
    if (!IsValid(OwnerController)) return false;
    if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwnerController->GetLocalPlayer())) {
        this->NavigationType = NewType;
        SetupMappingContext(Subsystem);
        SetupBinds();
        return true;
    }
    return false;
}

bool UMythicNavigationComponent::CanAcceptNavigationInput() const {
    if (!IsValid(OwnerController) || !OwnerController->IsLocalController() || !IsValid(CachedPlayerCharacter)
        || OwnerController->GetPawn()!=CachedPlayerCharacter || CachedPlayerCharacter->GetController()!=OwnerController
        || OwnerController->IsMoveInputIgnored()) return false;
    const auto* Mythic=Cast<AMythicPlayerController>(OwnerController);
    return !Mythic || !Mythic->IsGameplayAbilityInputBlocked()
        || (Mythic->ControlsComponent && Mythic->ControlsComponent->IsGameplayWheelOpen());
}

void UMythicNavigationComponent::IssueDestinationCommand(FVector Destination) {
    ClearDestinationCommand(true);
    if (!CanAcceptNavigationInput() || Destination.ContainsNaN()) return;
    APawn* Pawn=OwnerController->GetPawn();
    const auto* PreviousPath=OwnerController->FindComponentByClass<UPathFollowingComponent>();
    const FAIRequestID PreviousRequest=PreviousPath ? PreviousPath->GetCurrentRequestId() : FAIRequestID::InvalidRequest;
    UAIBlueprintHelperLibrary::SimpleMoveToLocation(OwnerController,Destination);
    auto* Path=OwnerController->FindComponentByClass<UPathFollowingComponent>();
    if (!CanAcceptNavigationInput() || !Path || OwnerController->GetPawn()!=Pawn || Path->GetStatus()!=EPathFollowingStatus::Moving || !Path->HasValidPath()
        || (Path==PreviousPath && Path->GetCurrentRequestId()==PreviousRequest)) return;
    CommandPawn=Pawn; CommandPath=Path; CommandRequest=Path->GetCurrentRequestId();
    CommandFinished=Path->OnRequestFinished.AddWeakLambda(this,[this](FAIRequestID Id,const FPathFollowingResult&) {
        if (Id==CommandRequest) {
            ClearDestinationCommand(false);
            if (!bDestinationPressActive && CachedPlayerObstacleCapsule) CachedPlayerObstacleCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
    });
}

void UMythicNavigationComponent::ClearDestinationCommand(bool bAbort) {
    auto* Path=CommandPath.Get(); const auto Request=CommandRequest;
    if (Path) Path->OnRequestFinished.Remove(CommandFinished);
    CommandFinished.Reset(); CommandPath.Reset(); CommandPawn.Reset(); CommandRequest=FAIRequestID::InvalidRequest;
    if (bAbort && Path && Request.IsValid() && Path->GetCurrentRequestId()==Request)
        Path->AbortMove(*this,FPathFollowingResultFlags::ForcedScript,Request);
}

void UMythicNavigationComponent::CancelDestinationCommand() {
    bDestinationPressActive=false; DestinationPressPawn.Reset(); FollowTime=0; targetActor=nullptr; bIsTouch=false;
    if (CachedPlayerObstacleCapsule) CachedPlayerObstacleCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ClearDestinationCommand(true);
}
void UMythicNavigationComponent::OnDestinationInputCanceled() { CancelDestinationCommand(); }

bool UMythicNavigationComponent::TryGetCurrentCommandDirection(const APawn* Pawn,FVector& Direction) const {
    Direction=FVector::ZeroVector;
    const auto* Path=CommandPath.Get();
    if (!CanAcceptNavigationInput() || NavigationType!=ENavigationType::DestinationBased || !Pawn || Pawn!=CommandPawn.Get()
        || Pawn!=OwnerController->GetPawn() || !Path || Path->GetOwner()!=OwnerController || !CommandRequest.IsValid()
        || Path->GetCurrentRequestId()!=CommandRequest || Path->GetStatus()!=EPathFollowingStatus::Moving || !Path->HasValidPath()) return false;
    const auto* Nav=Pawn->FindComponentByInterface<INavMovementInterface>();
    const auto* Updated=Nav ? Cast<UPrimitiveComponent>(Nav->GetUpdatedObject()) : nullptr;
    if (!Nav || Nav->GetOwnerAsObject()!=Pawn || Nav->GetPathFollowingAgent()!=static_cast<const IPathFollowingAgentInterface*>(Path)
        || !Updated || !Updated->IsRegistered() || Updated->GetOwner()!=Pawn) return false;
    Direction=(Path->GetCurrentTargetLocation()-Nav->GetFeetLocation()).GetSafeNormal2D();
    if (Direction.ContainsNaN() || Direction.IsNearlyZero()) { Direction=FVector::ZeroVector; return false; }
    return true;
}
