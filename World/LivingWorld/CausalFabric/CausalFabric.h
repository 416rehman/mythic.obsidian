// Mythic Living World System — Causal Fabric
// Shared, append-only world event log. Lock-free reads from game thread.

#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "CausalFabric.generated.h"

// ─────────────────────────────────────────────────────────────
// World Event — Single entry in the causal fabric
// ─────────────────────────────────────────────────────────────

/**
 * One objective event recorded in the world. NPCs never store their own copy.
 * They query the fabric with personality bias at read time (Tier 2-3 only).
 * Tier 0-1 receive pre-digested event tags pushed by processors.
 */
USTRUCT()
struct MYTHIC_API FMythicWorldEvent {
    GENERATED_BODY()

    /** Unique event ID for cross-referencing in the DAG */
    uint32 EventId = 0;

    /** ID of the causal parent event (0 = root/no parent) */
    uint32 ParentEventId = 0;

    /** Game world time when this event occurred */
    double WorldTime = 0.0;

    /** Cell where the event occurred */
    FMythicCellCoord Cell;

    /** Faction primarily involved (perpetrator's faction) */
    FMythicFactionId PrimaryFaction;

    /** Secondary faction (victim's faction, if applicable) */
    FMythicFactionId SecondaryFaction;

    /** Gameplay tag describing the event type (e.g. "Event.Combat.Kill", "Event.Crime.Theft") */
    FGameplayTag EventTag;

    /** Moral vector of this action — used for witness evaluation */
    FMythicMoralAction MoralVector;

    /** ID of the perpetrator entity (0 = no specific perpetrator, e.g. environmental event) */
    uint32 PerpEntityId = 0;

    /** ID of the victim entity (0 = no victim) */
    uint32 VictimEntityId = 0;

    /** Significance magnitude — how "important" this event is. Affects propagation priority. */
    float Significance = 0.0f;

    /**
     * Bitfield for fast filtering. Bits indicate event categories.
     * Processors can quickly mask-test against this instead of tag comparison.
     */
    uint16 CategoryFlags = 0;

    /** The action category for specific ability/magic categorization */
    EMythicActionCategory ActionCategory = EMythicActionCategory::Melee;

    /**
     * Visibility group of the perpetrator at the time of the event.
     * Used by WitnessProcessor for byte-compare LOS checks.
     * 0 = default (visible to all groups).
     */
    uint8 VisibilityGroup = 0;
};

// Category flag constants for fast bitfield filtering
namespace EMythicEventCategory {
    constexpr uint16 Combat = 1 << 0;
    constexpr uint16 Crime = 1 << 1;
    constexpr uint16 Death = 1 << 2;
    constexpr uint16 Trade = 1 << 3;
    constexpr uint16 Diplomacy = 1 << 4;
    constexpr uint16 Territory = 1 << 5;
    constexpr uint16 Social = 1 << 6;
    constexpr uint16 Environment = 1 << 7;
    constexpr uint16 Magic = 1 << 8;
    constexpr uint16 Scheme = 1 << 9;
    constexpr uint16 Encounter = 1 << 10; // a spawned world threat/event (raid, monster pack, …) — its own beat
}

// ─────────────────────────────────────────────────────────────
// Cell Event Index Entry — For spatial queries
// ─────────────────────────────────────────────────────────────

/** Maps a cell to a range of events in the ring buffer for spatial queries */
struct FMythicCellEventRange {
    /** Index of the oldest event in this cell still in the ring buffer */
    int32 OldestIndex = -1;

    /** Count of events in this cell currently in the buffer */
    int32 Count = 0;
};

// ─────────────────────────────────────────────────────────────
// Causal Fabric — Shared world event ring buffer
// ─────────────────────────────────────────────────────────────

/**
 * The Causal Fabric is a shared, append-only ring buffer of world events.
 *
 * Threading model:
 * - Background thread (world sim) writes events via AppendEvent()
 * - Game thread reads via query functions (protected by ReadLock)
 * - Double-buffered: background thread writes to the write buffer,
 *   then copies to read buffer during CommitWrites under WriteLock.
 *
 * Memory: Fixed capacity ring buffer. When full, oldest events are overwritten.
 * Old events past the configurable horizon are considered archived.
 */
UCLASS()
class MYTHIC_API UMythicCausalFabric : public UObject {
    GENERATED_BODY()

public:
    /** Initialize the fabric with the given capacity. Must be called before use. */
    void Initialize(int32 InCapacity);

    // ─── Write Interface (Background Thread Only) ─────────

    /** Append a new event to the fabric. Returns the assigned EventId. Thread-safe for single writer. */
    uint32 AppendEvent(const FMythicWorldEvent &Event);

    /** Commit pending writes — swaps the read snapshot so game thread sees new events. */
    void CommitWrites();

    // ─── Read Interface (Game Thread — Thread-Safe via RWLock) ─────────

    /** Get an event by ID. Returns nullptr if the event has been overwritten (outside ring). */
    const FMythicWorldEvent *GetEvent(uint32 EventId) const;

    /** Get the most recent N events (up to buffer capacity), oldest-first. Returns a COPY taken under the read
     *  lock — safe to use off the sim thread (the previous TArrayView aliased ReadBuffer, which CommitWrites
     *  overwrites on the sim thread; it also dropped the wrapped tail on a wrap-straddling window). */
    TArray<FMythicWorldEvent> GetRecentEvents(int32 MaxCount) const;

    /**
     * Query events in a specific cell within a time range. Budget-capped by MaxResults. Returns value COPIES taken
     * under the read lock (NOT pointers into ReadBuffer, which CommitWrites memcpy-overwrites on the sim thread — the
     * off-thread BDI consumer would otherwise read torn data). Mirrors GetRecentEvents/QueryEventsByFaction.
     */
    void QueryEventsByCell(
        const FMythicCellCoord &Cell,
        double MinWorldTime,
        double MaxWorldTime,
        int32 MaxResults,
        TArray<FMythicWorldEvent> &OutEvents) const;

    /**
     * Query events matching a category bitmask within a time range. Fast path for processors that need "all combat
     * events in the last N seconds." Returns value COPIES under the read lock (see QueryEventsByCell).
     */
    void QueryEventsByCategory(
        uint16 CategoryMask,
        double MinWorldTime,
        double MaxWorldTime,
        int32 MaxResults,
        TArray<FMythicWorldEvent> &OutEvents) const;

    /** Get the total number of events ever recorded (monotonically increasing) */
    uint32 GetTotalEventCount() const { return NextEventId.load(std::memory_order_relaxed); }

    /** Get the current buffer capacity */
    int32 GetCapacity() const { return Capacity; }

    /** Thread-safe RW lock for reads/commits */
    mutable FRWLock FabricLock;

    /**
     * Query events where PrimaryFaction matches the given faction.
     * Scans the read buffer from newest to oldest.
     * Used by TickCrystallization for cultural memory promotion.
     *
     * @param Faction       Faction to filter by
     * @param OutEvents     Output array (cleared before use)
     * @param MaxResults    Budget cap on results
     */
    void QueryEventsByFaction(
        const FMythicFactionId &Faction,
        TArray<FMythicWorldEvent> &OutEvents,
        int32 MaxResults) const;

    // ─── Serialization ───────────────────────────────────

    /**
     * Serialize the fabric's event ring buffer for save/load.
     * Writes all valid events in the current buffer to the archive.
     */
    virtual void Serialize(FArchive &Ar) override;

private:
    /** Ring buffer capacity — set once at init, configurable via data asset */
    int32 Capacity = 0;

    /** Next event ID to assign. Monotonically increasing. */
    std::atomic<uint32> NextEventId{1};

    /** Write buffer — only touched by background thread */
    TArray<FMythicWorldEvent> WriteBuffer;

    /** Read buffer — snapshot for game thread, copied on CommitWrites under WriteLock */
    TArray<FMythicWorldEvent> ReadBuffer;

    /** Spatial index for the write buffer (Cell -> Array of EventIds) */
    TMap<FMythicCellCoord, TArray<uint32>> WriteSpatialIndex;

    /** Spatial index for the read buffer */
    TMap<FMythicCellCoord, TArray<uint32>> ReadSpatialIndex;

    /** Write head position in the ring buffer */
    int32 WriteHead = 0;

    /** Number of events currently in the write buffer */
    int32 WriteCount = 0;

    /** Read head position — snapshot of WriteHead at last commit */
    int32 ReadHead = 0;

    /** Number of events in the read buffer */
    int32 ReadCount = 0;

    /**
     * The EventId that corresponds to WriteHead=0 in the current ring.
     * Used to translate EventId → ring index.
     */
    uint32 BaseEventId = 1;

    /**
     * Committed (read-side) snapshot of the EventId-translation basis, captured in CommitWrites under the write lock
     * so EventIdToIndex stays consistent with the read buffer. Reading the LIVE BaseEventId/NextEventId from the game
     * thread raced the lock-free sim-thread AppendEvent (which advances them between a commit and a read), returning
     * the wrong event or nullptr for a live one — and BaseEventId is non-atomic, so that was also a torn read.
     */
    uint32 ReadBaseEventId = 1;
    uint32 ReadNewestEventId = 0;

    /** Translate an EventId to a ring buffer index. Returns -1 if outside current ring. */
    int32 EventIdToIndex(uint32 EventId, int32 HeadPos, int32 Count) const;
};
