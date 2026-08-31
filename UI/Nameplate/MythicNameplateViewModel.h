#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UI/Nameplate/MythicNameplateTypes.h"
#include "MythicNameplateViewModel.generated.h"

/** Allocation-stable local state consumed by one pooled nameplate widget; it never binds gameplay delegates. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicNameplateViewModel : public UObject {
    GENERATED_BODY()

public:
    UMythicNameplateViewModel();

    /** Returns the current immutable viewer-safe projection; Silent/invalid means this pool slot is released. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Nameplate")
    const FMythicNameplateProjection &GetProjection() const { return Projection; }

    /** Returns the split-screen-relative pixel position supplied by the owning local-player director. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Nameplate")
    FVector2D GetScreenPosition() const { return ScreenPosition; }

    /** Returns the resolved visibility alpha in [0,1]; Reduced Motion may snap rather than interpolate it. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Nameplate")
    float GetPresentationAlpha() const { return PresentationAlpha; }

    /** Applies already-sanitized local projection state; only the local director/layer should call this. */
    void Apply(const FMythicNameplateProjection &InProjection,
               FVector2D InScreenPosition, float InPresentationAlpha);

    /** Updates only placement/alpha between 10 Hz projection decisions, preserving immutable semantic fields. */
    void ApplyPlacement(FVector2D InScreenPosition, float InPresentationAlpha);

    /** Clears all projection state before the fixed pool slot is reused by another embodiment. */
    void Reset();

private:
    UPROPERTY(Transient)
    FMythicNameplateProjection Projection;

    UPROPERTY(Transient)
    FVector2D ScreenPosition = FVector2D::ZeroVector;

    UPROPERTY(Transient)
    float PresentationAlpha = 0.0f;
};
