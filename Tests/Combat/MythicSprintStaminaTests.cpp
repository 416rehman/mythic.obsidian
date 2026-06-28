// Mythic — stamina-gated sprint unit tests.
// Covers the pure decisions the server-side sprint drain (UMythicLifeComponent::ApplyRegen) is built on: the per-tick
// stamina drain and the exhaustion-recovery hysteresis gate. The live drain/exhaustion behaviour is server-driven and
// PIE-verified; these lock the math.
// Run via: Session Frontend → Automation → Mythic.Combat.SprintStamina

#include "Misc/AutomationTest.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSprintStaminaTest,
    "Mythic.Combat.SprintStamina",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSprintStaminaTest::RunTest(const FString &Parameters) {
    // ── ComputeStaminaAfterSprintTick(Cur, DrainPerSecond, DeltaSeconds) = max(0, Cur − drain×dt) ──
    TestEqual(TEXT("drains drain×dt"), UMythicLifeComponent::ComputeStaminaAfterSprintTick(100.0f, 15.0f, 0.5f), 92.5f);
    TestEqual(TEXT("clamps to 0 — never negative"), UMythicLifeComponent::ComputeStaminaAfterSprintTick(5.0f, 15.0f, 1.0f), 0.0f);
    TestEqual(TEXT("exactly empties at the boundary"), UMythicLifeComponent::ComputeStaminaAfterSprintTick(7.5f, 15.0f, 0.5f), 0.0f);
    TestEqual(TEXT("zero drain is a no-op"), UMythicLifeComponent::ComputeStaminaAfterSprintTick(50.0f, 0.0f, 0.5f), 50.0f);
    TestEqual(TEXT("negative drain clamped to 0 (no free stamina)"), UMythicLifeComponent::ComputeStaminaAfterSprintTick(50.0f, -10.0f, 0.5f), 50.0f);
    TestEqual(TEXT("negative dt clamped to 0"), UMythicLifeComponent::ComputeStaminaAfterSprintTick(50.0f, 15.0f, -0.5f), 50.0f);

    // ── ShouldRecoverFromExhaustion(Cur, Max, RecoverFraction) = Cur >= clamp(Frac)×Max (Max<=0 → false) ──
    TestTrue(TEXT("recovers AT the threshold"), UMythicLifeComponent::ShouldRecoverFromExhaustion(20.0f, 100.0f, 0.2f));
    TestTrue(TEXT("recovers above the threshold"), UMythicLifeComponent::ShouldRecoverFromExhaustion(50.0f, 100.0f, 0.2f));
    TestFalse(TEXT("not recovered below the threshold"), UMythicLifeComponent::ShouldRecoverFromExhaustion(19.9f, 100.0f, 0.2f));
    TestFalse(TEXT("a zero stamina pool never recovers (degenerate)"), UMythicLifeComponent::ShouldRecoverFromExhaustion(0.0f, 0.0f, 0.2f));
    TestTrue(TEXT("fraction >1 clamps to full-max, met at max"), UMythicLifeComponent::ShouldRecoverFromExhaustion(100.0f, 100.0f, 2.0f));
    TestFalse(TEXT("fraction >1 clamps to full-max, unmet below"), UMythicLifeComponent::ShouldRecoverFromExhaustion(99.0f, 100.0f, 2.0f));
    TestTrue(TEXT("fraction 0 always recovered"), UMythicLifeComponent::ShouldRecoverFromExhaustion(0.0f, 100.0f, 0.0f));

    return true;
}
