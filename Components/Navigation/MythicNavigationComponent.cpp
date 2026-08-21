


#include "MythicNavigationComponent.h"

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
    CacheAndPrepareCharacter(NewPawn);
}

void UMythicNavigationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
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
    if (owningPawn) {
        this->CachedPlayerCharacter = Cast<ACharacter>(owningPawn);

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
                                                               &UMythicNavigationComponent::OnSetDestinationReleased).GetHandle());

            BindHandles.Add(EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this,
                                                               &UMythicNavigationComponent::OnInputStarted).GetHandle());
            BindHandles.Add(EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this,
                                                               &UMythicNavigationComponent::OnTouchTriggered).GetHandle());
            BindHandles.Add(EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this,
                                                               &UMythicNavigationComponent::OnTouchReleased).GetHandle());
            BindHandles.Add(EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this,
                                                               &UMythicNavigationComponent::OnTouchReleased).GetHandle());

            break;
        }
    }
}

void UMythicNavigationComponent::OnObstacleOverlapped(UPrimitiveComponent *PrimitiveComponent, AActor *Actor,
                                                      UPrimitiveComponent *PrimitiveComponent1, int I, bool bArg, const FHitResult &HitResult) {
    if (NavigationType != ENavigationType::DestinationBased) {
        return;
    }
    if (Actor != CachedPlayerCharacter) {
        UE_LOG(Myth, Warning, TEXT("Obstacle %s"), *Actor->GetName());
        this->OwnerController->StopMovement();
        UAIBlueprintHelperLibrary::SimpleMoveToLocation(this->OwnerController, this->CachedDestination);

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


void UMythicNavigationComponent::OnInputStarted() {
    this->OwnerController->StopMovement();

    this->HandleMoveToDestinationActor();

    if (this->bUseObstacleAvoidance) {
        this->AvoidingObstacleSince = 0.0f;
        if (this->CachedPlayerObstacleCapsule) {
            this->CachedPlayerObstacleCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        }
    }
}

void UMythicNavigationComponent::CacheLocationUnderCursor() {
    FHitResult Hit;
    bool bHitSuccessful = false;
    if (this->bIsTouch) {
        bHitSuccessful = this->OwnerController->GetHitResultUnderFinger(
            ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit);
    }
    else {
        bHitSuccessful = this->OwnerController->GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
    }

    if (bHitSuccessful) {
        this->CachedDestination = Hit.Location;
    }
}

void UMythicNavigationComponent::OnSetDestinationTriggered() {
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

    CacheLocationUnderCursor();

    APawn *ControlledPawn = this->OwnerController->GetPawn();
    if (ControlledPawn != nullptr) {
        FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
        ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
    }
}

void UMythicNavigationComponent::OnSetDestinationReleased() {
    if (this->FollowTime <= ShortPressThreshold && this->targetActor == nullptr) {
        CacheLocationUnderCursor();
        UAIBlueprintHelperLibrary::SimpleMoveToLocation(this->OwnerController, CachedDestination);
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator,
                                                       FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
    }
    else {
        if (this->bStopOnRelease && this->targetActor == nullptr) {
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
    if (this->FollowTime <= this->ShortPressThreshold) {
        UAIBlueprintHelperLibrary::SimpleMoveToLocation(this->OwnerController, this->CachedDestination);
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, this->FXCursor, this->CachedDestination,
                                                       FRotator::ZeroRotator, FVector(1.f, 1.f, 1.f), true, true,
                                                       ENCPoolMethod::None, true);
    }

    this->FollowTime = 0.f;
}

void UMythicNavigationComponent::OnTouchReleased() {
    bIsTouch = false;
    OnSetDestinationReleased();
}

bool UMythicNavigationComponent::HandleMoveToDestinationActor() {
    this->targetActor = nullptr;
    bool success = false;

    FHitResult Hit;
    bool bHitSuccessful = this->OwnerController->GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
    if (bHitSuccessful && Hit.GetActor() != CachedPlayerCharacter && Hit.GetActor()->FindComponentByClass<UDestinationComponent>()) {
        this->targetActor = Hit.GetActor();

        this->CachedDestination = Hit.ImpactPoint;
        UAIBlueprintHelperLibrary::SimpleMoveToLocation(this->OwnerController, this->CachedDestination);
        UE_LOG(Myth, Warning, TEXT("Target Actor: %s"), *this->targetActor->GetName());
        success = true;
    }
    this->RefreshInteractionEvents();
    return success;
}

void UMythicNavigationComponent::OnControlledInputStarted() {}

void UMythicNavigationComponent::OnMoveForwardTriggered(const FInputActionValue &Value) {
    if (CachedPlayerCharacter && OwnerController->PlayerCameraManager) {
        auto forward = UKismetMathLibrary::ProjectVectorOnToPlane(OwnerController->PlayerCameraManager->GetActorForwardVector(), FVector::UpVector);
        forward = forward.GetSafeNormal();
        CachedPlayerCharacter->AddMovementInput(forward, Value.Get<float>(), false);
    }
}

void UMythicNavigationComponent::OnMoveRightTriggered(const FInputActionValue &Value) {
    if (CachedPlayerCharacter && OwnerController->PlayerCameraManager) {
        auto right = UKismetMathLibrary::ProjectVectorOnToPlane(OwnerController->PlayerCameraManager->GetActorRightVector(), FVector::UpVector);
        right = right.GetSafeNormal();
        CachedPlayerCharacter->AddMovementInput(right, Value.Get<float>(), false);
    }
}

bool UMythicNavigationComponent::SetNavigationType(ENavigationType NewType) {
    if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwnerController->GetLocalPlayer())) {
        this->NavigationType = NewType;
        SetupMappingContext(Subsystem);
        SetupBinds();
        return true;
    }
    return false;
}
