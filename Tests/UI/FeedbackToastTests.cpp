// Mythic UI — Unified-feedback screen-toast presentation-math unit tests
// Covers: UMythicDamageNumberSubsystem::ComputeToastAlpha / ComputeToastSlideOffset / ComputeToastStackOffset
// (the pure fade, slide-in, and vertical-stack curves for screen-space toasts).
// Run via: Session Frontend -> Automation -> Mythic.UI.Feedback

#include "Misc/AutomationTest.h"
#include "UI/MythicDamageNumberSubsystem.h"

// ═══════════════════════════════════════════════════════════════
// Toast fade — ComputeToastAlpha
// (ramp 0->1 over FadeIn; hold 1; ramp 1->0 over FadeOut; clamped [0,1])
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFeedbackToastAlphaTest,
    "Mythic.UI.Feedback.ToastAlpha",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFeedbackToastAlphaTest::RunTest(const FString &Parameters) {
    auto Alpha = &UMythicDamageNumberSubsystem::ComputeToastAlpha;
    const float Tol = 1.e-4f;
    const float Life = 3.0f, In = 0.2f, Out = 0.4f;

    // Fade-in ramp 0 -> 1 over the first In seconds.
    TestEqual(TEXT("t=0 -> 0 (fade-in start)"), Alpha(0.0f, Life, In, Out), 0.0f, Tol);
    TestEqual(TEXT("t=half-In -> 0.5"), Alpha(0.1f, Life, In, Out), 0.5f, Tol);
    TestEqual(TEXT("t=In -> 1.0 (fade-in done)"), Alpha(0.2f, Life, In, Out), 1.0f, Tol);

    // Hold at 1.0 through the middle.
    TestEqual(TEXT("t=mid -> 1.0"), Alpha(1.5f, Life, In, Out), 1.0f, Tol);
    TestEqual(TEXT("t=fade-out-start -> 1.0"), Alpha(2.6f, Life, In, Out), 1.0f, Tol);

    // Fade-out ramp 1 -> 0 over the last Out seconds.
    TestEqual(TEXT("t=mid-fade-out -> 0.5"), Alpha(2.8f, Life, In, Out), 0.5f, Tol);
    TestEqual(TEXT("t=Lifetime -> 0"), Alpha(3.0f, Life, In, Out), 0.0f, Tol);

    // Clamp: past lifetime stays 0; degenerate lifetime -> 0.
    TestEqual(TEXT("t past lifetime -> 0 (clamped)"), Alpha(9.0f, Life, In, Out), 0.0f, Tol);
    TestEqual(TEXT("zero lifetime -> 0"), Alpha(0.5f, 0.0f, In, Out), 0.0f, Tol);

    // No fades configured -> full opacity throughout.
    TestEqual(TEXT("no fades -> 1.0 mid"), Alpha(1.5f, Life, 0.0f, 0.0f), 1.0f, Tol);
    TestEqual(TEXT("no fades -> 1.0 at start"), Alpha(0.0f, Life, 0.0f, 0.0f), 1.0f, Tol);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Toast slide-in — ComputeToastSlideOffset
// (eases SlideDistance -> 0 over FadeInTime via cubic ease-out; 0 once settled)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFeedbackToastSlideTest,
    "Mythic.UI.Feedback.ToastSlide",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFeedbackToastSlideTest::RunTest(const FString &Parameters) {
    auto Slide = &UMythicDamageNumberSubsystem::ComputeToastSlideOffset;
    const float Tol = 1.e-4f;
    const float In = 0.2f, Dist = 60.0f;

    // At spawn it is fully offset; once past FadeIn it has settled to 0.
    TestEqual(TEXT("t=0 -> full distance"), Slide(0.0f, In, Dist), Dist, Tol);
    TestEqual(TEXT("t<=0 -> full distance"), Slide(-1.0f, In, Dist), Dist, Tol);
    TestEqual(TEXT("t=In -> 0 (settled)"), Slide(0.2f, In, Dist), 0.0f, Tol);
    TestEqual(TEXT("t past In -> 0"), Slide(1.0f, In, Dist), 0.0f, Tol);

    // Degenerate fade-in -> instantly settled.
    TestEqual(TEXT("FadeIn<=0 -> 0"), Slide(0.05f, 0.0f, Dist), 0.0f, Tol);

    // Mid-way: strictly between 0 and Dist, and monotonically decreasing (ease-out moves fast early).
    const float Mid = Slide(0.1f, In, Dist);
    TestTrue(TEXT("mid in (0, Dist)"), Mid > 0.0f && Mid < Dist);
    TestTrue(TEXT("decreasing over time"), Slide(0.05f, In, Dist) > Slide(0.15f, In, Dist));
    // Cubic ease-out: at the half-way point in time it has already covered MORE than half the distance.
    TestTrue(TEXT("ease-out covers >half by mid-time"), Mid < Dist * 0.5f);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Toast stacking — ComputeToastStackOffset
// (vertical pixel offset = max(0, slot) * step)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFeedbackToastStackTest,
    "Mythic.UI.Feedback.ToastStack",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFeedbackToastStackTest::RunTest(const FString &Parameters) {
    auto Stack = &UMythicDamageNumberSubsystem::ComputeToastStackOffset;
    const float Tol = 1.e-4f;
    const float Step = 40.0f;

    TestEqual(TEXT("slot 0 -> 0 (at the anchor)"), Stack(0, Step), 0.0f, Tol);
    TestEqual(TEXT("slot 1 -> 1 step"), Stack(1, Step), 40.0f, Tol);
    TestEqual(TEXT("slot 3 -> 3 steps"), Stack(3, Step), 120.0f, Tol);
    TestEqual(TEXT("negative slot clamps to 0"), Stack(-2, Step), 0.0f, Tol);
    // Linear / monotonic.
    TestTrue(TEXT("higher slot stacks further"), Stack(4, Step) > Stack(2, Step));

    return true;
}
