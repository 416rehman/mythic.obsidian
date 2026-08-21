
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "GAS/Progression/MythicRenownRules.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_REPUTATION_BAND_FEARED)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_REPUTATION_BAND_RENOWNED)

struct FMythicPartyReputation {
    int32 MaxTier = -1;
    float AvgTier = -1.0f;
    float VendettaHeat = 0.0f;
    int32 NumPlayers = 0;

    bool IsValid() const { return NumPlayers > 0 && MaxTier >= 0; }
};

struct FMythicReputationBandConfig {
    int32 RenownedMinTier = static_cast<int32>(EMythicRenownTier::Honored);
    int32 FearedMaxTier   = static_cast<int32>(EMythicRenownTier::Hostile);
    int32 TopTier         = static_cast<int32>(EMythicRenownTier::Exalted);
    float FearedHeatThreshold = 20.0f;
    float HeatFullScale       = 100.0f;
};

struct FMythicReputationEncounterMath {
    static bool BandMatches(const FGameplayTag &RequiredBand, const FMythicPartyReputation &Rep,
                            const FMythicReputationBandConfig &Cfg = FMythicReputationBandConfig{}) {
        if (!Rep.IsValid()) {
            return false;
        }
        if (RequiredBand == TAG_REPUTATION_BAND_RENOWNED.GetTag()) {
            return Rep.MaxTier >= Cfg.RenownedMinTier;
        }
        if (RequiredBand == TAG_REPUTATION_BAND_FEARED.GetTag()) {
            return Rep.MaxTier <= Cfg.FearedMaxTier || Rep.VendettaHeat >= Cfg.FearedHeatThreshold;
        }
        return false;
    }

    static float BandIntensity(const FGameplayTag &RequiredBand, const FMythicPartyReputation &Rep,
                               const FMythicReputationBandConfig &Cfg = FMythicReputationBandConfig{}) {
        if (!Rep.IsValid()) {
            return 0.0f;
        }
        if (RequiredBand == TAG_REPUTATION_BAND_RENOWNED.GetTag()) {
            const int32 Span = FMath::Max(1, Cfg.TopTier - Cfg.RenownedMinTier + 1);
            return FMath::Clamp(static_cast<float>(Rep.MaxTier - Cfg.RenownedMinTier + 1) / static_cast<float>(Span),
                                0.0f, 1.0f);
        }
        if (RequiredBand == TAG_REPUTATION_BAND_FEARED.GetTag()) {
            const int32 Span = FMath::Max(1, Cfg.FearedMaxTier + 1);
            const float TierPart = Rep.MaxTier <= Cfg.FearedMaxTier
                                       ? static_cast<float>(Cfg.FearedMaxTier - Rep.MaxTier + 1) / static_cast<float>(Span)
                                       : 0.0f;
            const float HeatPart = Cfg.HeatFullScale > 0.0f
                                       ? FMath::Clamp(Rep.VendettaHeat / Cfg.HeatFullScale, 0.0f, 1.0f)
                                       : 0.0f;
            return FMath::Clamp(TierPart + HeatPart, 0.0f, 1.0f);
        }
        return 0.0f;
    }

    static float ScaleWeight(float BaseWeight, const FGameplayTag &RequiredBand, const FMythicPartyReputation &Rep,
                             float ReputationWeightScale,
                             const FMythicReputationBandConfig &Cfg = FMythicReputationBandConfig{}) {
        if (!RequiredBand.IsValid()) {
            return BaseWeight;
        }
        if (!BandMatches(RequiredBand, Rep, Cfg)) {
            return 0.0f;
        }
        const float Scale = FMath::Max(0.0f, ReputationWeightScale);
        return BaseWeight * (1.0f + Scale * BandIntensity(RequiredBand, Rep, Cfg));
    }

    static int32 ScalePackSize(int32 BaseCount, const FGameplayTag &RequiredBand, const FMythicPartyReputation &Rep,
                               float ReputationWeightScale, int32 MaxCount = 20,
                               const FMythicReputationBandConfig &Cfg = FMythicReputationBandConfig{}) {
        const int32 ClampHi = FMath::Max(1, MaxCount);
        if (!RequiredBand.IsValid() || !BandMatches(RequiredBand, Rep, Cfg)) {
            return FMath::Clamp(BaseCount, 1, ClampHi);
        }
        const float Scale = FMath::Max(0.0f, ReputationWeightScale);
        const float Scaled = static_cast<float>(BaseCount) * (1.0f + Scale * BandIntensity(RequiredBand, Rep, Cfg));
        return FMath::Clamp(FMath::RoundToInt(Scaled), 1, ClampHi);
    }
};
