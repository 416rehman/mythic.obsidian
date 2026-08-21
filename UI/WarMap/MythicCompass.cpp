
#include "UI/WarMap/MythicCompass.h"

float FMythicCompass::CompassBearingDegrees(float ViewYawDeg, const FVector& From, const FVector& To) {
    const float DeltaX = static_cast<float>(To.X - From.X);
    const float DeltaY = static_cast<float>(To.Y - From.Y);

    if (FMath::IsNearlyZero(DeltaX) && FMath::IsNearlyZero(DeltaY)) {
        return 0.0f;
    }

    const float TargetYawDeg = FMath::RadiansToDegrees(FMath::Atan2(DeltaY, DeltaX));
    return FMath::UnwindDegrees(TargetYawDeg - ViewYawDeg);
}

float FMythicCompass::CompassStripX(float BearingDeg, float HalfFovDeg, float StripPixelWidth) {
    if (HalfFovDeg <= 0.0f || StripPixelWidth <= 0.0f) {
        return -1.0f;
    }

    if (FMath::Abs(BearingDeg) > HalfFovDeg) {
        return -1.0f;
    }

    const float Normalized = (BearingDeg + HalfFovDeg) / (2.0f * HalfFovDeg);
    return Normalized * StripPixelWidth;
}
