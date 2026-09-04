#include "Settings/MythicAgentDetailSettings.h"

UMythicAgentDetailSettings::UMythicAgentDetailSettings() {
    NPCRotation.bRotateInMovementComponent = true;
    NPCRotation.YawRateDegreesPerSecond = 640.0f;
}

FName UMythicAgentDetailSettings::GetCategoryName() const {
    return TEXT("Game");
}

FRotator UMythicAgentDetailSettings::MakeRotationRate(const FMythicAgentRotationConfig &Config) {
    const float Yaw = FMath::Clamp(Config.YawRateDegreesPerSecond, 0.0f, 3600.0f);
    return FRotator(0.0f, Yaw, 0.0f);
}
