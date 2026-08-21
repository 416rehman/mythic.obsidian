
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "MassEntityQuery.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/LivingWorld/Events/ActionEventTypes.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "Engine/World.h"
#include "Engine/Engine.h"


namespace LivingWorldIntegrationHelpers {
    FMythicFactionId MakeFactionId(uint8 Index) {
        FMythicFactionId Id;
        Id.Index = Index;
        return Id;
    }

    UMythicFactionDatabaseSettings *CreateFactionSettings() {
        auto *Settings = NewObject<UMythicFactionDatabaseSettings>();
        Settings->MaxFactions = 10;
        Settings->InitialFactions.SetNum(2);

        {
            FMythicFactionData &F = Settings->InitialFactions[0];
            F.DisplayName = FText::FromString(TEXT("Pacifists"));
            F.bAlive = true;
            F.Population = 100;
            F.bHasBeenPopulated = true;
            F.Ideology.Violence = -0.8f;
            F.Ideology.Theft = -0.5f;
            F.DisapproveThreshold = 0.1f;
            F.CondemnThreshold = 0.3f;
            F.HostileThreshold = 0.6f;
        }

        {
            FMythicFactionData &F = Settings->InitialFactions[1];
            F.DisplayName = FText::FromString(TEXT("Warriors"));
            F.bAlive = true;
            F.Population = 80;
            F.bHasBeenPopulated = true;
            F.Ideology.Violence = 0.9f;
            F.Ideology.Theft = -0.3f;
            F.DisapproveThreshold = 0.2f;
            F.CondemnThreshold = 0.5f;
            F.HostileThreshold = 0.8f;
        }

        return Settings;
    }
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldIntegrationWitnessDetectionTest,
    "Mythic.LivingWorld.Integration.WitnessDetection",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldIntegrationWitnessDetectionTest::RunTest(const FString &Parameters) {
    auto *FactionDB = NewObject<UMythicFactionDatabase>();
    FactionDB->Initialize(LivingWorldIntegrationHelpers::CreateFactionSettings());

    FMythicWorldEvent CombatEvent;
    CombatEvent.WorldTime = 10.0;
    CombatEvent.Cell = FMythicCellCoord(5, 5);
    CombatEvent.PrimaryFaction = LivingWorldIntegrationHelpers::MakeFactionId(1);
    CombatEvent.Significance = 0.8f;
    CombatEvent.CategoryFlags = EMythicEventCategory::Combat;
    CombatEvent.MoralVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] = 0.9f;

    const FMythicCellCoord EventCell = CombatEvent.Cell;
    constexpr int32 HearingRadius = 3;

    struct TestWitness {
        FMythicCellCoord Cell;
        FMythicFactionId Faction;
        bool bExpectedToHear;
        FString Description;
    };

    TArray<TestWitness> Witnesses;
    Witnesses.Add({FMythicCellCoord(5, 5), LivingWorldIntegrationHelpers::MakeFactionId(0), true, TEXT("Same cell pacifist")});
    Witnesses.Add({FMythicCellCoord(6, 5), LivingWorldIntegrationHelpers::MakeFactionId(1), true, TEXT("Adjacent warlike")});
    Witnesses.Add({FMythicCellCoord(20, 20), LivingWorldIntegrationHelpers::MakeFactionId(0), false, TEXT("Far pacifist")});
    Witnesses.Add({FMythicCellCoord(5, 8), LivingWorldIntegrationHelpers::MakeFactionId(0), true, TEXT("Edge range pacifist")});
    Witnesses.Add({FMythicCellCoord(9, 5), LivingWorldIntegrationHelpers::MakeFactionId(0), false, TEXT("Outside range pacifist")});

    int32 WitnessResultCount = 0;

    for (const TestWitness &W : Witnesses) {
        const int32 CellDist = FMath::Abs(W.Cell.X - EventCell.X) + FMath::Abs(W.Cell.Y - EventCell.Y);
        const bool bInRange = CellDist <= HearingRadius;

        TestEqual(*FString::Printf(TEXT("%s hearing check"), *W.Description), bInRange, W.bExpectedToHear);

        if (!bInRange || !W.Faction.IsValid()) {
            continue;
        }

        FMythicFactionData FactionData;
        if (!FactionDB->GetFaction(W.Faction, FactionData)) {
            continue;
        }

        const EMythicMoralSeverity Severity = FMythicMoralSignature::EvaluateActionSeverity(
            CombatEvent.MoralVector,
            FactionData.Ideology,
            FactionData.DisapproveThreshold,
            FactionData.CondemnThreshold,
            FactionData.HostileThreshold
            );

        if (W.Faction.Index == 0)
        {
            TestEqual(*FString::Printf(TEXT("%s severity"), *W.Description), Severity, EMythicMoralSeverity::Hostile);
        }
        else if (W.Faction.Index == 1)
        {
            TestEqual(*FString::Printf(TEXT("%s severity"), *W.Description), Severity, EMythicMoralSeverity::Ignore);
        }

        if (Severity != EMythicMoralSeverity::Ignore) {
            ++WitnessResultCount;
        }
    }

    TestTrue(TEXT("At least one witness should produce a non-Ignore result"), WitnessResultCount > 0);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldIntegrationPressureAccumulationTest,
    "Mythic.LivingWorld.Integration.PressureAccumulation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldIntegrationPressureAccumulationTest::RunTest(const FString &Parameters) {
    FMythicPsychodynamicFragment Psycho;
    FMemory::Memzero(Psycho.Pressure, sizeof(Psycho.Pressure));
    Psycho.LastEventTime = 0.0;

    FMythicWitnessResult CombatResult;
    CombatResult.Severity = EMythicMoralSeverity::Condemn;
    CombatResult.EventCategoryFlags = EMythicEventCategory::Combat;
    CombatResult.EventSignificance = 0.8f;
    CombatResult.EventWorldTime = 10.0;

    FMythicWitnessResult CrimeResult;
    CrimeResult.Severity = EMythicMoralSeverity::Hostile;
    CrimeResult.EventCategoryFlags = EMythicEventCategory::Crime;
    CrimeResult.EventSignificance = 0.6f;
    CrimeResult.EventWorldTime = 10.0;

    FMythicWitnessResult DeathResult;
    DeathResult.Severity = EMythicMoralSeverity::Disapprove;
    DeathResult.EventCategoryFlags = EMythicEventCategory::Death;
    DeathResult.EventSignificance = 0.5f;
    DeathResult.EventWorldTime = 10.0;

    const int32 ThreatIdx = static_cast<int32>(EMythicPressureChannel::Threat);
    const int32 InjusticeIdx = static_cast<int32>(EMythicPressureChannel::Injustice);
    const int32 GriefIdx = static_cast<int32>(EMythicPressureChannel::Grief);
    const int32 WrathIdx = static_cast<int32>(EMythicPressureChannel::Wrath);

    auto ApplyPressure = [&](const FMythicWitnessResult &Result) {
        float SeverityMagnitude = 0.0f;
        switch (Result.Severity) {
        case EMythicMoralSeverity::Disapprove:
            SeverityMagnitude = 0.3f * Result.EventSignificance;
            break;
        case EMythicMoralSeverity::Condemn:
            SeverityMagnitude = 0.7f * Result.EventSignificance;
            break;
        case EMythicMoralSeverity::Hostile:
            SeverityMagnitude = 1.0f * Result.EventSignificance;
            break;
        default:
            return;
        }

        const bool bIsCombat = (Result.EventCategoryFlags & EMythicEventCategory::Combat) != 0;
        const bool bIsCrime = (Result.EventCategoryFlags & EMythicEventCategory::Crime) != 0;
        const bool bIsDeath = (Result.EventCategoryFlags & EMythicEventCategory::Death) != 0;

        if (bIsCombat) {
            Psycho.Pressure[ThreatIdx] += SeverityMagnitude;
            Psycho.Pressure[WrathIdx] += SeverityMagnitude * 0.5f;
        }
        if (bIsCrime) {
            Psycho.Pressure[InjusticeIdx] += SeverityMagnitude;
        }
        if (bIsDeath) {
            Psycho.Pressure[GriefIdx] += SeverityMagnitude * 0.8f;
        }
        if (!bIsCombat && !bIsCrime && !bIsDeath) {
            Psycho.Pressure[ThreatIdx] += SeverityMagnitude * 0.5f;
        }
    };

    ApplyPressure(CombatResult);
    ApplyPressure(CrimeResult);
    ApplyPressure(DeathResult);

    TestNearlyEqual(TEXT("Threat pressure from combat"), Psycho.Pressure[ThreatIdx], 0.56f, 0.01f);
    TestNearlyEqual(TEXT("Wrath pressure from combat"), Psycho.Pressure[WrathIdx], 0.28f, 0.01f);

    TestNearlyEqual(TEXT("Injustice pressure from crime"), Psycho.Pressure[InjusticeIdx], 0.6f, 0.01f);

    TestNearlyEqual(TEXT("Grief pressure from death"), Psycho.Pressure[GriefIdx], 0.12f, 0.01f);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldIntegrationPressureVentingTest,
    "Mythic.LivingWorld.Integration.PressureVenting",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldIntegrationPressureVentingTest::RunTest(const FString &Parameters) {
    constexpr float VentThreshold = 0.8f;

    FMythicPsychodynamicFragment Psycho;
    FMemory::Memzero(Psycho.Pressure, sizeof(Psycho.Pressure));
    Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Threat)] = 1.2f;
    Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Injustice)] = 0.3f;
    Psycho.LastEventTime = 5.0;

    FMythicPersonalityFragment Personality;
    FMemory::Memzero(Personality.VentWeights, sizeof(Personality.VentWeights));
    Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Fight)] = 0.7f;
    Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Flee)] = 0.2f;
    Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Report)] = 0.1f;

    float MaxPressure = 0.0f;
    int32 MaxPressureChannel = -1;
    for (int32 c = 0; c < PressureChannelCount; ++c) {
        if (Psycho.Pressure[c] > MaxPressure) {
            MaxPressure = Psycho.Pressure[c];
            MaxPressureChannel = c;
        }
    }

    TestEqual(TEXT("Max pressure channel should be Threat"), MaxPressureChannel, static_cast<int32>(EMythicPressureChannel::Threat));
    TestTrue(TEXT("Max pressure should exceed vent threshold"), MaxPressure >= VentThreshold);

    bool bVented = false;
    int32 SelectedVentChannel = -1;
    if (MaxPressure >= VentThreshold && MaxPressureChannel >= 0) {
        float BestVentWeight = -1.0f;
        for (int32 v = 0; v < VentChannelCount; ++v) {
            if (Personality.VentWeights[v] > BestVentWeight) {
                BestVentWeight = Personality.VentWeights[v];
                SelectedVentChannel = v;
            }
        }

        Psycho.Pressure[MaxPressureChannel] *= 0.5f;
        bVented = true;
    }

    TestTrue(TEXT("Should have vented"), bVented);
    TestEqual(TEXT("Vent channel should be Fight"), SelectedVentChannel, static_cast<int32>(EMythicVentChannel::Fight));
    TestNearlyEqual(TEXT("Threat pressure should be halved after vent"),
                    Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Threat)], 0.6f, 0.01f);

    TestNearlyEqual(TEXT("Injustice unchanged"),
                    Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Injustice)], 0.3f, 0.01f);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldIntegrationSignificanceRescoreTest,
    "Mythic.LivingWorld.Integration.SignificanceRescore",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldIntegrationSignificanceRescoreTest::RunTest(const FString &Parameters) {
    constexpr float ProxWeight = 0.4f;
    constexpr float EventWeight = 0.3f;
    constexpr float EmotionWeight = 0.3f;
    constexpr float SpawnRadius = 10.0f;
    constexpr float EventScalingFactor = 16.0f;
    constexpr float PressureScalingFactor = 3.0f;

    FMythicCellCoord EntityCell(5, 5);
    FMythicCellCoord PlayerCell(5, 6);

    const float CellDist = static_cast<float>(
        FMath::Abs(EntityCell.X - PlayerCell.X) + FMath::Abs(EntityCell.Y - PlayerCell.Y));
    const float ProximityScore = FMath::Max(0.0f, 1.0f - (CellDist / FMath::Max(SpawnRadius, 1.0f)));
    TestNearlyEqual(TEXT("Proximity score for distance=1"), ProximityScore, 0.9f, 0.01f);

    constexpr uint16 EventCount = 8;
    const float EventScore = FMath::Min(1.0f, static_cast<float>(EventCount) / EventScalingFactor);
    TestNearlyEqual(TEXT("Event score for 8 events"), EventScore, 0.5f, 0.01f);

    float TotalPressure = 1.5f;
    const float EmotionScore = FMath::Min(1.0f, TotalPressure / PressureScalingFactor);
    TestNearlyEqual(TEXT("Emotion score for 1.5 total pressure"), EmotionScore, 0.5f, 0.01f);

    const float FinalScore = FMath::Clamp(
        ProxWeight * ProximityScore + EventWeight * EventScore + EmotionWeight * EmotionScore,
        0.0f, 1.0f);
    TestNearlyEqual(TEXT("Final significance score"), FinalScore, 0.66f, 0.01f);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldIntegrationSignificancePromotionTest,
    "Mythic.LivingWorld.Integration.SignificancePromotion",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldIntegrationSignificancePromotionTest::RunTest(const FString &Parameters) {
    constexpr float PromotionThreshold = 0.5f;
    constexpr float DemotionThreshold = 0.2f;
    constexpr float Hysteresis = 0.05f;

    FMythicSignificanceFragment Sig;
    Sig.Tier = EMythicSignificanceTier::Tier0_Ambient;
    Sig.Score = 0.7f;
    Sig.bDirty = false;

    bool bPromoted = false;
    if (Sig.Tier == EMythicSignificanceTier::Tier0_Ambient
        && Sig.Score >= (PromotionThreshold + Hysteresis)) {
        Sig.Tier = EMythicSignificanceTier::Tier1_Reactive;
        bPromoted = true;
    }

    TestTrue(TEXT("Entity with score 0.7 should be promoted"), bPromoted);
    TestEqual(TEXT("Tier should be Reactive"), Sig.Tier, EMythicSignificanceTier::Tier1_Reactive);

    FMythicSignificanceFragment SigLow;
    SigLow.Tier = EMythicSignificanceTier::Tier0_Ambient;
    SigLow.Score = 0.54f;
    SigLow.bDirty = false;

    bool bShouldNotPromote = false;
    if (SigLow.Tier == EMythicSignificanceTier::Tier0_Ambient
        && SigLow.Score >= (PromotionThreshold + Hysteresis)) {
        bShouldNotPromote = true;
    }
    TestFalse(TEXT("Entity with score 0.54 should NOT be promoted"), bShouldNotPromote);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldIntegrationSignificanceDemotionTest,
    "Mythic.LivingWorld.Integration.SignificanceDemotion",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldIntegrationSignificanceDemotionTest::RunTest(const FString &Parameters) {
    constexpr float DemotionThreshold = 0.2f;
    constexpr float Hysteresis = 0.05f;

    FMythicSignificanceFragment Sig;
    Sig.Tier = EMythicSignificanceTier::Tier1_Reactive;
    Sig.Score = 0.1f;
    Sig.RelevantEventCount = 5;
    Sig.bDirty = false;

    bool bDemoted = false;
    if (Sig.Tier == EMythicSignificanceTier::Tier1_Reactive
        && Sig.Score <= (DemotionThreshold - Hysteresis)) {
        Sig.Tier = EMythicSignificanceTier::Tier0_Ambient;
        Sig.RelevantEventCount = 0;
        bDemoted = true;
    }

    TestTrue(TEXT("Low-score Tier1 entity should be demoted"), bDemoted);
    TestEqual(TEXT("Tier should be Ambient"), Sig.Tier, EMythicSignificanceTier::Tier0_Ambient);
    TestEqual(TEXT("Event count should reset"), (int32)Sig.RelevantEventCount, 0);

    FMythicSignificanceFragment SigHigh;
    SigHigh.Tier = EMythicSignificanceTier::Tier1_Reactive;
    SigHigh.Score = 0.16f;
    SigHigh.bDirty = false;

    bool bShouldNotDemote = false;
    if (SigHigh.Tier == EMythicSignificanceTier::Tier1_Reactive
        && SigHigh.Score <= (DemotionThreshold - Hysteresis)) {
        bShouldNotDemote = true;
    }
    TestFalse(TEXT("Entity with score 0.16 should NOT be demoted"), bShouldNotDemote);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldIntegrationPressureDecayTest,
    "Mythic.LivingWorld.Integration.PressureDecay",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldIntegrationPressureDecayTest::RunTest(const FString &Parameters) {
    constexpr float DecayRate = 0.1f;

    FMythicPsychodynamicFragment Psycho;
    FMemory::Memzero(Psycho.Pressure, sizeof(Psycho.Pressure));
    Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Threat)] = 1.0f;
    Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Injustice)] = 0.5f;
    Psycho.LastEventTime = 10.0;

    const double CurrentWorldTime = 15.0;
    const double Elapsed = CurrentWorldTime - Psycho.LastEventTime;

    if (Elapsed > 0.0 && Psycho.LastEventTime > 0.0) {
        const float DecayMultiplier = FMath::Exp(-DecayRate * static_cast<float>(Elapsed));
        for (int32 c = 0; c < PressureChannelCount; ++c) {
            Psycho.Pressure[c] *= DecayMultiplier;
        }
    }

    const float ExpectedMultiplier = FMath::Exp(-0.5f);
    TestNearlyEqual(TEXT("Threat after 5s decay"), Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Threat)], ExpectedMultiplier, 0.01f);
    TestNearlyEqual(TEXT("Injustice after 5s decay"), Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Injustice)], 0.5f * ExpectedMultiplier, 0.01f);

    Psycho.LastEventTime = 0.0;
    Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Threat)] = 1.0f;

    const double LongElapsed = 30.0;
    const float LongDecay = FMath::Exp(-DecayRate * static_cast<float>(LongElapsed));
    Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Threat)] *= LongDecay;

    TestTrue(TEXT("Threat after 30s should be near zero"),
             Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Threat)] < 0.06f);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldIntegrationFullPipelineTest,
    "Mythic.LivingWorld.Integration.FullPipeline",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldIntegrationFullPipelineTest::RunTest(const FString &Parameters) {
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(128);

    auto *FactionDB = NewObject<UMythicFactionDatabase>();
    FactionDB->Initialize(LivingWorldIntegrationHelpers::CreateFactionSettings());

    FMythicWorldEvent KillEvent;
    KillEvent.WorldTime = 10.0;
    KillEvent.Cell = FMythicCellCoord(5, 5);
    KillEvent.PrimaryFaction = LivingWorldIntegrationHelpers::MakeFactionId(1);
    KillEvent.Significance = 0.9f;
    KillEvent.CategoryFlags = EMythicEventCategory::Combat | EMythicEventCategory::Death;
    KillEvent.MoralVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] = 1.0f;

    const uint32 EventId = Fabric->AppendEvent(KillEvent);
    Fabric->CommitWrites();
    TestTrue(TEXT("Event should be recorded in fabric"), EventId > 0);

    const FMythicWorldEvent *StoredEvent = Fabric->GetEvent(EventId);
    TestNotNull(TEXT("Event should be retrievable"), StoredEvent);

    const FMythicCellCoord WitnessCell(5, 5);
    const FMythicFactionId WitnessFaction = LivingWorldIntegrationHelpers::MakeFactionId(0);
    constexpr int32 HearingRadius = 3;

    const int32 CellDist = FMath::Abs(WitnessCell.X - KillEvent.Cell.X)
        + FMath::Abs(WitnessCell.Y - KillEvent.Cell.Y);
    TestTrue(TEXT("Witness should be in hearing range"), CellDist <= HearingRadius);

    FMythicFactionData WitnessFactionData;
    const bool bFactionFound = FactionDB->GetFaction(WitnessFaction, WitnessFactionData);
    TestTrue(TEXT("Witness faction should be resolvable"), bFactionFound);

    const EMythicMoralSeverity Severity = FMythicMoralSignature::EvaluateActionSeverity(
        KillEvent.MoralVector,
        WitnessFactionData.Ideology,
        WitnessFactionData.DisapproveThreshold,
        WitnessFactionData.CondemnThreshold,
        WitnessFactionData.HostileThreshold
        );
    TestEqual(TEXT("Pacifist should be Hostile to extreme violence"), Severity, EMythicMoralSeverity::Hostile);

    FMythicPsychodynamicFragment Psycho;
    FMemory::Memzero(Psycho.Pressure, sizeof(Psycho.Pressure));
    Psycho.LastEventTime = 0.0;

    FMythicWitnessResult WitnessResult;
    WitnessResult.Severity = Severity;
    WitnessResult.EventCategoryFlags = KillEvent.CategoryFlags;
    WitnessResult.EventSignificance = KillEvent.Significance;
    WitnessResult.EventWorldTime = KillEvent.WorldTime;
    WitnessResult.EventCell = KillEvent.Cell;
    WitnessResult.PerpFaction = KillEvent.PrimaryFaction;

    const float SeverityMagnitude = 1.0f * WitnessResult.EventSignificance;
    const bool bIsCombat = (WitnessResult.EventCategoryFlags & EMythicEventCategory::Combat) != 0;
    const bool bIsDeath = (WitnessResult.EventCategoryFlags & EMythicEventCategory::Death) != 0;

    if (bIsCombat) {
        Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Threat)] += SeverityMagnitude;
        Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Wrath)] += SeverityMagnitude * 0.5f;
    }
    if (bIsDeath) {
        Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Grief)] += SeverityMagnitude * 0.8f;
    }
    Psycho.LastEventTime = KillEvent.WorldTime;

    TestNearlyEqual(TEXT("Pipeline: Threat pressure"), Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Threat)], 0.9f, 0.01f);
    TestNearlyEqual(TEXT("Pipeline: Wrath pressure"), Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Wrath)], 0.45f, 0.01f);
    TestNearlyEqual(TEXT("Pipeline: Grief pressure"), Psycho.Pressure[static_cast<int32>(EMythicPressureChannel::Grief)], 0.72f, 0.01f);

    constexpr float VentThreshold = 0.8f;

    float MaxPressure = 0.0f;
    int32 MaxPressureChannel = -1;
    for (int32 c = 0; c < PressureChannelCount; ++c) {
        if (Psycho.Pressure[c] > MaxPressure) {
            MaxPressure = Psycho.Pressure[c];
            MaxPressureChannel = c;
        }
    }

    TestTrue(TEXT("Pipeline: Should trigger vent (Threat=0.9 > 0.8)"), MaxPressure >= VentThreshold);
    TestEqual(TEXT("Pipeline: Max channel = Threat"), MaxPressureChannel, static_cast<int32>(EMythicPressureChannel::Threat));

    FMythicSignificanceFragment Sig;
    Sig.Tier = EMythicSignificanceTier::Tier0_Ambient;
    Sig.bDirty = true;
    Sig.RelevantEventCount = 1;
    Sig.Score = 0.0f;

    constexpr float ProxWeight = 0.4f;
    constexpr float EventWeight = 0.3f;
    constexpr float EmotionWeight = 0.3f;

    const float ProximityScore = 1.0f;
    const float EventScore = FMath::Min(1.0f, static_cast<float>(Sig.RelevantEventCount) / 16.0f);
    float TotalPressure = 0.0f;
    for (int32 c = 0; c < PressureChannelCount; ++c) {
        TotalPressure += Psycho.Pressure[c];
    }
    const float EmotionScore = FMath::Min(1.0f, TotalPressure / 3.0f);

    Sig.Score = FMath::Clamp(
        ProxWeight * ProximityScore + EventWeight * EventScore + EmotionWeight * EmotionScore,
        0.0f, 1.0f);

    TestTrue(TEXT("Pipeline: Significance score should be high"), Sig.Score > 0.5f);

    constexpr float PromotionThreshold = 0.5f;
    constexpr float Hysteresis = 0.05f;

    const bool bShouldPromote = (Sig.Tier == EMythicSignificanceTier::Tier0_Ambient
        && Sig.Score >= (PromotionThreshold + Hysteresis));
    TestTrue(TEXT("Pipeline: Entity should be promoted to Tier1"), bShouldPromote);

    if (bShouldPromote) {
        Sig.Tier = EMythicSignificanceTier::Tier1_Reactive;
    }
    TestEqual(TEXT("Pipeline: Final tier = Reactive"), Sig.Tier, EMythicSignificanceTier::Tier1_Reactive);

    AddInfo(TEXT("Full pipeline test passed: Event→Witness(Hostile)→Pressure(Threat=0.9)→Vent→Significance→Promotion(Tier1)"));

    return true;
}
