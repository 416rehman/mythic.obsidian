// Mythic Living World — Persistent NPC Registry Implementation

#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/LivingWorldTypes.h"

void UMythicPersistentNPCRegistry::RegisterDeath(
    uint32 NameHash,
    const FMythicFactionId& Faction,
    const FGameplayTag& RoleTag,
    const FMythicCellCoord& Cell,
    double WorldTime,
    UMythicSocialGraph* SocialGraph,
    UMythicCausalFabric* Fabric,
    UMythicSettlementRegistry* SettlementRegistry,
    TArray<int32>& OutGrievingEntities)
{
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

    // ─── 2. Sever social graph edges ───
    if (SocialGraph) {
        // Note: Edge severing happens automatically during SocialGraph->PruneStaleEdges() 
        // because the underlying MassEntity is destroyed. Grief pressure is generated 
        // organically during that periodic pruning when the invalid handles are found.
    }

    // ─── 3. Write death event to Causal Fabric ───
    if (Fabric) {
        FMythicWorldEvent DeathEvent;
        DeathEvent.WorldTime = WorldTime;
        DeathEvent.Cell = Cell;
        DeathEvent.PrimaryFaction = Faction;
        DeathEvent.EventTag = FGameplayTag::RequestGameplayTag(FName("World.Event.Death.Permanent"));
        DeathEvent.PerpEntityId = NameHash;
        DeathEvent.Significance = 1.0f; // Max significance — permanent death
        DeathEvent.CategoryFlags = EMythicEventCategory::Death;

        FMythicMoralAction MoralVector;
        MoralVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] = -0.9f;
        MoralVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Mercy)] = -0.5f;
        DeathEvent.MoralVector = MoralVector;

        Fabric->AppendEvent(DeathEvent);
    }

    // ─── 4. Role Vacation and Succession ───
    if (SettlementRegistry) {
        SettlementRegistry->HandleNPCDeath(NameHash, WorldTime);
    }
}

void UMythicPersistentNPCRegistry::Serialize(FArchive& Ar)
{
    int32 Version = 1;
    Ar << Version;

    int32 Count = DeathRecords.Num();
    Ar << Count;

    if (Ar.IsSaving()) {
        for (const FMythicPersistentDeathRecord& Record : DeathRecords) {
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
    } else {
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

            FMythicPersistentDeathRecord& Record = DeathRecords[i];
            Record.NameHash = Hash;
            Record.Faction.Index = FactionIdx;
            Record.RoleTag = FGameplayTag::RequestGameplayTag(FName(*RoleStr));
            Record.DeathTime = Time;
            Record.DeathCell = FMythicCellCoord(CellX, CellY);

            DeadNPCHashes.Add(Hash);
        }
    }
}
