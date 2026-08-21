#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"

struct FMythicPendingResourceDelta {
    FMythicFactionId FactionId;
    EMythicResourceType Axis = EMythicResourceType::Food;
    float Delta = 0.0f;
};

namespace MythicPlayerEconomyDelta {
    inline uint32 MakeKey(const FMythicFactionId &FactionId, EMythicResourceType Axis) {
        return (static_cast<uint32>(FactionId.Index) << 8) | static_cast<uint32>(Axis);
    }

    inline float ClampAppliedDelta(float TotalDelta, float MaxAbsPerKeyPerTick) {
        if (MaxAbsPerKeyPerTick <= 0.0f) {
            return 0.0f;
        }
        return FMath::Clamp(TotalDelta, -MaxAbsPerKeyPerTick, MaxAbsPerKeyPerTick);
    }

    inline TArray<FMythicPendingResourceDelta> DrainClamped(
        const TArray<FMythicPendingResourceDelta> &Pending, float MaxAbsPerKeyPerTick,
        TFunctionRef<void(const FMythicFactionId &, EMythicResourceType, float)> ApplyDelta) {
        TMap<uint32, FMythicPendingResourceDelta> Totals;
        for (const FMythicPendingResourceDelta &Row : Pending) {
            if (!Row.FactionId.IsValid() || Row.Delta == 0.0f) {
                continue;
            }
            FMythicPendingResourceDelta &Acc = Totals.FindOrAdd(MakeKey(Row.FactionId, Row.Axis));
            Acc.FactionId = Row.FactionId;
            Acc.Axis = Row.Axis;
            Acc.Delta += Row.Delta;
        }

        TArray<FMythicPendingResourceDelta> Remainder;
        for (const TPair<uint32, FMythicPendingResourceDelta> &Pair : Totals) {
            const FMythicPendingResourceDelta &Total = Pair.Value;
            const float Applied = ClampAppliedDelta(Total.Delta, MaxAbsPerKeyPerTick);
            if (Applied != 0.0f) {
                ApplyDelta(Total.FactionId, Total.Axis, Applied);
            }
            const float Left = Total.Delta - Applied;
            if (!FMath::IsNearlyZero(Left)) {
                FMythicPendingResourceDelta Carry;
                Carry.FactionId = Total.FactionId;
                Carry.Axis = Total.Axis;
                Carry.Delta = Left;
                Remainder.Add(Carry);
            }
        }
        return Remainder;
    }
}
