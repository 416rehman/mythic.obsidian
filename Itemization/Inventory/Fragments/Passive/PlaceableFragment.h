// Mythic — Placeable item fragment
// Marks an inventory item as something the player can deploy into the world (a chest, crafting station, campfire,
// bed…) and carries the data-driven rules that govern where it may be placed. The placement DECISION is a pure
// static so the client ghost-preview and the server deploy-authority share one source of truth.

#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "PlaceableFragment.generated.h"

class AActor;

/** Outcome of evaluating a candidate placement against a placeable's rules. */
UENUM(BlueprintType)
enum class EPlaceablePlacementResult : uint8 {
    Valid,           // OK to deploy here
    NoSurface,       // a ground-requiring placeable was aimed at empty space
    OutOfReach,      // the spot is farther from the player than MaxPlacementReach
    SurfaceTooSteep, // the hit surface is tilted past MaxGroundSlopeDegrees
    Obstructed       // something is already occupying the spot
};

/**
 * Why a deploy attempt resolved the way it did. The server uses this to decide whether to spawn+consume, and the
 * acting client uses it to show legible "can't place here" feedback (out-of-reach vs blocked vs not-yours).
 */
UENUM(BlueprintType)
enum class EPlaceableDeployResult : uint8 {
    Deployed,        // every gate passed: the server should spawn the deployed actor and consume the item
    NotAuthorized,   // authority / inventory-ownership gate failed (must be server + own-or-open-container)
    SlotEmpty,       // the source slot holds no placeable item
    NoDeployedClass, // the placeable fragment has no DeployedActorClass assigned (content error)
    PlacementInvalid // the candidate spot failed EvaluatePlacement (too far / too steep / blocked / no surface)
};

/**
 * A single candidate placement, gathered from a world trace + overlap test. Pure data: the client fills one of
 * these from its own trace to drive the ghost preview, and the server fills its own from an authoritative trace —
 * both then feed it to the SAME evaluation rule, so the two can never disagree.
 */
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

/**
 * Client-side presentation of a placement verdict for the ghost-preview / HUD: whether the player may confirm the
 * deploy, the ghost tint colour, and a short player-facing reason. Single-sources the verdict→feedback mapping so the
 * ghost mesh, any prompt text, and the HUD all agree. Purely LOCAL/cosmetic — never replicated (the server stays
 * authoritative via PlanDeploy); this only drives what the acting client sees.
 */
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

/**
 * Fragment: marks an item as deployable into the world and carries its placement rules.
 *
 * This fragment is the DATA + DECISION foundation of the "place a deployable" verb. It does not itself spawn the
 * actor — a follow-up server deploy action reads DeployedActorClass and calls EvaluatePlacement to authorize the
 * spot, and the client ghost-preview calls the SAME EvaluatePlacement so the preview can never show "placeable"
 * where the server would reject (no misprediction). Slot / accepted-type / stacking rules come from the item as
 * usual. Add this fragment to an ItemDefinition to make a carryable, deployable object.
 */
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

    // Minimum surface-normal Z that satisfies MaxGroundSlopeDegrees — i.e. cos(slope). Precomputed from the
    // authored degrees so the per-frame client preview evaluates the rule without any trig.
    float GetMinSurfaceNormalZ() const;

    // Evaluate a candidate placement against THIS fragment's authored rules.
    EPlaceablePlacementResult EvaluatePlacement(const FPlaceablePlacementQuery &Query) const;

    // Pure placement-rule decision — the single source of truth shared by the client ghost-preview and the server
    // deploy authority. No engine state, no side effects: identical inputs give an identical verdict on every
    // machine, so a client never previews a spot the server will reject.
    static EPlaceablePlacementResult EvaluatePlacement(const FPlaceablePlacementQuery &Query,
                                                       float MaxReach,
                                                       float MinSurfaceNormalZ,
                                                       bool bRequireGround);

    // Pure server-side deploy-gate decision: composes the authority/ownership gate, the slot/item check, the
    // content (DeployedActorClass) check, and the placement verdict into one ordered outcome. Authority is checked
    // FIRST so a failed gate never leaks slot/placement state and never mutates anything. The deploy RPC computes
    // each input from authoritative state, calls this, and only spawns + consumes when the result is Deployed.
    static EPlaceableDeployResult PlanDeploy(bool bAuthorizedInventory,
                                             bool bSlotHasPlaceableItem,
                                             bool bHasDeployedClass,
                                             EPlaceablePlacementResult Placement);

    // Build the placement query from a world-trace result, single-sourcing the candidate-point + distance
    // convention so the client ghost-preview and the server deploy-authority feed EvaluatePlacement byte-identical
    // inputs (the no-misprediction guarantee). Candidate point = the surface hit when the trace hit, else the trace
    // endpoint (for floating placeables); reach is measured to that candidate, not the raw trace length.
    static FPlaceablePlacementQuery BuildPlacementQuery(bool bDidHit,
                                                        const FVector &ImpactPoint,
                                                        const FVector &ImpactNormal,
                                                        const FVector &TraceEnd,
                                                        const FVector &InstigatorLocation,
                                                        bool bHasBlockingOverlap);

    // Map a placement verdict to its client-side presentation (confirmable? + ghost tint + short reason). The client
    // drives its ghost-preview off this each frame; the server stays authoritative via PlanDeploy. Pure + static so
    // the verdict→feedback mapping is single-sourced and unit-testable.
    static FPlaceablePreview DescribePlacement(EPlaceablePlacementResult Result);
};
