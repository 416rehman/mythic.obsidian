
#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "CollisionQueryParams.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MythicAnimNotify_SphereOverlap.generated.h"

/**
 * Sweeps a sphere at a point on the attacker and raises one gameplay event carrying everything it found.
 *
 * The trace itself is C++ because it runs per swing and because what a blade passes through is not a gameplay
 * decision — what happens to those targets stays in the ability, which already loops target data and de-duplicates
 * per swing.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Mythic Sphere Overlap"))
class MYTHIC_API UMythicAnimNotify_SphereOverlap final : public UAnimNotify {
    GENERATED_BODY()

public:
    /** Local-space offset from the attacker used as the center of the authoritative overlap sphere. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hitbox",
              meta = (ToolTip = "Local-space offset from the attacker used as the overlap-sphere center."))
    FVector HitboxLocationOffset = FVector::ZeroVector;

    /** Radius in centimetres of the authoritative overlap sphere; zero disables the notify. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hitbox",
              meta = (ClampMin = "0.0",
                      ToolTip = "Radius in centimetres of the authoritative overlap sphere. Zero disables the notify."))
    float HitboxRadius = 100.0f;

    /** Exact gameplay-event tag raised on the attacker's ASC; an empty tag disables the notify. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hitbox",
              meta = (ToolTip = "Exact gameplay-event tag raised on the attacker's ASC. Empty disables the notify."))
    FGameplayTag SendToEventWithTag;

    /**
     * Most actors one swing may hit, nearest first. 0 means everything inside the radius — which is what the radius
     * already claims, so a cap is for weapons that should NOT cleave rather than the default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hitbox",
              meta = (ClampMin = "0",
                      ToolTip = "Maximum targets kept nearest-first. Zero keeps every valid target in the sphere."))
    int32 MaxTargets = 0;

    virtual void Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation,
                        const FAnimNotifyEventReference &EventReference) override;

    /** Returns whether serialized query data is finite and safe to execute at runtime. */
    static bool IsRuntimeQueryConfigurationValid(
        float Radius, const FVector &LocationOffset, int32 TargetCap);

    /** Builds the one canonical melee object mask, including the resource ISMs' Destructible object channel. */
    static FCollisionObjectQueryParams BuildRuntimeObjectQueryParams();

    /**
     * Orders a swing's hits nearest-first and trims them to the cap. Separated from the trace so the ordering rule
     * is testable: a capped swing must keep the targets closest to the blade, not whichever the query returned first.
     */
    static void OrderAndCapHits(TArray<FHitResult> &Hits, const FVector &Origin, int32 MaxTargets);
};
