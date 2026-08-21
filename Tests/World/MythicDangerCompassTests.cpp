
#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Territory/MythicDanger.h"
#include "World/LivingWorld/Encounters/EncounterTemplate.h"
#include "UI/WarMap/MythicCompass.h"
#include "UI/WarMap/MythicWarMapTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDangerCompassTest,
    "Mythic.World.DangerCompass",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDangerCompassTest::RunTest(const FString& Parameters) {
    const FMythicDangerTierParams P;

    TestEqual(TEXT("Chebyshev (0,0)->(3,4) = 4"),
              FMythicDanger::ChebyshevDistance(FMythicCellCoord(0, 0), FMythicCellCoord(3, 4)), 4);
    TestEqual(TEXT("Chebyshev same cell = 0"),
              FMythicDanger::ChebyshevDistance(FMythicCellCoord(5, 5), FMythicCellCoord(5, 5)), 0);
    TestEqual(TEXT("Chebyshev negatives (-2,1)->(1,-3) = 4"),
              FMythicDanger::ChebyshevDistance(FMythicCellCoord(-2, 1), FMythicCellCoord(1, -3)), 4);
    TestEqual(TEXT("Chebyshev X-dominant (0,0)->(7,2) = 7"),
              FMythicDanger::ChebyshevDistance(FMythicCellCoord(0, 0), FMythicCellCoord(7, 2)), 7);
    TestEqual(TEXT("Chebyshev symmetric A<->B"),
              FMythicDanger::ChebyshevDistance(FMythicCellCoord(2, 9), FMythicCellCoord(-4, 1)),
              FMythicDanger::ChebyshevDistance(FMythicCellCoord(-4, 1), FMythicCellCoord(2, 9)));

    TestEqual(TEXT("Capital cell is always Safe (even far + strong)"),
              FMythicDanger::ComputeDangerTier(1000, 1.0f, true, P), EMythicDangerTier::Safe);
    TestEqual(TEXT("At-core, no strength -> Safe"),
              FMythicDanger::ComputeDangerTier(0, 0.0f, false, P), EMythicDangerTier::Safe);
    TestEqual(TEXT("Distance 3 -> Low"),
              FMythicDanger::ComputeDangerTier(3, 0.0f, false, P), EMythicDangerTier::Low);
    TestEqual(TEXT("Distance 8 -> Moderate"),
              FMythicDanger::ComputeDangerTier(8, 0.0f, false, P), EMythicDangerTier::Moderate);
    TestEqual(TEXT("Distance 15 -> High"),
              FMythicDanger::ComputeDangerTier(15, 0.0f, false, P), EMythicDangerTier::High);
    TestEqual(TEXT("Distance 25 -> Extreme"),
              FMythicDanger::ComputeDangerTier(25, 0.0f, false, P), EMythicDangerTier::Extreme);
    TestEqual(TEXT("Far + max strength clamps to Extreme (COUNT-1)"),
              FMythicDanger::ComputeDangerTier(100000, 1.0f, false, P), EMythicDangerTier::Extreme);
    TestTrue(TEXT("Result never reaches the sentinel COUNT"),
             static_cast<uint8>(FMythicDanger::ComputeDangerTier(100000, 1.0f, false, P)) <
                 static_cast<uint8>(EMythicDangerTier::COUNT));

    TestEqual(TEXT("At-core, max strength -> Moderate (strength contribution)"),
              FMythicDanger::ComputeDangerTier(0, 1.0f, false, P), EMythicDangerTier::Moderate);

    {
        uint8 Prev = 0;
        bool bMono = true;
        for (int32 D = 0; D <= 40; ++D) {
            const uint8 Cur = static_cast<uint8>(FMythicDanger::ComputeDangerTier(D, 0.4f, false, P));
            if (Cur < Prev) { bMono = false; break; }
            Prev = Cur;
        }
        TestTrue(TEXT("ComputeDangerTier is monotonic non-decreasing in distance"), bMono);
    }

    {
        uint8 Prev = 0;
        bool bMono = true;
        for (int32 S = 0; S <= 20; ++S) {
            const float Strength = static_cast<float>(S) / 20.0f;
            const uint8 Cur = static_cast<uint8>(FMythicDanger::ComputeDangerTier(10, Strength, false, P));
            if (Cur < Prev) { bMono = false; break; }
            Prev = Cur;
        }
        TestTrue(TEXT("ComputeDangerTier is monotonic non-decreasing in strength"), bMono);
    }

    TestEqual(TEXT("Safe tier adds 0 members"),
              MythicEncounterDefaults::DangerScaledEntityCount(3, EMythicDangerTier::Safe), 3);
    TestEqual(TEXT("Extreme tier adds 4 members"),
              MythicEncounterDefaults::DangerScaledEntityCount(3, EMythicDangerTier::Extreme), 7);
    TestEqual(TEXT("Danger bump clamps to MaxEntityCount"),
              MythicEncounterDefaults::DangerScaledEntityCount(19, EMythicDangerTier::Extreme, 20), 20);
    TestEqual(TEXT("Result is never below 1"),
              MythicEncounterDefaults::DangerScaledEntityCount(0, EMythicDangerTier::Safe), 1);

    const FVector Eye(0.0f, 0.0f, 0.0f);
    TestTrue(TEXT("Target dead ahead ~ 0 deg"),
             FMath::Abs(FMythicCompass::CompassBearingDegrees(0.0f, Eye, FVector(100.0f, 0.0f, 0.0f))) < 1.0f);
    TestTrue(TEXT("Target behind ~ +/-180 deg"),
             FMath::Abs(FMath::Abs(FMythicCompass::CompassBearingDegrees(0.0f, Eye, FVector(-100.0f, 0.0f, 0.0f))) - 180.0f) < 1.0f);
    TestTrue(TEXT("Target to the right is > 0"),
             FMythicCompass::CompassBearingDegrees(0.0f, Eye, FVector(0.0f, 100.0f, 0.0f)) > 0.0f);
    TestTrue(TEXT("Target to the left is < 0"),
             FMythicCompass::CompassBearingDegrees(0.0f, Eye, FVector(0.0f, -100.0f, 0.0f)) < 0.0f);
    TestTrue(TEXT("Bearing is relative to view yaw (facing target -> ~0)"),
             FMath::Abs(FMythicCompass::CompassBearingDegrees(90.0f, Eye, FVector(0.0f, 100.0f, 0.0f))) < 1.0f);
    {
        const float B = FMythicCompass::CompassBearingDegrees(170.0f, Eye, FVector(-100.0f, -1.0f, 0.0f));
        TestTrue(TEXT("Bearing stays within [-180,180]"), B >= -180.0f && B <= 180.0f);
    }
    TestEqual(TEXT("Coincident points -> 0 bearing"),
              FMythicCompass::CompassBearingDegrees(37.0f, Eye, Eye), 0.0f);

    TestEqual(TEXT("Bearing 0 -> strip center"),
              FMythicCompass::CompassStripX(0.0f, 45.0f, 200.0f), 100.0f, 0.01f);
    TestEqual(TEXT("Bearing +HalfFov -> right edge"),
              FMythicCompass::CompassStripX(45.0f, 45.0f, 200.0f), 200.0f, 0.01f);
    TestEqual(TEXT("Bearing -HalfFov -> left edge (0)"),
              FMythicCompass::CompassStripX(-45.0f, 45.0f, 200.0f), 0.0f, 0.01f);
    TestTrue(TEXT("Bearing just beyond HalfFov is culled (negative)"),
             FMythicCompass::CompassStripX(46.0f, 45.0f, 200.0f) < 0.0f);
    TestTrue(TEXT("Bearing well beyond HalfFov is culled (negative)"),
             FMythicCompass::CompassStripX(120.0f, 45.0f, 200.0f) < 0.0f);
    TestTrue(TEXT("Within-arc result stays in [0, width]"),
             [] {
                 const float X = FMythicCompass::CompassStripX(20.0f, 45.0f, 200.0f);
                 return X >= 0.0f && X <= 200.0f;
             }());
    TestTrue(TEXT("Degenerate HalfFov is culled (negative)"),
             FMythicCompass::CompassStripX(0.0f, 0.0f, 200.0f) < 0.0f);
    TestTrue(TEXT("Degenerate strip width is culled (negative)"),
             FMythicCompass::CompassStripX(0.0f, 45.0f, 0.0f) < 0.0f);

    TestTrue(TEXT("Objective marker kind precedes COUNT"),
             static_cast<uint8>(EMythicWarMapMarkerKind::Objective) < static_cast<uint8>(EMythicWarMapMarkerKind::COUNT));
    TestTrue(TEXT("Waypoint marker kind precedes COUNT"),
             static_cast<uint8>(EMythicWarMapMarkerKind::Waypoint) < static_cast<uint8>(EMythicWarMapMarkerKind::COUNT));
    TestEqual(TEXT("FMythicCompassMarker defaults to Waypoint kind"),
              FMythicCompassMarker().Kind, EMythicWarMapMarkerKind::Waypoint);

    return true;
}
