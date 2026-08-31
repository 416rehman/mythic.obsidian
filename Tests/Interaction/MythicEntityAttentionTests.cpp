#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/Attention/MythicEntityAttentionRules.h"
#include "UObject/UnrealType.h"
#include "World/Entity/MythicEntityId.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEntityAttentionScoreTest,
    "Mythic.Interaction.EntityAttention.ScorePriority",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicEntityAttentionScoreTest::RunTest(
    const FString &Parameters) {
    (void)Parameters;
    const FMythicEntityAttentionConfig Config;

    FMythicEntityAttentionScoreInput Input;
    Input.DistanceCentimeters = 1200.0f;
    Input.ViewAlignment = 0.95f;
    Input.bOnScreen = true;
    Input.bHasLineOfSight = true;
    const float AmbientScore =
        FMythicEntityAttentionRules::CalculateScore(Input, Config);

    Input.PriorityClass = EMythicEntityAttentionPriorityClass::Awareness;
    const float AwarenessScore =
        FMythicEntityAttentionRules::CalculateScore(Input, Config);
    Input.PriorityClass = EMythicEntityAttentionPriorityClass::Opportunity;
    const float OpportunityScore =
        FMythicEntityAttentionRules::CalculateScore(Input, Config);
    Input.PriorityClass = EMythicEntityAttentionPriorityClass::Safety;
    const float SafetyScore =
        FMythicEntityAttentionRules::CalculateScore(Input, Config);

    TestTrue(TEXT("awareness outranks identical ambient evidence"),
             AwarenessScore > AmbientScore);
    TestTrue(TEXT("opportunity outranks identical awareness evidence"),
             OpportunityScore > AwarenessScore);
    TestTrue(TEXT("safety outranks identical opportunity evidence"),
             SafetyScore > OpportunityScore);

    Input.bInteractionTarget = true;
    const float InteractionScore =
        FMythicEntityAttentionRules::CalculateScore(Input, Config);
    Input.bHardTarget = true;
    const float HardTargetScore =
        FMythicEntityAttentionRules::CalculateScore(Input, Config);
    Input.bInspectTarget = true;
    const float InspectScore =
        FMythicEntityAttentionRules::CalculateScore(Input, Config);
    TestTrue(TEXT("hard target dominates interaction override"),
             HardTargetScore > InteractionScore);
    TestTrue(TEXT("inspect target is the strongest explicit owner"),
             InspectScore > HardTargetScore);
    TestTrue(TEXT("any explicit owner bypasses ordinary focus dwell"),
             FMythicEntityAttentionRules::IsForcedFocus(Input));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEntityAttentionHysteresisTest,
    "Mythic.Interaction.EntityAttention.Hysteresis",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicEntityAttentionHysteresisTest::RunTest(
    const FString &Parameters) {
    (void)Parameters;
    const FMythicEntityAttentionConfig Config;
    constexpr float BoundaryEpsilon = 0.001f;

    TestFalse(TEXT("ordinary focus waits below its acquire boundary"),
              FMythicEntityAttentionRules::CanAcquireFocus(
                  FMath::Max(0.0f, Config.FocusAcquireDwellSeconds
                                       - BoundaryEpsilon),
                  false, Config));
    TestTrue(TEXT("ordinary focus acquires at its dwell boundary"),
             FMythicEntityAttentionRules::CanAcquireFocus(
                 Config.FocusAcquireDwellSeconds, false, Config));
    TestTrue(TEXT("explicit focus bypasses acquire dwell"),
             FMythicEntityAttentionRules::CanAcquireFocus(0.0f, true,
                                                          Config));
    TestTrue(TEXT("incumbent survives just inside release grace"),
             FMythicEntityAttentionRules::CanRetainFocus(
                 FMath::Max(0.0f, Config.FocusReleaseGraceSeconds
                                      - BoundaryEpsilon),
                 Config));
    TestFalse(TEXT("incumbent releases after grace"),
              FMythicEntityAttentionRules::CanRetainFocus(
                  Config.FocusReleaseGraceSeconds + BoundaryEpsilon,
                  Config));

    TestFalse(TEXT("a same-score challenger never churns focus"),
              FMythicEntityAttentionRules::ShouldReplaceFocus(
                  100.0f, 100.0f, 1.0f, false, Config));
    TestFalse(TEXT("a stronger challenger still waits for dwell"),
              FMythicEntityAttentionRules::ShouldReplaceFocus(
                  100.0f, 115.0f,
                  FMath::Max(0.0f, Config.ReplacementDwellSeconds
                                       - BoundaryEpsilon),
                  false, Config));
    TestTrue(TEXT("exact advantage and dwell boundaries replace"),
             FMythicEntityAttentionRules::ShouldReplaceFocus(
                 100.0f, 115.0f, Config.ReplacementDwellSeconds,
                 false, Config));
    TestTrue(TEXT("explicit challenger preempts immediately"),
             FMythicEntityAttentionRules::ShouldReplaceFocus(
                 10000.0f, 1.0f, 0.0f, true, Config));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEntityAttentionBudgetTest,
    "Mythic.Interaction.EntityAttention.HardBudgets",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicEntityAttentionBudgetTest::RunTest(
    const FString &Parameters) {
    (void)Parameters;
    FMythicEntityAttentionTraceBudget Budget(8);
    for (int32 Index = 0; Index < 8; ++Index) {
        TestTrue(FString::Printf(TEXT("trace %d remains in budget"),
                                Index + 1),
                 Budget.TryConsume());
    }
    TestFalse(TEXT("ninth trace fails closed"), Budget.TryConsume());
    TestEqual(TEXT("budget records exactly eight traces"), Budget.Used,
              8);

    FMythicEntityAttentionConfig Unsafe;
    Unsafe.MaxLineOfSightTracesPerPass = 99;
    Unsafe.MaxSpatialCandidatesPerPass = 999;
    Unsafe.MaxRetainedEventSubjects = 999;
    Unsafe.MaxRegistryFallbackChecksPerPass = 999;
    Unsafe.MaxPublishedObservations = 99;
    Unsafe.MaxEvaluatedCandidates = 1;
    Unsafe.DecisionRateHz = 1000.0f;
    Unsafe.FocusMinimumViewDot = -1.0f;
    Unsafe.GazeMinimumViewDot = 0.5f;
    Unsafe.GazeReleaseMinimumViewDot = 0.75f;
    const FMythicEntityAttentionConfig Sanitized =
        FMythicEntityAttentionRules::SanitizeConfig(Unsafe);
    TestEqual(TEXT("runtime can never exceed eight LOS traces"),
              Sanitized.MaxLineOfSightTracesPerPass, 8);
    TestEqual(TEXT("spatial broadphase consumption is hard capped"),
              Sanitized.MaxSpatialCandidatesPerPass, 256);
    TestEqual(TEXT("event subject retention is hard capped"),
              Sanitized.MaxRetainedEventSubjects, 128);
    TestEqual(TEXT("registry fallback work is hard capped"),
              Sanitized.MaxRegistryFallbackChecksPerPass, 64);
    TestEqual(TEXT("runtime publishes no more than pool capacity"),
              Sanitized.MaxPublishedObservations, 16);
    TestTrue(TEXT("evaluated set always covers published set"),
             Sanitized.MaxEvaluatedCandidates
                 >= Sanitized.MaxPublishedObservations);
    TestEqual(TEXT("decision cadence is capped against scan storms"),
              Sanitized.DecisionRateHz, 10.0f);
    TestTrue(TEXT("focus cone cannot be wider than gaze cone"),
             Sanitized.FocusMinimumViewDot
                  >= Sanitized.GazeMinimumViewDot);
    TestTrue(TEXT("recent-gaze release cone cannot be narrower than acquire"),
             Sanitized.GazeReleaseMinimumViewDot
                  <= Sanitized.GazeMinimumViewDot);
    TestNull(TEXT("attention observation never reflects a canonical entity ID"),
             FindFProperty<FProperty>(
                 FMythicEntityAttentionObservation::StaticStruct(),
                 TEXT("EntityId")));
    TestNull(TEXT("attention observation never reflects a persistent name seed"),
             FindFProperty<FProperty>(
                 FMythicEntityAttentionObservation::StaticStruct(),
                 TEXT("NameSeed")));
    TestNull(TEXT("weak actor resolution stays outside the Blueprint payload"),
             FindFProperty<FProperty>(
                 FMythicEntityAttentionObservation::StaticStruct(),
                 TEXT("Actor")));
    TestNull(TEXT("weak component resolution stays outside the Blueprint payload"),
             FindFProperty<FProperty>(
                 FMythicEntityAttentionObservation::StaticStruct(),
                  TEXT("Component")));
    TestNotNull(TEXT("directional incoming-combat evidence is explicit"),
                FindFProperty<FBoolProperty>(
                    FMythicEntityAttentionObservation::StaticStruct(),
                    TEXT("bRecentIncomingCombatSignal")));
    bool bReflectsCanonicalIdentity = false;
    for (TFieldIterator<FProperty> Property(
             FMythicEntityAttentionObservation::StaticStruct());
         Property; ++Property) {
        const FStructProperty *StructProperty =
            CastField<FStructProperty>(*Property);
        bReflectsCanonicalIdentity |= StructProperty
            && StructProperty->Struct == FMythicEntityId::StaticStruct();
    }
    TestFalse(TEXT("no renamed field can smuggle canonical identity into observations"),
              bReflectsCanonicalIdentity);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEntityAttentionGazeResidencyRulesTest,
    "Mythic.Interaction.EntityAttention.GazeResidencyRules",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicEntityAttentionGazeResidencyRulesTest::RunTest(
    const FString &Parameters) {
    (void)Parameters;
    FMythicEntityAttentionConfig Config;
    Config.GazeMinimumViewDot = 0.35f;
    Config.GazeReleaseMinimumViewDot = 0.30f;
    Config.SlotReleaseGraceSeconds = 0.40f;

    TestEqual(TEXT("new gaze uses the authored acquire cone"),
              FMythicEntityAttentionRules::GetGazeMinimumViewDot(
                  false, Config),
              0.35f);
    TestEqual(TEXT("recent gaze uses the wider release cone"),
              FMythicEntityAttentionRules::GetGazeMinimumViewDot(
                  true, Config),
              0.30f);
    TestTrue(TEXT("recent gaze survives just inside slot grace"),
             FMythicEntityAttentionRules::CanRetainRecentGaze(
                 0.399, Config));
    TestTrue(TEXT("recent gaze survives the exact slot-grace boundary"),
             FMythicEntityAttentionRules::CanRetainRecentGaze(
                 0.40, Config));
    TestFalse(TEXT("recent gaze expires after slot grace"),
              FMythicEntityAttentionRules::CanRetainRecentGaze(
                  0.401, Config));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEntityAttentionVisibilityHysteresisTest,
    "Mythic.Interaction.EntityAttention.VisibilityHysteresis",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicEntityAttentionVisibilityHysteresisTest::RunTest(
    const FString &Parameters) {
    (void)Parameters;
    FMythicEntityAttentionConfig Config;
    Config.LineOfSightCacheLifetimeSeconds = 0.25f;
    Config.OcclusionHideGraceSeconds = 0.12f;
    Config.OcclusionRevealGraceSeconds = 0.08f;

    FMythicEntityAttentionVisibilityState State;
    TestTrue(TEXT("the first positive sample initializes visible"),
             FMythicEntityAttentionRules::UpdateStableLineOfSight(
                 State, true, 10.0, Config));
    TestTrue(TEXT("a fresh occlusion does not hide immediately"),
             FMythicEntityAttentionRules::UpdateStableLineOfSight(
                 State, false, 10.01, Config));
    TestTrue(TEXT("visibility survives just inside hide grace"),
             FMythicEntityAttentionRules::UpdateStableLineOfSight(
                 State, false, 10.129, Config));
    TestFalse(TEXT("continuous occlusion hides just after the boundary"),
              FMythicEntityAttentionRules::UpdateStableLineOfSight(
                  State, false, 10.131, Config));

    TestFalse(TEXT("a fresh reveal does not show immediately"),
              FMythicEntityAttentionRules::UpdateStableLineOfSight(
                  State, true, 20.0, Config));
    TestFalse(TEXT("occluded state survives just inside reveal grace"),
              FMythicEntityAttentionRules::UpdateStableLineOfSight(
                  State, true, 20.079, Config));
    TestTrue(TEXT("continuous visibility reveals just after the boundary"),
             FMythicEntityAttentionRules::UpdateStableLineOfSight(
                 State, true, 20.081, Config));

    const double DeferredLimit =
        Config.LineOfSightCacheLifetimeSeconds
        + Config.OcclusionHideGraceSeconds;
    TestTrue(TEXT("budget deferral preserves a fresh stable positive"),
             FMythicEntityAttentionRules::ShouldPreserveDeferredLineOfSight(
                 State, DeferredLimit, Config));
    TestFalse(TEXT("budget deferral cannot preserve visibility forever"),
              FMythicEntityAttentionRules::ShouldPreserveDeferredLineOfSight(
                  State, DeferredLimit + 0.001, Config));

    FMythicEntityAttentionVisibilityState NeverVisible;
    FMythicEntityAttentionRules::UpdateStableLineOfSight(
        NeverVisible, false, 30.0, Config);
    TestFalse(TEXT("budget deferral never creates visibility"),
              FMythicEntityAttentionRules::ShouldPreserveDeferredLineOfSight(
                  NeverVisible, 0.0, Config));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
