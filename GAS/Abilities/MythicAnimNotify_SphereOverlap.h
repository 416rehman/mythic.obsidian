
#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
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
UCLASS(meta = (DisplayName = "Mythic Sphere Overlap"))
class MYTHIC_API UMythicAnimNotify_SphereOverlap : public UAnimNotify {
    GENERATED_BODY()

public:
    // Where the sphere sits, in the attacker's local space.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hitbox")
    FVector HitboxLocationOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hitbox", meta = (ClampMin = "0.0"))
    float HitboxRadius = 100.0f;

    // Event the ability waits on. Empty = the notify does nothing.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hitbox")
    FGameplayTag SendToEventWithTag;

    /**
     * Most actors one swing may hit, nearest first. 0 means everything inside the radius — which is what the radius
     * already claims, so a cap is for weapons that should NOT cleave rather than the default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hitbox", meta = (ClampMin = "0"))
    int32 MaxTargets = 0;

    virtual void Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation,
                        const FAnimNotifyEventReference &EventReference) override;

    /**
     * Orders a swing's hits nearest-first and trims them to the cap. Separated from the trace so the ordering rule
     * is testable: a capped swing must keep the targets closest to the blade, not whichever the query returned first.
     */
    static void OrderAndCapHits(TArray<FHitResult> &Hits, const FVector &Origin, int32 MaxTargets);
};
