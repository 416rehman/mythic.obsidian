
#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "PlaceableFragment.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EPlaceablePlacementResult : uint8 {
    Valid,
    NoSurface,
    OutOfReach,
    SurfaceTooSteep,
    Obstructed
};

UENUM(BlueprintType)
enum class EPlaceableDeployResult : uint8 {
    Deployed,
    NotAuthorized,
    SlotEmpty,
    NoDeployedClass,
    PlacementInvalid
};

USTRUCT(BlueprintType)
struct FPlaceablePlacementQuery {
    GENERATED_BODY()

    // Did the placement trace hit a surface at all?
    UPROPERTY(BlueprintReadWrite, Category = "Placeable")
    bool bDidHitSurface = false;

    // Up-component of the hit surface normal (1 = perfectly flat ground, 0 = vertical wall). World Z is up.
    UPROPERTY(BlueprintReadWrite, Category = "Placeable")
    float SurfaceNormalZ = 1.0f;

    // Distance (cm) from the placing player to the candidate point.
    UPROPERTY(BlueprintReadWrite, Category = "Placeable")
    float DistanceFromInstigator = 0.0f;

    // Did an overlap test at the candidate point find blocking geometry (or another deployable)?
    UPROPERTY(BlueprintReadWrite, Category = "Placeable")
    bool bHasBlockingOverlap = false;
};

USTRUCT(BlueprintType)
struct FPlaceablePreview {
    GENERATED_BODY()

    // May the player commit the deploy at this spot? (true only for a Valid placement.)
    UPROPERTY(BlueprintReadOnly, Category = "Placeable")
    bool bCanConfirm = false;

    // Tint for the translucent ghost mesh: green when placeable, red when not.
    UPROPERTY(BlueprintReadOnly, Category = "Placeable")
    FLinearColor TintColor = FLinearColor::Red;

    // Short player-facing explanation when blocked (empty when placeable).
    UPROPERTY(BlueprintReadOnly, Category = "Placeable")
    FText Reason;
};

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UPlaceableFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Placeable)

    // The actor deployed into the world when this item is placed. Soft so the (potentially heavy) deployable
    // blueprint isn't pulled into memory merely by holding the item; the deploy action resolves it on use.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placeable")
    TSoftClassPtr<AActor> DeployedActorClass;

    // Furthest (cm) from the player the item may be deployed. Defaults to the interaction reach (200cm) so placing
    // feels consistent with other world interactions.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placeable", meta = (ClampMin = "0.0"))
    float MaxPlacementReach = 200.0f;

    // Steepest surface (degrees from horizontal) this may rest on. 0 = perfectly flat only; 90 = any surface.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placeable", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float MaxGroundSlopeDegrees = 35.0f;

    // Radius (cm) around the candidate point that must be clear of blocking geometry for the deploy to succeed.
    // Read by the deploy/preview overlap test (it sizes the sweep); the rule here consumes the resulting boolean.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placeable", meta = (ClampMin = "0.0"))
    float RequiredClearanceRadius = 50.0f;

    // If true the item must be placed on a surface (a trace miss / over-steep wall is rejected). Set false for the
    // rare placeable that may hang in mid-air.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placeable")
    bool bRequireGroundSurface = true;

    float GetMinSurfaceNormalZ() const;

    EPlaceablePlacementResult EvaluatePlacement(const FPlaceablePlacementQuery &Query) const;

    static EPlaceablePlacementResult EvaluatePlacement(const FPlaceablePlacementQuery &Query,
                                                       float MaxReach,
                                                       float MinSurfaceNormalZ,
                                                       bool bRequireGround);

    static EPlaceableDeployResult PlanDeploy(bool bAuthorizedInventory,
                                             bool bSlotHasPlaceableItem,
                                             bool bHasDeployedClass,
                                             EPlaceablePlacementResult Placement);

    static FPlaceablePlacementQuery BuildPlacementQuery(bool bDidHit,
                                                        const FVector &ImpactPoint,
                                                        const FVector &ImpactNormal,
                                                        const FVector &TraceEnd,
                                                        const FVector &InstigatorLocation,
                                                        bool bHasBlockingOverlap);

    static FPlaceablePreview DescribePlacement(EPlaceablePlacementResult Result);

    static FText DescribeDeployFailure(EPlaceableDeployResult DeployResult, EPlaceablePlacementResult PlacementResult);
};
