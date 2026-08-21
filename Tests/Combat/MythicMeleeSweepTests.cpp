
#include "Misc/AutomationTest.h"

#include "Engine/HitResult.h"

#include "GAS/Abilities/MythicAnimNotify_SphereOverlap.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicMeleeSweepTest,
    "Mythic.Combat.MeleeSweep",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicMeleeSweepTest::RunTest(const FString &Parameters) {
    const FVector Origin(0, 0, 0);

    auto MakeHit = [](float X) {
        FHitResult Hit;
        Hit.ImpactPoint = FVector(X, 0, 0);
        return Hit;
    };

    // A sweep query returns hits in whatever order the scene gave them, so the cap must not keep an arbitrary few.
    {
        TArray<FHitResult> Hits = {MakeHit(300.0f), MakeHit(50.0f), MakeHit(150.0f)};
        UMythicAnimNotify_SphereOverlap::OrderAndCapHits(Hits, Origin, 0);
        TestEqual(TEXT("no cap keeps every hit"), Hits.Num(), 3);
        TestEqual(TEXT("the nearest comes first"), Hits[0].ImpactPoint.X, 50.0);
        TestEqual(TEXT("then the next nearest"), Hits[1].ImpactPoint.X, 150.0);
        TestEqual(TEXT("then the furthest"), Hits[2].ImpactPoint.X, 300.0);
    }
    {
        TArray<FHitResult> Hits = {MakeHit(300.0f), MakeHit(50.0f), MakeHit(150.0f)};
        UMythicAnimNotify_SphereOverlap::OrderAndCapHits(Hits, Origin, 2);
        TestEqual(TEXT("a cap trims the swing"), Hits.Num(), 2);
        // The point of ordering before capping: a capped swing keeps what the blade reached first.
        TestEqual(TEXT("a capped swing keeps the nearest"), Hits[0].ImpactPoint.X, 50.0);
        TestEqual(TEXT("and the second nearest, not whatever the query returned first"), Hits[1].ImpactPoint.X, 150.0);
    }
    {
        TArray<FHitResult> Hits = {MakeHit(10.0f)};
        UMythicAnimNotify_SphereOverlap::OrderAndCapHits(Hits, Origin, 5);
        TestEqual(TEXT("a cap above the count changes nothing"), Hits.Num(), 1);

        TArray<FHitResult> Empty;
        UMythicAnimNotify_SphereOverlap::OrderAndCapHits(Empty, Origin, 3);
        TestEqual(TEXT("an empty swing stays empty"), Empty.Num(), 0);
    }
    {
        // Distance is measured from the sphere, not the world origin, or a cap keeps the wrong targets.
        const FVector Offset(1000, 0, 0);
        TArray<FHitResult> Hits = {MakeHit(0.0f), MakeHit(950.0f)};
        UMythicAnimNotify_SphereOverlap::OrderAndCapHits(Hits, Offset, 1);
        TestEqual(TEXT("nearest is relative to the sweep origin"), Hits[0].ImpactPoint.X, 950.0);
    }

    return true;
}
