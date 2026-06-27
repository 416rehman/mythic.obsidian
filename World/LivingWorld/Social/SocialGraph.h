// Mythic Living World — Social Graph
// Shared sparse adjacency list for NPC-to-NPC relationships.
// Used by BDI brain, party system, belief propagation, and crime reporting.

#pragma once

#include "CoreMinimal.h"
#include "HAL/CriticalSection.h" // FRWLock (GraphLock)
#include "Mass/EntityHandle.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "SocialGraph.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythSocialGraph, Log, All);

// ─────────────────────────────────────────────────────────────
// Social Relation Types
// ─────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EMythicSocialRelation : uint8 {
    /** Positive connection through shared events/proximity */
    Friend UMETA(DisplayName = "Friend"),

    /** Blood/clan bond (generated at entity creation) */
    Family UMETA(DisplayName = "Family"),

    /** Antagonistic — generated from negative moral evaluation */
    Rival UMETA(DisplayName = "Rival"),

    /** Owes a life debt — generated from rescue/healing events */
    Debt UMETA(DisplayName = "Debt"),

    /** Professional/trade relationship */
    Associate UMETA(DisplayName = "Associate"),

    /** Mentorship/authority relationship */
    Subordinate UMETA(DisplayName = "Subordinate"),

    COUNT UMETA(Hidden)
};

// ─────────────────────────────────────────────────────────────
// Social Edge — One directional relationship
// ─────────────────────────────────────────────────────────────

/**
 * A single directed relationship edge in the social graph.
 * Edges are lightweight — no UObject overhead, no reflection.
 * ~24 bytes per edge.
 */
struct FMythicSocialEdge {
    /** Target entity handle */
    FMassEntityHandle TargetEntity;

    /** Type of relationship */
    EMythicSocialRelation Relation = EMythicSocialRelation::Friend;

    /** Strength [0.0, 1.0] — decays over time. Pruned below threshold. */
    float Strength = 0.5f;

    /** World time of the last interaction that refreshed this edge */
    double LastInteractionTime = 0.0;

    /** Faction of the target entity at edge creation (cached for fast query, not dynamic) */
    FMythicFactionId TargetFaction;
};

// ─────────────────────────────────────────────────────────────
// Social Graph — Sparse adjacency list
// ─────────────────────────────────────────────────────────────

/**
 * Shared social graph for all NPC-to-NPC relationships.
 * Owned by UMythicLivingWorldSubsystem.
 *
 * Threading:
 * - Guarded by GraphLock (FRWLock): read-lock on the query path, write-lock on mutation. The BDI cognition worker
 *   thread calls GetEdges/GetEdgesByRelation off the game thread, so the lock IS required for safe concurrent access
 *   (mirrors UMythicCausalFabric's FabricLock). Game-thread event processors take the write lock.
 *
 * Performance:
 * - TMap<FMassEntityHandle, TArray<FMythicSocialEdge>> — O(1) entity lookup
 * - Fixed max edges per entity prevents unbounded growth
 * - Budget-capped pruning prevents frame spikes
 * - Total memory: ~24B per edge × MaxEdges × active entity count
 *   At 100 active entities × 8 max edges = ~19KB
 *
 * Lifecycle:
 * - Edges created by event processors (witness → social interaction)
 * - Edges decay over time (lazy — evaluated on access or prune pass)
 * - Edges severed on entity death (triggers Grief pressure on connected entities)
 * - Pruned edges below strength threshold are removed
 */
UCLASS()
class MYTHIC_API UMythicSocialGraph : public UObject {
    GENERATED_BODY()

public:
    /**
     * Initialize the social graph with configuration.
     * @param InMaxEdgesPerEntity    Hard cap on edges per entity (prevents O(n) fan-out)
     * @param InPruneStrengthThreshold  Edges below this strength are removed during prune
     * @param InEdgeDecayRate        Strength decay per second of world time since last interaction
     */
    void Initialize(int32 InMaxEdgesPerEntity, float InPruneStrengthThreshold, float InEdgeDecayRate);

    // ─── Edge CRUD ────────────────────────────────────────

    /**
     * Add or strengthen an edge between two entities.
     * If the edge already exists, refreshes LastInteractionTime and boosts Strength.
     * If MaxEdges reached, the weakest edge is evicted.
     *
     * @param Source        The entity creating the edge
     * @param Target        The entity being connected to
     * @param Relation      Type of relationship
     * @param InitStrength  Initial strength [0.0, 1.0] (or strength boost if edge exists)
     * @param WorldTime     Current world time for decay tracking
     * @param TargetFaction Cached faction of the target
     */
    void AddOrStrengthenEdge(
        FMassEntityHandle Source,
        FMassEntityHandle Target,
        EMythicSocialRelation Relation,
        float InitStrength,
        double WorldTime,
        FMythicFactionId TargetFaction);

    /**
     * Remove a specific edge between two entities.
     * @return true if an edge was found and removed
     */
    bool RemoveEdge(FMassEntityHandle Source, FMassEntityHandle Target);

    /**
     * Remove ALL edges involving an entity (both as source and as target).
     * Call this when an entity is destroyed/despawned. SYNCHRONOUS and complete in one call — it is NOT batched across
     * frames. Incoming-edge removal scans every source's adjacency list: O(E × M), where E = entities with edges and
     * M = edges each (≤ MaxEdgesPerEntity). E is bounded by the social graph's real population — hydrated/cognitive
     * NPCs only, ~hundreds — not the full entity count, so it is fine at scale; but because it is unthrottled, do not
     * bulk-call it for many entities in a single frame (that would multiply the scan). (A reverse target→sources index
     * would make it O(incoming) — logged as a deferred optimization, unnecessary at the current graph population.)
     *
     * @param Entity       The entity being removed
     * @param OutSeveredConnections  Entities that lost a connection (for Grief pressure)
     */
    void RemoveAllEdges(FMassEntityHandle Entity, TArray<FMassEntityHandle> &OutSeveredConnections);

    // ─── Queries ──────────────────────────────────────────

    /**
     * Get all edges FROM a source entity.
     * Applies lazy decay to edge strengths before returning.
     *
     * @param Source    Entity to query
     * @param WorldTime Current world time for decay calculation
     * @param OutEdges  Output array of edges (copied, safe to iterate)
     * @return Number of edges found
     */
    int32 GetEdges(FMassEntityHandle Source, double WorldTime, TArray<FMythicSocialEdge> &OutEdges) const;

    /**
     * Get edges of a specific relation type FROM a source entity.
     * @return Number of matching edges
     */
    int32 GetEdgesByRelation(
        FMassEntityHandle Source,
        EMythicSocialRelation Relation,
        double WorldTime,
        TArray<FMythicSocialEdge> &OutEdges) const;

    /**
     * Check if a specific edge exists between two entities.
     * @param OutEdge  If found, filled with the edge data (with decay applied)
     * @return true if edge exists
     */
    bool HasEdge(FMassEntityHandle Source, FMassEntityHandle Target, double WorldTime, FMythicSocialEdge &OutEdge) const;

    /**
     * Get total edge count across all entities.
     */
    int32 GetTotalEdgeCount() const;

    /** Get number of entities that have at least one edge. Read-locks GraphLock (the BDI cognition worker mutates
     *  AdjacencyMap off the game thread) — defined in the .cpp so it can lock, matching GetTotalEdgeCount. */
    int32 GetEntityCount() const;

    // ─── Maintenance ──────────────────────────────────────

    /**
     * Prune stale edges across the graph. Budget-capped per call.
     * Should be called periodically (e.g., every few seconds on a timer).
     *
     * @param WorldTime        Current world time for decay evaluation
     * @param MaxEntitiesPerCall  Max entities to process this call (budget cap)
     * @return Number of edges pruned
     */
    int32 PruneStaleEdges(double WorldTime, int32 MaxEntitiesPerCall = 10);

private:
    /** Sparse adjacency: entity → outgoing edges. TArray per entity (small, cache-friendly). */
    TMap<FMassEntityHandle, TArray<FMythicSocialEdge>> AdjacencyMap;

    /** Guards AdjacencyMap — the BDI cognition worker reads (GetEdges) off the game thread while game-thread
     *  processors mutate. Read-locked on queries, write-locked on mutation. mutable so const queries can lock. */
    mutable FRWLock GraphLock;

    /** Max outgoing edges per entity */
    int32 MaxEdgesPerEntity = 8;

    /** Edges with strength below this are pruned */
    float PruneStrengthThreshold = 0.05f;

    /** Strength decay per second of world time */
    float EdgeDecayRate = 0.001f;

    /** Round-robin iterator for budget-capped pruning */
    int32 PruneIteratorIndex = 0;

public:
    /** Apply lazy exponential decay to an edge's strength: S × e^(-rate·Δt); returns the strength unchanged when the
     *  rate is non-positive or no world time has elapsed since the last interaction. Pure + static (no graph state)
     *  so the relationship-aging rule is unit-testable. */
    static float ApplyDecay(const FMythicSocialEdge &Edge, double WorldTime, float DecayRate);
};
