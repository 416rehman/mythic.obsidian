
#include "Misc/AutomationTest.h"
#include "AI/NPCs/MythicSocialVerbs.h"
#include "Mass/Fragments/MythicMassFragments.h"

namespace {
    constexpr float kHostileThreshold = -50.0f;
    constexpr float kFriendlyThreshold = 50.0f;

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
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialGreetBandsTest,
    "Mythic.LivingWorld.SocialVerbs.Greet.Bands",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialGreetBandsTest::RunTest(const FString &Parameters) {
    const FMythicPersonalityFragment P = MakePersonality();

    const FMythicSocialReactionResult Friendly = Resolve(EMythicSocialVerb::Greet, P, 75.0f);
    TestEqual(TEXT("Greet at friendly standing → Warm"), Friendly.Reaction, EMythicSocialReaction::Warm);

    const FMythicSocialReactionResult Neutral = Resolve(EMythicSocialVerb::Greet, P, 0.0f);
    TestEqual(TEXT("Greet at neutral standing → Neutral"), Neutral.Reaction, EMythicSocialReaction::Neutral);

    const FMythicSocialReactionResult Hostile = Resolve(EMythicSocialVerb::Greet, P, -75.0f);
    TestEqual(TEXT("Greet at hostile standing → Cold"), Hostile.Reaction, EMythicSocialReaction::Cold);

    TestEqual(TEXT("Greet never changes standing (friendly)"), Friendly.StandingDelta, 0.0f);
    TestEqual(TEXT("Greet never changes standing (neutral)"), Neutral.StandingDelta, 0.0f);
    TestEqual(TEXT("Greet never changes standing (hostile)"), Hostile.StandingDelta, 0.0f);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialComplimentTest,
    "Mythic.LivingWorld.SocialVerbs.Compliment.Standing",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialComplimentTest::RunTest(const FString &Parameters) {
    const FMythicPersonalityFragment P = MakePersonality();

    const FMythicSocialReactionResult Neutral = Resolve(EMythicSocialVerb::Compliment, P, 0.0f);
    TestEqual(TEXT("Compliment at neutral → Warm"), Neutral.Reaction, EMythicSocialReaction::Warm);
    TestTrue(TEXT("Compliment at neutral gains standing"), Neutral.StandingDelta > 0.0f);

    const FMythicSocialReactionResult Friendly = Resolve(EMythicSocialVerb::Compliment, P, 75.0f);
    TestEqual(TEXT("Compliment at friendly → Warm"), Friendly.Reaction, EMythicSocialReaction::Warm);
    TestTrue(TEXT("Compliment at friendly gains standing"), Friendly.StandingDelta > 0.0f);

    const FMythicSocialReactionResult Hostile = Resolve(EMythicSocialVerb::Compliment, P, -75.0f);
    TestEqual(TEXT("Compliment at hostile → Cold"), Hostile.Reaction, EMythicSocialReaction::Cold);
    TestEqual(TEXT("Cold compliment grants no standing"), Hostile.StandingDelta, 0.0f);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialAngeredTest,
    "Mythic.LivingWorld.SocialVerbs.Hostile.Angered",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialAngeredTest::RunTest(const FString &Parameters) {
    const FMythicPersonalityFragment Brawler = MakePersonality( 0.9f);
    const FMythicSocialReactionResult R = Resolve(EMythicSocialVerb::Bully, Brawler, 0.0f);
    TestEqual(TEXT("High-Fight Bully → Angered"), R.Reaction, EMythicSocialReaction::Angered);
    TestTrue(TEXT("Angered sets hostile"), R.bSetHostile);
    TestFalse(TEXT("Angered does not alert guards"), R.bAlertGuards);
    TestTrue(TEXT("Bully costs standing (negative)"), R.StandingDelta < 0.0f);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialIntimidatedTest,
    "Mythic.LivingWorld.SocialVerbs.Hostile.Intimidated",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialIntimidatedTest::RunTest(const FString &Parameters) {
    const FMythicPersonalityFragment Coward = MakePersonality( 0.1f, 0.9f, 0.2f, 0.0f);
    const FMythicSocialReactionResult R = Resolve(EMythicSocialVerb::Threaten, Coward, 0.0f);
    TestEqual(TEXT("High-Flee low-Fight Threaten → Intimidated"), R.Reaction, EMythicSocialReaction::Intimidated);
    TestFalse(TEXT("Intimidated does not set hostile"), R.bSetHostile);
    TestFalse(TEXT("Intimidated does not alert guards"), R.bAlertGuards);

    const FMythicPersonalityFragment Submissive = MakePersonality( 0.1f, 0.1f, 0.9f, 0.0f);
    const FMythicSocialReactionResult R2 = Resolve(EMythicSocialVerb::Provoke, Submissive, 0.0f);
    TestEqual(TEXT("High-Submit low-Fight Provoke → Intimidated"), R2.Reaction, EMythicSocialReaction::Intimidated);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialCallGuardsTest,
    "Mythic.LivingWorld.SocialVerbs.Hostile.CallGuards",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialCallGuardsTest::RunTest(const FString &Parameters) {
    const FMythicPersonalityFragment Enforcer = MakePersonality( 0.2f, 0.0f, 0.0f, 0.8f);
    const FMythicSocialReactionResult R = Resolve(EMythicSocialVerb::Provoke, Enforcer, 0.0f);
    TestEqual(TEXT("High-Enforce low-Fight Provoke → CallGuards"), R.Reaction, EMythicSocialReaction::CallGuards);
    TestTrue(TEXT("CallGuards sets alert flag"), R.bAlertGuards);
    TestFalse(TEXT("CallGuards does not personally engage"), R.bSetHostile);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialOrderingTest,
    "Mythic.LivingWorld.SocialVerbs.Hostile.Ordering",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialOrderingTest::RunTest(const FString &Parameters) {
    const FMythicPersonalityFragment Mid = MakePersonality( 0.50f);

    const FMythicSocialReactionResult Bully = Resolve(EMythicSocialVerb::Bully, Mid, 0.0f);
    const FMythicSocialReactionResult Threaten = Resolve(EMythicSocialVerb::Threaten, Mid, 0.0f);
    const FMythicSocialReactionResult Provoke = Resolve(EMythicSocialVerb::Provoke, Mid, 0.0f);

    TestEqual(TEXT("Mid-Fight Bully angers (lowest threshold)"), Bully.Reaction, EMythicSocialReaction::Angered);
    TestEqual(TEXT("Mid-Fight Threaten angers (mid threshold)"), Threaten.Reaction, EMythicSocialReaction::Angered);
    TestNotEqual(TEXT("Mid-Fight Provoke does NOT anger (highest threshold)"), Provoke.Reaction, EMythicSocialReaction::Angered);

    TestTrue(TEXT("|Bully delta| > |Threaten delta|"), FMath::Abs(Bully.StandingDelta) > FMath::Abs(Threaten.StandingDelta));
    TestTrue(TEXT("|Threaten delta| > |Provoke delta|"), FMath::Abs(Threaten.StandingDelta) > FMath::Abs(Provoke.StandingDelta));
    TestTrue(TEXT("Bully delta is negative"), Bully.StandingDelta < 0.0f);
    TestTrue(TEXT("Threaten delta is negative"), Threaten.StandingDelta < 0.0f);
    TestTrue(TEXT("Provoke delta is negative"), Provoke.StandingDelta < 0.0f);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocialDeterminismTest,
    "Mythic.LivingWorld.SocialVerbs.Determinism",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocialDeterminismTest::RunTest(const FString &Parameters) {
    const FMythicPersonalityFragment P = MakePersonality( 0.6f, 0.3f, 0.2f, 0.7f);
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
