// Mythic Living World — Persistent NPC Registry
// Tracks permanent NPC deaths (Tier 2-3) and prevents resurrection on load.

#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "PersistentNPCRegistry.generated.h"

class UMythicSocialGraph;
class UMythicCausalFabric;

/**
 * Persistent NPC death record — stored per permanently-dead NPC.
 * Only Tier 2-3 NPCs get permanent records. Tier 0-1 deaths are not tracked
 * (population count decrements, spawner refills naturally).
 */
USTRUCT()
struct FMythicPersistentDeathRecord {
    GENERATED_BODY()

    /** NameHash of the dead NPC (unique identity) */
    uint32 NameHash = 0;

    /** Faction the NPC belonged to at time of death */
    FMythicFactionId Faction;

    /** Role tag the NPC held (for succession system) */
    FGameplayTag RoleTag;

    /** World time of death (for chronological ordering) */
    double DeathTime = 0.0;

    /** Cell where death occurred */
    FMythicCellCoord DeathCell;
};

/**
 * Registry of permanently-dead NPCs.
 *
 * Design rules (from LIVING_WORLD_SYSTEM.md MUST constraints):
 * - Tier 0 deaths: NOT tracked. Population count decrements, spawner refills.
 * - Tier 1 deaths: Same as Tier 0 — entity recycled, hydrated state lost.
 * - Tier 2-3 deaths: PERMANENT. Gone forever. Social edges severed (→ Grief).
 *   Role vacated (→ succession). Causal events preserved. Saved/loaded.
 * - No respawn mechanic for Tier 2-3 NPCs.
 *
 * Thread safety: Created and accessed on game thread only.
 */
UCLASS()
class MYTHIC_API UMythicPersistentNPCRegistry : public UObject {
    GENERATED_BODY()

public:
    /**
     * Register a permanent NPC death.
     * - Adds to dead set (prevents future promotion/resurrection)
     * - Severs social graph edges (returns entities needing Grief pressure)
     * - Vacates held role (triggers succession in SettlementRegistry)
     * - Writes death event to Causal Fabric
     *
     * @param NameHash    Unique identity of dead NPC
     * @param Faction     Faction at time of death
     * @param RoleTag     Role held (for succession)
     * @param Cell        Cell where death occurred
     * @param WorldTime   Time of death
     * @param SocialGraph Social graph (for edge severing)
     * @param Fabric      Causal fabric (for death event)
     * @param OutGrievingEntities  Filled with entities connected to this NPC (need Grief pressure)
     */
    void RegisterDeath(
        uint32 NameHash,
        const FMythicFactionId& Faction,
        const FGameplayTag& RoleTag,
        const FMythicCellCoord& Cell,
        double WorldTime,
        UMythicSocialGraph* SocialGraph,
        UMythicCausalFabric* Fabric,
        class UMythicSettlementRegistry* SettlementRegistry,
        TArray<int32>& OutGrievingEntities);

    /** Check if a NameHash belongs to a permanently dead NPC */
    bool IsPermaDead(uint32 NameHash) const {
        return DeadNPCHashes.Contains(NameHash);
    }

    /** Get all death records (for UI/debug display) */
    const TArray<FMythicPersistentDeathRecord>& GetDeathRecords() const { return DeathRecords; }

    /** Get count of permanent deaths */
    int32 GetDeathCount() const { return DeadNPCHashes.Num(); }

    /** Serialization for save/load */
    void Serialize(FArchive& Ar);

private:
    /** Fast hash set for O(1) permadeath check */
    TSet<uint32> DeadNPCHashes;

    /** Ordered death records (for chronological display and event queries) */
    TArray<FMythicPersistentDeathRecord> DeathRecords;
};
