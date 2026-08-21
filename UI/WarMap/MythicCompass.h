
#pragma once

#include "CoreMinimal.h"
#include "UI/WarMap/MythicWarMapTypes.h"
#include "MythicCompass.generated.h"


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicCompassMarker {
    GENERATED_BODY()

    /** World-space location this marker is anchored to. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass")
    FVector WorldLocation = FVector::ZeroVector;

    /** What the marker represents (reuses the war-map marker taxonomy so icons/colors are shared with the map). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Compass")
    EMythicWarMapMarkerKind Kind = EMythicWarMapMarkerKind::Waypoint;
};


struct MYTHIC_API FMythicCompass {
    static float CompassBearingDegrees(float ViewYawDeg, const FVector& From, const FVector& To);

    static float CompassStripX(float BearingDeg, float HalfFovDeg, float StripPixelWidth);
};
