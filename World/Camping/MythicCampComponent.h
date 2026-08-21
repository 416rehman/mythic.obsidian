#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MythicCampComponent.generated.h"

class UItemDefinition;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicCampComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicCampComponent();

    /** P2 fold input: which comfort category this piece contributes to (Comfort.Fire / Comfort.Shelter / Comfort.Rack).
     *  Invalid = contributes nothing (still counts against the anti-litter cap). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp")
    FGameplayTag ComfortCategoryTag;

    /** P2 fold input: points this piece contributes before per-category diminishing returns. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp", meta = (ClampMin = "0.0"))
    float ComfortPoints = 1.0f;

    /** Camp ANCHOR flag: anchors (campfires) define where a camp exists; non-anchor pieces only add comfort. Auto-set
     *  TRUE at BeginPlay when the owner also carries a UMythicCampfireComponent, so campfire content can't misauthor it. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp")
    bool bCampAnchor = false;

    /** OPTIONAL refund: the item definition this placeable was deployed from. When the per-player cap collapses this
     *  piece (oldest first), one unit of this item is dropped back at the piece's location via the standard item-reward
     *  path (the deploy flow consumed the item — the collapse returns it). Unset = collapse without refund. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camp")
    TSoftObjectPtr<UItemDefinition> SourceItemDefinition;

    const FString &GetOwnerPlayerKey() const { return OwnerPlayerKey; }

    double GetRegisteredAtServerTime() const { return RegisteredAtServerTime; }
    void SetRegisteredAtServerTime(double Time) { RegisteredAtServerTime = Time; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void ResolveOwnerPlayerKey();

    FString OwnerPlayerKey;
    double RegisteredAtServerTime = 0.0;
};
