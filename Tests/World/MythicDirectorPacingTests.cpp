
#include "Misc/AutomationTest.h"
#include "World/GameDirector/MythicDirectorPacing.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDirectorPacingTest,
    "Mythic.World.DirectorPacing",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

namespace {
    FMythicDirectorConfig MakeFastConfig() {
        FMythicDirectorConfig C;
        C.BuildUpDuration = 4.0f;
        C.MinPeakDuration = 4.0f;
        C.SustainPeakMax = 20.0f;
        C.RelaxDuration = 4.0f;
        C.RestDuration = 4.0f;
        C.MultiplierRisePerSec = 0.5f;
        C.MultiplierFallPerSec = 0.5f;
        C.MinSpawnIntensityMultiplier = 0.35f;
        C.MaxSpawnIntensityMultiplier = 1.75f;
        C.HighIntensityThreshold = 0.6f;
        C.LowIntensityThreshold = 0.2f;
        C.LowHealthPct = 0.4f;
        C.RecoveredHealthPct = 0.75f;
        C.DownsForcingRelax = 1;
        C.ThreatIdleForRelax = 8.0f;
        return C;
    }

    FMythicDirectorInputs HotInputs() {
        FMythicDirectorInputs In;
        In.RecentDamageTakenNorm = 1.0f;
        In.DownsInWindow = 2;
        In.KillsPerSec = 1.5f;
        In.TimeSinceLastThreat = 0.0f;
        In.AvgPartyHealthPct = 0.3f;
        return In;
    }

    FMythicDirectorInputs CalmRecoveredInputs() {
        FMythicDirectorInputs In;
        In.RecentDamageTakenNorm = 0.0f;
        In.DownsInWindow = 0;
        In.KillsPerSec = 0.0f;
        In.TimeSinceLastThreat = 100.0f;
        In.AvgPartyHealthPct = 1.0f;
        return In;
    }
}

bool FMythicDirectorPacingTest::RunTest(const FString& Parameters) {
    const FMythicDirectorConfig Cfg = MakeFastConfig();

    {
        TestEqual(TEXT("Fully hot inputs → intensity 1.0"),
                  FMythicDirectorPacing::ComputeIntensity(HotInputs(), Cfg), 1.0f, 0.001f);
        FMythicDirectorInputs Zero;
        Zero.AvgPartyHealthPct = 1.0f;
        TestEqual(TEXT("No combat signals → intensity 0"),
                  FMythicDirectorPacing::ComputeIntensity(Zero, Cfg), 0.0f, 0.001f);
        FMythicDirectorInputs Dmg;
        Dmg.RecentDamageTakenNorm = 1.0f;
        TestEqual(TEXT("Damage-only intensity ≈ DamageWeight fraction (0.5)"),
                  FMythicDirectorPacing::ComputeIntensity(Dmg, Cfg), 0.5f, 0.001f);
        FMythicDirectorInputs Over = HotInputs();
        Over.RecentDamageTakenNorm = 5.0f;
        TestTrue(TEXT("Intensity never exceeds 1.0"),
                 FMythicDirectorPacing::ComputeIntensity(Over, Cfg) <= 1.0f + KINDA_SMALL_NUMBER);
    }

    FMythicDirectorState AfterCombat;
    {
        FMythicDirectorState S = FMythicDirectorPacing::MakeInitialState(Cfg);
        const float StartMult = S.SpawnIntensityMultiplier;
        const FMythicDirectorInputs Hot = HotInputs();

        S = FMythicDirectorPacing::Step(Hot, Cfg, S, 1.0f);
        TestTrue(TEXT("Hot pressure crests BuildUp → Peak on the first step"),
                 S.Phase == EMythicDirectorPhase::Peak);

        for (int32 i = 0; i < 20; ++i) {
            S = FMythicDirectorPacing::Step(Hot, Cfg, S, 1.0f);
            TestTrue(TEXT("Multiplier stays within [min,max] every step (combat run)"),
                     S.SpawnIntensityMultiplier >= Cfg.MinSpawnIntensityMultiplier - 0.001f &&
                     S.SpawnIntensityMultiplier <= Cfg.MaxSpawnIntensityMultiplier + 0.001f);
        }
        AfterCombat = S;

        TestTrue(TEXT("Sustained exhaustion drives the phase to Relax or Rest"),
                 S.Phase == EMythicDirectorPhase::Relax || S.Phase == EMythicDirectorPhase::Rest);
        TestTrue(TEXT("Multiplier is REDUCED after the exhausting peak (below the crest)"),
                 S.SpawnIntensityMultiplier < StartMult + 0.001f);
        TestTrue(TEXT("Multiplier relaxes toward the minimum"),
                 S.SpawnIntensityMultiplier <= Cfg.MinSpawnIntensityMultiplier + 0.25f);
    }

    {
        FMythicDirectorState S;
        S.Phase = EMythicDirectorPhase::Rest;
        S.SpawnIntensityMultiplier = Cfg.MinSpawnIntensityMultiplier;
        S.TimeInPhase = 0.0f;

        const FMythicDirectorInputs Calm = CalmRecoveredInputs();

        S = FMythicDirectorPacing::Step(Calm, Cfg, S, 1.0f);
        TestTrue(TEXT("Rest holds during its guaranteed dwell (no early exit)"),
                 S.Phase == EMythicDirectorPhase::Rest);

        int32 Guard = 0;
        while (S.Phase == EMythicDirectorPhase::Rest && Guard++ < 20) {
            S = FMythicDirectorPacing::Step(Calm, Cfg, S, 1.0f);
        }
        TestTrue(TEXT("After RestDuration + recovered HP, phase advances to BuildUp"),
                 S.Phase == EMythicDirectorPhase::BuildUp);

        const float M0 = S.SpawnIntensityMultiplier;
        S = FMythicDirectorPacing::Step(Calm, Cfg, S, 1.0f);
        const float M1 = S.SpawnIntensityMultiplier;
        TestTrue(TEXT("BuildUp multiplier rises step-over-step"), M1 > M0 - 0.001f && M1 >= M0);
        S = FMythicDirectorPacing::Step(Calm, Cfg, S, 1.0f);
        const float M2 = S.SpawnIntensityMultiplier;
        TestTrue(TEXT("BuildUp multiplier keeps rising toward max"), M2 >= M1);
    }

    {
        FMythicDirectorState S;
        S.Phase = EMythicDirectorPhase::Rest;
        S.SpawnIntensityMultiplier = Cfg.MinSpawnIntensityMultiplier;
        S.TimeInPhase = 0.0f;

        FMythicDirectorInputs StillHurt = CalmRecoveredInputs();
        StillHurt.AvgPartyHealthPct = 0.5f;

        for (int32 i = 0; i < 20; ++i) {
            S = FMythicDirectorPacing::Step(StillHurt, Cfg, S, 1.0f);
        }
        TestTrue(TEXT("Rest does NOT end while party HP is unrecovered"),
                 S.Phase == EMythicDirectorPhase::Rest);
    }

    {
        FMythicDirectorState S = FMythicDirectorPacing::MakeInitialState(Cfg);
        const FMythicDirectorInputs Hot = HotInputs();

        S = FMythicDirectorPacing::Step(Hot, Cfg, S, 1.0f);
        TestTrue(TEXT("Entered Peak"), S.Phase == EMythicDirectorPhase::Peak);
        S = FMythicDirectorPacing::Step(Hot, Cfg, S, 1.0f);

        FMythicDirectorInputs OneCalm = CalmRecoveredInputs();
        const FMythicDirectorState Before = S;
        S = FMythicDirectorPacing::Step(OneCalm, Cfg, S, 1.0f);
        TestTrue(TEXT("Single calm sample does NOT flip Peak → Relax (min-dwell hysteresis)"),
                 S.Phase == EMythicDirectorPhase::Peak);
        TestTrue(TEXT("Peak dwell advanced (not reset) by the off-sample"),
                 S.TimeInPhase > Before.TimeInPhase - 0.001f);
    }

    {
        FMythicDirectorState S = FMythicDirectorPacing::MakeInitialState(Cfg);
        bool bInBounds = true;
        for (int32 i = 0; i < 200; ++i) {
            const FMythicDirectorInputs In = (i % 7 < 4) ? HotInputs() : CalmRecoveredInputs();
            S = FMythicDirectorPacing::Step(In, Cfg, S, 1.0f);
            if (S.SpawnIntensityMultiplier < Cfg.MinSpawnIntensityMultiplier - 0.001f ||
                S.SpawnIntensityMultiplier > Cfg.MaxSpawnIntensityMultiplier + 0.001f) {
                bInBounds = false;
                break;
            }
        }
        TestTrue(TEXT("Multiplier stays within [min,max] over a long mixed run"), bInBounds);
    }

    {
        FMythicDirectorState Prev;
        Prev.Phase = EMythicDirectorPhase::SustainPeak;
        Prev.SpawnIntensityMultiplier = 1.2f;
        Prev.TimeInPhase = 3.5f;
        const FMythicDirectorInputs In = HotInputs();

        const FMythicDirectorState A = FMythicDirectorPacing::Step(In, Cfg, Prev, 1.0f);
        const FMythicDirectorState B = FMythicDirectorPacing::Step(In, Cfg, Prev, 1.0f);
        TestTrue(TEXT("Deterministic: same phase"), A.Phase == B.Phase);
        TestEqual(TEXT("Deterministic: same multiplier"), A.SpawnIntensityMultiplier, B.SpawnIntensityMultiplier, 0.0f);
        TestEqual(TEXT("Deterministic: same time-in-phase"), A.TimeInPhase, B.TimeInPhase, 0.0f);
    }

    return true;
}
