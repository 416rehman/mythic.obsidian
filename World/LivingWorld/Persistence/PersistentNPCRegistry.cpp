// Mythic Living World — Persistent NPC Registry Implementation

#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldTypes.h"

void UMythicPersistentNPCRegistry::RegisterDeath(
    uint32 NameHash,
    const FMythicFactionId &Faction,
    const FGameplayTag &RoleTag,
    const FMythicCellCoord &Cell,
    double WorldTime,
    UMythicLivingWorldSubsystem *OwningLWS) {
    // Guard against duplicate registration
    if (DeadNPCHashes.Contains(NameHash)) {
        return;
    }

    // ─── 1. Add to permanent death set ───
    DeadNPCHashes.Add(NameHash);

    FMythicPersistentDeathRecord Record;
    Record.NameHash = NameHash;
    Record.Faction = Faction;
    Record.RoleTag = RoleTag;
    Record.DeathTime = WorldTime;
    Record.DeathCell = Cell;
    DeathRecords.Add(Record);

    UE_LOG(LogMythLivingWorld, Log, TEXT("Permanent death registered: NameHash=%u, Faction=%d, Role=%s, Cell=(%d,%d)"),
           NameHash, Faction.Index, *RoleTag.ToString(), Cell.X, Cell.Y);

    // ─── 2. Grief from this death ───
    // Grief pressure is NOT generated here. The World.Event.Death.Permanent event submitted below (section 3) is
    // witnessed by the PressureProcessor, which raises Grief on nearby entities — that is the live grief path.
    // (A social-graph grief channel via edge-severing was never wired: PruneStaleEdges only decays edges, it
    // generates no grief. If that channel is wanted later it is a separate feature — tracked in BACKLOG.)

    // ─── 3. Write death event to Causal Fabric (via the thread-safe queue) ───
    // RegisterDeath runs on the GAME thread (HandleNPCDeath's OnDeath delegate), so it MUST route through the
    // subsystem's SubmitWorldEvent queue (drained by the sim thread) — a direct CausalFabric->AppendEvent here would
    // race the sim-thread single-writer (matches the EncounterDirector/PartySubsystem fixes).
    if (OwningLWS) {
        FMythicWorldEvent DeathEvent;
        DeathEvent.WorldTime = WorldTime;
        DeathEvent.Cell = Cell;
        DeathEvent.PrimaryFaction = Faction;
        DeathEvent.EventTag = FGameplayTag::RequestGameplayTag(FName("World.Event.Death.Permanent"));
        DeathEvent.PerpEntityId = NameHash;
        DeathEvent.Significance = 1.0f; // Max significance — permanent death
        DeathEvent.CategoryFlags = EMythicEventCategory::Death;

        // Canonical kill moral vector (Rule 3 single source — same constant the live kill path uses). Positive
        // Violence = harm, so anti-violence factions condemn it; the prior inline -0.9 inverted that (review HIGH).
        DeathEvent.MoralVector = FMythicMoralSignature::MakeKillActionMoralVector();

        OwningLWS->SubmitWorldEvent(DeathEvent);
    }

    // ─── 4. Role Vacation and Succession (via the subsystem's SimulationLock-guarded wrapper) ───
    // Routed through OwningLWS (NOT a direct registry call): HandleNPCDeath walks the Settlements TMap that the sim
    // thread rehashes/mutates under SimulationLock — a direct game-thread walk here would race a rehash (TMap UAF/crash).
    if (OwningLWS) {
        OwningLWS->HandleNPCDeathSettlements(NameHash, WorldTime);
    }
}

void UMythicPersistentNPCRegistry::Serialize(FArchive &Ar) {
    int32 Version = 2; // v2 (R18-M7): appends NextSpawnSerial after the death records
    Ar << Version;

    int32 Count = DeathRecords.Num();
    Ar << Count;

    if (Ar.IsSaving()) {
        for (const FMythicPersistentDeathRecord &Record : DeathRecords) {
            // Serialize each field explicitly for version stability
            uint32 Hash = Record.NameHash;
            int32 FactionIdx = Record.Faction.Index;
            FString RoleStr = Record.RoleTag.ToString();
            double Time = Record.DeathTime;
            int32 CellX = Record.DeathCell.X;
            int32 CellY = Record.DeathCell.Y;

            Ar << Hash;
            Ar << FactionIdx;
            Ar << RoleStr;
            Ar << Time;
            Ar << CellX;
            Ar << CellY;
        }
    }
    else {
        // Bound-check before SetNum: a desynced/corrupted stream can yield a garbage Count; an unbounded SetNum (and the
        // Empty(Count) reserves) would then attempt a massive allocation (OOM/crash). 1,000,000 is far above any
        // legitimate death-record count. SetError flags the archive so the load is treated as failed.
        if (Count < 0 || Count > 1000000) {
            Ar.SetError();
            return;
        }
        DeadNPCHashes.Empty(Count);
        DeathRecords.Empty(Count);
        DeathRecords.SetNum(Count);

        for (int32 i = 0; i < Count; ++i) {
            uint32 Hash;
            int32 FactionIdx;
            FString RoleStr;
            double Time;
            int32 CellX, CellY;

            Ar << Hash;
            Ar << FactionIdx;
            Ar << RoleStr;
            Ar << Time;
            Ar << CellX;
            Ar << CellY;

            FMythicPersistentDeathRecord &Record = DeathRecords[i];
            Record.NameHash = Hash;
            Record.Faction.Index = FactionIdx;
            // ErrorIfNotFound=false: a death record's RoleTag is historical display/succession data. Most perma-dead
            // NPCs are non-leaders with NO role, so RoleStr round-trips as "None" — and a tag could also be removed
            // between save and load. The old default-true call logged an Error on EVERY load of such a record (and the
            // tag still resolved to empty anyway). Resolve quietly to an empty tag instead.
            Record.RoleTag = FGameplayTag::RequestGameplayTag(FName(*RoleStr), /*ErrorIfNotFound=*/false);
            Record.DeathTime = Time;
            Record.DeathCell = FMythicCellCoord(CellX, CellY);

            DeadNPCHashes.Add(Hash);
        }
    }

    // ─── v2 (R18-M7): persisted global monotonic spawn serial ───
    // Seeds FMythicNPCGenerator::GenerateNameHash (replaces the old wave-local SpawnIdx). MUST persist: a reset to 0
    // would regenerate NameHashes already in DeadNPCHashes and wrongly perma-dead freshly-spawned NPCs. Appended
    // AFTER the death records and gated on Version>=2 so v1 saves still load byte-aligned (the registry is part of
    // the unframed LWS stream). Symmetric for save + load (Ar << on a uint32).
    if (Version >= 2) {
        Ar << NextSpawnSerial;
    }
}
