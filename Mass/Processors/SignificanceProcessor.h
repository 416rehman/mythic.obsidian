
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "SignificanceProcessor.generated.h"

class APlayerController;

struct FMythicPlayerView {
    FVector CamLocation = FVector::ZeroVector;

    FVector CamForward = FVector::ForwardVector;

    float CosHalfCone = -1.0f;
};

UCLASS()
class MYTHIC_API UMythicSignificanceProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicSignificanceProcessor();

    static float ComputeProximityScore(const FMythicCellCoord &EntityCell, TConstArrayView<FMythicCellCoord> PlayerCells, float SpawnRadius);

    static bool ShouldRescore(bool bDirty, EMythicSignificanceTier Tier);

    static bool QualifiesForPromotion(float Score, float Threshold, float Hysteresis);

    static bool QualifiesForDemotion(float Score, float Threshold, float Hysteresis);

    static bool IsInCloseView(const FVector &WorldPos, TConstArrayView<FMythicPlayerView> PlayerViews, float MinSpawnDistance);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    FMassEntityQuery AllSignificanceQuery;

    float TimeSinceLastTick = 0.0f;

    TMap<TWeakObjectPtr<const APlayerController>, double> PlayerFirstSeenTime;

    TMap<TWeakObjectPtr<const APlayerController>, FMythicCellCoord> PlayerLastCell;
};
