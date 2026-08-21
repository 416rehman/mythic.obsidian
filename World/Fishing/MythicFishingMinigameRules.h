
#pragma once

#include "CoreMinimal.h"
#include "MythicFishingMinigameRules.generated.h"

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicFishingMinigameConfig {
    GENERATED_BODY()

    /** Minimum seconds of WAIT before the bite window opens (seeded per cast within [Min, Max]). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minigame", meta = (ClampMin = "0.0"))
    float WaitMinSeconds = 2.0f;

    /** Maximum seconds of WAIT before the bite window opens. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minigame", meta = (ClampMin = "0.0"))
    float WaitMaxSeconds = 6.0f;

    /** Length of the bite window (seconds) a Hook event must land inside. Generous default — co-op-friendly, not a QTE gauntlet. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minigame", meta = (ClampMin = "0.1"))
    float BiteWindowSeconds = 0.9f;

    /** Hard cap on the FIGHT phase (seconds). Outlasting it = the fish escapes (bait lost). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minigame", meta = (ClampMin = "1.0"))
    float FightMaxSeconds = 20.0f;

    /** Base seconds into the FIGHT before the first surge sweeps (a seeded jitter of up to half a period is added). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minigame", meta = (ClampMin = "0.0"))
    float SurgeFirstDelaySeconds = 1.5f;

    /** Seconds between surge starts during the FIGHT. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minigame", meta = (ClampMin = "0.5"))
    float SurgePeriodSeconds = 4.0f;

    /** How long each surge lasts (pulling inside it breaks the line). Must be < SurgePeriodSeconds to leave reel gaps. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minigame", meta = (ClampMin = "0.1"))
    float SurgeDurationSeconds = 1.2f;

    /** Successful (between-surge) pulls required to land the catch. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minigame", meta = (ClampMin = "1"))
    int32 PullsToLand = 5;

    /** AUTO-RESOLVE VALVE: at/above this fishing mastery level, TRASH-tier catches (bTrashTier entries) skip the
     *  minigame and resolve on the classic quiet channel. 0 = valve disabled (every catch plays the beats). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Minigame", meta = (ClampMin = "0"))
    int32 AutoResolveTrashAtMastery = 0;
};

struct FMythicFishingMinigameParams {
    float WaitSeconds = 3.0f;
    float BiteWindowSeconds = 0.9f;
    float FightMaxSeconds = 20.0f;
    float SurgeFirstDelay = 1.5f;
    float SurgePeriod = 4.0f;
    float SurgeDuration = 1.2f;
    int32 PullsToLand = 5;
};

enum class EMythicFishingPhase : uint8 {
    Wait,
    Bite,
    MissedBite,
    Fight,
    Escaped
};

enum class EMythicFishingPullResult : uint8 {
    Ignored,
    Reel,
    Landed,
    LineBreak
};

struct FMythicFishingMinigameRules {
    static FMythicFishingMinigameParams MakeParams(const FMythicFishingMinigameConfig &Config, int32 Seed) {
        FRandomStream Rng(Seed);
        FMythicFishingMinigameParams P;
        const float WaitMin = FMath::Max(0.0f, Config.WaitMinSeconds);
        const float WaitMax = FMath::Max(WaitMin, Config.WaitMaxSeconds);
        P.WaitSeconds = Rng.FRandRange(WaitMin, WaitMax);
        P.BiteWindowSeconds = FMath::Max(0.1f, Config.BiteWindowSeconds);
        P.FightMaxSeconds = FMath::Max(1.0f, Config.FightMaxSeconds);
        P.SurgePeriod = FMath::Max(0.5f, Config.SurgePeriodSeconds);
        P.SurgeDuration = FMath::Clamp(Config.SurgeDurationSeconds, 0.1f, P.SurgePeriod * 0.75f);
        P.SurgeFirstDelay = FMath::Max(0.0f, Config.SurgeFirstDelaySeconds) + Rng.FRandRange(0.0f, P.SurgePeriod * 0.5f);
        P.PullsToLand = FMath::Max(1, Config.PullsToLand);
        return P;
    }

    static float BiteWindowStart(const FMythicFishingMinigameParams &P) { return P.WaitSeconds; }
    static float BiteWindowEnd(const FMythicFishingMinigameParams &P) { return P.WaitSeconds + P.BiteWindowSeconds; }

    static bool IsInBiteWindow(const FMythicFishingMinigameParams &P, float Elapsed) {
        return Elapsed >= BiteWindowStart(P) && Elapsed < BiteWindowEnd(P);
    }

    static bool IsBiteMissed(const FMythicFishingMinigameParams &P, float Elapsed, bool bHooked) {
        return !bHooked && Elapsed >= BiteWindowEnd(P);
    }

    static bool IsInSurge(const FMythicFishingMinigameParams &P, float FightElapsed) {
        if (FightElapsed < P.SurgeFirstDelay) {
            return false;
        }
        const float IntoCycle = FMath::Fmod(FightElapsed - P.SurgeFirstDelay, P.SurgePeriod);
        return IntoCycle < P.SurgeDuration;
    }

    static bool HasFishEscaped(const FMythicFishingMinigameParams &P, float FightElapsed) {
        return FightElapsed >= P.FightMaxSeconds;
    }

    static EMythicFishingPullResult ScorePull(const FMythicFishingMinigameParams &P, float FightElapsed, int32 PullsSoFar, bool bHooked) {
        if (!bHooked) {
            return EMythicFishingPullResult::Ignored;
        }
        if (IsInSurge(P, FightElapsed)) {
            return EMythicFishingPullResult::LineBreak;
        }
        return (PullsSoFar + 1 >= P.PullsToLand) ? EMythicFishingPullResult::Landed : EMythicFishingPullResult::Reel;
    }

    static EMythicFishingPhase PhaseAtTime(const FMythicFishingMinigameParams &P, float Elapsed, bool bHooked, float HookElapsed) {
        if (bHooked) {
            return HasFishEscaped(P, Elapsed - HookElapsed) ? EMythicFishingPhase::Escaped : EMythicFishingPhase::Fight;
        }
        if (Elapsed < BiteWindowStart(P)) {
            return EMythicFishingPhase::Wait;
        }
        if (Elapsed < BiteWindowEnd(P)) {
            return EMythicFishingPhase::Bite;
        }
        return EMythicFishingPhase::MissedBite;
    }

    static bool ShouldAutoResolve(int32 MasteryLevel, int32 AutoResolveAtLevel, bool bTrashCatch) {
        return AutoResolveAtLevel > 0 && bTrashCatch && MasteryLevel >= AutoResolveAtLevel;
    }

    static float ChannelCeiling(const FMythicFishingMinigameParams &P) {
        return P.WaitSeconds + P.BiteWindowSeconds + P.FightMaxSeconds + 2.0f;
    }
};
