#pragma once

#include "CoreMinimal.h"

struct FMythicChargeState {
    static int32 ComputeChargesAfterElapsed(int32 Cur, int32 Max, float Elapsed, float RechargeSeconds) {
        if (Max < 1) {
            Max = 1;
        }
        if (Cur >= Max) {
            return Max;
        }
        if (RechargeSeconds <= 0.0f) {
            return Max;
        }
        if (Elapsed <= 0.0f) {
            return Cur < 0 ? 0 : Cur;
        }
        const int32 Gained = FMath::FloorToInt(Elapsed / RechargeSeconds);
        const int32 Base = Cur < 0 ? 0 : Cur;
        return FMath::Min(Max, Base + Gained);
    }
};
