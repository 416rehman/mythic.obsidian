
#include "Misc/AutomationTest.h"
#include "Serialization/MemoryWriter.h"
#include "World/Farming/MythicFarmingRules.h"
#include "World/Gathering/MythicYieldQuality.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFarmingDepthTest,
    "Mythic.World.FarmingDepth",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFarmingDepthTest::RunTest(const FString &Parameters) {
    using Rules = FMythicFarmingRules;
    const FMythicFarmingConfig DefaultConfig;
    const FMythicYieldQualityRules QualityRules;

    {
        const float Decay = 1.0f / 3600.0f;
        TestEqual(TEXT("no elapsed time → unchanged"), Rules::MoistureAtTime(1.0f, 100.0, 100.0, Decay), 1.0f);
        TestTrue(TEXT("30min → ~0.5"), FMath::IsNearlyEqual(Rules::MoistureAtTime(1.0f, 0.0, 1800.0, Decay), 0.5f, 0.001f));
        TestEqual(TEXT("60min → exactly dry"), Rules::MoistureAtTime(1.0f, 0.0, 3600.0, Decay), 0.0f);
        TestEqual(TEXT("way past dry → floors at 0"), Rules::MoistureAtTime(1.0f, 0.0, 999999.0, Decay), 0.0f);
        TestEqual(TEXT("negative gap clamps → unchanged"), Rules::MoistureAtTime(0.7f, 500.0, 100.0, Decay), 0.7f);
        TestEqual(TEXT("zero decay → moisture never moves"), Rules::MoistureAtTime(0.3f, 0.0, 1.0e6, 0.0f), 0.3f);
        TestEqual(TEXT("over-1 input clamps to 1"), Rules::MoistureAtTime(5.0f, 0.0, 0.0, Decay), 1.0f);
    }

    {
        const float Decay = 1.0f / 3600.0f;
        FMythicMoistureSample S = Rules::SampleMoistureWindow(1.0f, 3600.0f, Decay, 0.5f);
        TestEqual(TEXT("hour from full → dry at end"), S.NewMoisture, 0.0f);
        TestTrue(TEXT("hour from full → 1800s wet"), FMath::IsNearlyEqual(S.WetSeconds, 1800.0f, 0.1f));
        TestTrue(TEXT("hour from full → 0s at zero"), FMath::IsNearlyEqual(S.DrySeconds, 0.0f, 0.1f));

        S = Rules::SampleMoistureWindow(1.0f, 7200.0f, Decay, 0.5f);
        TestEqual(TEXT("2h from full → dry"), S.NewMoisture, 0.0f);
        TestTrue(TEXT("2h from full → still 1800s wet"), FMath::IsNearlyEqual(S.WetSeconds, 1800.0f, 0.1f));
        TestTrue(TEXT("2h from full → 3600s bone-dry"), FMath::IsNearlyEqual(S.DrySeconds, 3600.0f, 0.1f));

        S = Rules::SampleMoistureWindow(0.4f, 600.0f, Decay, 0.5f);
        TestEqual(TEXT("sub-threshold start → 0 wet"), S.WetSeconds, 0.0f);

        S = Rules::SampleMoistureWindow(0.8f, 1000.0f, 0.0f, 0.5f);
        TestEqual(TEXT("no decay → moisture holds"), S.NewMoisture, 0.8f);
        TestEqual(TEXT("no decay, wet start → whole window wet"), S.WetSeconds, 1000.0f);
        TestEqual(TEXT("no decay, wet start → no dry time"), S.DrySeconds, 0.0f);
        S = Rules::SampleMoistureWindow(0.0f, 1000.0f, 0.0f, 0.5f);
        TestEqual(TEXT("no decay, dry start → whole window dry"), S.DrySeconds, 1000.0f);
    }

    {
        TestEqual(TEXT("default config, bone-dry → speed 1.0"), Rules::GrowthTimeScale(0.0f, DefaultConfig.DryGrowthSpeedMultiplier), 1.0f);
        TestEqual(TEXT("default config, full → speed 1.0"), Rules::GrowthTimeScale(1.0f, DefaultConfig.DryGrowthSpeedMultiplier), 1.0f);

        TestEqual(TEXT("authored 0.5, dry → 0.5"), Rules::GrowthTimeScale(0.0f, 0.5f), 0.5f);
        TestEqual(TEXT("authored 0.5, wet → 1.0"), Rules::GrowthTimeScale(1.0f, 0.5f), 1.0f);
        float Prev = 0.0f;
        for (int32 i = 0; i <= 10; ++i) {
            const float Speed = Rules::GrowthTimeScale(i / 10.0f, 0.5f);
            TestTrue(TEXT("speed is monotonic in moisture"), Speed >= Prev);
            TestTrue(TEXT("speed within [MinGrowthSpeed, 1]"), Speed >= Rules::MinGrowthSpeed && Speed <= 1.0f);
            Prev = Speed;
        }
        TestEqual(TEXT("authored 0, dry → floors at MinGrowthSpeed"), Rules::GrowthTimeScale(0.0f, 0.0f), Rules::MinGrowthSpeed);
    }

    {
        TestFalse(TEXT("default config (0) → NEVER withers"), Rules::WitherCheck(1.0e9f, DefaultConfig.WitherAfterDrySeconds));
        TestFalse(TEXT("below the authored span → alive"), Rules::WitherCheck(3599.0f, 3600.0f));
        TestTrue(TEXT("exactly at the span (inclusive) → withered"), Rules::WitherCheck(3600.0f, 3600.0f));
        TestTrue(TEXT("past the span → withered"), Rules::WitherCheck(9000.0f, 3600.0f));
    }

    {
        const FGameplayTag Spring = FGameplayTag::RequestGameplayTag(FName("Environment.Season.Spring"));
        const FGameplayTag Winter = FGameplayTag::RequestGameplayTag(FName("Environment.Season.Winter"));
        FGameplayTagContainer SpringOnly;
        SpringOnly.AddTag(Spring);
        const FGameplayTagContainer AnySeason;

        TestTrue(TEXT("no authored seasons → plant any time"), Rules::CanPlantInSeason(AnySeason, Winter));
        TestTrue(TEXT("no calendar source → never blocks"), Rules::CanPlantInSeason(SpringOnly, FGameplayTag()));
        TestTrue(TEXT("matching season → plantable"), Rules::CanPlantInSeason(SpringOnly, Spring));
        TestFalse(TEXT("wrong season → blocked"), Rules::CanPlantInSeason(SpringOnly, Winter));
    }

    {
        auto Quality = [&](const FMythicCropQualityInputs &In, float Roll) {
            return Rules::ComputeCropQuality(In, QualityRules, DefaultConfig, Roll);
        };
        const FMythicCropQualityInputs Bare;

        TestEqual(TEXT("bare inputs, mid roll → Common"), Quality(Bare, 0.15f), EMythicYieldQuality::Common);
        TestEqual(TEXT("bare inputs, worst roll → Common (never Ragged)"), Quality(Bare, 0.999f), EMythicYieldQuality::Common);

        {
            FMythicCropQualityInputs In = Bare;
            In.WetUptimeFraction = 1.0f;
            TestEqual(TEXT("wet uptime moves the tier"), Quality(In, 0.15f), EMythicYieldQuality::Fine);
        }
        {
            FMythicCropQualityInputs In = Bare;
            In.bFertilized = true;
            TestEqual(TEXT("fertilizer moves the tier"), Quality(In, 0.15f), EMythicYieldQuality::Fine);
        }
        {
            FMythicCropQualityInputs In = Bare;
            In.bGraveEssence = true;
            TestEqual(TEXT("grave essence moves the tier"), Quality(In, 0.15f), EMythicYieldQuality::Fine);
        }
        {
            FMythicCropQualityInputs In = Bare;
            In.PollinationMagnitude = 1.0f;
            TestEqual(TEXT("pollination moves the tier"), Quality(In, 0.15f), EMythicYieldQuality::Fine);
        }
        {
            FMythicCropQualityInputs In = Bare;
            In.bSeasonMatch = true;
            TestEqual(TEXT("season match moves the tier"), Quality(In, 0.15f), EMythicYieldQuality::Fine);
        }
        {
            FMythicCropQualityInputs In = Bare;
            In.FarmingLevel = 25;
            TestEqual(TEXT("farming level moves the tier (P1 mastery ladder)"), Quality(In, 0.15f), EMythicYieldQuality::Fine);
        }

        {
            FMythicCropQualityInputs In;
            In.WetUptimeFraction = 1.0f;
            In.bFertilized = true;
            In.bGraveEssence = true;
            In.PollinationMagnitude = 2.0f;
            In.bSeasonMatch = true;
            TestEqual(TEXT("full stack, low roll → Pristine"), Quality(In, 0.05f), EMythicYieldQuality::Pristine);
            TestEqual(TEXT("full stack, bad roll → still Common"), Quality(In, 0.95f), EMythicYieldQuality::Common);
        }

        {
            const float Roll = 0.12f;
            const int32 BareTier = FMythicYieldQuality::TierIndex(Quality(Bare, Roll));
            FMythicCropQualityInputs In = Bare;
            In.WetUptimeFraction = 0.5f;
            TestTrue(TEXT("adding wet never lowers"), FMythicYieldQuality::TierIndex(Quality(In, Roll)) >= BareTier);
            In.bFertilized = true;
            const int32 WetTier = FMythicYieldQuality::TierIndex(Quality(In, Roll));
            In.PollinationMagnitude = 1.5f;
            TestTrue(TEXT("adding pollination never lowers"), FMythicYieldQuality::TierIndex(Quality(In, Roll)) >= WetTier);
        }

        {
            FMythicYieldQualityRules FloorRules;
            FloorRules.BasePristineChance = 0.0f;
            FloorRules.PristineChancePerMasteryLevel = 0.0f;
            FloorRules.BaseFineChance = 0.30f;
            FloorRules.FineChancePerMasteryLevel = 0.0f;
            FloorRules.PristineFloorAtMasteryLevel = 1;

            FMythicCropQualityInputs FloorIn;
            FloorIn.FarmingLevel = 5;
            TestEqual(TEXT("Fine-band roll at/above the Pristine mastery floor lifts to Pristine"),
                      Rules::ComputeCropQuality(FloorIn, FloorRules, DefaultConfig, 0.10f), EMythicYieldQuality::Pristine);

            FloorIn.FarmingLevel = 0;
            TestEqual(TEXT("Fine-band roll below the floor level stays Fine"),
                      Rules::ComputeCropQuality(FloorIn, FloorRules, DefaultConfig, 0.10f), EMythicYieldQuality::Fine);
        }
    }

    {
        TestEqual(TEXT("mag 1, roll .05 → +1"), Rules::PollinationBonusYield(1.0f, DefaultConfig, 0.05f), 1);
        TestEqual(TEXT("mag 1, roll .10 → +0 (strict <)"), Rules::PollinationBonusYield(1.0f, DefaultConfig, 0.10f), 0);
        TestEqual(TEXT("no coverage → never"), Rules::PollinationBonusYield(0.0f, DefaultConfig, 0.0f), 0);
        TestEqual(TEXT("mag 100 caps at 2 → roll .19 hits"), Rules::PollinationBonusYield(100.0f, DefaultConfig, 0.19f), 1);
        TestEqual(TEXT("mag 100 caps at 2 → roll .21 misses"), Rules::PollinationBonusYield(100.0f, DefaultConfig, 0.21f), 0);
    }

    {
        TestEqual(TEXT("no gap → deposits hold"), Rules::HabituationAtTime(3.0f, 100.0, 100.0, 0.001f), 3.0f);
        TestTrue(TEXT("deposits decay"), FMath::IsNearlyEqual(Rules::HabituationAtTime(3.0f, 0.0, 1000.0, 0.001f), 2.0f, 0.001f));
        TestEqual(TEXT("deposits floor at 0"), Rules::HabituationAtTime(1.0f, 0.0, 1.0e9, 0.001f), 0.0f);

        TestEqual(TEXT("fresh scarecrow → full deterrence"), Rules::DeterrenceEffectiveness(2.0f, 0.0f, 0.5f), 2.0f);
        TestTrue(TEXT("habituation erodes deterrence"),
                 Rules::DeterrenceEffectiveness(2.0f, 2.0f, 0.5f) < Rules::DeterrenceEffectiveness(2.0f, 1.0f, 0.5f));
        TestEqual(TEXT("factor 0 → habituation-immune"), Rules::DeterrenceEffectiveness(2.0f, 50.0f, 0.0f), 2.0f);
    }

    {
        TArray<uint8> LegacyBytes;
        {
            FMemoryWriter Writer(LegacyBytes);
            FString CropPath = TEXT("/Game/Mythic/Farming/DA_Crop_Wheat.DA_Crop_Wheat");
            int32 Stage = 2;
            double RemainingSeconds = 42.5;
            Writer << CropPath;
            Writer << Stage;
            Writer << RemainingSeconds;
        }

        FMythicFarmPlotSaveData Out;
        TestTrue(TEXT("v0 legacy payload decodes"), Rules::DecodePlotSave(LegacyBytes, Out));
        TestEqual(TEXT("v0 detected as version 0"), static_cast<int32>(Out.Version), 0);
        TestEqual(TEXT("v0 crop path preserved"), Out.CropPath, FString(TEXT("/Game/Mythic/Farming/DA_Crop_Wheat.DA_Crop_Wheat")));
        TestEqual(TEXT("v0 stage preserved"), Out.Stage, 2);
        TestEqual(TEXT("v0 remaining preserved"), Out.RemainingSeconds, 42.5);
        TestEqual(TEXT("v0 → full moisture default"), static_cast<int32>(Out.MoistureQ), 255);
        TestFalse(TEXT("v0 → not withered"), Out.bWithered);
        TestEqual(TEXT("v0 → zero wet uptime"), Out.WetUptimeSeconds, 0.0f);
        TestEqual(TEXT("v0 → zero grow time"), Out.GrowTimeSeconds, 0.0f);
        TestEqual(TEXT("v0 → zero dry streak"), Out.DryStreakSeconds, 0.0f);
        TestTrue(TEXT("v0 → no fertilizer tag"), Out.FertilizerTagName.IsEmpty());
        TestTrue(TEXT("v0 → no grave essence tag"), Out.GraveEssenceTagName.IsEmpty());

        TArray<uint8> EmptyLegacy;
        {
            FMemoryWriter Writer(EmptyLegacy);
            FString CropPath;
            int32 Stage = -1;
            double RemainingSeconds = 0.0;
            Writer << CropPath;
            Writer << Stage;
            Writer << RemainingSeconds;
        }
        FMythicFarmPlotSaveData EmptyOut;
        TestTrue(TEXT("v0 empty-plot payload decodes"), Rules::DecodePlotSave(EmptyLegacy, EmptyOut));
        TestEqual(TEXT("v0 empty plot → stage -1"), EmptyOut.Stage, -1);
        TestTrue(TEXT("v0 empty plot → no crop"), EmptyOut.CropPath.IsEmpty());
    }

    {
        FMythicFarmPlotSaveData In;
        In.CropPath = TEXT("/Game/Mythic/Farming/DA_Crop_Pumpkin.DA_Crop_Pumpkin");
        In.Stage = 3;
        In.RemainingSeconds = 128.25;
        In.MoistureQ = 97;
        In.WetUptimeSeconds = 456.5f;
        In.GrowTimeSeconds = 900.0f;
        In.DryStreakSeconds = 12.0f;
        In.bWithered = true;
        In.FertilizerTagName = TEXT("Item.Fertilizer.Bonemeal");
        In.GraveEssenceTagName = TEXT("AI.Kind.Creature");

        TArray<uint8> Bytes;
        Rules::EncodePlotSave(In, Bytes);
        FMythicFarmPlotSaveData Out;
        TestTrue(TEXT("v1 payload decodes"), Rules::DecodePlotSave(Bytes, Out));
        TestEqual(TEXT("v1 version stamped"), static_cast<int32>(Out.Version), static_cast<int32>(Rules::SaveVersion));
        TestEqual(TEXT("v1 crop path round-trips"), Out.CropPath, In.CropPath);
        TestEqual(TEXT("v1 stage round-trips"), Out.Stage, In.Stage);
        TestEqual(TEXT("v1 remaining round-trips"), Out.RemainingSeconds, In.RemainingSeconds);
        TestEqual(TEXT("v1 moisture round-trips"), static_cast<int32>(Out.MoistureQ), static_cast<int32>(In.MoistureQ));
        TestEqual(TEXT("v1 wet uptime round-trips"), Out.WetUptimeSeconds, In.WetUptimeSeconds);
        TestEqual(TEXT("v1 grow time round-trips"), Out.GrowTimeSeconds, In.GrowTimeSeconds);
        TestEqual(TEXT("v1 dry streak round-trips"), Out.DryStreakSeconds, In.DryStreakSeconds);
        TestTrue(TEXT("v1 withered round-trips"), Out.bWithered);
        TestEqual(TEXT("v1 fertilizer tag round-trips"), Out.FertilizerTagName, In.FertilizerTagName);
        TestEqual(TEXT("v1 essence tag round-trips"), Out.GraveEssenceTagName, In.GraveEssenceTagName);

        TArray<uint8> Garbage = {0xFF, 0x01};
        FMythicFarmPlotSaveData GarbageOut;
        TestFalse(TEXT("garbage payload refuses"), Rules::DecodePlotSave(Garbage, GarbageOut));
        TestFalse(TEXT("empty payload refuses"), Rules::DecodePlotSave(TArray<uint8>(), GarbageOut));
    }

    return true;
}
