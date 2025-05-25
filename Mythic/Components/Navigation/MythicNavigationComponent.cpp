// 


#include "MythicNavigationComponent.h"

#include "DestinationComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
// #include "InteractableComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetMathLibrary.h"
#include "Mythic/Mythic.h"


// Sets default values for this component's properties
UMythicNavigationComponent::UMythicNavigationComponent() {
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	this->ShortPressThreshold = 0.3f;
	this->ObstacleAvoidanceDistance = 100.0f;
	this->bUseObstacleAvoidance = true;
}

// Called when the game starts
void UMythicNavigationComponent::BeginPlay() {
	Super::BeginPlay();
	OwnerController = Cast<APlayerController>(GetOwner());

	if (OwnerController) {
		//Add Input Mapping Context
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwnerController->GetLocalPlayer())) {
			SetupMappingContext(Subsystem);
			SetupBinds();
		}

		// bind to on-posses event of the controller
		OwnerController->OnPossessedPawnChanged.AddDynamic(this, &UMythicNavigationComponent::OnPossessedPawnChanged);
		CacheAndPrepareCharacter(OwnerController->GetPawn());
	} else {
		UE_LOG(Mythic, Warning, TEXT("MythicNavigationComponent is not attached to a PlayerController. Please attach it to a PlayerController to enable navigation."));
	}
}

void UMythicNavigationComponent::OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn) {
	CacheAndPrepareCharacter(NewPawn);
}

void UMythicNavigationComponent::CacheAndPrepareCharacter(APawn* owningPawn) {
	if (owningPawn) {
		this->CachedPlayerCharacter = Cast<ACharacter>(owningPawn);
		
        if (this->NavigationType == ENavigationType::DestinationBased) {
        	if (this->bUseObstacleAvoidance) { this->CachePlayerObstacleCapsule(); }
        	this->CachePlayerInteractionSphere();
        }
	}
}

void UMythicNavigationComponent::CachePlayerInteractionSphere() {
	auto SphereComponents = this->CachedPlayerCharacter->GetComponentsByTag(USphereComponent::StaticClass(), "InteractionSphere");
	if (SphereComponents.Num() > 0) {
		this->CachedPlayerInteractionSphere = Cast<USphereComponent>(SphereComponents[0]);
	}

	if (this->CachedPlayerInteractionSphere) {
		this->CachedPlayerInteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &UMythicNavigationComponent::OnInteractionOverlapped);
		RefreshInteractionEvents();	// disable interaction events until the player starts moving
	}
	else {
		UE_LOG(Mythic, Warning, TEXT("No Sphere Component with tag 'InteractionSphere' found on the character. Please add a sphere component with the tag 'InteractionSphere' to the character to enable interaction."));
	}
}

void UMythicNavigationComponent::CachePlayerObstacleCapsule() {
	auto CapsuleComponents = this->CachedPlayerCharacter->GetComponentsByTag(UCapsuleComponent::StaticClass(), "NavigationCapsule");
	this->CachedPlayerObstacleCapsule = Cast<UCapsuleComponent>(CapsuleComponents[0]);

	if (this->CachedPlayerObstacleCapsule) {
		UE_LOG(Mythic, Warning, TEXT("NavigationCapsule found"));
		this->CachedPlayerObstacleCapsule->OnComponentBeginOverlap.AddDynamic(this, &UMythicNavigationComponent::OnObstacleOverlapped);
	}
	else {
		UE_LOG(Mythic, Warning, TEXT("No Capsule Component with tag 'NavigationCapsule' found on the character. Adding a capsule component with the tag 'NavigationCapsule' to the character to enable collision avoidance."));
		AddNavigationCapsuleToChar();
	}
	this->CachedPlayerObstacleCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void UMythicNavigationComponent::AddNavigationCapsuleToChar() {
	// Remove the old capsule component
	if (this->CachedPlayerObstacleCapsule) {
		this->CachedPlayerObstacleCapsule->DestroyComponent();
	}

	// Add a new capsule component
	this->CachedPlayerObstacleCapsule = NewObject<UCapsuleComponent>(CachedPlayerCharacter);
	this->CachedPlayerObstacleCapsule->ComponentTags.Add("NavigationCapsule");
	this->CachedPlayerObstacleCapsule->SetupAttachment(CachedPlayerCharacter->GetMesh());

	// Set the capsule component properties
	this->CachedPlayerObstacleCapsule->SetCapsuleHalfHeight(this->ObstacleAvoidanceDistance);
	this->CachedPlayerObstacleCapsule->SetCapsuleRadius(40.f);

	// Position the capsule component
	this->CachedPlayerObstacleCapsule->SetRelativeRotation(FRotator(0.0f, 0.0f, 90.0f));
	this->CachedPlayerObstacleCapsule->SetRelativeLocation(FVector(0, this->ObstacleAvoidanceDistance, 50.0f));

	// Set the collision profile
	this->CachedPlayerObstacleCapsule->SetCollisionProfileName("OverlapAll");
	this->CachedPlayerObstacleCapsule->SetGenerateOverlapEvents(true);
	this->CachedPlayerObstacleCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	this->CachedPlayerObstacleCapsule->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic);
	this->CachedPlayerObstacleCapsule->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);

	// Bind
	this->CachedPlayerObstacleCapsule->OnComponentBeginOverlap.AddDynamic(this, &UMythicNavigationComponent::OnObstacleOverlapped);

	// Register the component
	this->CachedPlayerObstacleCapsule->RegisterComponent();
}

bool UMythicNavigationComponent::HandleInteraction(AActor* overlappedActor) {
	// // Interact with the first interactable component found on the overlapped actor (First component = highest priority)
	// auto InteractionComponent = overlappedActor->FindComponentByClass<UInteractableComponent>();
	// if (InteractionComponent) {			
	// 	this->OwnerController->StopMovement();
	// 	InteractionComponent->Interact(this->CachedPlayerCharacter, this->OwnerController);
	// 	return true;
	// }
	return false;
}

void UMythicNavigationComponent::OnInteractionOverlapped(UPrimitiveComponent* PrimitiveComponent, AActor* Actor,
                                                         UPrimitiveComponent* PrimitiveComponent1, int I, bool bArg, const FHitResult& HitResult) {
	if (this->targetActor == Actor) {
		this->targetActor = nullptr;
		
		if (HandleInteraction(Actor)) {
			UE_LOG(Mythic, Warning, TEXT("Interaction Handled"));
		} else {
			// Attack handling here
		}
	}
}


void UMythicNavigationComponent::SetupMappingContext(UEnhancedInputLocalPlayerSubsystem* Subsystem) {
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
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(OwnerController->InputComponent)) {

		// Add new binds
		switch (NavigationType) {
		case ENavigationType::Controlled:
			EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Started, this,&UMythicNavigationComponent::OnControlledInputStarted);
			EnhancedInputComponent->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this,&UMythicNavigationComponent::OnMoveForwardTriggered);
			EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Started, this,&UMythicNavigationComponent::OnControlledInputStarted);
			EnhancedInputComponent->BindAction(MoveRightAction, ETriggerEvent::Triggered, this,&UMythicNavigationComponent::OnMoveRightTriggered);
			
			break;
			
		default:
			// Setup mouse input events
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Started, this,&UMythicNavigationComponent::OnInputStarted);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Triggered, this,&UMythicNavigationComponent::OnSetDestinationTriggered);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Completed, this,&UMythicNavigationComponent::OnSetDestinationReleased);
			EnhancedInputComponent->BindAction(SetDestinationClickAction, ETriggerEvent::Canceled, this, &UMythicNavigationComponent::OnSetDestinationReleased);

			// Setup touch input events
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Started, this,&UMythicNavigationComponent::OnInputStarted);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Triggered, this,&UMythicNavigationComponent::OnTouchTriggered);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Completed, this,&UMythicNavigationComponent::OnTouchReleased);
			EnhancedInputComponent->BindAction(SetDestinationTouchAction, ETriggerEvent::Canceled, this,&UMythicNavigationComponent::OnTouchReleased);
			
			break;
		}
	}
}

void UMythicNavigationComponent::OnObstacleOverlapped(UPrimitiveComponent* PrimitiveComponent, AActor* Actor,
	UPrimitiveComponent* PrimitiveComponent1, int I, bool bArg, const FHitResult& HitResult) {
	if (NavigationType != ENavigationType::DestinationBased) {
		return;
	}
	// if the actor is not the character
	if (Actor != CachedPlayerCharacter) {
		UE_LOG(Mythic, Warning, TEXT("Obstacle %s"), *Actor->GetName());
		// We stop the movement
		this->OwnerController->StopMovement();
		// Use AI to move around the obstacle
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
	
	// Obstacle Avoidance
	if (this->bUseObstacleAvoidance) {
		this->AvoidingObstacleSince = 0.0f;
		if (this->CachedPlayerObstacleCapsule) {
			this->CachedPlayerObstacleCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
	}
}

void UMythicNavigationComponent::CacheLocationUnderCursor() {
	// We look for the location in the world where the player has pressed the input
	FHitResult Hit;
	bool bHitSuccessful = false;
	if (this->bIsTouch) {
		bHitSuccessful = this->OwnerController->GetHitResultUnderFinger(
			ETouchIndex::Touch1, ECollisionChannel::ECC_Visibility, true, Hit);
	}
	else {
		bHitSuccessful = this->OwnerController->GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, true, Hit);
	}

	// If we hit a surface, cache the location
	if (bHitSuccessful) {
		this->CachedDestination = Hit.Location;
	}
}

void UMythicNavigationComponent::OnSetDestinationTriggered() {
	// We flag that the input is being pressed
	this->FollowTime += GetWorld()->GetDeltaSeconds();

	// If target actor is set, we don't need to do anything, as the inputStarted already handled the movement
	if (this->targetActor) {
		UE_LOG(Mythic, Warning, TEXT("Skipping Set Destination Triggered. Target Actor: %s"), *this->targetActor->GetName());
		return;
	}

	// Obstacle Avoidance
	if (this->bUseObstacleAvoidance) {
		// If currently avoiding an obstacle
		if (this->AvoidingObstacleSince > 0.f) {
			// If we have been avoiding for more than 0.5 seconds
			if (this->GetWorld()->TimeSeconds - this->AvoidingObstacleSince > 0.50f) {
				UE_LOG(Mythic, Warning, TEXT("Resetting Avoiding Obstacle Since"));
				this->AvoidingObstacleSince = 0.0f;
				this->OwnerController->StopMovement();
			}
			// If we have been avoiding for less than 0.5 seconds we do nothing
			else {
				UE_LOG(Mythic, Warning, TEXT("Avoiding Obstacle"));
				return;
			}
		}		
	}
	
	CacheLocationUnderCursor();

	// Move towards mouse pointer or touch
	APawn* ControlledPawn = this->OwnerController->GetPawn();
	if (ControlledPawn != nullptr) {
		FVector WorldDirection = (CachedDestination - ControlledPawn->GetActorLocation()).GetSafeNormal();
		ControlledPawn->AddMovementInput(WorldDirection, 1.0, false);
	}
}

void UMythicNavigationComponent::OnSetDestinationReleased() {
	if (this->FollowTime <= ShortPressThreshold && this->targetActor == nullptr) {
		CacheLocationUnderCursor();
		// We move there and spawn some particles
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this->OwnerController, CachedDestination);
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, FXCursor, CachedDestination, FRotator::ZeroRotator,
		                                               FVector(1.f, 1.f, 1.f), true, true, ENCPoolMethod::None, true);
	}
	// If it was a long press
	else {
		if (this->bStopOnRelease && this->targetActor == nullptr) this->OwnerController->StopMovement();
		
		// Obstacle Avoidance
		if (this->bUseObstacleAvoidance) {
			if (this->CachedPlayerObstacleCapsule) {
				this->CachedPlayerObstacleCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			}
		}
	}

	FollowTime = 0.f;
}

void UMythicNavigationComponent::OnTouchTriggered() {
	// If it was a short press
	if (this->FollowTime <= this->ShortPressThreshold) {
		// We move there and spawn some particles
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
		
		// We set the destination
		this->CachedDestination = Hit.ImpactPoint;
		// We move the character to the destination
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this->OwnerController, this->CachedDestination);
		UE_LOG(Mythic, Warning, TEXT("Target Actor: %s"), *this->targetActor->GetName());
		success = true;
	}
	// Refresh interaction events
	this->RefreshInteractionEvents();
	return success;
}

void UMythicNavigationComponent::OnControlledInputStarted() {}

void UMythicNavigationComponent::OnMoveForwardTriggered(const FInputActionValue& Value) {
	if (CachedPlayerCharacter && OwnerController->PlayerCameraManager) {
	    // Since the camera is pointing down, figure out the final forward vector
	    auto forward = UKismetMathLibrary::ProjectVectorOnToPlane(OwnerController->PlayerCameraManager->GetActorForwardVector(), FVector::UpVector);
	    forward = forward / forward.Length();
		CachedPlayerCharacter->AddMovementInput(forward, Value.Get<float>(), false);
	}
}

void UMythicNavigationComponent::OnMoveRightTriggered(const FInputActionValue& Value) {
	if (CachedPlayerCharacter) {
	    auto right = UKismetMathLibrary::ProjectVectorOnToPlane(OwnerController->PlayerCameraManager->GetActorRightVector(), FVector::UpVector);
	    right = right / right.Length();
		CachedPlayerCharacter->AddMovementInput(right, Value.Get<float>(), false);
	}
}

bool UMythicNavigationComponent::SetNavigationType(ENavigationType NewType) {
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(OwnerController->GetLocalPlayer())) {
		this->NavigationType = NewType;
		SetupMappingContext(Subsystem);
		SetupBinds();
		return true;
	}
	return false;
}