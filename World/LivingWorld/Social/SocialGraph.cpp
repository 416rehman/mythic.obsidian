// Mythic Living World — Social Graph Implementation
// Sparse adjacency list with lazy decay, budget-capped pruning, and O(1) entity lookup.

#include "World/LivingWorld/Social/SocialGraph.h"

#include "Misc/ScopeRWLock.h" // FReadScopeLock / FWriteScopeLock

DEFINE_LOG_CATEGORY(LogMythSocialGraph);

// ─────────────────────────────────────────────────────────────
// Initialize
// ─────────────────────────────────────────────────────────────

void UMythicSocialGraph::Initialize(int32 InMaxEdgesPerEntity, float InPruneStrengthThreshold, float InEdgeDecayRate) {
    MaxEdgesPerEntity = FMath::Clamp(InMaxEdgesPerEntity, 1, 32);
    PruneStrengthThreshold = FMath::Max(InPruneStrengthThreshold, 0.001f);
    EdgeDecayRate = FMath::Max(InEdgeDecayRate, 0.0f);
    PruneIteratorIndex = 0;

    UE_LOG(LogMythSocialGraph, Log,
           TEXT("SocialGraph initialized: MaxEdges=%d, PruneThreshold=%.3f, DecayRate=%.4f"),
           MaxEdgesPerEntity, PruneStrengthThreshold, EdgeDecayRate);
}

// ─────────────────────────────────────────────────────────────
// Edge CRUD
// ─────────────────────────────────────────────────────────────

void UMythicSocialGraph::AddOrStrengthenEdge(
    FMassEntityHandle Source,
    FMassEntityHandle Target,
    EMythicSocialRelation Relation,
    float InitStrength,
    double WorldTime,
    FMythicFactionId TargetFaction) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSocialGraph_AddOrStrengthen);

    FWriteScopeLock Lock(GraphLock);

    if (!Source.IsValid() || !Target.IsValid() || Source == Target) {
        return;
    }

    TArray<FMythicSocialEdge> &Edges = AdjacencyMap.FindOrAdd(Source);

    // Check if edge already exists — strengthen it
    for (FMythicSocialEdge &Edge : Edges) {
        if (Edge.TargetEntity == Target) {
            // Refresh interaction time and boost strength (clamped to 1.0)
            Edge.Strength = FMath::Min(Edge.Strength + InitStrength, 1.0f);
            Edge.LastInteractionTime = WorldTime;
            Edge.Relation = Relation; // Update relation type if changed
            return;
        }
    }

    // New edge — check capacity
    if (Edges.Num() >= MaxEdgesPerEntity) {
        // Evict weakest edge (with decay applied)
        int32 WeakestIndex = 0;
        float WeakestStrength = ApplyDecay(Edges[0], WorldTime, EdgeDecayRate);

        for (int32 i = 1; i < Edges.Num(); ++i) {
            const float DecayedStrength = ApplyDecay(Edges[i], WorldTime, EdgeDecayRate);
            if (DecayedStrength < WeakestStrength) {
                WeakestStrength = DecayedStrength;
                WeakestIndex = i;
            }
        }

        Edges.RemoveAtSwap(WeakestIndex);
    }

    // Add new edge
    FMythicSocialEdge &NewEdge = Edges.AddDefaulted_GetRef();
    NewEdge.TargetEntity = Target;
    NewEdge.Relation = Relation;
    NewEdge.Strength = FMath::Clamp(InitStrength, 0.0f, 1.0f);
    NewEdge.LastInteractionTime = WorldTime;
    NewEdge.TargetFaction = TargetFaction;
}

bool UMythicSocialGraph::RemoveEdge(FMassEntityHandle Source, FMassEntityHandle Target) {
    FWriteScopeLock Lock(GraphLock);
    TArray<FMythicSocialEdge> *Edges = AdjacencyMap.Find(Source);
    if (!Edges) {
        return false;
    }

    for (int32 i = 0; i < Edges->Num(); ++i) {
        if ((*Edges)[i].TargetEntity == Target) {
            Edges->RemoveAtSwap(i);

            // Clean up empty entries
            if (Edges->Num() == 0) {
                AdjacencyMap.Remove(Source);
            }
            return true;
        }
    }

    return false;
}

void UMythicSocialGraph::RemoveAllEdges(FMassEntityHandle Entity, TArray<FMassEntityHandle> &OutSeveredConnections) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSocialGraph_RemoveAllEdges);

    FWriteScopeLock Lock(GraphLock);

    OutSeveredConnections.Reset();

    // Remove outgoing edges
    if (TArray<FMythicSocialEdge> *Edges = AdjacencyMap.Find(Entity)) {
        for (const FMythicSocialEdge &Edge : *Edges) {
            OutSeveredConnections.Add(Edge.TargetEntity);
        }
        AdjacencyMap.Remove(Entity);
    }

    // Remove incoming edges (Entity appears as target in other entities' lists)
    // This is O(n×m) where n=entities, m=edges per entity, but:
    // - Only called on entity death (rare)
    // - Typical graph is sparse (100 entities × 8 edges = 800 checks)
    for (auto It = AdjacencyMap.CreateIterator(); It; ++It) {
        TArray<FMythicSocialEdge> &Edges = It->Value;
        for (int32 i = Edges.Num() - 1; i >= 0; --i) {
            if (Edges[i].TargetEntity == Entity) {
                // This source entity lost a connection — notify for Grief
                if (!OutSeveredConnections.Contains(It->Key)) {
                    OutSeveredConnections.Add(It->Key);
                }
                Edges.RemoveAtSwap(i);
            }
        }

        // Clean up empty adjacency entries
        if (Edges.Num() == 0) {
            It.RemoveCurrent();
        }
    }
}

// ─────────────────────────────────────────────────────────────
// Queries
// ─────────────────────────────────────────────────────────────

int32 UMythicSocialGraph::GetEdges(FMassEntityHandle Source, double WorldTime, TArray<FMythicSocialEdge> &OutEdges) const {
    FReadScopeLock Lock(GraphLock);
    OutEdges.Reset();

    const TArray<FMythicSocialEdge> *Edges = AdjacencyMap.Find(Source);
    if (!Edges) {
        return 0;
    }

    OutEdges.Reserve(Edges->Num());
    for (const FMythicSocialEdge &Edge : *Edges) {
        FMythicSocialEdge DecayedEdge = Edge;
        DecayedEdge.Strength = ApplyDecay(Edge, WorldTime, EdgeDecayRate);
        OutEdges.Add(DecayedEdge);
    }

    return OutEdges.Num();
}

int32 UMythicSocialGraph::GetEdgesByRelation(
    FMassEntityHandle Source,
    EMythicSocialRelation Relation,
    double WorldTime,
    TArray<FMythicSocialEdge> &OutEdges) const {
    FReadScopeLock Lock(GraphLock);
    OutEdges.Reset();

    const TArray<FMythicSocialEdge> *Edges = AdjacencyMap.Find(Source);
    if (!Edges) {
        return 0;
    }

    for (const FMythicSocialEdge &Edge : *Edges) {
        if (Edge.Relation == Relation) {
            FMythicSocialEdge DecayedEdge = Edge;
            DecayedEdge.Strength = ApplyDecay(Edge, WorldTime, EdgeDecayRate);
            OutEdges.Add(DecayedEdge);
        }
    }

    return OutEdges.Num();
}

bool UMythicSocialGraph::HasEdge(FMassEntityHandle Source, FMassEntityHandle Target, double WorldTime, FMythicSocialEdge &OutEdge) const {
    FReadScopeLock Lock(GraphLock);
    const TArray<FMythicSocialEdge> *Edges = AdjacencyMap.Find(Source);
    if (!Edges) {
        return false;
    }

    for (const FMythicSocialEdge &Edge : *Edges) {
        if (Edge.TargetEntity == Target) {
            OutEdge = Edge;
            OutEdge.Strength = ApplyDecay(Edge, WorldTime, EdgeDecayRate);
            return true;
        }
    }

    return false;
}

int32 UMythicSocialGraph::GetTotalEdgeCount() const {
    FReadScopeLock Lock(GraphLock);
    int32 Total = 0;
    for (const auto &Pair : AdjacencyMap) {
        Total += Pair.Value.Num();
    }
    return Total;
}

// ─────────────────────────────────────────────────────────────
// Maintenance
// ─────────────────────────────────────────────────────────────

int32 UMythicSocialGraph::PruneStaleEdges(double WorldTime, int32 MaxEntitiesPerCall) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSocialGraph_PruneStaleEdges);

    FWriteScopeLock Lock(GraphLock);

    if (AdjacencyMap.Num() == 0) {
        return 0;
    }

    int32 TotalPruned = 0;
    int32 EntitiesProcessed = 0;

    // Convert to array for stable indexed iteration
    TArray<FMassEntityHandle> Keys;
    AdjacencyMap.GetKeys(Keys);

    if (Keys.Num() == 0) {
        return 0;
    }

    // Wrap the iterator index
    PruneIteratorIndex = PruneIteratorIndex % Keys.Num();

    for (int32 i = 0; i < Keys.Num() && EntitiesProcessed < MaxEntitiesPerCall; ++i) {
        const int32 CurrentIndex = (PruneIteratorIndex + i) % Keys.Num();
        const FMassEntityHandle &Key = Keys[CurrentIndex];

        TArray<FMythicSocialEdge> *Edges = AdjacencyMap.Find(Key);
        if (!Edges) {
            continue;
        }

        // Prune edges below threshold (iterate backwards for safe removal)
        for (int32 j = Edges->Num() - 1; j >= 0; --j) {
            const float DecayedStrength = ApplyDecay((*Edges)[j], WorldTime, EdgeDecayRate);
            if (DecayedStrength < PruneStrengthThreshold) {
                Edges->RemoveAtSwap(j);
                ++TotalPruned;
            }
            else {
                // Write back decayed strength (lazy decay materialization)
                (*Edges)[j].Strength = DecayedStrength;
                (*Edges)[j].LastInteractionTime = WorldTime;
            }
        }

        // Remove empty adjacency entries
        if (Edges->Num() == 0) {
            AdjacencyMap.Remove(Key);
        }

        ++EntitiesProcessed;
    }

    // Advance iterator for next call
    PruneIteratorIndex = (PruneIteratorIndex + EntitiesProcessed) % FMath::Max(Keys.Num(), 1);

    return TotalPruned;
}

// ─────────────────────────────────────────────────────────────
// Internals
// ─────────────────────────────────────────────────────────────

float UMythicSocialGraph::ApplyDecay(const FMythicSocialEdge &Edge, double WorldTime, float DecayRate) {
    if (DecayRate <= 0.0f || WorldTime <= Edge.LastInteractionTime) {
        return Edge.Strength;
    }

    // Exponential decay: S(t) = S₀ × e^(-rate × Δt)
    // Same pattern as PressureProcessor for consistency
    const double Elapsed = WorldTime - Edge.LastInteractionTime;
    return Edge.Strength * FMath::Exp(-DecayRate * static_cast<float>(Elapsed));
}
