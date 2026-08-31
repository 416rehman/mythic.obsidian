
#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/Entity/MythicEntityId.h"
#include "CausalFabric.generated.h"


USTRUCT()
struct MYTHIC_API FMythicWorldEvent {
    GENERATED_BODY()

    uint32 EventId = 0;

    uint32 ParentEventId = 0;

    double WorldTime = 0.0;

    FMythicCellCoord Cell;

    FMythicFactionId PrimaryFaction;

    FMythicFactionId SecondaryFaction;

    FGameplayTag EventTag;

    FMythicMoralAction MoralVector;

    /** Authority/private canonical subject identity; never included in public LivingWorld proxies. */
    FMythicEntityId PerpEntityId;

    /** Deterministic subject-name input retained separately from canonical identity. */
    uint32 PerpNameSeed = 0;

    /** Authority/private canonical victim identity; never included in public LivingWorld proxies. */
    FMythicEntityId VictimEntityId;

    /** Deterministic victim-name input retained separately from canonical identity. */
    uint32 VictimNameSeed = 0;

    float Significance = 0.0f;

    uint16 CategoryFlags = 0;

    EMythicActionCategory ActionCategory = EMythicActionCategory::Melee;

    uint8 VisibilityGroup = 0;
};

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
    constexpr uint16 Encounter = 1 << 10;
}


struct FMythicCellEventRange {
    int32 OldestIndex = -1;

    int32 Count = 0;
};


UCLASS()
class MYTHIC_API UMythicCausalFabric : public UObject {
    GENERATED_BODY()

public:
    void Initialize(int32 InCapacity);


    uint32 AppendEvent(const FMythicWorldEvent &Event);

    void CommitWrites();


    const FMythicWorldEvent *GetEvent(uint32 EventId) const;

    TArray<FMythicWorldEvent> GetRecentEvents(int32 MaxCount) const;

    void QueryEventsByCell(
        const FMythicCellCoord &Cell,
        double MinWorldTime,
        double MaxWorldTime,
        int32 MaxResults,
        TArray<FMythicWorldEvent> &OutEvents) const;

    void QueryEventsByCategory(
        uint16 CategoryMask,
        double MinWorldTime,
        double MaxWorldTime,
        int32 MaxResults,
        TArray<FMythicWorldEvent> &OutEvents) const;

    uint32 GetTotalEventCount() const { return NextEventId.load(std::memory_order_relaxed); }

    int32 GetCapacity() const { return Capacity; }

    mutable FRWLock FabricLock;

    void QueryEventsByFaction(
        const FMythicFactionId &Faction,
        TArray<FMythicWorldEvent> &OutEvents,
        int32 MaxResults) const;


    virtual void Serialize(FArchive &Ar) override;

private:
    int32 Capacity = 0;

    std::atomic<uint32> NextEventId{1};

    TArray<FMythicWorldEvent> WriteBuffer;

    TArray<FMythicWorldEvent> ReadBuffer;

    TMap<FMythicCellCoord, TArray<uint32>> WriteSpatialIndex;

    TMap<FMythicCellCoord, TArray<uint32>> ReadSpatialIndex;

    int32 WriteHead = 0;

    int32 WriteCount = 0;

    int32 ReadHead = 0;

    int32 ReadCount = 0;

    uint32 BaseEventId = 1;

    uint32 ReadBaseEventId = 1;
    uint32 ReadNewestEventId = 0;

    int32 EventIdToIndex(uint32 EventId, int32 HeadPos, int32 Count) const;
};
