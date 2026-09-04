#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MythicAgentDetailSettings.generated.h"

/**
 * How fast an AI pawn turns once rotation is produced inside PerformMovement rather than written from the
 * controller's tick. Yaw is the only axis a walking humanoid turns on; ShouldRemainVertical() zeroes the other two.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAgentRotationConfig {
    GENERATED_BODY()

    /**
     * True produces AI facing inside PerformMovement. False restores the engine default, where the controller's tick
     * writes the pawn transform through APawn::FaceRotation; kept as the rollback and A/B measurement path.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation")
    bool bRotateInMovementComponent = true;

    /** Degrees per second an AI pawn yaws toward its controller's desired rotation. Zero means it never turns. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rotation", meta = (ClampMin = "0.0", ClampMax = "3600.0"))
    float YawRateDegreesPerSecond = 640.0f;
};

/**
 * Floor-query cost for AI characters. The player is never given these; only AMythicNPCCharacter reads them.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicAgentFloorConfig {
    GENERATED_BODY()

    /**
     * False lets a near-stationary AI reuse its cached floor instead of sweeping every frame. The movement component
     * still forces a full check after a real move, a teleport, or an invalidated cache, so this trades no accuracy
     * for the sweep an idle NPC does not need. True restores the engine default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Floor")
    bool bAlwaysCheckFloor = false;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mythic Agent Detail"))
class MYTHIC_API UMythicAgentDetailSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    UMythicAgentDetailSettings();

    virtual FName GetCategoryName() const override;

    /** Turn rate every AI-controlled Mythic character uses. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Rotation")
    FMythicAgentRotationConfig NPCRotation;

    /** Floor-query cost for AI characters. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Movement")
    FMythicAgentFloorConfig NPCFloor;

    /** Clamps the authored yaw rate into the FRotator the character movement component consumes. */
    static FRotator MakeRotationRate(const FMythicAgentRotationConfig &Config);
};
