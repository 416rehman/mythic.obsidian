#pragma once

#include "CoreMinimal.h"
#include "MythicMountTypes.generated.h"

USTRUCT(BlueprintType)
struct FMythicMountRecord {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Mount")
    FGuid MountId;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Mount")
    FName CustomName;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Mount")
    uint8 SpeciesId = 0;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Mount")
    int32 BondXP = 0;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Mount")
    bool bActive = false;

    /**
     * WAVE O (O6) — SADDLEBAG SPEC, handed to the mounts lane (B5 coordination note). The extra carry WEIGHT capacity
     * this mount contributes while summoned: the encumbrance fold adds it to the rider's Soft/Hard capacities
     * (MythicEncumbrance::ComputeTier inputs), so a saddlebagged mount is how heavy cargo runs beat the Overloaded
     * fast-travel/speed gates — hauling capacity is a MOUNT dividend, not a stat stick. 0 (default) = no saddlebags
     * (byte-identical). CONTENT/OWNER: set per species/tier at taming or via a saddlebag gear verb; the encumbrance
     * call-site fold is the mounts lane's insertion, spec'd here so the record already persists the field (SaveGame,
     * tagged-property — pre-existing saves load it at 0 with zero migration).
     */
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Mount")
    float SaddlebagCapacity = 0.0f;
};

UENUM(BlueprintType)
enum class EMountGateResult : uint8 {
    Ok,
    Dead,
    AlreadyMounted,
    InCombat,
    NotOwner,
    OutOfRange
};

namespace MythicMountStatics {
    inline constexpr int32 MaxBondLevel = 10;

    inline constexpr float GallopBondBonusPerLevel = 0.05f;

    inline EMountGateResult CanMount(bool bMountAlive, bool bRiderAlreadyMounted, bool bInCombat, bool bOwnershipMatch,
                                     float DistSq, float RangeSq) {
        if (!bMountAlive) {
            return EMountGateResult::Dead;
        }
        if (!bOwnershipMatch) {
            return EMountGateResult::NotOwner;
        }
        if (bRiderAlreadyMounted) {
            return EMountGateResult::AlreadyMounted;
        }
        if (bInCombat) {
            return EMountGateResult::InCombat;
        }
        if (RangeSq > 0.0f && DistSq > RangeSq) {
            return EMountGateResult::OutOfRange;
        }
        return EMountGateResult::Ok;
    }

    inline bool CanSummon(bool bHasActiveMount, bool bInCombat, double TimeSinceLast, double Cooldown) {
        if (!bHasActiveMount || bInCombat) {
            return false;
        }
        return Cooldown <= 0.0 || TimeSinceLast >= Cooldown;
    }

    inline int32 BondXPForLevel(int32 Level) {
        const int32 L = FMath::Clamp(Level, 0, MaxBondLevel);
        return 100 * L * L;
    }

    inline int32 BondLevelFromXP(int32 XP) {
        int32 Level = 0;
        while (Level < MaxBondLevel && XP >= BondXPForLevel(Level + 1)) {
            ++Level;
        }
        return Level;
    }

    inline float ComputeGallopSpeed(float Base, int32 BondLevel, float Stamina01) {
        if (Base <= 0.0f) {
            return 0.0f;
        }
        if (Stamina01 <= KINDA_SMALL_NUMBER) {
            return Base;
        }
        const int32 L = FMath::Clamp(BondLevel, 0, MaxBondLevel);
        return Base * (1.0f + GallopBondBonusPerLevel * static_cast<float>(L));
    }

    inline float DrainStamina(float Current, float DeltaSeconds, float RatePerSecond, float Max) {
        const float Delta = FMath::Max(0.0f, DeltaSeconds) * FMath::Max(0.0f, RatePerSecond);
        return FMath::Clamp(Current - Delta, 0.0f, FMath::Max(0.0f, Max));
    }

    inline float RegenStamina(float Current, float DeltaSeconds, float RatePerSecond, float Max) {
        const float Delta = FMath::Max(0.0f, DeltaSeconds) * FMath::Max(0.0f, RatePerSecond);
        return FMath::Clamp(Current + Delta, 0.0f, FMath::Max(0.0f, Max));
    }
}
