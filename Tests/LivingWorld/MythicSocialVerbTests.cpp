// Mythic Living World — Social Verb Reaction Tests
// Covers the pure, deterministic trait→reaction mapping delivered in the Social Interaction Verbs cluster step:
//   - UMythicSocialVerbLibrary::ResolveReaction (friendly-verb standing bands; hostile-verb anger/cower/guard logic;
//     per-verb standing magnitudes + sign; Bully<Threaten<Provoke anger ordering; determinism)
//   - UMythicSocialVerbLibrary::DefaultBarkFor (never empty for any verb/reaction pair)
// Run via: Session Frontend → Automation → Mythic.LivingWorld.SocialVerbs

#include "Misc/AutomationTest.h"
#include "AI/NPCs/MythicSocialVerbs.h"
#include "Mass/Fragments/MythicMassFragments.h" // FMythicPersonalityFragment + EMythicVentChannel

namespace {
    constexpr float kHostileThreshold = -50.0f;
    constexpr float kFriendlyThreshold = 50.0f;

    // Build a personality with all vent weights zero except the named overrides.
    FMythicPersonalityFragment MakePersonality(float Fight = 0.0f, float Flee = 0.0f, float Submit = 0.0f, float Enforce = 0.0f) {
        FMythicPersonalityFragment P;
        for (int32 i = 0; i < static_cast<int32>(EMythicVentChannel::COUNT); ++i) {
            P.VentWeights[i] = 0.0f;
        }
        P.VentWeights[static_cast<int32>(EMythicVentChannel::Fight)] = Fight;
        P.VentWeights[static_cast<int32>(EMythicVentChannel::Flee)] = Flee;
        P.VentWeights[static_cast<int32>(EMythicVentChannel::Submit)] = Submit;
        P.VentWeights[static_cast<int32>(EMythicVentChannel::Enforce)] = Enforce;
        return P;
    }

    FMythicSocialReactionResult Resolve(EMythicSocialVerb V, const FMythicPersonalityFragment &P, float Standing) {
        return UMythicSocialVerbLibrary::ResolveReaction(V, P, Standing, kHostileThreshold, kFriendlyThreshold);
    }
} // namespace

// ─────────────────────────────────────────────────────────────
// Greet — standing bands map to Warm / Neutral / Cold
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialGreetBandsTest,
    "Mythic.LivingWorld.SocialVerbs.Greet.Bands",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialGreetBandsTest::RunTest(const FString &Parameters) {
    const FMythicPersonalityFragment P = MakePersonality();

    // Friendly standing → Warm.
    const FMythicSocialReactionResult Friendly = Resolve(EMythicSocialVerb::Greet, P, /*Standing*/ 75.0f);
    TestEqual(TEXT("Greet at friendly standing → Warm"), Friendly.Reaction, EMythicSocialReaction::Warm);

    // Neutral standing → Neutral.
    const FMythicSocialReactionResult Neutral = Resolve(EMythicSocialVerb::Greet, P, /*Standing*/ 0.0f);
    TestEqual(TEXT("Greet at neutral standing → Neutral"), Neutral.Reaction, EMythicSocialReaction::Neutral);

    // Hostile standing → Cold.
    const FMythicSocialReactionResult Hostile = Resolve(EMythicSocialVerb::Greet, P, /*Standing*/ -75.0f);
    TestEqual(TEXT("Greet at hostile standing → Cold"), Hostile.Reaction, EMythicSocialReaction::Cold);

    // Greet never moves standing.
    TestEqual(TEXT("Greet never changes standing (friendly)"), Friendly.StandingDelta, 0.0f);
    TestEqual(TEXT("Greet never changes standing (neutral)"), Neutral.StandingDelta, 0.0f);
    TestEqual(TEXT("Greet never changes standing (hostile)"), Hostile.StandingDelta, 0.0f);
    return true;
}

// ─────────────────────────────────────────────────────────────
// Compliment — gains standing only when not Cold
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialComplimentTest,
    "Mythic.LivingWorld.SocialVerbs.Compliment.Standing",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialComplimentTest::RunTest(const FString &Parameters) {
    const FMythicPersonalityFragment P = MakePersonality();

    // Neutral standing → Warm (a compliment lands) + positive standing.
    const FMythicSocialReactionResult Neutral = Resolve(EMythicSocialVerb::Compliment, P, 0.0f);
    TestEqual(TEXT("Compliment at neutral → Warm"), Neutral.Reaction, EMythicSocialReaction::Warm);
    TestTrue(TEXT("Compliment at neutral gains standing"), Neutral.StandingDelta > 0.0f);

    // Friendly standing → Warm + positive standing.
    const FMythicSocialReactionResult Friendly = Resolve(EMythicSocialVerb::Compliment, P, 75.0f);
    TestEqual(TEXT("Compliment at friendly → Warm"), Friendly.Reaction, EMythicSocialReaction::Warm);
    TestTrue(TEXT("Compliment at friendly gains standing"), Friendly.StandingDelta > 0.0f);

    // Hostile standing → Cold + NO standing change (a rebuff isn't won over).
    const FMythicSocialReactionResult Hostile = Resolve(EMythicSocialVerb::Compliment, P, -75.0f);
    TestEqual(TEXT("Compliment at hostile → Cold"), Hostile.Reaction, EMythicSocialReaction::Cold);
    TestEqual(TEXT("Cold compliment grants no standing"), Hostile.StandingDelta, 0.0f);
    return true;
}

// ─────────────────────────────────────────────────────────────
// Hostile verbs — aggressive NPC is Angered + turns hostile + loses standing
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialAngeredTest,
    "Mythic.LivingWorld.SocialVerbs.Hostile.Angered",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialAngeredTest::RunTest(const FString &Parameters) {
    // High Fight weight → angers on Bully (Bully has the lowest anger threshold).
    const FMythicPersonalityFragment Brawler = MakePersonality(/*Fight*/ 0.9f);
    const FMythicSocialReactionResult R = Resolve(EMythicSocialVerb::Bully, Brawler, 0.0f);
    TestEqual(TEXT("High-Fight Bully → Angered"), R.Reaction, EMythicSocialReaction::Angered);
    TestTrue(TEXT("Angered sets hostile"), R.bSetHostile);
    TestFalse(TEXT("Angered does not alert guards"), R.bAlertGuards);
    TestTrue(TEXT("Bully costs standing (negative)"), R.StandingDelta < 0.0f);
    return true;
}

// ─────────────────────────────────────────────────────────────
// Hostile verbs — coward (high Flee/Submit, low Fight) is Intimidated
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialIntimidatedTest,
    "Mythic.LivingWorld.SocialVerbs.Hostile.Intimidated",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialIntimidatedTest::RunTest(const FString &Parameters) {
    // High Flee, low Fight, low Enforce → Intimidated, no hostility/guards.
    const FMythicPersonalityFragment Coward = MakePersonality(/*Fight*/ 0.1f, /*Flee*/ 0.9f, /*Submit*/ 0.2f, /*Enforce*/ 0.0f);
    const FMythicSocialReactionResult R = Resolve(EMythicSocialVerb::Threaten, Coward, 0.0f);
    TestEqual(TEXT("High-Flee low-Fight Threaten → Intimidated"), R.Reaction, EMythicSocialReaction::Intimidated);
    TestFalse(TEXT("Intimidated does not set hostile"), R.bSetHostile);
    TestFalse(TEXT("Intimidated does not alert guards"), R.bAlertGuards);

    // High Submit also cowers.
    const FMythicPersonalityFragment Submissive = MakePersonality(/*Fight*/ 0.1f, /*Flee*/ 0.1f, /*Submit*/ 0.9f, /*Enforce*/ 0.0f);
    const FMythicSocialReactionResult R2 = Resolve(EMythicSocialVerb::Provoke, Submissive, 0.0f);
    TestEqual(TEXT("High-Submit low-Fight Provoke → Intimidated"), R2.Reaction, EMythicSocialReaction::Intimidated);
    return true;
}

// ─────────────────────────────────────────────────────────────
// Hostile verbs — enforcer (high Enforce, not a brawler) calls guards
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialCallGuardsTest,
    "Mythic.LivingWorld.SocialVerbs.Hostile.CallGuards",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialCallGuardsTest::RunTest(const FString &Parameters) {
    // High Enforce, Fight below the anger threshold → CallGuards (escalates without personally engaging).
    const FMythicPersonalityFragment Enforcer = MakePersonality(/*Fight*/ 0.2f, /*Flee*/ 0.0f, /*Submit*/ 0.0f, /*Enforce*/ 0.8f);
    const FMythicSocialReactionResult R = Resolve(EMythicSocialVerb::Provoke, Enforcer, 0.0f);
    TestEqual(TEXT("High-Enforce low-Fight Provoke → CallGuards"), R.Reaction, EMythicSocialReaction::CallGuards);
    TestTrue(TEXT("CallGuards sets alert flag"), R.bAlertGuards);
    TestFalse(TEXT("CallGuards does not personally engage"), R.bSetHostile);
    return true;
}

// ─────────────────────────────────────────────────────────────
// Anger-threshold ordering Bully < Threaten < Provoke + magnitude |Bully| > |Threaten| > |Provoke|
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialOrderingTest,
    "Mythic.LivingWorld.SocialVerbs.Hostile.Ordering",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialOrderingTest::RunTest(const FString &Parameters) {
    // A mid Fight weight (0.50) sits at-or-above Bully's threshold (0.45) and Threaten's (0.50) but BELOW Provoke's
    // (0.55): so the SAME NPC is Angered by a Bully/Threaten yet only Cold (not angered) by a Provoke.
    const FMythicPersonalityFragment Mid = MakePersonality(/*Fight*/ 0.50f);

    const FMythicSocialReactionResult Bully = Resolve(EMythicSocialVerb::Bully, Mid, 0.0f);
    const FMythicSocialReactionResult Threaten = Resolve(EMythicSocialVerb::Threaten, Mid, 0.0f);
    const FMythicSocialReactionResult Provoke = Resolve(EMythicSocialVerb::Provoke, Mid, 0.0f);

    TestEqual(TEXT("Mid-Fight Bully angers (lowest threshold)"), Bully.Reaction, EMythicSocialReaction::Angered);
    TestEqual(TEXT("Mid-Fight Threaten angers (mid threshold)"), Threaten.Reaction, EMythicSocialReaction::Angered);
    TestNotEqual(TEXT("Mid-Fight Provoke does NOT anger (highest threshold)"), Provoke.Reaction, EMythicSocialReaction::Angered);

    // Standing magnitude ordering: Bully (20) > Threaten (15) > Provoke (10), all negative.
    TestTrue(TEXT("|Bully delta| > |Threaten delta|"), FMath::Abs(Bully.StandingDelta) > FMath::Abs(Threaten.StandingDelta));
    TestTrue(TEXT("|Threaten delta| > |Provoke delta|"), FMath::Abs(Threaten.StandingDelta) > FMath::Abs(Provoke.StandingDelta));
    TestTrue(TEXT("Bully delta is negative"), Bully.StandingDelta < 0.0f);
    TestTrue(TEXT("Threaten delta is negative"), Threaten.StandingDelta < 0.0f);
    TestTrue(TEXT("Provoke delta is negative"), Provoke.StandingDelta < 0.0f);
    return true;
}

// ─────────────────────────────────────────────────────────────
// Determinism — same inputs produce byte-identical results
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialDeterminismTest,
    "Mythic.LivingWorld.SocialVerbs.Determinism",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialDeterminismTest::RunTest(const FString &Parameters) {
    const FMythicPersonalityFragment P = MakePersonality(/*Fight*/ 0.6f, /*Flee*/ 0.3f, /*Submit*/ 0.2f, /*Enforce*/ 0.7f);
    for (uint8 v = 0; v < static_cast<uint8>(EMythicSocialVerb::COUNT); ++v) {
        const EMythicSocialVerb V = static_cast<EMythicSocialVerb>(v);
        for (const float Standing : {-75.0f, 0.0f, 75.0f}) {
            const FMythicSocialReactionResult A = Resolve(V, P, Standing);
            const FMythicSocialReactionResult B = Resolve(V, P, Standing);
            TestEqual(TEXT("Reaction is deterministic"), A.Reaction, B.Reaction);
            TestEqual(TEXT("StandingDelta is deterministic"), A.StandingDelta, B.StandingDelta);
            TestEqual(TEXT("bSetHostile is deterministic"), A.bSetHostile, B.bSetHostile);
            TestEqual(TEXT("bAlertGuards is deterministic"), A.bAlertGuards, B.bAlertGuards);
        }
    }
    return true;
}

// ─────────────────────────────────────────────────────────────
// DefaultBarkFor — never empty for any (verb, reaction) pair
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialBarkNonEmptyTest,
    "Mythic.LivingWorld.SocialVerbs.Bark.NonEmpty",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialBarkNonEmptyTest::RunTest(const FString &Parameters) {
    for (uint8 v = 0; v < static_cast<uint8>(EMythicSocialVerb::COUNT); ++v) {
        for (uint8 r = 0; r < static_cast<uint8>(EMythicSocialReaction::COUNT); ++r) {
            const FText Bark = UMythicSocialVerbLibrary::DefaultBarkFor(
                static_cast<EMythicSocialVerb>(v), static_cast<EMythicSocialReaction>(r));
            TestFalse(TEXT("DefaultBarkFor is never empty"), Bark.IsEmpty());
        }
    }
    return true;
}
