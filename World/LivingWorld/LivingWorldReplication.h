// Mythic Living World — Replication Proxies
// Lightweight, delta-compressed structs for syncing high-level Living World state to clients.
// Clients do not run the world sim thread, but need faction wealth, territory boundaries, etc.

#pragma once

#include "CoreMinimal.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "LivingWorldReplication.generated.h"

// ─────────────────────────────────────────────────────────────
// Faction Proxy — Networked representation of a Faction
// ─────────────────────────────────────────────────────────────

USTRUCT()
struct MYTHIC_API FMythicFactionProxyItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY()
    FMythicFactionId FactionId;

    UPROPERTY()
    EMythicFactionStatus Status = EMythicFactionStatus::Dormant;

    // Approximated population for UI / client logic
    UPROPERTY()
    int32 Population = 0;

    // Total controlled cells
    UPROPERTY()
    int32 ControlledCellCount = 0;

    // Relative wealth level [0-255 mapped to 0-max] for UI
    UPROPERTY()
    uint8 WealthLevel = 0;

    // Relative military strength [0-255 mapped to 0-1] for UI
    UPROPERTY()
    uint8 MilitaryStrength = 0;

    void PreReplicatedRemove(const struct FMythicFactionProxyArray& InArraySerializer) {}
    void PostReplicatedAdd(const struct FMythicFactionProxyArray& InArraySerializer) {}
    void PostReplicatedChange(const struct FMythicFactionProxyArray& InArraySerializer) {}
};

/** Fast array for replicating faction proxies */
USTRUCT()
struct MYTHIC_API FMythicFactionProxyArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicFactionProxyItem> Items;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicFactionProxyItem, FMythicFactionProxyArray>(Items, DeltaParms, *this);
    }
};

template<>
struct TStructOpsTypeTraits<FMythicFactionProxyArray> : public TStructOpsTypeTraitsBase2<FMythicFactionProxyArray> {
    enum { WithNetDeltaSerializer = true };
};

// ─────────────────────────────────────────────────────────────
// Territory Cell Proxy — Networked territory grid changes
// ─────────────────────────────────────────────────────────────

USTRUCT()
struct MYTHIC_API FMythicTerritoryProxyItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY()
    FMythicCellCoord Cell;

    UPROPERTY()
    FMythicFactionId ControllingFaction;

    UPROPERTY()
    uint8 ContestedLevel = 0;

    void PreReplicatedRemove(const struct FMythicTerritoryProxyArray& InArraySerializer) {}
    void PostReplicatedAdd(const struct FMythicTerritoryProxyArray& InArraySerializer) {}
    void PostReplicatedChange(const struct FMythicTerritoryProxyArray& InArraySerializer) {}
};

/** Fast array for replicating territory cell changes */
USTRUCT()
struct MYTHIC_API FMythicTerritoryProxyArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicTerritoryProxyItem> Items;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms) {
        return FFastArraySerializer::FastArrayDeltaSerialize<FMythicTerritoryProxyItem, FMythicTerritoryProxyArray>(Items, DeltaParms, *this);
    }
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

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** The replicated list of faction states */
    UPROPERTY(Replicated)
    FMythicFactionProxyArray FactionProxies;

    /** The replicated list of territory cell ownership changes */
    UPROPERTY(Replicated)
    FMythicTerritoryProxyArray TerritoryProxies;

    /** Fast lookup for faction proxies */
    TMap<FMythicFactionId, int32> FactionProxyIndex;

    /** Fast lookup for territory proxies */
    TMap<FMythicCellCoord, int32> TerritoryProxyIndex;

    /** Sync current subsystem state into these arrays (called on Server by Subsystem) */
    void SyncProxies(class UMythicLivingWorldSubsystem* Subsystem);
};
