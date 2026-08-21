
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"

void UMythicPersistentNPCRegistry::RegisterDeath(
    uint32 NameHash,
    const FMythicFactionId &Faction,
    const FGameplayTag &RoleTag,
    const FMythicCellCoord &Cell,
    double WorldTime,
    UMythicLivingWorldSubsystem *OwningLWS) {
    if (DeadNPCHashes.Contains(NameHash)) {
        return;
    }

    DeadNPCHashes.Add(NameHash);

    FMythicPersistentDeathRecord Record;
    Record.NameHash = NameHash;
    Record.Faction = Faction;
    Record.RoleTag = RoleTag;
    Record.DeathTime = WorldTime;
    Record.DeathCell = Cell;
    constexpr int32 MaxDeathRecords = 256;
    if (DeathRecords.Num() >= MaxDeathRecords) {
        DeathRecords.RemoveAt(0, DeathRecords.Num() - MaxDeathRecords + 1);
    }
    DeathRecords.Add(Record);

    UE_LOG(LogMythLivingWorld, Log, TEXT("Permanent death registered: NameHash=%u, Faction=%d, Role=%s, Cell=(%d,%d)"),
           NameHash, Faction.Index, *RoleTag.ToString(), Cell.X, Cell.Y);


    if (OwningLWS) {
        FMythicWorldEvent DeathEvent;
        DeathEvent.WorldTime = WorldTime;
        DeathEvent.Cell = Cell;
        DeathEvent.PrimaryFaction = Faction;
        DeathEvent.EventTag = TAG_WORLD_EVENT_DEATH_PERMANENT;
        DeathEvent.PerpEntityId = NameHash;
        DeathEvent.Significance = 1.0f;
        DeathEvent.CategoryFlags = EMythicEventCategory::Death;

        DeathEvent.MoralVector = FMythicMoralSignature::MakeKillActionMoralVector();

        OwningLWS->SubmitWorldEvent(DeathEvent);
    }

    if (OwningLWS) {
        OwningLWS->HandleNPCDeathSettlements(NameHash, WorldTime);
    }
}

void UMythicPersistentNPCRegistry::Serialize(FArchive &Ar) {
    int32 Version = 3;
    Ar << Version;

    int32 Count = DeathRecords.Num();
    Ar << Count;

    if (Ar.IsSaving()) {
        for (const FMythicPersistentDeathRecord &Record : DeathRecords) {
            uint32 Hash = Record.NameHash;
            int32 FactionIdx = Record.Faction.Index;
            FGameplayTag RoleTag = Record.RoleTag;
            double Time = Record.DeathTime;
            int32 CellX = Record.DeathCell.X;
            int32 CellY = Record.DeathCell.Y;

            Ar << Hash;
            Ar << FactionIdx;
            Ar << RoleTag;
            Ar << Time;
            Ar << CellX;
            Ar << CellY;
        }
    }
    else {
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
            FGameplayTag RoleTag;
            double Time;
            int32 CellX, CellY;

            Ar << Hash;
            Ar << FactionIdx;
            if (Version >= 3) {
                Ar << RoleTag;
            }
            else {
                FString RoleStr;
                Ar << RoleStr;
                RoleTag = FGameplayTag::RequestGameplayTag(FName(*RoleStr),false);
            }
            Ar << Time;
            Ar << CellX;
            Ar << CellY;

            FMythicPersistentDeathRecord &Record = DeathRecords[i];
            Record.NameHash = Hash;
            Record.Faction.Index = FactionIdx;
            Record.RoleTag = RoleTag;
            Record.DeathTime = Time;
            Record.DeathCell = FMythicCellCoord(CellX, CellY);

            DeadNPCHashes.Add(Hash);
        }
    }

    if (Version >= 2) {
        Ar << NextSpawnSerial;
    }
}
