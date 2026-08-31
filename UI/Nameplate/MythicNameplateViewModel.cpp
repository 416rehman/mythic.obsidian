#include "UI/Nameplate/MythicNameplateViewModel.h"

UMythicNameplateViewModel::UMythicNameplateViewModel() {
    Projection.Statuses.Reserve(4);
}

void UMythicNameplateViewModel::Apply(
    const FMythicNameplateProjection &InProjection,
    const FVector2D InScreenPosition, const float InPresentationAlpha) {
    Projection = InProjection;
    ScreenPosition = InScreenPosition;
    PresentationAlpha = FMath::Clamp(InPresentationAlpha, 0.0f, 1.0f);
}

void UMythicNameplateViewModel::ApplyPlacement(
    const FVector2D InScreenPosition, const float InPresentationAlpha) {
    ScreenPosition = InScreenPosition;
    PresentationAlpha = FMath::Clamp(InPresentationAlpha, 0.0f, 1.0f);
}

void UMythicNameplateViewModel::Reset() {
    TArray<FMythicNameplateStatusCandidate> ReservedStatuses =
        MoveTemp(Projection.Statuses);
    Projection = FMythicNameplateProjection();
    Projection.Statuses = MoveTemp(ReservedStatuses);
    Projection.Statuses.Reset();
    ScreenPosition = FVector2D::ZeroVector;
    PresentationAlpha = 0.0f;
}
