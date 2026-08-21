
#include "World/LivingWorld/CausalFabric/CausalFabric.h"

void UMythicCausalFabric::Initialize(int32 InCapacity) {
    check(InCapacity > 0);
    if (InCapacity <= 0) {
        UE_LOG(LogMythCausalFabric, Warning,
               TEXT("CausalFabric::Initialize: InCapacity %d <= 0 (FabricCapacity bypassed its editor ClampMin); clamping to 1."),
               InCapacity);
    }
    Capacity = FMath::Max(1, InCapacity);

    WriteBuffer.SetNum(Capacity);
    ReadBuffer.SetNum(Capacity);
    WriteSpatialIndex.Empty();
    ReadSpatialIndex.Empty();
    WriteHead = 0;
    WriteCount = 0;
    ReadHead = 0;
    ReadCount = 0;
    BaseEventId = 1;
    ReadBaseEventId = 1;
    ReadNewestEventId = 0;
    NextEventId.store(1, std::memory_order_relaxed);

    UE_LOG(LogMythCausalFabric, Log, TEXT("Causal Fabric initialized with capacity %d"), Capacity);
}

uint32 UMythicCausalFabric::AppendEvent(const FMythicWorldEvent &InEvent) {
    const uint32 AssignedId = NextEventId.fetch_add(1, std::memory_order_relaxed);

    FMythicWorldEvent &Slot = WriteBuffer[WriteHead];

    if (WriteCount == Capacity && Slot.EventId > 0) {
        if (TArray<uint32> *OldCellEvents = WriteSpatialIndex.Find(Slot.Cell)) {
            OldCellEvents->RemoveSwap(Slot.EventId);
            if (OldCellEvents->IsEmpty()) {
                WriteSpatialIndex.Remove(Slot.Cell);
            }
        }
    }

    Slot = InEvent;
    Slot.EventId = AssignedId;
    if (Slot.WorldTime == 0.0) {
        Slot.WorldTime = FPlatformTime::Seconds();
    }

    WriteSpatialIndex.FindOrAdd(Slot.Cell).Add(AssignedId);

    WriteHead = (WriteHead + 1) % Capacity;
    WriteCount = FMath::Min(WriteCount + 1, Capacity);

    if (WriteCount == Capacity) {
        BaseEventId = AssignedId - Capacity + 1;
    }

    return AssignedId;
}

void UMythicCausalFabric::CommitWrites() {
    FWriteScopeLock Lock(FabricLock);

    FMemory::Memcpy(ReadBuffer.GetData(), WriteBuffer.GetData(), Capacity * sizeof(FMythicWorldEvent));
    ReadSpatialIndex = WriteSpatialIndex;
    ReadHead = WriteHead;
    ReadCount = WriteCount;
    ReadBaseEventId = BaseEventId;
    ReadNewestEventId = NextEventId.load(std::memory_order_relaxed) - 1;
}

const FMythicWorldEvent *UMythicCausalFabric::GetEvent(uint32 EventId) const {
    FReadScopeLock Lock(FabricLock);

    const int32 Index = EventIdToIndex(EventId, ReadHead, ReadCount);
    if (Index < 0) {
        return nullptr;
    }
    return &ReadBuffer[Index];
}

TArray<FMythicWorldEvent> UMythicCausalFabric::GetRecentEvents(int32 MaxCount) const {
    FReadScopeLock Lock(FabricLock);

    TArray<FMythicWorldEvent> Out;
    const int32 Count = FMath::Min(MaxCount, ReadCount);
    if (Count <= 0) {
        return Out;
    }
    Out.Reserve(Count);

    const int32 StartIndex = ((ReadHead - Count) % Capacity + Capacity) % Capacity;
    const int32 FirstLen = FMath::Min(Count, Capacity - StartIndex);
    Out.Append(ReadBuffer.GetData() + StartIndex, FirstLen);
    if (FirstLen < Count) {
        Out.Append(ReadBuffer.GetData(), Count - FirstLen);
    }
    return Out;
}

void UMythicCausalFabric::QueryEventsByCell(
    const FMythicCellCoord &Cell,
    double MinWorldTime,
    double MaxWorldTime,
    int32 MaxResults,
    TArray<FMythicWorldEvent> &OutEvents) const {
    OutEvents.Reset();

    FReadScopeLock Lock(FabricLock);

    if (ReadCount <= 0) {
        return;
    }

    const TArray<uint32> *CellEvents = ReadSpatialIndex.Find(Cell);
    if (!CellEvents) {
        return;
    }

    for (int32 i = CellEvents->Num() - 1; i >= 0 && OutEvents.Num() < MaxResults; --i) {
        const uint32 EventId = (*CellEvents)[i];
        const int32 Index = EventIdToIndex(EventId, ReadHead, ReadCount);
        if (Index < 0) {
            continue;
        }
        const FMythicWorldEvent &Event = ReadBuffer[Index];
        if (Event.WorldTime >= MinWorldTime && Event.WorldTime <= MaxWorldTime) {
            OutEvents.Add(Event);
        }
    }
}

void UMythicCausalFabric::QueryEventsByCategory(
    uint16 CategoryMask,
    double MinWorldTime,
    double MaxWorldTime,
    int32 MaxResults,
    TArray<FMythicWorldEvent> &OutEvents) const {
    OutEvents.Reset();

    FReadScopeLock Lock(FabricLock);

    if (ReadCount <= 0) {
        return;
    }

    for (int32 i = 0; i < ReadCount && OutEvents.Num() < MaxResults; ++i) {
        const int32 Index = ((ReadHead - 1 - i) % Capacity + Capacity) % Capacity;
        const FMythicWorldEvent &Event = ReadBuffer[Index];
        if (Event.WorldTime >= MinWorldTime && Event.WorldTime <= MaxWorldTime && (Event.CategoryFlags & CategoryMask) != 0) {
            OutEvents.Add(Event);
        }
    }
}

int32 UMythicCausalFabric::EventIdToIndex(uint32 EventId, int32 HeadPos, int32 Count) const {
    if (EventId == 0 || Capacity <= 0) {
        return -1;
    }

    const uint32 OldestId = ReadBaseEventId;
    const uint32 NewestId = ReadNewestEventId;

    if (EventId < OldestId || EventId > NewestId) {
        return -1;
    }

    const int32 Age = static_cast<int32>(NewestId - EventId);
    if (Age >= Count) {
        return -1;
    }

    const int32 Index = ((HeadPos - 1 - Age) % Capacity + Capacity) % Capacity;
    return Index;
}

void UMythicCausalFabric::QueryEventsByFaction(
    const FMythicFactionId &Faction,
    TArray<FMythicWorldEvent> &OutEvents,
    int32 MaxResults) const {
    OutEvents.Reset();

    FReadScopeLock Lock(FabricLock);

    if (ReadCount <= 0 || !Faction.IsValid()) {
        return;
    }

    for (int32 i = 0; i < ReadCount && OutEvents.Num() < MaxResults; ++i) {
        const int32 Index = ((ReadHead - 1 - i) % Capacity + Capacity) % Capacity;
        const FMythicWorldEvent &Event = ReadBuffer[Index];

        if (Event.PrimaryFaction == Faction) {
            OutEvents.Add(Event);
        }
    }
}

void UMythicCausalFabric::Serialize(FArchive &Ar) {
    int32 Version = 1;
    Ar << Version;

    Ar << Capacity;

    if (Ar.IsLoading()) {
        if (Capacity < 0 || Capacity > 10000000) {
            Ar.SetError();
            return;
        }
        WriteBuffer.SetNum(Capacity);
        ReadBuffer.SetNum(Capacity);
    }

    Ar << WriteHead;
    Ar << WriteCount;
    Ar << BaseEventId;

    if (Ar.IsLoading()) {
        if (WriteCount < 0 || WriteCount > Capacity || WriteHead < 0 || WriteHead > Capacity) {
            Ar.SetError();
            return;
        }
    }

    uint32 NextId = NextEventId.load(std::memory_order_relaxed);
    Ar << NextId;
    if (Ar.IsLoading()) {
        NextEventId.store(NextId, std::memory_order_relaxed);
    }

    const int32 EventCount = FMath::Min(Capacity, WriteBuffer.Num());
    if (EventCount < Capacity) {
        UE_LOG(LogTemp, Verbose, TEXT("CausalFabric::Serialize: Capacity %d but buffer holds %d; serialising %d."),
               Capacity, WriteBuffer.Num(), EventCount);
    }
    for (int32 i = 0; i < EventCount; ++i) {
        FMythicWorldEvent &Event = WriteBuffer[i];
        Ar << Event.EventId;
        Ar << Event.ParentEventId;
        Ar << Event.WorldTime;
        Ar << Event.Cell.X;
        Ar << Event.Cell.Y;
        Ar << Event.PrimaryFaction.Index;
        Ar << Event.SecondaryFaction.Index;
        Ar << Event.EventTag;
        Ar << Event.PerpEntityId;
        Ar << Event.VictimEntityId;
        Ar << Event.Significance;
        Ar << Event.CategoryFlags;

        for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
            Ar << Event.MoralVector.AxisValues[Axis];
        }
    }

    if (Ar.IsLoading()) {
        WriteSpatialIndex.Empty();

        TArray<FMythicWorldEvent *> ValidEvents;
        for (int32 i = 0; i < EventCount; ++i) {
            if (WriteBuffer[i].EventId > 0) {
                ValidEvents.Add(&WriteBuffer[i]);
            }
        }

        ValidEvents.Sort([](const FMythicWorldEvent &A, const FMythicWorldEvent &B) {
            return A.EventId < B.EventId;
        });

        for (const FMythicWorldEvent *Ev : ValidEvents) {
            WriteSpatialIndex.FindOrAdd(Ev->Cell).Add(Ev->EventId);
        }

        CommitWrites();
    }
}
