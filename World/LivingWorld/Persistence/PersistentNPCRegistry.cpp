#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"

#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"

namespace {
constexpr int32 PersistentIdentityRegistryVersion = 2;
constexpr int32 MaxSerializedIdentityRecords = 1'000'000;
constexpr int32 MaxSerializedDeathRecords = 1'000'000;
constexpr int32 MaxGuidAllocationAttempts = 32;

void SerializeEntityId(FArchive &Ar, FMythicEntityId &EntityId) {
    uint8 Domain = static_cast<uint8>(EntityId.GetDomain());
    FGuid Guid = EntityId.GetAuthorityGuid();

    Ar << Domain;
    Ar << Guid.A;
    Ar << Guid.B;
    Ar << Guid.C;
    Ar << Guid.D;

    if (Ar.IsLoading()) {
        EntityId = FMythicEntityId::FromAuthorityGuid(
            static_cast<EMythicEntityDomain>(Domain), Guid);
    }
}
}

bool UMythicPersistentNPCRegistry::IsValidProvenance(
    const EMythicEntityIdentityProvenance Provenance) {
    return Provenance > EMythicEntityIdentityProvenance::Invalid
           && Provenance <= EMythicEntityIdentityProvenance::Runtime;
}

FMythicEntityId UMythicPersistentNPCRegistry::AllocateEntityIdentity(
    const uint32 NameSeed,
    const EMythicEntityIdentityProvenance Provenance) {
    check(IsInGameThread());

    if (!IsValidProvenance(Provenance)) {
        UE_LOG(LogMythLivingWorld, Error,
               TEXT("Identity allocation rejected invalid provenance %u."),
               static_cast<uint8>(Provenance));
        return FMythicEntityId();
    }

    for (int32 Attempt = 0; Attempt < MaxGuidAllocationAttempts; ++Attempt) {
        const FMythicEntityId Candidate = FMythicEntityId::FromAuthorityGuid(
            EMythicEntityDomain::LivingWorld, FGuid::NewGuid());
        if (!Candidate.IsValid() || IdentityRecordIndex.Contains(Candidate)) {
            continue;
        }

        if (NextIdentityAllocationSequence == 0
            || NextIdentityAllocationSequence == MAX_uint64) {
            UE_LOG(LogMythLivingWorld, Error,
                   TEXT("Identity allocation sequence exhausted; refusing to alias persistent identities."));
            return FMythicEntityId();
        }

        FMythicPersistentEntityIdentityRecord Record;
        Record.EntityId = Candidate;
        Record.NameSeed = NameSeed;
        Record.Provenance = Provenance;
        Record.AllocationSequence = NextIdentityAllocationSequence++;

        const int32 RecordIndex = IdentityRecords.Add(Record);
        IdentityRecordIndex.Add(Candidate, RecordIndex);
        return Candidate;
    }

    UE_LOG(LogMythLivingWorld, Error,
           TEXT("Failed to allocate a collision-free canonical identity after %d attempts."),
           MaxGuidAllocationAttempts);
    return FMythicEntityId();
}

const FMythicPersistentEntityIdentityRecord *
UMythicPersistentNPCRegistry::FindIdentityRecord(
    const FMythicEntityId &EntityId) const {
    if (!EntityId.IsValid()
        || EntityId.GetDomain() != EMythicEntityDomain::LivingWorld) {
        return nullptr;
    }

    const int32 *RecordIndex = IdentityRecordIndex.Find(EntityId);
    return RecordIndex && IdentityRecords.IsValidIndex(*RecordIndex)
               ? &IdentityRecords[*RecordIndex]
               : nullptr;
}

bool UMythicPersistentNPCRegistry::MarkRetainedByLearnedDossier(
    const FMythicEntityId &EntityId) {
    check(IsInGameThread());
    const int32 *FoundIndex = IdentityRecordIndex.Find(EntityId);
    if (!FoundIndex || !IdentityRecords.IsValidIndex(*FoundIndex)) {
        return false;
    }

    IdentityRecords[*FoundIndex].Retention |=
        EMythicEntityIdentityRetention::LearnedDossier;
    return true;
}

bool UMythicPersistentNPCRegistry::IsRetainedByLearnedDossier(
    const FMythicEntityId &EntityId) const {
    const FMythicPersistentEntityIdentityRecord *Record =
        FindIdentityRecord(EntityId);
    return Record
           && EnumHasAnyFlags(Record->Retention,
                              EMythicEntityIdentityRetention::LearnedDossier);
}

bool UMythicPersistentNPCRegistry::ReleaseUnreferencedEntityIdentity(
    const FMythicEntityId &EntityId) {
    check(IsInGameThread());
    if (!EntityId.IsValid() || DeadEntityIds.Contains(EntityId)
        || IsRetainedByLearnedDossier(EntityId)) {
        return false;
    }

    const int32 *FoundIndex = IdentityRecordIndex.Find(EntityId);
    if (!FoundIndex || !IdentityRecords.IsValidIndex(*FoundIndex)) {
        return false;
    }

    const int32 RemovedIndex = *FoundIndex;
    const int32 LastIndex = IdentityRecords.Num() - 1;
    IdentityRecordIndex.Remove(EntityId);
    if (RemovedIndex != LastIndex) {
        const FMythicEntityId MovedEntityId = IdentityRecords[LastIndex].EntityId;
        IdentityRecords.RemoveAtSwap(RemovedIndex, 1, EAllowShrinking::No);
        IdentityRecordIndex.FindChecked(MovedEntityId) = RemovedIndex;
    }
    else {
        IdentityRecords.Pop(EAllowShrinking::No);
    }
    return true;
}

bool UMythicPersistentNPCRegistry::RegisterDeath(
    const FMythicEntityId &EntityId,
    const FMythicFactionId &Faction,
    const FGameplayTag &RoleTag,
    const FMythicCellCoord &Cell,
    const double WorldTime,
    UMythicLivingWorldSubsystem *OwningLWS) {
    check(IsInGameThread());

    const FMythicPersistentEntityIdentityRecord *IdentityRecord =
        FindIdentityRecord(EntityId);
    if (!IdentityRecord) {
        UE_LOG(LogMythLivingWorld, Error,
               TEXT("Permanent death rejected an invalid or unregistered canonical entity ID."));
        return false;
    }
    if (DeadEntityIds.Contains(EntityId)) {
        return false;
    }

    DeadEntityIds.Add(EntityId);

    FMythicPersistentDeathRecord Record;
    Record.EntityId = EntityId;
    Record.Faction = Faction;
    Record.RoleTag = RoleTag;
    Record.DeathTime = WorldTime;
    Record.DeathCell = Cell;
    DeathRecords.Add(Record);

    UE_LOG(LogMythLivingWorld, Log,
           TEXT("Permanent death registered: %s, Faction=%d, Role=%s, Cell=(%d,%d)"),
           *EntityId.ToDebugString(), Faction.Index, *RoleTag.ToString(), Cell.X, Cell.Y);

    if (OwningLWS) {
        FMythicWorldEvent DeathEvent;
        DeathEvent.WorldTime = WorldTime;
        DeathEvent.Cell = Cell;
        DeathEvent.PrimaryFaction = Faction;
        DeathEvent.EventTag = TAG_WORLD_EVENT_DEATH_PERMANENT;
        DeathEvent.PerpEntityId = EntityId;
        DeathEvent.PerpNameSeed = IdentityRecord->NameSeed;
        DeathEvent.Significance = 1.0f;
        DeathEvent.CategoryFlags = EMythicEventCategory::Death;
        DeathEvent.MoralVector = FMythicMoralSignature::MakeKillActionMoralVector();

        OwningLWS->SubmitWorldEvent(DeathEvent);
        OwningLWS->HandlePermanentEntityDeath(EntityId, WorldTime);
    }

    return true;
}

int32 UMythicPersistentNPCRegistry::AllocateNameSeedSerial() {
    check(IsInGameThread());
    const int32 Serial = static_cast<int32>(NextNameSeedSerial);
    ++NextNameSeedSerial;
    return Serial;
}

void UMythicPersistentNPCRegistry::ResetSerializedState() {
    IdentityRecords.Reset();
    IdentityRecordIndex.Reset();
    DeadEntityIds.Reset();
    DeathRecords.Reset();
    NextNameSeedSerial = 0;
    NextIdentityAllocationSequence = 1;
}

bool UMythicPersistentNPCRegistry::RebuildIdentityIndex() {
    IdentityRecordIndex.Reset();
    IdentityRecordIndex.Reserve(IdentityRecords.Num());

    TSet<uint64> AllocationSequences;
    uint64 MaxAllocationSequence = 0;
    for (int32 Index = 0; Index < IdentityRecords.Num(); ++Index) {
        const FMythicPersistentEntityIdentityRecord &Record = IdentityRecords[Index];
        if (!Record.EntityId.IsValid()
            || Record.EntityId.GetDomain() != EMythicEntityDomain::LivingWorld
            || !IsValidProvenance(Record.Provenance)
            || Record.AllocationSequence == 0
            || IdentityRecordIndex.Contains(Record.EntityId)
            || AllocationSequences.Contains(Record.AllocationSequence)) {
            return false;
        }

        IdentityRecordIndex.Add(Record.EntityId, Index);
        AllocationSequences.Add(Record.AllocationSequence);
        MaxAllocationSequence = FMath::Max(MaxAllocationSequence,
                                            Record.AllocationSequence);
    }

    return NextIdentityAllocationSequence > MaxAllocationSequence
           && NextIdentityAllocationSequence != 0;
}

void UMythicPersistentNPCRegistry::Serialize(FArchive &Ar) {
    int32 Version = PersistentIdentityRegistryVersion;
    Ar << Version;
    if (Ar.IsLoading() && Version != PersistentIdentityRegistryVersion) {
        ResetSerializedState();
        Ar.SetError();
        return;
    }

    Ar << NextNameSeedSerial;
    Ar << NextIdentityAllocationSequence;

    int32 IdentityCount = IdentityRecords.Num();
    Ar << IdentityCount;
    if (IdentityCount < 0 || IdentityCount > MaxSerializedIdentityRecords) {
        if (Ar.IsLoading()) {
            ResetSerializedState();
        }
        Ar.SetError();
        return;
    }

    if (Ar.IsLoading()) {
        IdentityRecords.SetNum(IdentityCount);
    }
    for (FMythicPersistentEntityIdentityRecord &Record : IdentityRecords) {
        SerializeEntityId(Ar, Record.EntityId);
        Ar << Record.NameSeed;

        uint8 Provenance = static_cast<uint8>(Record.Provenance);
        Ar << Provenance;
        if (Ar.IsLoading()) {
            Record.Provenance =
                static_cast<EMythicEntityIdentityProvenance>(Provenance);
        }

        Ar << Record.AllocationSequence;

        uint8 Retention = static_cast<uint8>(Record.Retention);
        Ar << Retention;
        if (Ar.IsLoading()) {
            constexpr uint8 AllowedRetention = static_cast<uint8>(
                EMythicEntityIdentityRetention::LearnedDossier);
            if ((Retention & ~AllowedRetention) != 0) {
                ResetSerializedState();
                Ar.SetError();
                return;
            }
            Record.Retention =
                static_cast<EMythicEntityIdentityRetention>(Retention);
        }
    }

    if (Ar.IsLoading() && !RebuildIdentityIndex()) {
        ResetSerializedState();
        Ar.SetError();
        return;
    }

    int32 DeathCount = DeathRecords.Num();
    Ar << DeathCount;
    if (DeathCount < 0 || DeathCount > MaxSerializedDeathRecords) {
        if (Ar.IsLoading()) {
            ResetSerializedState();
        }
        Ar.SetError();
        return;
    }

    if (Ar.IsLoading()) {
        DeathRecords.SetNum(DeathCount);
        DeadEntityIds.Reset();
        DeadEntityIds.Reserve(DeathCount);
    }
    for (FMythicPersistentDeathRecord &Record : DeathRecords) {
        SerializeEntityId(Ar, Record.EntityId);
        Ar << Record.Faction.Index;
        Ar << Record.RoleTag;
        Ar << Record.DeathTime;
        Ar << Record.DeathCell.X;
        Ar << Record.DeathCell.Y;

        if (Ar.IsLoading()) {
            if (!ContainsEntityIdentity(Record.EntityId)
                || DeadEntityIds.Contains(Record.EntityId)) {
                ResetSerializedState();
                Ar.SetError();
                return;
            }
            DeadEntityIds.Add(Record.EntityId);
        }
    }
}
