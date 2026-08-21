
#include "Misc/AutomationTest.h"
#include "World/LivingWorld/EmergentQuests/MythicApexHuntRules.h"
#include "World/Hunting/MythicSpoorRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicApexHuntTest,
    "Mythic.World.ApexHunts",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicApexHuntTest::RunTest(const FString &Parameters) {
    using Rules = FMythicApexHuntRules;

    {
        TestFalse(TEXT("master OFF → never offers"), Rules::ShouldOfferApexHunt(false, true, 0.0f, 5.0f, false, -1.0, 0.0f));
        TestTrue(TEXT("master ON + all gates pass → offers"), Rules::ShouldOfferApexHunt(true, true, 0.0f, 5.0f, false, -1.0, 0.0f));
    }

    {
        TestFalse(TEXT("below Full tier → no offer"), Rules::ShouldOfferApexHunt(true, false, 0.0f, 5.0f, false, -1.0, 0.0f));
        TestTrue(TEXT("Full tier → offer"), Rules::ShouldOfferApexHunt(true, true, 0.0f, 5.0f, false, -1.0, 0.0f));
    }

    {
        TestTrue(TEXT("healthy population (pressure below threshold) → offer"),
                 Rules::ShouldOfferApexHunt(true, true, 4.9f, 5.0f, false, -1.0, 0.0f));
        TestFalse(TEXT("pressure AT the threshold → no offer"),
                  Rules::ShouldOfferApexHunt(true, true, 5.0f, 5.0f, false, -1.0, 0.0f));
        TestFalse(TEXT("over-hunted → no offer"),
                  Rules::ShouldOfferApexHunt(true, true, 50.0f, 5.0f, false, -1.0, 0.0f));
        TestTrue(TEXT("threshold <= 0 disables the population gate"),
                 Rules::ShouldOfferApexHunt(true, true, 999.0f, 0.0f, false, -1.0, 0.0f));
    }

    {
        TestFalse(TEXT("a live offer for the species blocks a second (idempotence)"),
                  Rules::ShouldOfferApexHunt(true, true, 0.0f, 5.0f, true, -1.0, 0.0f));
        TestFalse(TEXT("inside the cooldown → no re-offer"),
                  Rules::ShouldOfferApexHunt(true, true, 0.0f, 5.0f, false, 100.0, 1800.0f));
        TestTrue(TEXT("cooldown elapsed → re-offer"),
                 Rules::ShouldOfferApexHunt(true, true, 0.0f, 5.0f, false, 1800.0, 1800.0f));
        TestTrue(TEXT("never offered (negative sentinel) passes the cooldown"),
                 Rules::ShouldOfferApexHunt(true, true, 0.0f, 5.0f, false, -1.0, 1800.0f));
    }

    {
        using Spoor = FMythicSpoorRules;

        TestEqual(TEXT("clear weather keeps the base lifetime"), Spoor::EffectiveLifetime(600.0f, false, 0.5f), 600.0f);
        TestEqual(TEXT("rain halves it (default multiplier)"), Spoor::EffectiveLifetime(600.0f, true, 0.5f), 300.0f);

        TestEqual(TEXT("fresh at age 0"), Spoor::FreshnessAtAge(0.0f, 600.0f), 1.0f);
        TestEqual(TEXT("half-life = half-fresh"), Spoor::FreshnessAtAge(300.0f, 600.0f), 0.5f);
        TestEqual(TEXT("expired clamps at 0"), Spoor::FreshnessAtAge(9999.0f, 600.0f), 0.0f);

        TestFalse(TEXT("above the stale threshold reads fine"), Spoor::IsStale(0.5f, 0.15f));
        TestTrue(TEXT("below it the trail is cold"), Spoor::IsStale(0.1f, 0.15f));
        TestFalse(TEXT("threshold 0 never stales"), Spoor::IsStale(0.0f, 0.0f));

        const FVector From(0, 0, 100);
        const FVector Anchor(10000, 0, 0);
        const FVector Step = Spoor::NextStepLocation(From, Anchor, 2500.0f, 25.0f, 0.5f);
        TestTrue(TEXT("dead-on jitter steps straight toward the anchor"), FMath::IsNearlyEqual(Step.X, 2500.0f, 1.0f) && FMath::IsNearlyEqual(Step.Y, 0.0f, 1.0f));
        TestEqual(TEXT("step preserves Z (spawner grounds it)"), Step.Z, 100.0);

        const FVector Jittered = Spoor::NextStepLocation(From, Anchor, 2500.0f, 25.0f, 1.0f);
        TestTrue(TEXT("jittered step still strides one StepDistance"), FMath::IsNearlyEqual(FVector::Dist2D(From, Jittered), 2500.0f, 1.0f));
        TestTrue(TEXT("same roll → same step (deterministic)"),
                 Spoor::NextStepLocation(From, Anchor, 2500.0f, 25.0f, 1.0f).Equals(Jittered, 0.01f));

        const FVector NearAnchor(9000, 0, 50);
        TestTrue(TEXT("within one stride → final step"), Spoor::IsFinalStep(NearAnchor, Anchor, 2500.0f));
        const FVector Final = Spoor::NextStepLocation(NearAnchor, Anchor, 2500.0f, 25.0f, 0.0f);
        TestTrue(TEXT("final step lands ON the anchor (XY)"), FMath::IsNearlyEqual(Final.X, Anchor.X, 0.01f) && FMath::IsNearlyEqual(Final.Y, Anchor.Y, 0.01f));
    }

    return true;
}
