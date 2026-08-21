
#pragma once

#include "CoreMinimal.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace MythicCampfireFuel {
constexpr uint8 FuelPayloadVersion = 1;

inline double RemainingBurnSeconds(double DeadlineServerTime, double Now) {
    return FMath::Max(0.0, DeadlineServerTime - Now);
}

inline bool IsBurning(double DeadlineServerTime, double Now) {
    return RemainingBurnSeconds(DeadlineServerTime, Now) > 0.0;
}

inline double AddFuelSeconds(double CurrentDeadline, double Now, double SecondsToAdd, double MaxRemainingSeconds) {
    const double Anchor = FMath::Max(CurrentDeadline, Now);
    double NewDeadline = Anchor + FMath::Max(0.0, SecondsToAdd);
    if (MaxRemainingSeconds > 0.0) {
        NewDeadline = FMath::Min(NewDeadline, Now + MaxRemainingSeconds);
    }
    return NewDeadline;
}

inline void SerializeFuel(TArray<uint8> &OutData, double RemainingSeconds) {
    FMemoryWriter Writer(OutData);
    uint8 Version = FuelPayloadVersion;
    double Remaining = FMath::Max(0.0, RemainingSeconds);
    Writer << Version;
    Writer << Remaining;
}

inline double DeserializeFuel(const TArray<uint8> &InData, double DefaultRemainingSeconds) {
    if (InData.Num() == 0) {
        return FMath::Max(0.0, DefaultRemainingSeconds);
    }
    FMemoryReader Reader(InData);
    uint8 Version = 0;
    Reader << Version;
    if (Version != FuelPayloadVersion || Reader.TotalSize() - Reader.Tell() < static_cast<int64>(sizeof(double))) {
        return FMath::Max(0.0, DefaultRemainingSeconds);
    }
    double Remaining = 0.0;
    Reader << Remaining;
    return FMath::Max(0.0, Remaining);
}
}
