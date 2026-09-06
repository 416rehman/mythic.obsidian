#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "InputCoreTypes.h"
#include "MythicControlsSettings.generated.h"

UENUM(BlueprintType)
enum class EMythicControl : uint8 {
    MoveForward, MoveBackward, MoveLeft, MoveRight,
    Primary, Secondary, Jump, Dash, Crouch, Interact,
    SkillQ, SkillE, SkillR, SkillT, AbilityLayer,
    Heal, Utility, LockTarget, Ping, Inspect, CameraMode, CharacterMenu, SystemMenu,
    Count UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicControlBinding {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadOnly) EMythicControl Control = EMythicControl::Primary;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Label;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FKey Keyboard;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) FKey Gamepad;
    UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bGamepadAbilityLayer = false;
};

/** Defaults are authored once; effective keys and persistence belong to Enhanced Input user profiles. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Mythic Combat Controls"))
class MYTHIC_API UMythicControlsSettings : public UDeveloperSettings {
    GENERATED_BODY()
public:
    UMythicControlsSettings();
    UPROPERTY(Config, EditAnywhere, Category="Controls") TArray<FMythicControlBinding> Bindings;
    UPROPERTY(Config, EditAnywhere, Category="Wheels", meta=(ClampMin="0.15",ClampMax="0.6")) float WheelHoldSeconds = 0.30f;
    UPROPERTY(Config, EditAnywhere, Category="Traversal", meta=(ClampMin="40",ClampMax="600")) float DashDistance = 400;
    UPROPERTY(Config, EditAnywhere, Category="Traversal", meta=(ClampMin="0.1",ClampMax="0.6")) float DashDuration = 0.20f;
    UPROPERTY(Config, EditAnywhere, Category="Traversal", meta=(ClampMin="0",ClampMax="100")) float DashStaminaCost = 10;
    UPROPERTY(Config, EditAnywhere, Category="Traversal", meta=(ClampMin="0.2",ClampMax="5")) float DashCooldown = 0.60f;
    UPROPERTY(Config, EditAnywhere, Category="Targeting") float LockRange = 2000;
    UPROPERTY(Config, EditAnywhere, Category="Targeting") float LockBreakRange = 2400;
    UPROPERTY(Config, EditAnywhere, Category="Targeting") float LockAngleDegrees = 20;
    UPROPERTY(Config, EditAnywhere, Category="Targeting") float LockOcclusionGrace = 0.75f;
    /** Raw viewport pixels around the actual WorldCursor/virtual reticle. Server validates nominations separately. */
    UPROPERTY(Config, EditAnywhere, Category="Targeting", meta=(ClampMin="1", ToolTip="Acquisition radius around the cursor in viewport pixels.")) float LockCursorRadius = 250;
    UPROPERTY(Config, EditAnywhere, Category="Communication") float PingRange = 10000;
    UPROPERTY(Config, EditAnywhere, Category="Communication") float PingLifetime = 8;
    const FMythicControlBinding* Find(EMythicControl Control) const;
    static FName MappingName(EMythicControl Control, bool bGamepad);
};
