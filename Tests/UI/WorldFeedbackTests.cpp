// Mythic UI — World-feedback presentation-math unit tests
// Covers: UMythicWorldFeedbackSubsystem::ComputeAlpha / ComputeRise (the pure fade + rise curves).
// Run via: Session Frontend -> Automation -> Mythic.UI.WorldFeedback

#include "Misc/AutomationTest.h"
#include "UI/MythicWorldFeedbackSubsystem.h"

// ═══════════════════════════════════════════════════════════════
// Callout fade — UMythicWorldFeedbackSubsystem::ComputeAlpha
// (ramp 0->1 over FadeIn at the start; hold 1; ramp 1->0 over FadeOut at the end; clamped [0,1])
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWorldFeedbackAlphaTest,
    "Mythic.UI.WorldFeedback.Alpha",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWorldFeedbackAlphaTest::RunTest(const FString &Parameters) {
    auto Alpha = &UMythicWorldFeedbackSubsystem::ComputeAlpha;
    const float Tol = 1.e-4f;
    const float Life = 2.0f, In = 0.2f, Out = 0.5f;

    // Fade-in ramp 0 -> 1 over the first In seconds.
    TestEqual(TEXT("t=0 -> 0 (fade-in start)"), Alpha(0.0f, Life, In, Out), 0.0f, Tol);
    TestEqual(TEXT("t=half-In -> 0.5"), Alpha(0.1f, Life, In, Out), 0.5f, Tol);
    TestEqual(TEXT("t=In -> 1.0 (fade-in done)"), Alpha(0.2f, Life, In, Out), 1.0f, Tol);

    // Hold at 1.0 through the middle.
    TestEqual(TEXT("t=mid -> 1.0"), Alpha(1.0f, Life, In, Out), 1.0f, Tol);
    TestEqual(TEXT("t=fade-out-start -> 1.0"), Alpha(1.5f, Life, In, Out), 1.0f, Tol);

    // Fade-out ramp 1 -> 0 over the last Out seconds.
    TestEqual(TEXT("t=mid-fade-out -> 0.5"), Alpha(1.75f, Life, In, Out), 0.5f, Tol);
    TestEqual(TEXT("t=Lifetime -> 0"), Alpha(2.0f, Life, In, Out), 0.0f, Tol);

    // Clamp: past lifetime stays 0; degenerate lifetime -> 0.
    TestEqual(TEXT("t past lifetime -> 0 (clamped)"), Alpha(5.0f, Life, In, Out), 0.0f, Tol);
    TestEqual(TEXT("zero lifetime -> 0"), Alpha(0.5f, 0.0f, In, Out), 0.0f, Tol);

    // No fades configured -> full opacity throughout.
    TestEqual(TEXT("no fades -> 1.0 mid"), Alpha(1.0f, Life, 0.0f, 0.0f), 1.0f, Tol);
    TestEqual(TEXT("no fades -> 1.0 at start"), Alpha(0.0f, Life, 0.0f, 0.0f), 1.0f, Tol);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Callout rise — UMythicWorldFeedbackSubsystem::ComputeRise
// (monotonic world-units risen = max(0,Elapsed) * RiseSpeed)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWorldFeedbackRiseTest,
    "Mythic.UI.WorldFeedback.Rise",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWorldFeedbackRiseTest::RunTest(const FString &Parameters) {
    auto Rise = &UMythicWorldFeedbackSubsystem::ComputeRise;
    const float Tol = 1.e-4f;

    TestEqual(TEXT("1s @ 40 -> 40"), Rise(1.0f, 40.0f), 40.0f, Tol);
    TestEqual(TEXT("0.5s @ 40 -> 20"), Rise(0.5f, 40.0f), 20.0f, Tol);
    TestEqual(TEXT("0s -> 0"), Rise(0.0f, 40.0f), 0.0f, Tol);
    TestEqual(TEXT("negative elapsed clamps to 0"), Rise(-1.0f, 40.0f), 0.0f, Tol);
    TestEqual(TEXT("zero speed -> 0"), Rise(2.0f, 0.0f), 0.0f, Tol);
    // Monotonic in time.
    TestTrue(TEXT("rises with time"), Rise(2.0f, 40.0f) > Rise(1.0f, 40.0f));

    return true;
}
