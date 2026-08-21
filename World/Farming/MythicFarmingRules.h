
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "World/Gathering/MythicYieldQuality.h"
#include "MythicFarmingRules.generated.h"

struct FMythicStageAdvance {
    int32 NewStageIndex = 0;
    float RemainingToNextStage = 0.0f;
};

struct FMythicMoistureSample {
    float NewMoisture = 0.0f;
    float WetSeconds = 0.0f;
    float DrySeconds = 0.0f;
};

struct FMythicCropQualityInputs {
    float WetUptimeFraction = 0.0f;
    bool bFertilized = false;
    bool bGraveEssence = false;
    float PollinationMagnitude = 0.0f;
    bool bSeasonMatch = false;
    int32 FarmingLevel = 0;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicFarmingConfig {
    GENERATED_BODY()

    // ── Moisture ──
    /** Moisture (0..1) lost per second. Default: full → bone-dry in one hour of real time. Purely cosmetic until the
     *  growth/wither knobs below are authored (growth speed defaults 1.0; wither defaults off). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moisture", meta = (ClampMin = "0.0"))
    float MoistureDecayPerSecond = 1.0f / 3600.0f;

    /** Moisture at/above this counts as WET for the wet-uptime quality accumulator. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moisture", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WetMoistureThreshold = 0.5f;

    /** Growth SPEED multiplier at moisture 0 (bone-dry). 1.0 (default) = moisture never affects growth — the INERT
     *  default (unwatered crops behave exactly like today). Author e.g. 0.5 to make dry crops grow at half speed;
     *  never a total stop (the plot floors the effective speed). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moisture", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DryGrowthSpeedMultiplier = 1.0f;

    /** Continuous bone-dry seconds after which a GROWING crop withers (withered → harvest yields the crop's compost
     *  feedstock rewards, never nothing and never a lost plot — C6). 0 (default) = crops NEVER wither. Mature crops
     *  never wither (today's park-forever harvest behaviour is preserved). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moisture", meta = (ClampMin = "0.0"))
    float WitherAfterDrySeconds = 0.0f;

    /** Rain waters for free: while the weather is Environment.Weather.Rain at a sample point, the window resolves as
     *  fully wet + moisture refills (same as irrigation coverage). Positive-only — safe to leave on. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Moisture")
    bool bRainRefillsMoisture = true;

    // ── Quality bonus chances (added on top of the shared P1 mastery ladder; tiers only MANIFEST when a crop def
    //    authors per-tier reward overrides, so these are inert on unauthored content) ──
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WetUptimeFineChanceBonus = 0.10f; // at 100% wet uptime

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WetUptimePristineChanceBonus = 0.03f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FertilizerFineChanceBonus = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FertilizerPristineChanceBonus = 0.03f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GraveEssenceFineChanceBonus = 0.10f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float GraveEssencePristineChanceBonus = 0.05f;

    /** Fine-chance bonus per point of summed Influence.Pollination magnitude at the plot (capped below). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PollinationFineChanceBonusPerMagnitude = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PollinationPristineChanceBonusPerMagnitude = 0.02f;

    /** Pollination magnitude is clamped to this before the per-magnitude bonuses (hive stacking has a ceiling). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0"))
    float PollinationMagnitudeCap = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SeasonMatchFineChanceBonus = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SeasonMatchPristineChanceBonus = 0.02f;

    /** Hard ceilings on the folded chances (rails — quality is never guaranteed). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxFineChance = 0.60f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quality", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxPristineChance = 0.30f;

    // ── Pollination extra-yield roll (the hive's second payoff besides quality) ──
    /** Chance of +1 harvest yield per point of (capped) pollination magnitude, resolved by an injected roll. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pollination", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PollinationYieldBonusChancePerMagnitude = 0.10f;

    // ── Gravebloom ──
    /** A corpse within this radius (cm) of an empty plot can be buried into it (the Gravebloom verb). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gravebloom", meta = (ClampMin = "100.0"))
    float GraveburyRadius = 400.0f;

    // ── Scarecrow habituation (the farm channel reads these through the pressure subsystem) ──
    /** Each deterred raid probe deposits this much habituation on the cell (creatures learn the scarecrow is a bluff). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scarecrow", meta = (ClampMin = "0.0"))
    float HabituationPerDeterredCheck = 1.0f;

    /** Habituation deposits shed per second (default: one deposit fades in ~30 minutes). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scarecrow", meta = (ClampMin = "0.0"))
    float HabituationDecayPerSecond = 1.0f / 1800.0f;

    /** How hard habituation erodes deterrence: effectiveness = magnitude / (1 + deposits × this). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Scarecrow", meta = (ClampMin = "0.0"))
    float HabituationEffectFactor = 0.5f;
};

struct FMythicFarmPlotSaveData {
    uint8 Version = 0;

    FString CropPath;
    int32 Stage = -1;
    double RemainingSeconds = 0.0;

    uint8 MoistureQ = 255;
    float WetUptimeSeconds = 0.0f;
    float GrowTimeSeconds = 0.0f;
    float DryStreakSeconds = 0.0f;
    bool bWithered = false;
    FString FertilizerTagName;
    FString GraveEssenceTagName;
};

struct FMythicFarmingRules {
    static FMythicStageAdvance ResolveStageAdvance(float ElapsedSeconds, TConstArrayView<float> StageDurations) {
        FMythicStageAdvance Out;
        float Remaining = FMath::Max(0.0f, ElapsedSeconds);
        for (int32 i = 0; i < StageDurations.Num(); ++i) {
            const float Dur = StageDurations[i];
            if (Remaining >= Dur) {
                Remaining -= FMath::Max(0.0f, Dur);
                Out.NewStageIndex = i + 1;
            }
            else {
                Out.RemainingToNextStage = Dur - Remaining;
                return Out;
            }
        }
        Out.RemainingToNextStage = 0.0f;
        return Out;
    }

    static bool CanPlant(bool bEmpty, bool bHasSeed, int32 FarmingLevel, int32 MinLevel) {
        return bEmpty && bHasSeed && (MinLevel <= 0 || FarmingLevel >= MinLevel);
    }

    static bool CanPlantInSeason(const FGameplayTagContainer &AllowedSeasons, const FGameplayTag &CurrentSeason) {
        if (AllowedSeasons.IsEmpty() || !CurrentSeason.IsValid()) {
            return true;
        }
        return AllowedSeasons.HasTag(CurrentSeason);
    }

    static bool CanHarvest(int32 Stage, int32 MatureStage) {
        return MatureStage >= 0 && Stage >= MatureStage;
    }

    static int32 ComputeHarvestYield(int32 BaseYield, int32 FarmingLevel, float BonusPerLevel, float Roll01) {
        if (BaseYield <= 0) {
            return 0;
        }
        const float Level = static_cast<float>(FMath::Max(0, FarmingLevel));
        const float Bonus = FMath::Max(0.0f, BonusPerLevel);
        const float Raw = static_cast<float>(BaseYield) * (1.0f + Level * Bonus);

        const float NearestInt = FMath::RoundToFloat(Raw);
        const float Scaled = FMath::IsNearlyEqual(Raw, NearestInt, KINDA_SMALL_NUMBER) ? NearestInt : Raw;

        const int32 Whole = FMath::FloorToInt(Scaled);
        const float Frac = Scaled - static_cast<float>(Whole);
        const float Roll = FMath::Clamp(Roll01, 0.0f, 1.0f);
        const int32 Extra = (Roll < Frac) ? 1 : 0;
        return FMath::Max(0, Whole + Extra);
    }

    static int32 PollinationBonusYield(float PollinationMagnitude, const FMythicFarmingConfig &Config, float Roll01) {
        const float Capped = FMath::Clamp(PollinationMagnitude, 0.0f, FMath::Max(0.0f, Config.PollinationMagnitudeCap));
        const float Chance = FMath::Clamp(Capped * FMath::Max(0.0f, Config.PollinationYieldBonusChancePerMagnitude), 0.0f, 1.0f);
        return (FMath::Clamp(Roll01, 0.0f, 1.0f) < Chance) ? 1 : 0;
    }

    static float MoistureAtTime(float MoistureAtSample, double SampleTime, double Now, float DecayPerSecond) {
        const double Gap = FMath::Max(0.0, Now - SampleTime);
        const float Decayed = FMath::Clamp(MoistureAtSample, 0.0f, 1.0f) -
                              FMath::Max(0.0f, DecayPerSecond) * static_cast<float>(Gap);
        return FMath::Clamp(Decayed, 0.0f, 1.0f);
    }

    static FMythicMoistureSample SampleMoistureWindow(float StartMoisture, float WindowSeconds, float DecayPerSecond,
                                                      float WetThreshold) {
        FMythicMoistureSample Out;
        const float M = FMath::Clamp(StartMoisture, 0.0f, 1.0f);
        const float Gap = FMath::Max(0.0f, WindowSeconds);
        const float Decay = FMath::Max(0.0f, DecayPerSecond);
        const float Threshold = FMath::Clamp(WetThreshold, 0.0f, 1.0f);

        if (Decay <= 0.0f) {
            Out.NewMoisture = M;
            Out.WetSeconds = (M >= Threshold) ? Gap : 0.0f;
            Out.DrySeconds = (M <= 0.0f) ? Gap : 0.0f;
            return Out;
        }

        Out.NewMoisture = FMath::Clamp(M - Decay * Gap, 0.0f, 1.0f);
        Out.WetSeconds = FMath::Clamp((M - Threshold) / Decay, 0.0f, Gap);
        const float SecondsToZero = M / Decay;
        Out.DrySeconds = FMath::Clamp(Gap - SecondsToZero, 0.0f, Gap);
        return Out;
    }

    static constexpr float MinGrowthSpeed = 0.05f;
    static float GrowthTimeScale(float Moisture01, float DryGrowthSpeedMultiplier) {
        const float Dry = FMath::Clamp(DryGrowthSpeedMultiplier, 0.0f, 1.0f);
        const float Speed = FMath::Lerp(Dry, 1.0f, FMath::Clamp(Moisture01, 0.0f, 1.0f));
        return FMath::Clamp(Speed, MinGrowthSpeed, 1.0f);
    }

    static bool WitherCheck(float DryStreakSeconds, float WitherAfterDrySeconds) {
        return WitherAfterDrySeconds > 0.0f && DryStreakSeconds >= WitherAfterDrySeconds;
    }

    static float WetUptimeFraction(float WetSeconds, float GrowSeconds) {
        return (GrowSeconds > 0.0f) ? FMath::Clamp(WetSeconds / GrowSeconds, 0.0f, 1.0f) : 0.0f;
    }

    static EMythicYieldQuality ComputeCropQuality(const FMythicCropQualityInputs &In, const FMythicYieldQualityRules &QualityRules,
                                                  const FMythicFarmingConfig &Config, float Roll01) {
        const float Wet = FMath::Clamp(In.WetUptimeFraction, 0.0f, 1.0f);
        const float Poll = FMath::Clamp(In.PollinationMagnitude, 0.0f, FMath::Max(0.0f, Config.PollinationMagnitudeCap));

        float FineChance = FMythicYieldQuality::FineChanceAtLevel(QualityRules, In.FarmingLevel);
        FineChance += Wet * FMath::Max(0.0f, Config.WetUptimeFineChanceBonus);
        FineChance += In.bFertilized ? FMath::Max(0.0f, Config.FertilizerFineChanceBonus) : 0.0f;
        FineChance += In.bGraveEssence ? FMath::Max(0.0f, Config.GraveEssenceFineChanceBonus) : 0.0f;
        FineChance += Poll * FMath::Max(0.0f, Config.PollinationFineChanceBonusPerMagnitude);
        FineChance += In.bSeasonMatch ? FMath::Max(0.0f, Config.SeasonMatchFineChanceBonus) : 0.0f;
        FineChance = FMath::Clamp(FineChance, 0.0f, FMath::Clamp(Config.MaxFineChance, 0.0f, 1.0f));

        float PristineChance = FMythicYieldQuality::PristineChanceAtLevel(QualityRules, In.FarmingLevel);
        PristineChance += Wet * FMath::Max(0.0f, Config.WetUptimePristineChanceBonus);
        PristineChance += In.bFertilized ? FMath::Max(0.0f, Config.FertilizerPristineChanceBonus) : 0.0f;
        PristineChance += In.bGraveEssence ? FMath::Max(0.0f, Config.GraveEssencePristineChanceBonus) : 0.0f;
        PristineChance += Poll * FMath::Max(0.0f, Config.PollinationPristineChanceBonusPerMagnitude);
        PristineChance += In.bSeasonMatch ? FMath::Max(0.0f, Config.SeasonMatchPristineChanceBonus) : 0.0f;
        PristineChance = FMath::Clamp(PristineChance, 0.0f, FMath::Clamp(Config.MaxPristineChance, 0.0f, 1.0f));

        const float Roll = FMath::Clamp(Roll01, 0.0f, 1.0f);
        if (Roll < PristineChance) {
            return EMythicYieldQuality::Pristine;
        }
        if (Roll < PristineChance + FineChance) {
            const EMythicYieldQuality FineFloor = FMythicYieldQuality::MasteryFloor(QualityRules, In.FarmingLevel, EMythicYieldQuality::Common);
            return FMythicYieldQuality::TierFromIndex(FMath::Max(
                FMythicYieldQuality::TierIndex(EMythicYieldQuality::Fine),
                FMythicYieldQuality::TierIndex(FineFloor)));
        }
        const EMythicYieldQuality Floor = FMythicYieldQuality::MasteryFloor(QualityRules, In.FarmingLevel, EMythicYieldQuality::Common);
        return Floor;
    }

    static float HabituationAtTime(float Deposits, double LastTime, double Now, float DecayPerSecond) {
        const double Gap = FMath::Max(0.0, Now - LastTime);
        return FMath::Max(0.0f, Deposits - FMath::Max(0.0f, DecayPerSecond) * static_cast<float>(Gap));
    }

    static float DeterrenceEffectiveness(float TotalMagnitude, float HabituationDeposits, float HabituationEffectFactor) {
        const float Mag = FMath::Max(0.0f, TotalMagnitude);
        const float Deposits = FMath::Max(0.0f, HabituationDeposits);
        const float Factor = FMath::Max(0.0f, HabituationEffectFactor);
        return Mag / (1.0f + Deposits * Factor);
    }

    static double RemainingSecondsToNextStage(double Deadline, double Now) {
        return FMath::Max(0.0, Deadline - Now);
    }

    static double RebuildDeadline(double Now, double RemainingSeconds) {
        return Now + FMath::Max(0.0, RemainingSeconds);
    }

    static constexpr int32 SaveMagic = 0x4D594650;
    static constexpr uint8 SaveVersion = 1;

    static void EncodePlotSave(const FMythicFarmPlotSaveData &Data, TArray<uint8> &OutBytes) {
        FMemoryWriter Writer(OutBytes);
        int32 Magic = SaveMagic;
        uint8 Version = SaveVersion;
        Writer << Magic;
        Writer << Version;

        FString CropPath = Data.CropPath;
        int32 Stage = Data.Stage;
        double RemainingSeconds = Data.RemainingSeconds;
        Writer << CropPath;
        Writer << Stage;
        Writer << RemainingSeconds;

        uint8 MoistureQ = Data.MoistureQ;
        float WetUptimeSeconds = Data.WetUptimeSeconds;
        float GrowTimeSeconds = Data.GrowTimeSeconds;
        float DryStreakSeconds = Data.DryStreakSeconds;
        uint8 Withered = Data.bWithered ? 1 : 0;
        FString FertilizerTagName = Data.FertilizerTagName;
        FString GraveEssenceTagName = Data.GraveEssenceTagName;
        Writer << MoistureQ;
        Writer << WetUptimeSeconds;
        Writer << GrowTimeSeconds;
        Writer << DryStreakSeconds;
        Writer << Withered;
        Writer << FertilizerTagName;
        Writer << GraveEssenceTagName;
    }

    static bool DecodePlotSave(const TArray<uint8> &InBytes, FMythicFarmPlotSaveData &Out) {
        Out = FMythicFarmPlotSaveData();
        if (InBytes.Num() == 0) {
            return false;
        }

        if (InBytes.Num() >= static_cast<int32>(sizeof(int32))) {
            int32 Lead = 0;
            FMemory::Memcpy(&Lead, InBytes.GetData(), sizeof(int32));
            if (Lead == SaveMagic) {
                FMemoryReader Reader(InBytes);
                int32 Magic = 0;
                Reader << Magic;
                Reader << Out.Version;
                Reader << Out.CropPath;
                Reader << Out.Stage;
                Reader << Out.RemainingSeconds;
                if (Out.Version >= 1) {
                    uint8 Withered = 0;
                    Reader << Out.MoistureQ;
                    Reader << Out.WetUptimeSeconds;
                    Reader << Out.GrowTimeSeconds;
                    Reader << Out.DryStreakSeconds;
                    Reader << Withered;
                    Reader << Out.FertilizerTagName;
                    Reader << Out.GraveEssenceTagName;
                    Out.bWithered = Withered != 0;
                }
                return !Reader.IsError();
            }
        }

        FMemoryReader Reader(InBytes);
        Reader << Out.CropPath;
        Reader << Out.Stage;
        Reader << Out.RemainingSeconds;
        Out.Version = 0;
        return !Reader.IsError();
    }
};
