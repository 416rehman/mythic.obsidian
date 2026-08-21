
#pragma once

#include "CoreMinimal.h"
#include "MythicDeathStakeTypes.generated.h"

USTRUCT(BlueprintType)
struct FMythicDeathStakeConfig {
    GENERATED_BODY()

    // Fraction of the player's CARRIED gold placed at stake on death (before danger scaling). 0 disables the stake
    // (a death then drops a pure 0-value marker). Clamped [0,1] by the rules.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeathStake", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StakeFractionOfGold = 0.25f;

    // Danger multiplier at RegionDanger01 == 1 (deadliest region). The stake's danger multiplier lerps 1 -> DangerScaleMax
    // across danger [0,1], so a death deep in hostile territory risks MORE of your gold than one in a safe zone. >= 1.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeathStake", meta = (ClampMin = "1.0"))
    float DangerScaleMax = 2.0f;

    // Absolute FLOOR on a non-zero stake: when the player has any gold, a death always risks at least this much (a
    // low-gold death still stings), capped so it never exceeds the gold actually carried.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeathStake", meta = (ClampMin = "0"))
    int32 MinStake = 10;

    // Absolute CAP on the stake regardless of gold/danger (bounds the worst-case loss on a rich player's death).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeathStake", meta = (ClampMin = "0"))
    int32 MaxStake = 100000;

    // Seconds the gravestone persists before it expires and the staked gold is PERMANENTLY lost (the existing flat
    // death-penalty floor made recoverable — recover in time to get it back, or it becomes the minimum loss). <= 0
    // disables expiry (the stone lingers until recovered).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DeathStake", meta = (ClampMin = "0.0"))
    float GravestoneLifetimeSeconds = 1200.0f; // 20 minutes
};

struct FMythicDeathStakeRules {
    static int32 ComputeStakeAmount(int32 CarriedGold, float RegionDanger01, const FMythicDeathStakeConfig &Config) {
        if (CarriedGold <= 0) {
            return 0;
        }
        const float Frac = FMath::Clamp(Config.StakeFractionOfGold, 0.0f, 1.0f);
        if (Frac <= 0.0f) {
            return 0;
        }
        const float Danger = FMath::Clamp(RegionDanger01, 0.0f, 1.0f);
        const float ScaleMax = FMath::Max(1.0f, Config.DangerScaleMax);
        const float DangerMult = 1.0f + Danger * (ScaleMax - 1.0f);

        const float Raw = static_cast<float>(CarriedGold) * Frac * DangerMult;
        int32 Amount = FMath::FloorToInt(Raw);

        const int32 UpperCap = FMath::Min(FMath::Max(0, Config.MaxStake), CarriedGold);
        const int32 MinS = FMath::Clamp(Config.MinStake, 0, UpperCap);
        return FMath::Clamp(Amount, MinS, UpperCap);
    }

    static bool CanRecover(bool bIsOwner, bool bIsPartyMember, bool bRecoverInRange) {
        if (!bRecoverInRange) {
            return false;
        }
        return bIsOwner || bIsPartyMember;
    }
};
