
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FMythicProductionAccrual {
    int32 StoredUnits = 0;
    float CarryoverSeconds = 0.0f;
};

struct FMythicApiaryRules {
    static FMythicProductionAccrual AccrueUnits(float CarryoverSeconds, float WindowSeconds, float ProductionMultiplier,
                                                float SecondsPerUnit, int32 StoredUnits, int32 MaxUnits) {
        FMythicProductionAccrual Out;
        const int32 Cap = FMath::Max(0, MaxUnits);
        Out.StoredUnits = FMath::Clamp(StoredUnits, 0, Cap);
        Out.CarryoverSeconds = FMath::Max(0.0f, CarryoverSeconds);
        if (SecondsPerUnit <= 0.0f) {
            return Out;
        }
        const float Productive = FMath::Max(0.0f, WindowSeconds) * FMath::Clamp(ProductionMultiplier, 0.0f, 1.0f);
        float Banked = Out.CarryoverSeconds + Productive;
        const int32 NewUnits = FMath::FloorToInt(Banked / SecondsPerUnit);
        Out.StoredUnits = FMath::Clamp(Out.StoredUnits + NewUnits, 0, Cap);
        Banked -= static_cast<float>(NewUnits) * SecondsPerUnit;
        Out.CarryoverSeconds = (Out.StoredUnits >= Cap) ? 0.0f : FMath::Max(0.0f, Banked);
        return Out;
    }

    static float SecondsToNextUnit(float CarryoverSeconds, float SecondsPerUnit, int32 StoredUnits, int32 MaxUnits) {
        if (SecondsPerUnit <= 0.0f || StoredUnits >= FMath::Max(0, MaxUnits)) {
            return 0.0f;
        }
        return FMath::Max(0.0f, SecondsPerUnit - FMath::Max(0.0f, CarryoverSeconds));
    }

    static int32 CountDistinctCropTypes(TConstArrayView<FGameplayTag> CropTypeTags) {
        TArray<FGameplayTag, TInlineAllocator<16>> Seen;
        for (const FGameplayTag &Tag : CropTypeTags) {
            if (Tag.IsValid() && !Seen.Contains(Tag)) {
                Seen.Add(Tag);
            }
        }
        return Seen.Num();
    }

    static int32 ResolveHoneyVariety(int32 DistinctCropTypes, TConstArrayView<int32> RowMinDistinct) {
        int32 BestIndex = -1;
        int32 BestMin = -1;
        for (int32 i = 0; i < RowMinDistinct.Num(); ++i) {
            const int32 Min = FMath::Max(0, RowMinDistinct[i]);
            if (DistinctCropTypes >= Min && Min > BestMin) {
                BestMin = Min;
                BestIndex = i;
            }
        }
        return BestIndex;
    }
};
