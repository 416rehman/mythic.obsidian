
#pragma once

#include "CoreMinimal.h"
#include "MythicRenownRules.generated.h"

UENUM(BlueprintType)
enum class EMythicRenownTier : uint8 {
    Hated,
    Hostile,
    Unfriendly,
    Neutral,
    Friendly,
    Honored,
    Revered,
    Exalted,
};

struct FMythicRenownRules {
    static constexpr int32 NumTiers = 8;

    static const TCHAR *TierName(EMythicRenownTier Tier) {
        switch (Tier) {
        case EMythicRenownTier::Hated: return TEXT("Hated");
        case EMythicRenownTier::Hostile: return TEXT("Hostile");
        case EMythicRenownTier::Unfriendly: return TEXT("Unfriendly");
        case EMythicRenownTier::Neutral: return TEXT("Neutral");
        case EMythicRenownTier::Friendly: return TEXT("Friendly");
        case EMythicRenownTier::Honored: return TEXT("Honored");
        case EMythicRenownTier::Revered: return TEXT("Revered");
        case EMythicRenownTier::Exalted: return TEXT("Exalted");
        default: return TEXT("Unknown");
        }
    }

    static EMythicRenownTier TierForValue(float Value, TConstArrayView<float> Thresholds) {
        int32 Count = 0;
        for (const float Boundary : Thresholds) {
            if (Value >= Boundary) {
                ++Count;
            }
        }
        return static_cast<EMythicRenownTier>(FMath::Clamp(Count, 0, NumTiers - 1));
    }

    static EMythicRenownTier ClampToMaxTier(float Value, EMythicRenownTier Cap, TConstArrayView<float> Thresholds) {
        const EMythicRenownTier Base = TierForValue(Value, Thresholds);
        return Base <= Cap ? Base : Cap;
    }

    static float VendorDiscountForTier(EMythicRenownTier Tier, TConstArrayView<float> Discounts) {
        const int32 Index = static_cast<int32>(Tier);
        return Discounts.IsValidIndex(Index) ? Discounts[Index] : 0.0f;
    }
};
