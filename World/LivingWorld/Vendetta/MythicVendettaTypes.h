
#pragma once

#include "CoreMinimal.h"
#include "MythicVendettaTypes.generated.h"


UENUM(BlueprintType)
enum class EMythicVendettaType : uint8 {
    None             = 0,
    BountyPosting    = 1,
    AssassinDispatch = 2,
    RetaliationRaid  = 3
};


USTRUCT(BlueprintType)
struct FMythicVendettaThresholds {
    GENERATED_BODY()

    // Threat at/above which a faction posts a BOUNTY (≈ two faction-member kills).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendetta", meta = (ClampMin = "0.0"))
    float BountyAt = 50.0f;

    // Threat at/above which a faction DISPATCHES AN ASSASSIN.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendetta", meta = (ClampMin = "0.0"))
    float AssassinAt = 120.0f;

    // Threat at/above which a faction mounts a RETALIATION RAID (also gated by MilitaryGateForRaid).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendetta", meta = (ClampMin = "0.0"))
    float RaidAt = 250.0f;

    // Minimum seconds between successive vendettas for the SAME (faction,player) — a grudge doesn't scheme every tick.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendetta", meta = (ClampMin = "0.0"))
    float CooldownSeconds = 120.0f;

    // A faction must be at least this militarily strong ([0,1], FMythicFactionData::MilitaryStrength) to RAID; weaker
    // factions fall back to an assassin even past RaidAt.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vendetta", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MilitaryGateForRaid = 0.5f;
};


namespace MythicVendetta {
inline constexpr float DefaultThreatSoftCap = 500.0f;

inline float AccumulatePlayerThreat(float Prior, float DeedMoralSeverity, float DeedWeight,
                                    float SoftCap = DefaultThreatSoftCap) {
    const float Contribution = FMath::Max(0.0f, DeedMoralSeverity) * FMath::Max(0.0f, DeedWeight);
    if (SoftCap <= 0.0f) {
        return FMath::Max(0.0f, Prior) + Contribution;
    }
    const float P = FMath::Clamp(Prior, 0.0f, SoftCap);
    const float Headroom = SoftCap - P;
    const float Scaled = Contribution * (Headroom / SoftCap);
    return FMath::Min(SoftCap, P + Scaled);
}

inline float DecayThreat(float Prior, float RatePerSec, float Dt) {
    const float Decay = FMath::Max(0.0f, RatePerSec) * FMath::Max(0.0f, Dt);
    return FMath::Max(0.0f, FMath::Max(0.0f, Prior) - Decay);
}

inline EMythicVendettaType SelectVendetta(float ThreatScore, float FactionMilitaryStrength,
                                          float SecondsSinceLastVendetta, const FMythicVendettaThresholds &T) {
    if (SecondsSinceLastVendetta < T.CooldownSeconds) {
        return EMythicVendettaType::None;
    }
    if (ThreatScore < T.BountyAt) {
        return EMythicVendettaType::None;
    }
    if (ThreatScore >= T.RaidAt && FactionMilitaryStrength >= T.MilitaryGateForRaid) {
        return EMythicVendettaType::RetaliationRaid;
    }
    if (ThreatScore >= T.AssassinAt) {
        return EMythicVendettaType::AssassinDispatch;
    }
    return EMythicVendettaType::BountyPosting;
}
}
