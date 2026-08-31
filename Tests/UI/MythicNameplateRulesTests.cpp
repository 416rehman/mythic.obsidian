#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "UI/Nameplate/MythicNameplateRules.h"
#include "UI/Settings/MythicUserSettings.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateDisclosureRulesTest,
    "Mythic.UI.Nameplate.Rules.Disclosure",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateDisclosureRulesTest::RunTest(const FString &Parameters) {
    FMythicNameplateDisclosureEvidence Evidence;
    TestEqual(TEXT("permission fails closed"), FMythicNameplateRules::ResolveDesiredDisclosure(Evidence),
              EMythicNameplateDisclosureTier::Silent);

    Evidence.bPresentationPermitted = true;
    Evidence.bHasLineOfSight = true;
    Evidence.bGazeAttention = true;
    TestEqual(TEXT("stable gaze earns identity-only Whisper"), FMythicNameplateRules::ResolveDesiredDisclosure(Evidence),
              EMythicNameplateDisclosureTier::Whisper);

    Evidence.bGazeAttention = false;
    Evidence.bPersonalSpaceAttention = true;
    TestEqual(TEXT("prequalified personal-space attention earns only Whisper"),
              FMythicNameplateRules::ResolveDesiredDisclosure(Evidence), EMythicNameplateDisclosureTier::Whisper);

    Evidence.bContextSignal = true;
    TestEqual(TEXT("context signal outranks ambient attention"), FMythicNameplateRules::ResolveDesiredDisclosure(Evidence),
              EMythicNameplateDisclosureTier::Context);

    Evidence.bFocusAttention = true;
    TestEqual(TEXT("deliberate attention earns Focus"), FMythicNameplateRules::ResolveDesiredDisclosure(Evidence),
              EMythicNameplateDisclosureTier::Focus);

    Evidence.bFocusAttention = false;
    Evidence.bHardTarget = true;
    TestEqual(TEXT("hard combat target immediately earns Focus"),
              FMythicNameplateRules::ResolveDesiredDisclosure(Evidence),
              EMythicNameplateDisclosureTier::Focus);

    Evidence.bHasLineOfSight = false;
    TestEqual(TEXT("ordinary plate never persists through occlusion"), FMythicNameplateRules::ResolveDesiredDisclosure(Evidence),
              EMythicNameplateDisclosureTier::Silent);

    Evidence.bPresentationPermitted = false;
    TestEqual(TEXT("targeting cannot bypass presentation permission"), FMythicNameplateRules::ResolveDesiredDisclosure(Evidence),
              EMythicNameplateDisclosureTier::Silent);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplatePresentationModeRulesTest,
    "Mythic.UI.Nameplate.Rules.PresentationModesDoNotGrantInformation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicNameplatePresentationModeRulesTest::RunTest(
    const FString &Parameters) {
    TestEqual(
        TEXT("Minimal suppresses an optional ambient identity"),
        FMythicNameplateRules::ApplyPresentationMode(
            EMythicNameplateDisclosureTier::Whisper,
            EMythicNameplatePresentationMode::Minimal, false),
        EMythicNameplateDisclosureTier::Silent);
    TestEqual(
        TEXT("Minimal retains a mandatory safety Context surface"),
        FMythicNameplateRules::ApplyPresentationMode(
            EMythicNameplateDisclosureTier::Context,
            EMythicNameplatePresentationMode::Minimal, true),
        EMythicNameplateDisclosureTier::Context);
    TestEqual(
        TEXT("Minimal cannot suppress deliberate Focus interaction"),
        FMythicNameplateRules::ApplyPresentationMode(
            EMythicNameplateDisclosureTier::Focus,
            EMythicNameplatePresentationMode::Minimal, false),
        EMythicNameplateDisclosureTier::Focus);
    TestEqual(
        TEXT("Expanded does not upgrade an ambient identity tier"),
        FMythicNameplateRules::ApplyPresentationMode(
            EMythicNameplateDisclosureTier::Whisper,
            EMythicNameplatePresentationMode::Expanded, false),
        EMythicNameplateDisclosureTier::Whisper);
    TestEqual(
        TEXT("Expanded cannot turn identity-only Whisper into Context"),
        FMythicNameplateRules::ApplyPresentationMode(
            EMythicNameplateDisclosureTier::Whisper,
            EMythicNameplatePresentationMode::Expanded, true),
        EMythicNameplateDisclosureTier::Whisper);
    TestEqual(
        TEXT("Expanded cannot create Focus from an entitled Context surface"),
        FMythicNameplateRules::ApplyPresentationMode(
            EMythicNameplateDisclosureTier::Context,
            EMythicNameplatePresentationMode::Expanded, true),
        EMythicNameplateDisclosureTier::Context);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateCueAndLaneRulesTest,
    "Mythic.UI.Nameplate.Rules.CuesAndLanes",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateCueAndLaneRulesTest::RunTest(const FString &Parameters) {
    TArray<EMythicNameplatePrimaryCue> Cues = {
        EMythicNameplatePrimaryCue::Faction,
        EMythicNameplatePrimaryCue::QuestOffer,
        EMythicNameplatePrimaryCue::AttackingViewer,
        EMythicNameplatePrimaryCue::Dying,
    };
    TestEqual(TEXT("dying outranks attack, quest, and faction"), FMythicNameplateRules::SelectPrimaryCue(Cues),
              EMythicNameplatePrimaryCue::Dying);

    Cues.Add(EMythicNameplatePrimaryCue::Dead);
    TestEqual(TEXT("dead is the terminal headline"), FMythicNameplateRules::SelectPrimaryCue(Cues),
              EMythicNameplatePrimaryCue::Dead);
    Algo::Reverse(Cues);
    TestEqual(TEXT("cue selection is independent of input ordering"), FMythicNameplateRules::SelectPrimaryCue(Cues),
              EMythicNameplatePrimaryCue::Dead);

    TestEqual(TEXT("attacking viewer reserves Safety"),
              FMythicNameplateRules::ResolveLane(EMythicNameplatePrimaryCue::AttackingViewer, false),
              EMythicNameplateLane::Safety);
    TestEqual(TEXT("turn-in reserves Opportunity"),
              FMythicNameplateRules::ResolveLane(EMythicNameplatePrimaryCue::QuestTurnIn, false),
              EMythicNameplateLane::Opportunity);
    TestEqual(TEXT("a corpse action is an Opportunity, never Safety"),
              FMythicNameplateRules::ResolveLane(
                  EMythicNameplatePrimaryCue::Dead, false),
              EMythicNameplateLane::Opportunity);
    TestEqual(TEXT("ambient activity uses Awareness"),
              FMythicNameplateRules::ResolveLane(EMythicNameplatePrimaryCue::ObservableActivity, false),
              EMythicNameplateLane::Awareness);
    TestEqual(TEXT("deliberate focus always owns Focus lane"),
              FMythicNameplateRules::ResolveLane(EMythicNameplatePrimaryCue::Faction, true),
              EMythicNameplateLane::Focus);
    TestEqual(TEXT("viewer-outgoing combat remains observable fighting"),
              FMythicNameplateRules::ResolveObservedFightingCue(true, false),
              EMythicNameplatePrimaryCue::ObservableActivity);
    TestEqual(TEXT("subject-to-viewer combat earns the attacking cue"),
              FMythicNameplateRules::ResolveObservedFightingCue(true, true),
              EMythicNameplatePrimaryCue::AttackingViewer);
    TestEqual(TEXT("incoming direction alone cannot invent combat"),
              FMythicNameplateRules::ResolveObservedFightingCue(false, true),
              EMythicNameplatePrimaryCue::ObservableActivity);

    const FMythicNameplateCapacityPolicy Capacity;
    TestEqual(TEXT("default pool is prewarmed to sixteen"), Capacity.PoolSize, 16);
    TestEqual(TEXT("default draw cap is twelve"), Capacity.MaxDrawnPlates, 12);
    TestEqual(TEXT("default Focus reservation"),
              FMythicNameplateRules::GetLaneCapacity(EMythicNameplateLane::Focus, Capacity), 1);
    TestEqual(TEXT("default Safety reservation"),
              FMythicNameplateRules::GetLaneCapacity(EMythicNameplateLane::Safety, Capacity), 6);
    TestEqual(TEXT("default Opportunity reservation"),
              FMythicNameplateRules::GetLaneCapacity(EMythicNameplateLane::Opportunity, Capacity), 3);
    TestEqual(TEXT("default Awareness reservation"),
              FMythicNameplateRules::GetLaneCapacity(EMythicNameplateLane::Awareness, Capacity), 2);
    TestEqual(TEXT("four pool entries remain for fade leases"), Capacity.FadeReserveSlots, 4);
    TestEqual(TEXT("default mode permits one ambient identity when attention is free"),
              FMythicNameplateRules::GetAmbientWhisperCapacity(
                  false, EMythicNameplatePresentationMode::Contextual,
                  Capacity),
              1);
    TestEqual(TEXT("expanded mode may use the bounded Awareness reservation"),
              FMythicNameplateRules::GetAmbientWhisperCapacity(
                  false, EMythicNameplatePresentationMode::Expanded,
                  Capacity),
              Capacity.AwarenessLaneSlots);
    TestEqual(TEXT("deliberate focus suppresses ambient identity clutter"),
              FMythicNameplateRules::GetAmbientWhisperCapacity(
                  true, EMythicNameplatePresentationMode::Expanded,
                  Capacity),
              0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateScreenDeclutterRulesTest,
    "Mythic.UI.Nameplate.Rules.ScreenDeclutter",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateScreenDeclutterRulesTest::RunTest(
    const FString &Parameters) {
    FMythicNameplateDeclutterPolicy Declutter;
    FVector2D ResolvedCenter;
    FBox2D ResolvedBounds;
    TArray<FBox2D> Occupied;

    TestTrue(TEXT("the highest-priority surface remains pinned"),
             FMythicNameplateRules::ResolveDeclutteredPlacement(
                 FVector2D(100.0f, 100.0f), FVector2D(120.0f, 40.0f),
                 EMythicNameplateLane::Focus, false, 1.0f,
                 Occupied, Declutter, ResolvedCenter, ResolvedBounds));
    TestEqual(TEXT("the pinned surface keeps its authored center"),
              ResolvedCenter, FVector2D(100.0f, 100.0f));
    Occupied.Add(ResolvedBounds);

    TestFalse(TEXT("ambient identity yields to an overlapping focus read"),
              FMythicNameplateRules::ResolveDeclutteredPlacement(
                  FVector2D(108.0f, 100.0f), FVector2D(128.0f, 24.0f),
                  EMythicNameplateLane::Awareness, false, 1.0f,
                  Occupied, Declutter, ResolvedCenter, ResolvedBounds));

    TestTrue(TEXT("a tactical read resolves immediately above focus"),
             FMythicNameplateRules::ResolveDeclutteredPlacement(
                 FVector2D(100.0f, 100.0f), FVector2D(120.0f, 40.0f),
                 EMythicNameplateLane::Safety, false, 1.0f,
                 Occupied, Declutter, ResolvedCenter, ResolvedBounds));
    TestEqual(TEXT("tactical displacement preserves the configured gap"),
              ResolvedCenter, FVector2D(100.0f, 54.0f));

    const FVector2D ClearCenter(177.0f, 100.0f);
    TestTrue(TEXT("a new surface may appear once normal padding is clear"),
             FMythicNameplateRules::ResolveDeclutteredPlacement(
                 ClearCenter, FVector2D(20.0f, 20.0f),
                 EMythicNameplateLane::Opportunity, false, 1.0f,
                 Occupied, Declutter, ResolvedCenter, ResolvedBounds));
    TestFalse(TEXT("a suppressed surface waits for release hysteresis"),
              FMythicNameplateRules::ResolveDeclutteredPlacement(
                  ClearCenter, FVector2D(20.0f, 20.0f),
                  EMythicNameplateLane::Awareness, true, 1.0f,
                  Occupied, Declutter, ResolvedCenter, ResolvedBounds));

    Declutter.MaxVerticalSteps = 0;
    TestFalse(TEXT("bounded tactical displacement fails closed at zero budget"),
              FMythicNameplateRules::ResolveDeclutteredPlacement(
                  FVector2D(100.0f, 100.0f), FVector2D(120.0f, 40.0f),
                  EMythicNameplateLane::Safety, false, 1.0f,
                  Occupied, Declutter, ResolvedCenter, ResolvedBounds));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateHealthAndTimingRulesTest,
    "Mythic.UI.Nameplate.Rules.HealthLevelAndHysteresis",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateHealthAndTimingRulesTest::RunTest(const FString &Parameters) {
    FMythicNameplateHealthContext Health;
    Health.bHealthPresentationPermitted = true;
    Health.bInjured = true;
    TestFalse(TEXT("Whisper is identity-only even when injured"),
              FMythicNameplateRules::ShouldShowHealth(EMythicNameplateDisclosureTier::Whisper, Health));
    TestFalse(TEXT("an ambient peaceful injury does not broadcast at Context"),
              FMythicNameplateRules::ShouldShowHealth(EMythicNameplateDisclosureTier::Context, Health));
    TestTrue(TEXT("deliberate Focus may read visible injury"),
             FMythicNameplateRules::ShouldShowHealth(EMythicNameplateDisclosureTier::Focus, Health));

    Health.bInjured = false;
    Health.bCurrentCombatTarget = true;
    TestTrue(TEXT("current target shows a full health bar"),
             FMythicNameplateRules::ShouldShowHealth(EMythicNameplateDisclosureTier::Context, Health));
    Health.bCurrentCombatTarget = false;
    Health.bDownedOrDying = true;
    TestTrue(TEXT("downed subject remains a safety read"),
             FMythicNameplateRules::ShouldShowHealth(EMythicNameplateDisclosureTier::Context, Health));
    Health.bHealthPresentationPermitted = false;
    TestFalse(TEXT("health permission fails closed despite safety state"),
              FMythicNameplateRules::ShouldShowHealth(EMythicNameplateDisclosureTier::Focus, Health));

    FMythicNameplateLevelContext Level;
    Level.bExactLevelPermitted = true;
    Level.bCombatCapable = true;
    TestFalse(TEXT("exact level never appears on Whisper"),
              FMythicNameplateRules::ShouldShowExactLevel(EMythicNameplateDisclosureTier::Whisper, Level));
    TestTrue(TEXT("permitted combatant level appears on Focus"),
             FMythicNameplateRules::ShouldShowExactLevel(EMythicNameplateDisclosureTier::Focus, Level));
    Level.bCurrentCombatTarget = true;
    TestTrue(TEXT("current combat target may show level on Context"),
             FMythicNameplateRules::ShouldShowExactLevel(EMythicNameplateDisclosureTier::Context, Level));
    Level.bCombatCapable = false;
    TestFalse(TEXT("a civilian never receives an ambient territory level"),
              FMythicNameplateRules::ShouldShowExactLevel(EMythicNameplateDisclosureTier::Focus, Level));

    const FMythicEntityAttentionConfig Timing;
    TestFalse(TEXT("Whisper waits for gaze dwell"),
              FMythicNameplateRules::ShouldPromote(EMythicNameplateDisclosureTier::Silent, EMythicNameplateDisclosureTier::Whisper,
                                                   0.119f, Timing));
    TestTrue(TEXT("Whisper promotes at dwell boundary"),
             FMythicNameplateRules::ShouldPromote(EMythicNameplateDisclosureTier::Silent, EMythicNameplateDisclosureTier::Whisper,
                                                  0.12f, Timing));
    TestFalse(TEXT("Focus waits for deliberate dwell"),
              FMythicNameplateRules::ShouldPromote(EMythicNameplateDisclosureTier::Context, EMythicNameplateDisclosureTier::Focus,
                                                   0.119f, Timing));
    TestTrue(TEXT("Focus promotes at dwell boundary"),
             FMythicNameplateRules::ShouldPromote(EMythicNameplateDisclosureTier::Context, EMythicNameplateDisclosureTier::Focus,
                                                  0.12f, Timing));
    TestFalse(TEXT("Focus survives inside release grace"),
              FMythicNameplateRules::ShouldDemote(EMythicNameplateDisclosureTier::Focus, EMythicNameplateDisclosureTier::Silent,
                                                  0.139f, Timing));
    TestTrue(TEXT("Focus demotes at release boundary"),
             FMythicNameplateRules::ShouldDemote(EMythicNameplateDisclosureTier::Focus, EMythicNameplateDisclosureTier::Silent,
                                                 0.14f, Timing));
    TestFalse(TEXT("Whisper survives a bounded missing attention pass"),
              FMythicNameplateRules::ShouldDemote(
                  EMythicNameplateDisclosureTier::Whisper,
                  EMythicNameplateDisclosureTier::Silent,
                  0.399f, Timing));
    TestTrue(TEXT("Whisper releases exactly at its presentation lease boundary"),
             FMythicNameplateRules::ShouldDemote(
                 EMythicNameplateDisclosureTier::Whisper,
                 EMythicNameplateDisclosureTier::Silent,
                 0.40f, Timing));
    TestFalse(TEXT("a sub-threshold challenger never begins a replacement lead"),
              FMythicNameplateRules::HasReplacementScoreLead(
                  100.0f, 114.999f, Timing));
    TestTrue(TEXT("a challenger begins its lead at the exact authored score threshold"),
             FMythicNameplateRules::HasReplacementScoreLead(
                 100.0f, 115.0f, Timing));
    TestFalse(TEXT("same-lane replacement requires dwell"),
              FMythicNameplateRules::ShouldReplaceIncumbent(100.0f, 115.0f, 0.089f, false, Timing));
    TestTrue(TEXT("same-lane replacement accepts exact score and dwell thresholds"),
             FMythicNameplateRules::ShouldReplaceIncumbent(100.0f, 115.0f, 0.09f, false, Timing));
    TestTrue(TEXT("valid higher-lane safety claim bypasses same-lane dwell"),
             FMythicNameplateRules::ShouldReplaceIncumbent(100.0f, 1.0f, 0.0f, true, Timing));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplatePassiveDistanceRulesTest,
    "Mythic.UI.Nameplate.Rules.PassiveDistanceAndTransitions",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicNameplatePassiveDistanceRulesTest::RunTest(
    const FString &Parameters) {
    const FMythicNameplatePassiveIdentityPolicy Policy;
    TestEqual(TEXT("Whisper full distance is three metres"),
              Policy.WhisperFullDistanceCentimeters, 300.0f);
    TestEqual(TEXT("Whisper acquire distance is four metres"),
              Policy.WhisperAcquireDistanceCentimeters, 400.0f);
    TestEqual(TEXT("Whisper release distance is five metres"),
              Policy.WhisperReleaseDistanceCentimeters, 500.0f);
    TestEqual(TEXT("ordinary Focus full distance is eight metres"),
              Policy.FocusFullDistanceCentimeters, 800.0f);
    TestEqual(TEXT("ordinary Focus acquire distance is ten metres"),
              Policy.FocusAcquireDistanceCentimeters, 1000.0f);
    TestEqual(TEXT("ordinary Focus release distance is twelve metres"),
              Policy.FocusReleaseDistanceCentimeters, 1200.0f);

    TestTrue(TEXT("new Whisper admits at its acquire boundary"),
             FMythicNameplateRules::ShouldAdmitPassiveSurface(
                 400.0f, 400.0f, 500.0f, false));
    TestFalse(TEXT("new Whisper cannot acquire outside four metres"),
              FMythicNameplateRules::ShouldAdmitPassiveSurface(
                  400.01f, 400.0f, 500.0f, false));
    TestTrue(TEXT("incumbent Whisper survives inside release edge"),
             FMythicNameplateRules::ShouldAdmitPassiveSurface(
                 499.99f, 400.0f, 500.0f, true));
    TestFalse(TEXT("incumbent Whisper releases at five metres"),
              FMythicNameplateRules::ShouldAdmitPassiveSurface(
                  500.0f, 400.0f, 500.0f, true));
    TestTrue(TEXT("new ordinary Focus admits at ten metres"),
             FMythicNameplateRules::ShouldAdmitPassiveSurface(
                 1000.0f, 1000.0f, 1200.0f, false));
    TestFalse(TEXT("ordinary Focus cannot acquire past ten metres"),
              FMythicNameplateRules::ShouldAdmitPassiveSurface(
                  1000.01f, 1000.0f, 1200.0f, false));

    FMythicNameplateDistancePresentation Distance =
        FMythicNameplateRules::ResolvePassiveDistancePresentation(
            300.0f, 300.0f, 500.0f, 0.90f, 0.90f, false);
    TestEqual(TEXT("Whisper is fully opaque through three metres"),
              Distance.Alpha, 0.90f);
    TestEqual(TEXT("Whisper is full scale through three metres"),
              Distance.Scale, 1.0f);
    Distance = FMythicNameplateRules::ResolvePassiveDistancePresentation(
        400.0f, 300.0f, 500.0f, 0.90f, 0.90f, false);
    TestTrue(TEXT("Whisper midpoint uses smoothstep alpha"),
             FMath::IsNearlyEqual(Distance.Alpha, 0.45f));
    TestTrue(TEXT("Whisper midpoint uses smoothstep scale"),
             FMath::IsNearlyEqual(Distance.Scale, 0.95f));
    Distance = FMythicNameplateRules::ResolvePassiveDistancePresentation(
        500.0f, 300.0f, 500.0f, 0.90f, 0.90f, false);
    TestTrue(TEXT("Whisper release edge is collapsed"),
             Distance.bBeyondRelease && Distance.Alpha == 0.0f);

    Distance = FMythicNameplateRules::ResolvePassiveDistancePresentation(
        1000.0f, 800.0f, 1200.0f, 1.0f, 0.85f, false);
    TestTrue(TEXT("ordinary Focus midpoint alpha is one half"),
             FMath::IsNearlyEqual(Distance.Alpha, 0.5f));
    TestTrue(TEXT("ordinary Focus midpoint scale is 0.925"),
             FMath::IsNearlyEqual(Distance.Scale, 0.925f));
    Distance = FMythicNameplateRules::ResolvePassiveDistancePresentation(
        1000.0f, 800.0f, 1200.0f, 1.0f, 0.85f, true);
    TestEqual(TEXT("High Contrast or Reduced Motion retains full alpha"),
              Distance.Alpha, 1.0f);
    TestEqual(TEXT("High Contrast or Reduced Motion retains unit scale"),
              Distance.Scale, 1.0f);

    TestTrue(TEXT("acquire midpoint eases to one half"),
             FMath::IsNearlyEqual(
                 FMythicNameplateRules::ResolveTemporalAlpha(
                     0.0f, false, 10.0, 10.05, 0.10f, 0.14f, false),
                 0.5f));
    TestTrue(TEXT("acquire completes at 0.10 seconds"),
             FMath::IsNearlyEqual(
                 FMythicNameplateRules::ResolveTemporalAlpha(
                     0.0f, false, 10.0, 10.10, 0.10f, 0.14f, false),
                 1.0f));
    TestTrue(TEXT("release midpoint eases to one half"),
             FMath::IsNearlyEqual(
                 FMythicNameplateRules::ResolveTemporalAlpha(
                     1.0f, true, 10.0, 10.07, 0.10f, 0.14f, false),
                 0.5f));
    TestTrue(TEXT("release completes at 0.14 seconds"),
             FMath::IsNearlyEqual(
                 FMythicNameplateRules::ResolveTemporalAlpha(
                     1.0f, true, 10.0, 10.14, 0.10f, 0.14f, false),
                 0.0f));
    TestEqual(TEXT("Reduced Motion snaps acquire"),
              FMythicNameplateRules::ResolveTemporalAlpha(
                  0.0f, false, 10.0, 10.0, 0.10f, 0.14f, true),
              1.0f);
    TestEqual(TEXT("Reduced Motion snaps release"),
              FMythicNameplateRules::ResolveTemporalAlpha(
                  1.0f, true, 10.0, 10.0, 0.10f, 0.14f, true),
              0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateStatusSelectionTest,
    "Mythic.UI.Nameplate.Rules.StatusSelection",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateStatusSelectionTest::RunTest(const FString &Parameters) {
    const FGameplayTag Burn = FGameplayTag::RequestGameplayTag(FName(TEXT("Status.Type.Burn")), false);
    const FGameplayTag Bleed = FGameplayTag::RequestGameplayTag(FName(TEXT("Status.Type.Bleed")), false);
    const FGameplayTag Slow = FGameplayTag::RequestGameplayTag(FName(TEXT("Status.Type.Slow")), false);
    const FGameplayTag Stun = FGameplayTag::RequestGameplayTag(FName(TEXT("Status.Type.Stun")), false);
    const FGameplayTag Poison = FGameplayTag::RequestGameplayTag(FName(TEXT("Status.Type.Poison")), false);
    if (!TestTrue(TEXT("canonical status tags are registered"),
                  Burn.IsValid() && Bleed.IsValid() && Slow.IsValid() && Stun.IsValid() && Poison.IsValid())) {
        return false;
    }

    auto Make = [](const FGameplayTag Tag, const EMythicNameplateStatusUrgency Urgency,
                   const bool bPermitted, const bool bViewerApplied, const int32 Priority,
                   const int32 TieBreak) {
        FMythicNameplateStatusCandidate Candidate;
        Candidate.StatusType = Tag;
        Candidate.Urgency = Urgency;
        Candidate.bPresentationPermitted = bPermitted;
        Candidate.bAppliedByViewerOrParty = bViewerApplied;
        Candidate.AuthoredPriority = Priority;
        Candidate.StableTieBreak = TieBreak;
        return Candidate;
    };

    TArray<FMythicNameplateStatusCandidate> Candidates = {
        Make(Burn, EMythicNameplateStatusUrgency::Damaging, true, false, 10, 4),
        Make(Stun, EMythicNameplateStatusUrgency::HardCrowdControl, true, false, 0, 3),
        Make(Slow, EMythicNameplateStatusUrgency::MovementControl, true, true, 100, 2),
        Make(Poison, EMythicNameplateStatusUrgency::LethalDamageOverTime, false, true, 100, 1),
        Make(Bleed, EMythicNameplateStatusUrgency::LethalDamageOverTime, true, false, 0, 0),
    };

    TArray<FMythicNameplateStatusCandidate> Selected;
    int32 Overflow = INDEX_NONE;
    FMythicNameplateRules::SelectStatusCandidates(Candidates, 2, Selected, Overflow);
    if (!TestEqual(TEXT("Context cap selects two"), Selected.Num(), 2)) {
        return false;
    }
    TestEqual(TEXT("hard control wins first icon"), Selected[0].StatusType, Stun);
    TestEqual(TEXT("permitted lethal damage wins second icon"), Selected[1].StatusType, Bleed);
    TestEqual(TEXT("two other permitted statuses collapse into overflow"), Overflow, 2);

    Algo::Reverse(Candidates);
    TArray<FMythicNameplateStatusCandidate> ReversedSelection;
    int32 ReversedOverflow = INDEX_NONE;
    FMythicNameplateRules::SelectStatusCandidates(Candidates, 2, ReversedSelection, ReversedOverflow);
    if (!TestEqual(TEXT("reordered input selects two"), ReversedSelection.Num(), 2)) {
        return false;
    }
    TestEqual(TEXT("reordered input selects the same first status"), ReversedSelection[0].StatusType, Stun);
    TestEqual(TEXT("reordered input selects the same second status"), ReversedSelection[1].StatusType, Bleed);
    TestEqual(TEXT("reordered input preserves overflow"), ReversedOverflow, Overflow);

    const FMythicNameplateStatusPolicy StatusPolicy;
    TestEqual(TEXT("Whisper has no status row"),
              FMythicNameplateRules::GetStatusIconCap(EMythicNameplateDisclosureTier::Whisper, StatusPolicy), 0);
    TestEqual(TEXT("Context defaults to two icons"),
              FMythicNameplateRules::GetStatusIconCap(EMythicNameplateDisclosureTier::Context, StatusPolicy), 2);
    TestEqual(TEXT("Focus defaults to three icons"),
              FMythicNameplateRules::GetStatusIconCap(EMythicNameplateDisclosureTier::Focus, StatusPolicy), 3);

    TArray<FMythicNameplateStatusCandidate> NoTierSelection;
    int32 NoTierOverflow = INDEX_NONE;
    FMythicNameplateRules::SelectStatusCandidates(
        Candidates, 0, NoTierSelection, NoTierOverflow);
    TestEqual(TEXT("zero-cap tier selects no statuses"),
              NoTierSelection.Num(), 0);
    TestEqual(TEXT("zero-cap tier never leaks eligible status count"),
              NoTierOverflow, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateWhisperSanitizerTest,
    "Mythic.UI.Nameplate.Rules.WhisperIsIdentityOnly",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateWhisperSanitizerTest::RunTest(
    const FString &Parameters) {
    FMythicNameplateProjection Projection;
    Projection.DisclosureTier = EMythicNameplateDisclosureTier::Whisper;
    Projection.VisualFamily = EMythicNameplateVisualFamily::Boss;
    Projection.AttentionState =
        EMythicNameplateAttentionState::HardCombatTarget;
    Projection.ResolvedName = FText::FromString(TEXT("Visible Name"));
    Projection.PrimaryCue = EMythicNameplatePrimaryCue::AttackingViewer;
    Projection.ResolvedSubtitle =
        FText::FromString(TEXT("Hidden Role and Faction"));
    Projection.bShowHealth = true;
    Projection.HealthFraction = 0.5f;
    Projection.bHealthPercentEligible = true;
    Projection.bCombatCapable = true;
    Projection.PresentedCombatRank = EMythicPresentedCombatRank::WorldBoss;
    Projection.ThreatBand = EMythicThreatBand::Overwhelming;
    Projection.bShowExactLevel = true;
    Projection.CombatLevel = 99;
    Projection.ResolvedLevelText = FText::FromString(TEXT("99"));
    Projection.Statuses.AddDefaulted();
    Projection.StatusOverflowCount = 7;

    FMythicNameplateRules::SanitizeProjectionForPresentation(Projection);
    TestEqual(TEXT("Whisper retains its identity label"),
              Projection.ResolvedName.ToString(), FString(TEXT("Visible Name")));
    TestEqual(TEXT("Whisper clears cue"), Projection.PrimaryCue,
              EMythicNameplatePrimaryCue::None);
    TestTrue(TEXT("Whisper clears peaceful subtitle"),
             Projection.ResolvedSubtitle.IsEmpty());
    TestEqual(TEXT("Whisper collapses attention to observation"),
              Projection.AttentionState,
              EMythicNameplateAttentionState::Observed);
    TestEqual(TEXT("Whisper collapses to identity geometry"),
              Projection.VisualFamily,
              EMythicNameplateVisualFamily::Identity);
    TestFalse(TEXT("Whisper clears health"), Projection.bShowHealth);
    TestEqual(TEXT("Whisper clears health fraction"),
              Projection.HealthFraction, 0.0f);
    TestFalse(TEXT("Whisper clears percent eligibility"),
              Projection.bHealthPercentEligible);
    TestFalse(TEXT("Whisper clears combat capability"),
              Projection.bCombatCapable);
    TestEqual(TEXT("Whisper clears presented combat rank"),
              Projection.PresentedCombatRank,
              EMythicPresentedCombatRank::Unknown);
    TestEqual(TEXT("Whisper clears danger"), Projection.ThreatBand,
              EMythicThreatBand::Unknown);
    TestFalse(TEXT("Whisper clears exact level"),
              Projection.bShowExactLevel);
    TestEqual(TEXT("Whisper clears level value"), Projection.CombatLevel, 0);
    TestTrue(TEXT("Whisper clears level text"),
             Projection.ResolvedLevelText.IsEmpty());
    TestEqual(TEXT("Whisper clears statuses"), Projection.Statuses.Num(), 0);
    TestEqual(TEXT("Whisper clears status overflow"),
              Projection.StatusOverflowCount, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateCorpseSanitizerTest,
    "Mythic.UI.Nameplate.Rules.DeadNeverBuildsPersistentPlate",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateCorpseSanitizerTest::RunTest(
    const FString &Parameters) {
    FMythicNameplateProjection Projection;
    Projection.DisclosureTier = EMythicNameplateDisclosureTier::Focus;
    Projection.VisualFamily = EMythicNameplateVisualFamily::Boss;
    Projection.AttentionState =
        EMythicNameplateAttentionState::HardCombatTarget;
    Projection.ResolvedName = FText::FromString(TEXT("Fallen Raider"));
    Projection.PrimaryCue = EMythicNameplatePrimaryCue::Dead;
    Projection.ResolvedSubtitle =
        FText::FromString(TEXT("Raider • Red Knives"));
    Projection.bShowHealth = true;
    Projection.HealthFraction = 0.35f;
    Projection.bHealthPercentEligible = true;
    Projection.bCombatCapable = true;
    Projection.PresentedCombatRank = EMythicPresentedCombatRank::Boss;
    Projection.ThreatBand = EMythicThreatBand::Deadly;
    Projection.bShowExactLevel = true;
    Projection.CombatLevel = 42;
    Projection.ResolvedLevelText = FText::FromString(TEXT("42"));
    Projection.Statuses.AddDefaulted();
    Projection.StatusOverflowCount = 3;

    FMythicNameplateRules::SanitizeProjectionForPresentation(Projection);
    TestEqual(TEXT("dead projection fails closed to Silent"),
              Projection.DisclosureTier,
              EMythicNameplateDisclosureTier::Silent);
    TestTrue(TEXT("dead projection clears its identity label"),
             Projection.ResolvedName.IsEmpty());
    TestEqual(TEXT("dead projection clears the skull cue"),
              Projection.PrimaryCue,
              EMythicNameplatePrimaryCue::None);
    TestEqual(TEXT("dead projection clears live attention emphasis"),
              Projection.AttentionState,
              EMythicNameplateAttentionState::None);
    TestEqual(TEXT("dead projection resets visual family"),
              Projection.VisualFamily,
              EMythicNameplateVisualFamily::Identity);
    TestTrue(TEXT("dead projection clears subtitle"),
             Projection.ResolvedSubtitle.IsEmpty());
    TestFalse(TEXT("dead projection clears health"), Projection.bShowHealth);
    TestEqual(TEXT("dead projection clears health fraction"),
              Projection.HealthFraction, 0.0f);
    TestFalse(TEXT("dead projection clears percent eligibility"),
              Projection.bHealthPercentEligible);
    TestFalse(TEXT("dead projection clears combat capability"),
              Projection.bCombatCapable);
    TestEqual(TEXT("dead projection clears presented combat rank"),
              Projection.PresentedCombatRank,
              EMythicPresentedCombatRank::Unknown);
    TestEqual(TEXT("dead projection clears danger"), Projection.ThreatBand,
              EMythicThreatBand::Unknown);
    TestFalse(TEXT("dead projection clears exact level"),
              Projection.bShowExactLevel);
    TestEqual(TEXT("dead projection clears level value"),
              Projection.CombatLevel, 0);
    TestTrue(TEXT("dead projection clears level text"),
             Projection.ResolvedLevelText.IsEmpty());
    TestEqual(TEXT("dead projection clears statuses"),
              Projection.Statuses.Num(), 0);
    TestEqual(TEXT("dead projection clears status overflow"),
              Projection.StatusOverflowCount, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateVisualFamilyOrthogonalityTest,
    "Mythic.UI.Nameplate.Rules.VisualFamilyOrthogonality",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateVisualFamilyOrthogonalityTest::RunTest(
    const FString &Parameters) {
    TestEqual(TEXT("unknown peaceful subject uses identity geometry"),
              FMythicNameplateRules::ResolveVisualFamily(
                  EMythicPresentedCombatRank::Unknown, false, false),
              EMythicNameplateVisualFamily::Identity);
    TestEqual(TEXT("standard committed combatant uses combat geometry"),
              FMythicNameplateRules::ResolveVisualFamily(
                  EMythicPresentedCombatRank::Standard, true, false),
              EMythicNameplateVisualFamily::Combat);
    TestEqual(TEXT("champion rank does not create another layout axis"),
              FMythicNameplateRules::ResolveVisualFamily(
                  EMythicPresentedCombatRank::Champion, true, false),
              EMythicNameplateVisualFamily::Combat);
    TestEqual(TEXT("urgent protected ally owns ally-safety geometry"),
              FMythicNameplateRules::ResolveVisualFamily(
                  EMythicPresentedCombatRank::Standard, true, true),
              EMythicNameplateVisualFamily::AllySafety);
    TestEqual(TEXT("authority-presented boss rank owns boss geometry"),
              FMythicNameplateRules::ResolveVisualFamily(
                  EMythicPresentedCombatRank::Boss, false, true),
              EMythicNameplateVisualFamily::Boss);
    TestEqual(TEXT("world boss is never reduced to ordinary combat geometry"),
              FMythicNameplateRules::ResolveVisualFamily(
                  EMythicPresentedCombatRank::WorldBoss, true, false),
              EMythicNameplateVisualFamily::Boss);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateProjectionEquivalenceTest,
    "Mythic.UI.Nameplate.Rules.ProjectionEquivalence",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateProjectionEquivalenceTest::RunTest(
    const FString &Parameters) {
    FMythicNameplateProjection A;
    A.DisclosureTier = EMythicNameplateDisclosureTier::Focus;
    A.VisualFamily = EMythicNameplateVisualFamily::Combat;
    A.AttentionState = EMythicNameplateAttentionState::Focused;
    A.ResolvedName = FText::FromString(TEXT("Target"));
    A.bShowHealth = true;
    A.HealthFraction = 0.75f;
    A.PresentedCombatRank = EMythicPresentedCombatRank::Elite;
    FMythicNameplateProjection B = A;

    TestTrue(TEXT("an unchanged sanitized DTO is equivalent"),
             FMythicNameplateRules::AreProjectionsEquivalent(A, B));
    B.HealthFraction = 0.5f;
    TestFalse(TEXT("a visible vitality edge republishes"),
              FMythicNameplateRules::AreProjectionsEquivalent(A, B));
    B = A;
    B.AttentionState = EMythicNameplateAttentionState::HardCombatTarget;
    TestFalse(TEXT("attention emphasis is an independent projection edge"),
              FMythicNameplateRules::AreProjectionsEquivalent(A, B));
    B = A;
    B.VisualFamily = EMythicNameplateVisualFamily::AllySafety;
    TestFalse(TEXT("visual family is an independent projection edge"),
              FMythicNameplateRules::AreProjectionsEquivalent(A, B));
    B = A;
    B.PresentedCombatRank = EMythicPresentedCombatRank::Champion;
    TestFalse(TEXT("presented rank is independent of visual family"),
              FMythicNameplateRules::AreProjectionsEquivalent(A, B));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateActionRailProjectionEquivalenceTest,
    "Mythic.UI.Nameplate.Rules.ActionRailProjectionEquivalence",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateActionRailProjectionEquivalenceTest::RunTest(
    const FString &Parameters) {
    TestNull(TEXT("base projection embeds no executable actions"),
             FindFProperty<FProperty>(
                 FMythicNameplateProjection::StaticStruct(),
                 TEXT("Actions")));

    const FGameplayTag Talk = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Context.Action.NPC.Talk")), false);
    if (!TestTrue(TEXT("canonical talk action tag is registered"),
                  Talk.IsValid())) {
        return false;
    }

    FMythicNameplateActionRailProjection A;
    FMythicNameplateActionProjection &Action = A.Actions.AddDefaulted_GetRef();
    Action.ActionTag = Talk;
    Action.ResolvedLabel = FText::FromString(TEXT("Talk"));
    Action.InputActionTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("UI.Action.ContextPrimary")), false);
    Action.OfferRevision = 7;
    Action.HoldDurationSeconds = 0.25f;
    if (!TestTrue(TEXT("action rail uses a canonical input action tag"),
                  Action.InputActionTag.IsValid())) {
        return false;
    }
    FMythicNameplateActionRailProjection B = A;

    TestTrue(TEXT("unchanged action rail is equivalent"),
             FMythicNameplateRules::AreActionRailProjectionsEquivalent(A, B));
    B.Actions[0].OfferRevision = 8;
    TestFalse(TEXT("offer revision republishes only the action rail"),
              FMythicNameplateRules::AreActionRailProjectionsEquivalent(A, B));
    B = A;
    B.Actions[0].HoldDurationSeconds = 0.5f;
    TestFalse(TEXT("hold-safety change republishes the action rail"),
              FMythicNameplateRules::AreActionRailProjectionsEquivalent(A, B));
    B = A;
    B.bInspectAvailable = true;
    B.ResolvedInspectLabel = FText::FromString(TEXT("Inspect"));
    TestFalse(TEXT("Inspect entitlement is action-rail state"),
              FMythicNameplateRules::AreActionRailProjectionsEquivalent(A, B));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
