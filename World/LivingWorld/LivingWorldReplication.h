// Mythic Living World — Replication Proxies
// Lightweight, delta-compressed structs for syncing high-level Living World state to clients.
// Clients do not run the world sim thread, but need faction wealth, territory boundaries, etc.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "LivingWorldTypes.h"
#include "Factions/FactionDatabase.h"
#include "GameFramework/Info.h"
#include "World/LivingWorld/Encounters/EncounterTemplate.h" // EMythicEncounterState + FGameplayTag
#include "LivingWorldReplication.generated.h"

// ─────────────────────────────────────────────────────────────
// Faction Proxy — Networked representation of a Faction
// ─────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicFactionProxyItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Faction")
    FMythicFactionId FactionId;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Faction")
    EMythicFactionStatus Status = EMythicFactionStatus::Dormant;

    // Approximated population for UI / client logic
    UPROPERTY(BlueprintReadOnly, Category = "Living World|Faction")
    int32 Population = 0;

    // Total controlled cells
    UPROPERTY(BlueprintReadOnly, Category = "Living World|Faction")
    int32 ControlledCellCount = 0;

    // Relative wealth level [0-255 mapped to 0-max] for UI
    UPROPERTY(BlueprintReadOnly, Category = "Living World|Faction")
    uint8 WealthLevel = 0;

    // Relative military strength [0-255 mapped to 0-1] for UI
    UPROPERTY(BlueprintReadOnly, Category = "Living World|Faction")
    uint8 MilitaryStrength = 0;

    // Client-side ingest hooks: route through the owning replicator so the client subsystem fires its change
    // delegate (UI updates). Defined out-of-line (need the full array + replicator types).
    void PreReplicatedRemove(const struct FMythicFactionProxyArray &InArraySerializer);
    void PostReplicatedAdd(const struct FMythicFactionProxyArray &InArraySerializer);
    void PostReplicatedChange(const struct FMythicFactionProxyArray &InArraySerializer);
};

/** Fast array for replicating faction proxies */
USTRUCT()
struct MYTHIC_API FMythicFactionProxyArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicFactionProxyItem> Items;

    // Non-replicated back-pointer to the owning replicator (set in the replicator's ctor on BOTH server + client),
    // so the FastArray item callbacks can reach it on the client to notify the subsystem. Not a UPROPERTY.
    class AMythicLivingWorldReplicator *OwnerReplicator = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicFactionProxyItem, FMythicFactionProxyArray>(Items, DeltaParms, *this);
    }
};

template <>
struct TStructOpsTypeTraits<FMythicFactionProxyArray> : public TStructOpsTypeTraitsBase2<FMythicFactionProxyArray> {
    enum { WithNetDeltaSerializer = true };
};

// ─────────────────────────────────────────────────────────────
// Territory Cell Proxy — Networked territory grid changes
// ─────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicTerritoryProxyItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Territory")
    FMythicCellCoord Cell;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Territory")
    FMythicFactionId ControllingFaction;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Territory")
    uint8 ContestedLevel = 0;

    // Client-side ingest hooks (see faction proxy). Defined out-of-line.
    void PreReplicatedRemove(const struct FMythicTerritoryProxyArray &InArraySerializer);
    void PostReplicatedAdd(const struct FMythicTerritoryProxyArray &InArraySerializer);
    void PostReplicatedChange(const struct FMythicTerritoryProxyArray &InArraySerializer);
};

/** Fast array for replicating territory cell changes */
USTRUCT()
struct MYTHIC_API FMythicTerritoryProxyArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicTerritoryProxyItem> Items;

    // Non-replicated back-pointer to the owning replicator (set in the replicator's ctor). Not a UPROPERTY.
    class AMythicLivingWorldReplicator *OwnerReplicator = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicTerritoryProxyItem, FMythicTerritoryProxyArray>(Items, DeltaParms, *this);
    }
};

// REQUIRED — without this trait the engine never treats FMythicTerritoryProxyArray as a net-delta serializer, so
// FastArrayDeltaSerialize (and thus the per-item PostReplicatedAdd/Change/PreReplicatedRemove callbacks that fire
// NotifyClientProxiesChanged) never run on clients — territory ownership changes would silently never refresh the
// client map/HUD. Mirrors the faction (above) and encounter (below) proxy traits.
template <>
struct TStructOpsTypeTraits<FMythicTerritoryProxyArray> : public TStructOpsTypeTraitsBase2<FMythicTerritoryProxyArray> {
    enum { WithNetDeltaSerializer = true };
};

// ─────────────────────────────────────────────────────────────
// Encounter Proxy — Networked active-encounter state (client map/HUD)
// ─────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEncounterProxyItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    // Stable instance id (matches FMythicActiveEncounter::EncounterId — the dedup/removal key). uint32 is not a
    // Blueprint-compatible type, so this stays C++-only (BP doesn't need the raw id — it keys UI off type/state/cell).
    UPROPERTY()
    uint32 EncounterId = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Encounter")
    FGameplayTag TemplateTag;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Encounter")
    EMythicEncounterState State = EMythicEncounterState::Pending;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Encounter")
    FMythicCellCoord Cell;

    UPROPERTY(BlueprintReadOnly, Category = "Living World|Encounter")
    FMythicFactionId OriginFaction;

    // Client-side ingest hooks (see faction proxy). Defined out-of-line.
    void PreReplicatedRemove(const struct FMythicEncounterProxyArray &InArraySerializer);
    void PostReplicatedAdd(const struct FMythicEncounterProxyArray &InArraySerializer);
    void PostReplicatedChange(const struct FMythicEncounterProxyArray &InArraySerializer);
};

/** Fast array for replicating active encounters (unlike factions/territory, items are REMOVED on completion). */
USTRUCT()
struct MYTHIC_API FMythicEncounterProxyArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicEncounterProxyItem> Items;

    // Non-replicated back-pointer to the owning replicator (set in the replicator's ctor). Not a UPROPERTY.
    class AMythicLivingWorldReplicator *OwnerReplicator = nullptr;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicEncounterProxyItem, FMythicEncounterProxyArray>(Items, DeltaParms, *this);
    }
};

template <>
struct TStructOpsTypeTraits<FMythicEncounterProxyArray> : public TStructOpsTypeTraitsBase2<FMythicEncounterProxyArray> {
    enum { WithNetDeltaSerializer = true };
};

// ─────────────────────────────────────────────────────────────
// Replicator Actor — Server->Client sync manager
// ─────────────────────────────────────────────────────────────

/**
 * Transient actor spawned by the Living World Subsystem on the server.
 * Replicates the fast arrays to all connected clients.
 * Clients use this to update their local UMythicLivingWorldSubsystem caches.
 */
UCLASS(NotBlueprintable, Transient)
class MYTHIC_API AMythicLivingWorldReplicator : public AInfo {
    GENERATED_BODY()

public:
    AMythicLivingWorldReplicator();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** The replicated list of faction states */
    UPROPERTY(Replicated)
    FMythicFactionProxyArray FactionProxies;

    /** The replicated list of territory cell ownership changes */
    UPROPERTY(Replicated)
    FMythicTerritoryProxyArray TerritoryProxies;

    /** The replicated list of currently-active encounters (added on spawn, removed on completion). */
    UPROPERTY(Replicated)
    FMythicEncounterProxyArray EncounterProxies;

    /** Fast lookup for faction proxies */
    TMap<FMythicFactionId, int32> FactionProxyIndex;

    /** Fast lookup for territory proxies */
    TMap<FMythicCellCoord, int32> TerritoryProxyIndex;

    /** Sync current subsystem state into these arrays (called on Server by Subsystem) */
    void SyncProxies(class UMythicLivingWorldSubsystem *Subsystem);

    /** True iff a territory proxy's CLIENT-VISIBLE fields (dominant faction, contested level) differ from the new grid
     *  state. SyncProxies uses this to avoid re-replicating cells whose influence shifted but whose dominant faction
     *  didn't change (GetChangedCells returns every influence-changed cell) — without it the delta re-sends unchanged
     *  proxies to every client each tick. Static + pure → unit-testable. */
    static bool TerritoryProxyNeedsUpdate(const FMythicTerritoryProxyItem &Existing, FMythicFactionId NewFaction, uint8 NewContestedLevel);

    // ─── Client read API (the replicated proxies are the client's living-world cache) ───

    /** CLIENT/SERVER: the faction proxy for a faction, or null if not currently replicated. Linear scan (factions
     *  are few — bounded by GetMaxFactions). */
    const FMythicFactionProxyItem *GetFactionProxy(FMythicFactionId FactionId) const;

    /** All currently-replicated faction proxies (active factions only — dormant/annihilated are not synced). */
    const TArray<FMythicFactionProxyItem> &GetAllFactionProxies() const { return FactionProxies.Items; }

    /** CLIENT/SERVER: the territory proxy for a cell. Returns false if that cell hasn't been synced. */
    bool GetTerritoryProxy(FMythicCellCoord Cell, FMythicTerritoryProxyItem &OutProxy) const;

    /** All currently-replicated territory proxies (changed cells). */
    const TArray<FMythicTerritoryProxyItem> &GetAllTerritoryProxies() const { return TerritoryProxies.Items; }

    /** All currently-replicated active encounters (for a client map/HUD: type, state, cell, faction). */
    const TArray<FMythicEncounterProxyItem> &GetAllEncounterProxies() const { return EncounterProxies.Items; }

    /** Called by the FastArray item callbacks on the CLIENT when proxies replicate in — broadcasts the subsystem's
     *  change delegate so UI/gameplay can react. No-op on the server (it is the source). */
    void NotifyClientProxiesChanged();

private:
    /** CLIENT only: the local subsystem this replicator registers with (so subsystem accessors + delegate work
     *  client-side, where the subsystem otherwise never learns about the server-spawned replicator). */
    TWeakObjectPtr<class UMythicLivingWorldSubsystem> ClientSubsystem;
};
