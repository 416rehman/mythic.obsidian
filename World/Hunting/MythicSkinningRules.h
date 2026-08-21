#pragma once

#include "CoreMinimal.h"
#include "World/Death/MythicCorpseTypes.h"
#include "World/Gathering/MythicYieldQuality.h"
#include "MythicSkinningRules.generated.h"

USTRUCT(BlueprintType)
struct FMythicKillContext {
    GENERATED_BODY()

    /** The killing blow was a critical hit (the clean-kill marker — a precise shot preserves the pelt). */
    UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Hunting")
    bool bCriticalKill = false;

    /** The lethal context carried BURN (a scorched carcass — botch input). */
    UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Hunting")
    bool bBurnKill = false;

    /** The lethal context carried BLEED (a torn-up hide — mild botch input). */
    UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Hunting")
    bool bBleedKill = false;

    /** The lethal context carried POISON (tainted meat — botch input). */
    UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Hunting")
    bool bPoisonKill = false;

    /** Excess lethal damage as a fraction of the victim's max health ((KillingBlow - RemainingHealth) / MaxHealth).
     *  0 = an exact kill; >= the config botch fraction = the carcass was obliterated. */
    UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Hunting")
    float OverkillFraction = 0.0f;

    /** Damage EVENTS the victim survived before dying (the health-damage-event counter — a cheap proxy for
     *  "bullet-riddled"). 0 = unknown/one-shot. */
    UPROPERTY(BlueprintReadWrite, SaveGame, Category = "Hunting")
    int32 HitsTaken = 0;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicPeltQualityConfig {
    GENERATED_BODY()

    /** Hits-taken at/above which the carcass counts as "riddled" (one botch point). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pelt Quality", meta = (ClampMin = "1"))
    int32 MessyHitsThreshold = 8;

    /** Overkill fraction at/above which the kill counts as "obliterated" (one botch point). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pelt Quality", meta = (ClampMin = "0.0"))
    float OverkillBotchFraction = 0.5f;

    /** Botch points at/above which the kill is BOTCHED → the pelt is Ragged flat (the hunting-only floor). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pelt Quality", meta = (ClampMin = "1"))
    int32 BotchScoreForRagged = 2;

    /** A CLEAN kill (base tier Fine) requires a critical killing blow AND at most this many hits taken. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pelt Quality", meta = (ClampMin = "0"))
    int32 CleanKillMaxHits = 3;
};

struct FMythicSkinningRules {
    static float ComputeSkinXpReward(float BaseXp, int32 SkinnerLevel, int32 NoGainAtOrAboveLevel) {
        if (BaseXp <= 0.0f) {
            return 0.0f;
        }
        if (NoGainAtOrAboveLevel > 0 && SkinnerLevel >= NoGainAtOrAboveLevel) {
            return 0.0f;
        }
        return BaseXp;
    }

    static int32 ComputeYieldCount(int32 BaseCount, int32 ProficiencyLevel, int32 Tier,
                                   float PerLevel = 0.1f, float PerTier = 1.0f) {
        const int32 Base = FMath::Max(0, BaseCount);
        const int32 Lvl = FMath::Max(0, ProficiencyLevel);
        const int32 TiersAboveNormal = FMath::Max(0, Tier - 1);
        const float Scaled = static_cast<float>(Base)
                           + static_cast<float>(Lvl) * FMath::Max(0.0f, PerLevel)
                           + static_cast<float>(TiersAboveNormal) * FMath::Max(0.0f, PerTier);
        return FMath::Max(0, FMath::FloorToInt(Scaled));
    }

    static TArray<int32> RollYield(int32 NumAvailable, int32 Count, FRandomStream &Rng) {
        TArray<int32> Out;
        if (NumAvailable <= 0 || Count <= 0) {
            return Out;
        }
        const int32 Take = FMath::Min(Count, NumAvailable);
        TArray<int32> Pool;
        Pool.SetNumUninitialized(NumAvailable);
        for (int32 i = 0; i < NumAvailable; ++i) {
            Pool[i] = i;
        }
        Out.Reserve(Take);
        for (int32 i = 0; i < Take; ++i) {
            const int32 j = Rng.RandRange(i, NumAvailable - 1);
            Pool.Swap(i, j);
            Out.Add(Pool[i]);
        }
        return Out;
    }


    static int32 BotchScore(const FMythicKillContext &Ctx, const FMythicPeltQualityConfig &Config) {
        int32 Score = 0;
        Score += Ctx.bBurnKill ? 1 : 0;
        Score += Ctx.bPoisonKill ? 1 : 0;
        Score += (Ctx.OverkillFraction >= FMath::Max(0.0f, Config.OverkillBotchFraction)) ? 1 : 0;
        Score += (Ctx.HitsTaken >= FMath::Max(1, Config.MessyHitsThreshold)) ? 1 : 0;
        return Score;
    }

    static EMythicYieldQuality BaseTierForKill(const FMythicKillContext &Ctx, const FMythicPeltQualityConfig &Config) {
        const int32 Score = BotchScore(Ctx, Config);
        if (Score >= FMath::Max(1, Config.BotchScoreForRagged)) {
            return EMythicYieldQuality::Ragged;
        }
        if (Score == 0 && !Ctx.bBleedKill && Ctx.bCriticalKill && Ctx.HitsTaken <= FMath::Max(0, Config.CleanKillMaxHits)) {
            return EMythicYieldQuality::Fine;
        }
        return EMythicYieldQuality::Common;
    }

    static int32 DecompPenaltyTiers(EMythicDecompStage Stage) {
        return static_cast<int32>(Stage);
    }

    static EMythicYieldQuality ResolveQuality(const FMythicKillContext &Ctx, EMythicDecompStage DecompStage, int32 HuntingLevel,
                                              const FMythicYieldQualityRules &YieldRules, const FMythicPeltQualityConfig &Config,
                                              float Rand01) {
        const EMythicYieldQuality Base = BaseTierForKill(Ctx, Config);
        if (Base == EMythicYieldQuality::Ragged) {
            return EMythicYieldQuality::Ragged;
        }
        const EMythicYieldQuality Rolled = FMythicYieldQuality::RollQuality(YieldRules, Rand01, HuntingLevel,
 EMythicYieldQuality::Ragged,
 Base);
        const int32 Penalized = FMythicYieldQuality::TierIndex(Rolled) - DecompPenaltyTiers(DecompStage);
        return FMythicYieldQuality::TierFromIndex(Penalized);
    }
};
