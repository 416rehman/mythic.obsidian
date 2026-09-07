#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"
#include "CommonInputTypeEnum.h"
#include "MythicAbilityIntentComponent.generated.h"

class UCameraRigAsset;
class UInputAction;
class UInputMappingContext;
class UInputComponent;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UCommonInputSubsystem;
class UCommonUIActionRouterBase;
struct FInputActionValue;

UENUM(BlueprintType)
enum class EMythicAbilityAimPolicy : uint8 { ViewCenter, WorldCursor };

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicCameraMode {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) TObjectPtr<UCameraRigAsset> CameraRig;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EMythicAbilityAimPolicy AbilityAimPolicy = EMythicAbilityAimPolicy::ViewCenter;
    /** Explicit presentation capability, authored only on a supported third-person rig. ViewCenter alone includes first-person. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bAllowLockYawAssist = false;
    /** Local rendering only. Authored on the first-person rig, independently of its aim policy. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bHideOwnerHead = false;
};

/** Cooked camera selection and input policy shared by the director and ability intent. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicAbilityCameraDefinition : public UDataAsset {
    GENERATED_BODY()
public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Ability Aim") TArray<FMythicCameraMode> CameraModes;
    UPROPERTY(EditDefaultsOnly, Category="Ability Aim") TObjectPtr<UInputMappingContext> ViewAimContext;
    UPROPERTY(EditDefaultsOnly, Category="Ability Aim") TObjectPtr<UInputMappingContext> WorldAimContext;
    /** Axis2D, with chord blockers but without inverted camera-look modifiers. */
    UPROPERTY(EditDefaultsOnly, Category="Ability Aim") TObjectPtr<UInputAction> VirtualReticleAction;
    UPROPERTY(EditDefaultsOnly, Category="Ability Aim", meta=(ClampMin="0.1", ClampMax="2.0")) float ReticleViewportSpeed = 0.65f;
};

/** Local camera and pointer intent only. The server resolves recipients using its own geometry. */
UCLASS(BlueprintType, ClassGroup=(Input), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicAbilityIntentComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UMythicAbilityIntentComponent();

    UFUNCTION(BlueprintCallable, Category="Ability Aim") bool CycleCameraMode();
    UFUNCTION(BlueprintCallable, Category="Ability Aim") bool SelectCameraMode(int32 Index);
    UFUNCTION(BlueprintPure, Category="Ability Aim") FMythicCameraMode GetSelectedCameraMode() const;
    UFUNCTION(BlueprintPure, Category="Ability Aim") int32 GetSelectedCameraModeIndex() const { return SelectedMode; }
    UFUNCTION(BlueprintCallable, Category="Ability Aim") bool TryCaptureAimDirection(float MaxRange, FVector& OutDirection);
    /** Position in this local player's constrained view, normalized 0..1. No world trace. */
    UFUNCTION(BlueprintCallable, Category="Ability Aim") bool GetLocalReticlePosition(FVector2D& Position);
    bool GetReticleViewportPixel(FVector2D& Position, bool& bUsesHardwareCursor);
    void InitializeLocalInput(UInputComponent* Input);
    void AdvanceReticle(float DeltaSeconds);
    void ResetHeldInput();
    /** Shared bounded camera-ray convergence, also usable by local aim previews. */
    static bool ResolveCameraAim(UWorld* World,AActor* Source,FVector Eye,FVector Camera,FVector Ray,float MaxRange,FVector& OutDirection);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Transient) TObjectPtr<UMythicAbilityCameraDefinition> Definition;
    bool GetLocalView(FIntRect& Rect) const;
    bool GetAimScreenPosition(FVector2D& Position, FIntRect& Rect);
    void ApplyModeContext();
    void ResetPointer();
    void OnDeviceChanged(ECommonInputType Type);
    void OnReticleInput(const FInputActionValue& Value);
    void ClearInputBindings();
    void RefreshGameplayInputConfig();
    void RefreshCursorPresentation();
    int32 SelectedMode = 0;
    FVector2D HeldAxes = FVector2D::ZeroVector;
    FVector2D VirtualPosition = FVector2D::ZeroVector;
    FIntRect PreviousView;
    bool bHasVirtualPosition = false;
    TWeakObjectPtr<UEnhancedInputComponent> BoundInput;
    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> BoundSubsystem;
    TWeakObjectPtr<UCommonInputSubsystem> BoundCommonInput;
    TWeakObjectPtr<UCommonUIActionRouterBase> BoundRouter;
    TWeakObjectPtr<UInputMappingContext> InstalledContext;
    TArray<uint32> BindingHandles;
    FDelegateHandle DeviceHandle;
    FDelegateHandle OwnerHandle;
    FDelegateHandle InputConfigHandle;
};
