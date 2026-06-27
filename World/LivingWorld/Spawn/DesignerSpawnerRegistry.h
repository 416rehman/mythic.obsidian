// Mythic Living World — Designer Spawner Registry
//
// AUTHORITATIVE persistence store for designer-spawner state, keyed by the spawner's stable FName DesignerId. Owned by
// UMythicLivingWorldSubsystem (a sibling of PersistentNPCRegistry / SettlementRegistry) and serialized inside the LWS
// save blob. Keying by DesignerId (not the placed-actor path name) keeps a spawner's SpawnsEver / perma-death identity
// stable across PIE/runtime path-name churn and across save/load.
//
// Thread safety: created + accessed on the game thread only (the spawner's eval timer is game-thread). Server-mutated
// only — the registry is created on clients too (harmless empty object) but never written client-side.

#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/Spawn/DesignerSpawnerTypes.h"
#include "DesignerSpawnerRegistry.generated.h"

/**
 * Registry of per-DesignerId spawner state. Mirrors the bound-checked, versioned Serialize idiom of
 * UMythicPersistentNPCRegistry so it composes safely with the unframed LWS save stream.
 */
UCLASS()
class MYTHIC_API UMythicDesignerSpawnerRegistry : public UObject {
    GENERATED_BODY()

public:
    /** Get (creating if absent) the mutable state for a DesignerId. */
    FMythicDesignerSpawnerState& FindOrAdd(FName DesignerId) {
        return States.FindOrAdd(DesignerId);
    }

    /** Read-only lookup; null if this DesignerId has no state yet. */
    const FMythicDesignerSpawnerState* Find(FName DesignerId) const {
        return States.Find(DesignerId);
    }

    /** Record one spawn (++SpawnsEver) for a DesignerId. */
    void RecordSpawn(FName DesignerId) {
        FMythicDesignerSpawnerState& S = States.FindOrAdd(DesignerId);
        ++S.SpawnsEver;
    }

    /** Record a death of one of a DesignerId's NPCs. Stamps LastDeathTime and latches bPermaDead (sticky, never cleared). */
    void RecordDeath(FName DesignerId, double WorldTime, bool bPerma) {
        FMythicDesignerSpawnerState& S = States.FindOrAdd(DesignerId);
        S.LastDeathTime = WorldTime;
        S.bPermaDead |= bPerma;
    }

    /** All DesignerIds with stored state (for the debugger). */
    void GetAllDesignerIds(TArray<FName>& OutIds) const {
        States.GetKeys(OutIds);
    }

    /** Versioned, bound-checked serialization (mirrors UMythicPersistentNPCRegistry::Serialize). */
    virtual void Serialize(FArchive& Ar) override;

private:
    /** DesignerId -> durable state. */
    UPROPERTY()
    TMap<FName, FMythicDesignerSpawnerState> States;
};
