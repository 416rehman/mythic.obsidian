#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayAbilitySpec.h"
#include "Engine/NetSerialization.h"
#include "GameFramework/Character.h"
#include "MythicTraversalComponent.generated.h"

class ACharacter;
class UMythicAbilitySystemComponent;
class UMythicGA_SystemDash;
class UAnimMontage;
class UPrimitiveComponent;

/** A validated capsule route, never accepted from a client. */
struct FMythicMantleRoute {
    TArray<FVector, TInlineAllocator<3>> Points;
    TWeakObjectPtr<UPrimitiveComponent> Support;
    FTransform SupportTransform;
    float Height = 0;
};

/** One current-pawn movement owner. No catalogue definition, practice, or saved movement. */
UCLASS(ClassGroup=(Mythic), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicTraversalComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UMythicTraversalComponent();
    void InitializeAbilitySystem(UMythicAbilitySystemComponent* InASC);
    UFUNCTION(BlueprintCallable, Category="Traversal") void Jump();
    UFUNCTION(BlueprintCallable, Category="Traversal") void ReleaseJump();
    UFUNCTION(BlueprintCallable, Category="Traversal") void Dash();
    UFUNCTION(BlueprintCallable, Category="Traversal") void ReleaseDash();
    UFUNCTION(BlueprintCallable, Category="Traversal") void CancelTraversal();
    bool IsMantling() const { return ActiveRequest != 0; }
    bool OwnsDashSpec(FGameplayAbilitySpecHandle Handle) const;
    static bool HasValidDashSettings();
    static bool BuildDashDestination(ACharacter* Character, FVector Direction, FVector& Destination);
    static bool BuildMantleRoute(ACharacter* Character, FVector Direction, FMythicMantleRoute& Route);
    static bool CaptureDirections(ACharacter* Character, FVector& Aim, FVector& Movement);
    static bool IsMovementAllowed(const ACharacter* Character);
    virtual void TickComponent(float Delta, ELevelTick TickType, FActorComponentTickFunction* Tick) override;
    virtual void EndPlay(EEndPlayReason::Type Reason) override;

    /** Retained visual assets only. Gameplay never executes the old Parkour Blueprint. */
    UPROPERTY(EditDefaultsOnly, Category="Traversal|Mantle") TObjectPtr<UAnimMontage> LowMantleMontage;
    UPROPERTY(EditDefaultsOnly, Category="Traversal|Mantle") TObjectPtr<UAnimMontage> HighMantleMontage;
    UPROPERTY(EditDefaultsOnly, Category="Traversal|Mantle", meta=(ClampMin="0.2",ClampMax="2.0")) float LowMantleSeconds=0.65f;
    UPROPERTY(EditDefaultsOnly, Category="Traversal|Mantle", meta=(ClampMin="0.2",ClampMax="2.0")) float HighMantleSeconds=1.0f;
private:
    UFUNCTION(Server,Reliable) void ServerRequestMantle(uint32 Request, FVector_NetQuantizeNormal Direction);
    UFUNCTION(Server,Reliable) void ServerCancelMantle(uint32 Request);
    UFUNCTION(Client,Reliable) void ClientMantleRejected(uint32 Request);
    UFUNCTION(NetMulticast,Reliable) void MulticastMantlePresentation(uint32 Request, bool bStart, bool bHigh, float Seconds);
    void BeginMantle(uint32 Request, FMythicMantleRoute&& Route);
    UFUNCTION() void OnMantleMovementModeChanged(ACharacter* Character, EMovementMode PreviousMode, uint8 PreviousCustomMode);
    bool StartMantleLeg();
    void FinishMantle(bool bCancelled);
    bool ValidateActiveMantle() const;
    UPROPERTY(Transient) TObjectPtr<UMythicAbilitySystemComponent> ASC;
    FGameplayAbilitySpecHandle DashHandle;
    FMythicMantleRoute ActiveRoute;
    uint32 NextRequest=0, LastServerRequest=0, PendingRequest=0, ActiveRequest=0, VisualRequest=0;
    uint16 MotionId=0;
    int32 Leg=0;
    double LegEnd=0, MantleEnd=0;
    float MantleDuration=0;
    bool bJumpHeld=false, bEnding=false, bOwnsMantleMode=false, bSettingMantleMode=false;
    uint8 PreviousMovementMode=0, PreviousCustomMovementMode=0;
    UPROPERTY(Transient) TObjectPtr<UAnimMontage> PlayingMontage;
    TWeakObjectPtr<class UAnimInstance> MantleAnim;
    uint8 PreviousRootMotionMode=0;
};
