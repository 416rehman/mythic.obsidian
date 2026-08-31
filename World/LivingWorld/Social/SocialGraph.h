
#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h"
#include "Mass/EntityHandle.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "SocialGraph.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythSocialGraph, Log, All);


UENUM(BlueprintType)
enum class EMythicSocialRelation : uint8 {
    Friend UMETA(DisplayName = "Friend"),

    Family UMETA(DisplayName = "Family"),

    Rival UMETA(DisplayName = "Rival"),

    Debt UMETA(DisplayName = "Debt"),

    Associate UMETA(DisplayName = "Associate"),

    Subordinate UMETA(DisplayName = "Subordinate"),

    COUNT UMETA(Hidden)
};


struct FMythicSocialEdge {
    FMassEntityHandle TargetEntity;

    EMythicSocialRelation Relation = EMythicSocialRelation::Friend;

    float Strength = 0.5f;

    double LastInteractionTime = 0.0;

    FMythicFactionId TargetFaction;
};


UCLASS()
class MYTHIC_API UMythicSocialGraph : public UObject {
    GENERATED_BODY()

public:
    void Initialize(int32 InMaxEdgesPerEntity, float InPruneStrengthThreshold, float InEdgeDecayRate);


    void AddOrStrengthenEdge(
        FMassEntityHandle Source,
        FMassEntityHandle Target,
        EMythicSocialRelation Relation,
        float InitStrength,
        double WorldTime,
        FMythicFactionId TargetFaction);

    bool RemoveEdge(FMassEntityHandle Source, FMassEntityHandle Target);

    void RemoveAllEdges(FMassEntityHandle Entity, TArray<FMassEntityHandle> &OutSeveredConnections);


    int32 GetEdges(FMassEntityHandle Source, double WorldTime, TArray<FMythicSocialEdge> &OutEdges) const;

    int32 GetEdgesByRelation(
        FMassEntityHandle Source,
        EMythicSocialRelation Relation,
        double WorldTime,
        TArray<FMythicSocialEdge> &OutEdges) const;

    bool HasEdge(FMassEntityHandle Source, FMassEntityHandle Target, double WorldTime, FMythicSocialEdge &OutEdge) const;

    int32 GetTotalEdgeCount() const;

    int32 GetEntityCount() const;

    /** Clears every Mass-handle edge during an in-place world restore before old entity handles are destroyed. */
    void ResetForLivingWorldRestore();


    int32 PruneStaleEdges(double WorldTime, int32 MaxEntitiesPerCall = 10);

private:
    TMap<FMassEntityHandle, TArray<FMythicSocialEdge>> AdjacencyMap;

    mutable FRWLock GraphLock;

    int32 MaxEdgesPerEntity = 8;

    float PruneStrengthThreshold = 0.05f;

    float EdgeDecayRate = 0.001f;

    int32 PruneIteratorIndex = 0;

public:
    static float ApplyDecay(const FMythicSocialEdge &Edge, double WorldTime, float DecayRate);
};
