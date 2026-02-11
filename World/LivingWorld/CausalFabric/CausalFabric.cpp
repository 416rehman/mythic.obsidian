// Mythic Living World System — Causal Fabric Implementation

#include "World/LivingWorld/CausalFabric/CausalFabric.h"

void UMythicCausalFabric::Initialize(int32 InCapacity) {
    check(InCapacity > 0);
    Capacity = InCapacity;

    WriteBuffer.SetNum(Capacity);
    ReadBuffer.SetNum(Capacity);
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
    Slot = InEvent;
    Slot.EventId = AssignedId;

    WriteHead = (WriteHead + 1) % Capacity;
    WriteCount = FMath::Min(WriteCount + 1, Capacity);

    // When the ring wraps, oldest events are overwritten.
    // Advance BaseEventId so EventId→index translation stays correct.
    if (WriteCount == Capacity) {
        BaseEventId = AssignedId - Capacity + 1;
    }

    return AssignedId;
}

void UMythicCausalFabric::CommitWrites() {
    // Snapshot the write buffer into the read buffer.
    // Single writer, so no lock needed — just a memcpy.
    FMemory::Memcpy(ReadBuffer.GetData(), WriteBuffer.GetData(), Capacity * sizeof(FMythicWorldEvent));
    ReadHead = WriteHead;
    ReadCount = WriteCount;
}

const FMythicWorldEvent *UMythicCausalFabric::GetEvent(uint32 EventId) const {
    const int32 Index = EventIdToIndex(EventId, ReadHead, ReadCount);
    if (Index < 0) {
        return nullptr;
    }
    return &ReadBuffer[Index];
}

TArrayView<const FMythicWorldEvent> UMythicCausalFabric::GetRecentEvents(int32 MaxCount) const {
    const int32 Count = FMath::Min(MaxCount, ReadCount);
    if (Count <= 0) {
        return TArrayView<const FMythicWorldEvent>();
    }

    // Return a view over the most recent contiguous events.
    // If the ring hasn't wrapped, this is straightforward.
    // If it has wrapped, we return the most recent contiguous segment (from ReadHead backwards).
    const int32 StartIndex = ((ReadHead - Count) % Capacity + Capacity) % Capacity;

    // Only return contiguous portion — if the range wraps, return from StartIndex to end of buffer
    if (StartIndex + Count <= Capacity) {
        return TArrayView<const FMythicWorldEvent>(ReadBuffer.GetData() + StartIndex, Count);
    }

    // Wrapped case: return the segment from StartIndex to end of buffer
    return TArrayView<const FMythicWorldEvent>(ReadBuffer.GetData() + StartIndex, Capacity - StartIndex);
}

void UMythicCausalFabric::QueryEventsByCell(
    const FMythicCellCoord &Cell,
    double MinWorldTime,
    double MaxWorldTime,
    int32 MaxResults,
    TArray<const FMythicWorldEvent *> &OutEvents) const {
    OutEvents.Reset();
    if (ReadCount <= 0) {
        return;
    }

    // Iterate from most recent to oldest — early exit on budget cap
    for (int32 i = 0; i < ReadCount && OutEvents.Num() < MaxResults; ++i) {
        const int32 Index = ((ReadHead - 1 - i) % Capacity + Capacity) % Capacity;
        const FMythicWorldEvent &Event = ReadBuffer[Index];

        // Early exit: events are newest-first, so once we pass MinWorldTime, we're done
        if (Event.WorldTime < MinWorldTime) {
            break;
        }

        if (Event.WorldTime <= MaxWorldTime && Event.Cell == Cell) {
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
