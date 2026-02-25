// Mythic Living World System — Causal Fabric Implementation

#include "World/LivingWorld/CausalFabric/CausalFabric.h"

void UMythicCausalFabric::Initialize(int32 InCapacity) {
    check(InCapacity > 0);
    Capacity = InCapacity;

    WriteBuffer.SetNum(Capacity);
    ReadBuffer.SetNum(Capacity);
    WriteSpatialIndex.Empty();
    ReadSpatialIndex.Empty();
    WriteHead = 0;
    WriteCount = 0;
    ReadHead = 0;
    ReadCount = 0;
    BaseEventId = 1;
    NextEventId.store(1, std::memory_order_relaxed);

    UE_LOG(LogMythCausalFabric, Log, TEXT("Causal Fabric initialized with capacity %d"), Capacity);
}

uint32 UMythicCausalFabric::AppendEvent(const FMythicWorldEvent &InEvent) {
    const uint32 AssignedId = NextEventId.fetch_add(1, std::memory_order_relaxed);

    FMythicWorldEvent &Slot = WriteBuffer[WriteHead];
    
    // O(1) In-place Pruning:
    // If the ring is wrapping around, we are about to overwrite the oldest event.
    // We strictly prune it from the spatial index exactly now.
    // This is vastly more performant than a periodic "background cleanup" because:
    // 1. It spreads the cost across individual appends (no latency spikes).
    // 2. It requires exactly one Map Lookup and one Array RemoveSwap (O(1)).
    // 3. A periodic cleanup would require iterating the entire Map and all Arrays (O(N) spike).
    if (WriteCount == Capacity && Slot.EventId > 0) {
        if (TArray<uint32>* OldCellEvents = WriteSpatialIndex.Find(Slot.Cell)) {
            OldCellEvents->RemoveSwap(Slot.EventId);
            if (OldCellEvents->IsEmpty()) {
                WriteSpatialIndex.Remove(Slot.Cell);
            }
        }
    }

    // Overwrite the slot with the new event
    Slot = InEvent;
    Slot.EventId = AssignedId;

    // Add to spatial index
    WriteSpatialIndex.FindOrAdd(Slot.Cell).Add(AssignedId);

    WriteHead = (WriteHead + 1) % Capacity;
    WriteCount = FMath::Min(WriteCount + 1, Capacity);

    // Advance BaseEventId so EventId→index translation stays correct.
    if (WriteCount == Capacity) {
        BaseEventId = AssignedId - Capacity + 1;
    }

    return AssignedId;
}

void UMythicCausalFabric::CommitWrites() {
    // Snapshot the write buffer into the read buffer.
    // Use an FRWLock to guarantee game thread readers don't see half-copied state.
    FWriteScopeLock Lock(FabricLock);
    
    FMemory::Memcpy(ReadBuffer.GetData(), WriteBuffer.GetData(), Capacity * sizeof(FMythicWorldEvent));
    ReadSpatialIndex = WriteSpatialIndex;
    ReadHead = WriteHead;
    ReadCount = WriteCount;
}

const FMythicWorldEvent *UMythicCausalFabric::GetEvent(uint32 EventId) const {
    FReadScopeLock Lock(FabricLock);
    
    const int32 Index = EventIdToIndex(EventId, ReadHead, ReadCount);
    if (Index < 0) {
        return nullptr;
    }
    return &ReadBuffer[Index];
}

TArrayView<const FMythicWorldEvent> UMythicCausalFabric::GetRecentEvents(int32 MaxCount) const {
    FReadScopeLock Lock(FabricLock);
    
    const int32 Count = FMath::Min(MaxCount, ReadCount);
    if (Count <= 0) {
        return TArrayView<const FMythicWorldEvent>();
    }

    const int32 StartIndex = ((ReadHead - Count) % Capacity + Capacity) % Capacity;

    if (StartIndex + Count <= Capacity) {
        return TArrayView<const FMythicWorldEvent>(ReadBuffer.GetData() + StartIndex, Count);
    }

    return TArrayView<const FMythicWorldEvent>(ReadBuffer.GetData() + StartIndex, Capacity - StartIndex);
}

void UMythicCausalFabric::QueryEventsByCell(
    const FMythicCellCoord &Cell,
    double MinWorldTime,
    double MaxWorldTime,
    int32 MaxResults,
    TArray<const FMythicWorldEvent *> &OutEvents) const {
    OutEvents.Reset();

    FReadScopeLock Lock(FabricLock);

    if (ReadCount <= 0) {
        return;
    }

    // O(1) Fast path via spatial index
    const TArray<uint32>* CellEvents = ReadSpatialIndex.Find(Cell);
    if (!CellEvents) {
        return;
    }

    // Events are appended chronologically, so iterate backwards to get newest first
    for (int32 i = CellEvents->Num() - 1; i >= 0 && OutEvents.Num() < MaxResults; --i) {
        const uint32 EventId = (*CellEvents)[i];
        const int32 Index = EventIdToIndex(EventId, ReadHead, ReadCount);
        
        if (Index < 0) {
            continue; // Event was overwritten in the ring buffer
        }

        const FMythicWorldEvent& Event = ReadBuffer[Index];

        // Break early if we hit an event older than our requested bounds
        if (Event.WorldTime < MinWorldTime) {
            break;
        }

        if (Event.WorldTime <= MaxWorldTime) {
            OutEvents.Add(&Event);
        }
    }
}

void UMythicCausalFabric::QueryEventsByCategory(
    uint16 CategoryMask,
    double MinWorldTime,
    double MaxWorldTime,
    int32 MaxResults,
    TArray<const FMythicWorldEvent *> &OutEvents) const {
    OutEvents.Reset();

    FReadScopeLock Lock(FabricLock);

    if (ReadCount <= 0) {
        return;
    }

    for (int32 i = 0; i < ReadCount && OutEvents.Num() < MaxResults; ++i) {
        const int32 Index = ((ReadHead - 1 - i) % Capacity + Capacity) % Capacity;
        const FMythicWorldEvent &Event = ReadBuffer[Index];

        if (Event.WorldTime < MinWorldTime) {
            break;
        }

        if (Event.WorldTime <= MaxWorldTime && (Event.CategoryFlags & CategoryMask) != 0) {
            OutEvents.Add(&Event);
        }
    }
}

int32 UMythicCausalFabric::EventIdToIndex(uint32 EventId, int32 HeadPos, int32 Count) const {
    if (EventId == 0 || Capacity <= 0) {
        return -1;
    }

    // Check if this event is still within the ring buffer's live range
    const uint32 OldestId = BaseEventId;
    const uint32 NewestId = NextEventId.load(std::memory_order_relaxed) - 1;

    if (EventId < OldestId || EventId > NewestId) {
        return -1;
    }

    // Ring index = (HeadPos - (NewestId - EventId) - 1) wrapped to capacity
    const int32 Age = static_cast<int32>(NewestId - EventId);
    if (Age >= Count) {
        return -1;
    }

    const int32 Index = ((HeadPos - 1 - Age) % Capacity + Capacity) % Capacity;
    return Index;
}

void UMythicCausalFabric::QueryEventsByFaction(
    const FMythicFactionId& Faction,
    TArray<FMythicWorldEvent>& OutEvents,
    int32 MaxResults) const {
    OutEvents.Reset();

    FReadScopeLock Lock(FabricLock);

    if (ReadCount <= 0 || !Faction.IsValid()) {
        return;
    }

    // Scan from newest to oldest, filtering by PrimaryFaction
    for (int32 i = 0; i < ReadCount && OutEvents.Num() < MaxResults; ++i) {
        const int32 Index = ((ReadHead - 1 - i) % Capacity + Capacity) % Capacity;
        const FMythicWorldEvent& Event = ReadBuffer[Index];

        if (Event.PrimaryFaction == Faction) {
            OutEvents.Add(Event);
        }
    }
}

void UMythicCausalFabric::Serialize(FArchive& Ar) {
    // Version for forward compatibility
    int32 Version = 1;
    Ar << Version;

    Ar << Capacity;

    if (Ar.IsLoading()) {
        WriteBuffer.SetNum(Capacity);
        ReadBuffer.SetNum(Capacity);
    }

    Ar << WriteHead;
    Ar << WriteCount;
    Ar << BaseEventId;

    uint32 NextId = NextEventId.load(std::memory_order_relaxed);
    Ar << NextId;
    if (Ar.IsLoading()) {
        NextEventId.store(NextId, std::memory_order_relaxed);
    }

    // Serialize all valid events in the write buffer
    for (int32 i = 0; i < Capacity; ++i) {
        FMythicWorldEvent& Event = WriteBuffer[i];
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

        // Serialize moral vector (array of floats)
        for (int32 Axis = 0; Axis < MoralAxisCount; ++Axis) {
            Ar << Event.MoralVector.AxisValues[Axis];
        }
    }

    if (Ar.IsLoading()) {
        WriteSpatialIndex.Empty();
        
        // Rebuild spatial index with correct chronological ordering
        TArray<FMythicWorldEvent*> ValidEvents;
        for (int32 i = 0; i < Capacity; ++i) {
            if (WriteBuffer[i].EventId > 0) {
                ValidEvents.Add(&WriteBuffer[i]);
            }
        }
        
        ValidEvents.Sort([](const FMythicWorldEvent& A, const FMythicWorldEvent& B) {
            return A.EventId < B.EventId;
        });
        
        for (const FMythicWorldEvent* Ev : ValidEvents) {
            WriteSpatialIndex.FindOrAdd(Ev->Cell).Add(Ev->EventId);
        }

        // Restore read buffer from loaded write buffer
        CommitWrites();
    }
}

