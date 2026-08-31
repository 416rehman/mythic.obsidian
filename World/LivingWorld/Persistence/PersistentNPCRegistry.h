#pragma once

#include "CoreMinimal.h"
#include "World/Entity/MythicEntityId.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "PersistentNPCRegistry.generated.h"

class UMythicLivingWorldSubsystem;

/** Sticky save-backed ownership that must outlive disconnected runtime owners. */
enum class EMythicEntityIdentityRetention : uint8 {
    None = 0,
    LearnedDossier = 1 << 0,
};
ENUM_CLASS_FLAGS(EMythicEntityIdentityRetention);

/** Authority allocation route that created a persistent LivingWorld entity identity. */
UENUM()
enum class EMythicEntityIdentityProvenance : uint8 {
    Invalid = 0,
    Encounter,
    CreatureEcology,
    SettlementPopulation,
    DynamicGroup,
    TerritoryPatrol,
    TerritoryTraveler,
    RouteTraveler,
    DesignerSpawner,
    AuthoredNPC,
    Runtime,
};

/** Serialized authority record for one logical LivingWorld entity; NameSeed is generation data, never its key. */
USTRUCT()
struct MYTHIC_API FMythicPersistentEntityIdentityRecord {
    GENERATED_BODY()

    FMythicEntityId EntityId;

    uint32 NameSeed = 0;

    EMythicEntityIdentityProvenance Provenance = EMythicEntityIdentityProvenance::Invalid;

    uint64 AllocationSequence = 0;

    /** Sticky durable ownership; transient owners such as parties and faction roles are queried transactionally. */
    EMythicEntityIdentityRetention Retention = EMythicEntityIdentityRetention::None;
};

/** Permanent-death tombstone keyed only by canonical authority identity. */
USTRUCT()
struct MYTHIC_API FMythicPersistentDeathRecord {
    GENERATED_BODY()

    FMythicEntityId EntityId;

    FMythicFactionId Faction;

    FGameplayTag RoleTag;

    double DeathTime = 0.0;

    FMythicCellCoord DeathCell;
};

/**
 * Single authority allocator and serialized record store for LivingWorld logical entity identity.
 *
 * Canonical IDs never leave authority/private simulation state. Public presentation uses fresh opaque presentation
 * handles. Name seeds may collide and are used only to reconstruct deterministic names, appearances, and schedules.
 */
UCLASS()
class MYTHIC_API UMythicPersistentNPCRegistry : public UObject {
    GENERATED_BODY()

public:
    /** Allocates and records a fresh canonical LivingWorld identity; invalid provenance fails closed. */
    FMythicEntityId AllocateEntityIdentity(
        uint32 NameSeed,
        EMythicEntityIdentityProvenance Provenance);

    /** Returns the immutable authority record for EntityId, or null for invalid/unregistered IDs. */
    const FMythicPersistentEntityIdentityRecord *FindIdentityRecord(
        const FMythicEntityId &EntityId) const;

    /** Returns true only when the exact typed canonical identity is registered. */
    bool ContainsEntityIdentity(const FMythicEntityId &EntityId) const {
        return FindIdentityRecord(EntityId) != nullptr;
    }

    /** Marks that at least one durable player dossier owns this identity, including while that player is offline. */
    bool MarkRetainedByLearnedDossier(const FMythicEntityId &EntityId);

    /** Returns true when offline-safe durable knowledge prevents this identity record from being retired. */
    bool IsRetainedByLearnedDossier(const FMythicEntityId &EntityId) const;

    /**
     * Adds one permanent-death tombstone. Invalid, foreign-domain, and unregistered IDs fail closed; duplicate deaths
     * are idempotent. Returns true only when a new tombstone was committed.
     */
    bool RegisterDeath(
        const FMythicEntityId &EntityId,
        const FMythicFactionId &Faction,
        const FGameplayTag &RoleTag,
        const FMythicCellCoord &Cell,
        double WorldTime,
        UMythicLivingWorldSubsystem *OwningLWS);

    /** Returns true when the exact canonical entity has a permanent-death tombstone. */
    bool IsPermaDead(const FMythicEntityId &EntityId) const {
        return EntityId.IsValid() && DeadEntityIds.Contains(EntityId);
    }

    /** Allocates a persisted deterministic-generation serial. It is not an entity identity. */
    int32 AllocateNameSeedSerial();

    /** Returns all immutable canonical identity records; consumers must use AllocationSequence when order matters. */
    const TArray<FMythicPersistentEntityIdentityRecord> &GetIdentityRecords() const {
        return IdentityRecords;
    }

    /** Returns all permanent-death tombstones in commit order. */
    const TArray<FMythicPersistentDeathRecord> &GetDeathRecords() const { return DeathRecords; }

    /** Returns the number of canonical identities known by the authority registry. */
    int32 GetIdentityCount() const { return IdentityRecords.Num(); }

    /** Returns the number of permanent-death tombstones. */
    int32 GetDeathCount() const { return DeadEntityIds.Num(); }

    virtual void Serialize(FArchive &Ar) override;

private:
    static bool IsValidProvenance(EMythicEntityIdentityProvenance Provenance);
    void ResetSerializedState();
    bool RebuildIdentityIndex();

    /** Removes an externally verified unreferenced record; only the LivingWorld retirement transaction may call it. */
    bool ReleaseUnreferencedEntityIdentity(const FMythicEntityId &EntityId);

    TArray<FMythicPersistentEntityIdentityRecord> IdentityRecords;
    TMap<FMythicEntityId, int32> IdentityRecordIndex;

    TSet<FMythicEntityId> DeadEntityIds;
    TArray<FMythicPersistentDeathRecord> DeathRecords;

    uint32 NextNameSeedSerial = 0;
    uint64 NextIdentityAllocationSequence = 1;

    friend class UMythicLivingWorldSubsystem;
};
