// 

#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "NiagaraSystem.h"
#include "Components/ActorComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "MythicNavigationComponent.generated.h"

UENUM()
enum ENavigationType
{
	Controlled,	// The player controls the movement with WASD or the joystick
	DestinationBased,	// The player clicks on the ground and the character moves to that location
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicNavigationComponent : public UActorComponent
{
	GENERATED_BODY()

	FVector CachedDestination;

	bool bIsTouch; // Is it a touch device
	float FollowTime; // For how long it has been pressed
	float ShortPressThreshold; // How long is a short press
	float AvoidingObstacleSince; // For how long it has been avoiding an obstacle

	UPROPERTY()
	ACharacter* CachedPlayerCharacter; // Cached character

	UPROPERTY()
	UCapsuleComponent* CachedPlayerObstacleCapsule;

	UPROPERTY()
	AActor* targetActor;

	UPROPERTY()
	USphereComponent* CachedPlayerInteractionSphere;

public:
	// Sets default values for this component's properties
	UMythicNavigationComponent();
	void CachePlayerInteractionSphere();
	void CachePlayerObstacleCapsule();
	void CacheAndPrepareCharacter(APawn* owningPawn);
	void SetupMappingContext(UEnhancedInputLocalPlayerSubsystem* Subsystem);
	void SetupBinds();

	UFUNCTION()
	void OnObstacleOverlapped(UPrimitiveComponent* PrimitiveComponent, AActor* Actor, UPrimitiveComponent* PrimitiveComponent1, int I, bool bArg, const FHitResult& HitResult);

	void RefreshInteractionEvents();
	void AddNavigationCapsuleToChar();
	bool HandleInteraction(AActor*);

	UFUNCTION()
	void OnInteractionOverlapped(UPrimitiveComponent* PrimitiveComponent, AActor* Actor, UPrimitiveComponent* PrimitiveComponent1, int I, bool bArg,
								 const FHitResult& HitResult);

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* DestinationBasedMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* ControlledMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	TEnumAsByte<ENavigationType> NavigationType;

	// The cursor to use when clicking on the screen
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Destination Based Navigation")
	UNiagaraSystem* FXCursor;
	
	/** SetDestination Click Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Destination Based Navigation", meta=(AllowPrivateAccess = "true"))
	class UInputAction* SetDestinationClickAction;

	/** SetDestination Touch Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Destination Based Navigation", meta=(AllowPrivateAccess = "true"))
	class UInputAction* SetDestinationTouchAction;

	/** Use Obstacle Avoidance */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Destination Based Navigation", meta=(AllowPrivateAccess = "true"))
	bool bUseObstacleAvoidance;
	
	/** Used as the length of the NavigationCapsule, if no "NavigationCapsule" tagged capsule is found on the character */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Destination Based Navigation", meta=(AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1000.0", EditCondition = "bUseObstacleAvoidance"))
	float ObstacleAvoidanceDistance;

	/** Stops any movement when input is released even if the character is not at the destination */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Destination Based Navigation", meta=(AllowPrivateAccess = "true"))
	bool bStopOnRelease;

	/** MoveForward Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Controlled Navigation", meta=(AllowPrivateAccess = "true"))
	class UInputAction* MoveForwardAction;

	/** MoveRight Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Controlled Navigation", meta=(AllowPrivateAccess = "true"))
	class UInputAction* MoveRightAction;
	
	/** Reference to the player controller */
	UPROPERTY(BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	APlayerController* OwnerController;

	
	
protected:
	UFUNCTION()
	void OnPossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);
	// Called when the game starts
	virtual void BeginPlay() override;
	
	/** DestinationBased handlers */
	void OnInputStarted();
	void CacheLocationUnderCursor();
	void OnSetDestinationTriggered();
	void OnSetDestinationReleased();
	void OnTouchTriggered();
	void OnTouchReleased();
	/** Checks if there is an actor under the cursor with the Destination tag, if so, it sets targetActor to that actor */
	bool HandleMoveToDestinationActor();
	
	/** Controlled Handlers */
	void OnControlledInputStarted();	// Caches the character
	void OnMoveForwardTriggered(const FInputActionValue& Value);
	void OnMoveRightTriggered(const FInputActionValue&);

	UFUNCTION(BlueprintCallable, Category = "Mythic Navigation")
	bool SetNavigationType(ENavigationType NewType);
};
