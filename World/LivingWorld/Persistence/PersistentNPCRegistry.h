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
     * - Vacates held role (triggers succession in SettlementRegistry, via OwningLWS)
     * - Writes a World.Event.Death.Permanent event to the Causal Fabric (via OwningLWS's thread-safe queue);
     *   Grief pressure is raised from that event by the witnessing PressureProcessor, not here.
     *
     * @param NameHash    Unique identity of dead NPC
     * @param Faction     Faction at time of death
     * @param RoleTag     Role held (for succession)
     * @param Cell        Cell where death occurred
     * @param WorldTime   Time of death
     * @param OwningLWS   Death event + settlement succession route through its thread-safe / SimulationLock-guarded wrappers
     */
    void RegisterDeath(
        uint32 NameHash,
        const FMythicFactionId &Faction,
        const FGameplayTag &RoleTag,
        const FMythicCellCoord &Cell,
        double WorldTime,
        class UMythicLivingWorldSubsystem *OwningLWS);

    /** Check if a NameHash belongs to a permanently dead NPC */
    bool IsPermaDead(uint32 NameHash) const {
        return DeadNPCHashes.Contains(NameHash);
    }

    /**
     * Allocate the next never-reused global spawn serial, fed to FMythicNPCGenerator::GenerateNameHash in place of
     * the old wave-local SpawnIdx (which restarted at 0 each refill wave, so two waves in the same faction+cell
     * aliased one NameHash — R18-M7). The serial is monotonic AND persisted (see Serialize), so a refill wave or a
     * reloaded session can never recreate a NameHash a still-living or permanently-dead NPC already holds. Mirrors
     * the collision-free monotonic-id pattern AMythicEncounterDirector already uses (NextEncounterId). Game-thread
     * only (the PopulationSpawnerProcessor sets bRequiresGameThreadExecution=true) — same contract as IsPermaDead.
     */
    int32 AllocateSpawnSerial() {
        const int32 Serial = static_cast<int32>(NextSpawnSerial);
        ++NextSpawnSerial;
        return Serial;
    }

    /** Get all death records (for UI/debug display) */
    const TArray<FMythicPersistentDeathRecord> &GetDeathRecords() const { return DeathRecords; }

    /** Get count of permanent deaths */
    int32 GetDeathCount() const { return DeadNPCHashes.Num(); }

    /** Serialization for save/load */
    virtual void Serialize(FArchive &Ar) override;

private:
    /** Fast hash set for O(1) permadeath check */
    TSet<uint32> DeadNPCHashes;

    /** Ordered death records (for chronological display and event queries) */
    TArray<FMythicPersistentDeathRecord> DeathRecords;

    /**
     * Next never-reused global spawn serial, fed to GenerateNameHash as the SpawnIndex. Monotonic-increasing;
     * persisted across save/load (Serialize v2) so a reloaded session never regenerates a NameHash already issued
     * (which could match the persisted DeadNPCHashes set and wrongly perma-dead a fresh NPC). Mirrors EncounterDirector's NextEncounterId.
     */
    uint32 NextSpawnSerial = 0;
};
