#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MythicControlsSettings.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "MythicControlsComponent.generated.h"

class UInputAction;
class UInputMappingContext;
class UEnhancedInputLocalPlayerSubsystem;
class UEnhancedInputUserSettings;
class UMythicGameplayWheel;
class UMythicControlsPage;
class UMythicAbilitySystemComponent;
class AMythicPlayerController;
class ACharacter;
class APawn;

/** The action selected on a real physical press never changes until that press is released. */
struct MYTHIC_API FMythicControlPress {
    EMythicControl Control = EMythicControl::Count;
    double StartedAt = 0;
    bool bWheelOpened = false;
};

/** Exact local physical owner captured before any wheel-opening callback. */
struct FMythicWheelOpenToken {
    uint64 InputEpoch=0;
    FKey Key;
    double StartedAt=0;
    EMythicControl Control=EMythicControl::Count;
    TWeakObjectPtr<APawn> Pawn;
};

DECLARE_MULTICAST_DELEGATE(FMythicControlsChanged);
DECLARE_MULTICAST_DELEGATE_OneParam(FMythicCameraAdjustmentChanged,bool);
DECLARE_MULTICAST_DELEGATE_ThreeParams(FMythicInteractionHoldProgress, const FMythicEntityPresentationInstance&, FGameplayTag, float);

/** Local input ownership resolved from Enhanced Input's persisted effective key profile. No gameplay RPCs here. */
UCLASS(ClassGroup=(Input), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicControlsComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UMythicControlsComponent();
    void InitializeLocalInput();
    bool HandleKey(FKey Key, EInputEvent Event);
    void AdvanceInput(float DeltaSeconds);
    void CancelHeldInput();
    bool IsGameplayWheelOpen() const;
    bool IsWheelOpenCurrent(const FMythicWheelOpenToken& Token) const;
    bool IsAbilityLayerHeld() const;
    bool IsCameraAdjustmentOpen() const { return bCameraAdjustment; }
    FKey GetEffectiveKey(EMythicControl Control, bool bGamepad) const;
    FText GetBindingLabel(EMythicControl Control, bool bGamepad) const;
    const UInputAction* GetInputAction(EMythicControl Control) const;
    FGameplayTag GetAbilityInputTag(EMythicControl Control) const;
    bool RemapControl(EMythicControl Control, bool bGamepad, FKey Key, FText& OutError);
    bool SwapControls(EMythicControl First, EMythicControl Second, bool bGamepad, FText& OutError);
    void ResetMappings();
    void OpenControlsPage();
    void BeginCameraAdjustment();
    void EndCameraAdjustment();
    void CloseWheel(bool bCommit);
    /** Only the synchronous overlay-close flush may preserve a pending physical traversal press. */
    bool IsClosingForTraversalInput() const { return bTraversalCloseHandoff; }
    void ExecuteWheelCommand(EMythicControl Command, int32 Option = INDEX_NONE);
    bool IsControlKey(FKey Key, EMythicControl Control) const;
    FVector GetMovementIntent() const;
    FMythicControlsChanged OnControlsChanged;
    FMythicCameraAdjustmentChanged OnCameraAdjustmentChanged;
    FMythicInteractionHoldProgress OnInteractionHoldProgress;
    static bool IsSkill(EMythicControl Control);
    static bool IsMovement(EMythicControl Control);
    static bool IsWheel(EMythicControl Control);
    static EMythicControl ResolvePress(const TArray<FMythicControlBinding>& Bindings, FKey Key, bool bLayerHeld);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;

private:
#if WITH_DEV_AUTOMATION_TESTS
    friend struct FMythicControlsJumpHandoffTestAccess;
    friend struct FMythicControlsProfileStartupTestAccess;
    friend struct FMythicControlsWheelMappingTestAccess;
#endif
    bool CloseOverlayForTraversal(FKey Key);
    AMythicPlayerController* Player() const;
    UMythicAbilitySystemComponent* AbilitySystem() const;
    void DispatchPress(EMythicControl Control);
    void DispatchRelease(EMythicControl Control, bool bCanceled);
    void AdvanceHoldActions();
    void StartWheel(FKey OpenerKey);
    void BeginInteraction();
    void EndInteraction(bool bCancel);
    void CompleteInteractionHold();
    void RefreshMappings();
    UFUNCTION() void HandleMappingsRebuilt();
    UFUNCTION() void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);
    UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MappingContext;
    UPROPERTY(Transient) TMap<EMythicControl,TObjectPtr<UInputAction>> Actions;
    UPROPERTY(Transient) TObjectPtr<UMythicGameplayWheel> Wheel;
    UPROPERTY(Transient) TObjectPtr<UMythicControlsPage> ControlsPage;
    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSubsystem;
    TWeakObjectPtr<UEnhancedInputUserSettings> UserSettings;
    TArray<FMythicControlBinding> EffectiveBindings;
    TMap<FKey,FMythicControlPress> Presses;
    TMap<EMythicControl,int32> HeldActions;
    TMap<EMythicControl,TWeakObjectPtr<UMythicAbilitySystemComponent>> HeldAbilitySystems;
    TWeakObjectPtr<ACharacter> HeldCrouchCharacter;
    TWeakObjectPtr<ACharacter> HeldJumpCharacter;
    TSet<FKey> SuppressedKeys;
    FTimerHandle HoldTimer;
    FTimerHandle InteractionTimer;
    FMythicEntityPresentationInstance InteractionSubject;
    FGameplayTag InteractionAction;
    bool bInteractionRequiresHold = false;
    double InteractionStartedAt = 0;
    float InteractionHoldSeconds = 0;
    bool bInitialized = false;
    bool bCameraAdjustment = false;
    bool bOpeningWheel = false;
    uint64 InputEpoch=1;
    bool bClosingWheel = false;
    bool bCancelingInput = false;
    bool bTraversalCloseHandoff = false;
    bool bTraversalCloseReleased = false;
    FKey TraversalCloseKey;
    bool bEditingMappings = false;
    bool bHasWheelPing = false;
    FMythicEntityPresentationInstance WheelPingSubject;
    FVector WheelPingPoint = FVector::ZeroVector;
};
