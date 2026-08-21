
#include "Misc/AutomationTest.h"
#include "UI/WarMap/MythicWarMapTypes.h"
#include "World/LivingWorld/Factions/FactionColor.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"

namespace {
    FMythicWarMapCell Claimed(uint8 Idx, bool bPlayer = false) {
        FMythicWarMapCell C;
        C.FactionIndex = Idx;
        C.bPlayerOwned = bPlayer;
        return C;
    }

    FColor GreyForIndex(uint8 Idx) {
        return FColor(Idx, Idx, Idx, 255);
    }
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapCoordIndexTest,
    "Mythic.LivingWorld.WarMap.CoordToTexelIndex",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapCoordIndexTest::RunTest(const FString& Parameters) {
    TestEqual(TEXT("(0,0) W=4 -> 0"), MythicWarMap::CoordToTexelIndex(0, 0, 4), 0);
    TestEqual(TEXT("(3,0) W=4 -> 3"), MythicWarMap::CoordToTexelIndex(3, 0, 4), 3);
    TestEqual(TEXT("(0,1) W=4 -> 4"), MythicWarMap::CoordToTexelIndex(0, 1, 4), 4);
    TestEqual(TEXT("(2,3) W=4 -> 14"), MythicWarMap::CoordToTexelIndex(2, 3, 4), 14);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapCellToColorTest,
    "Mythic.LivingWorld.WarMap.CellToColor",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapCellToColorTest::RunTest(const FString& Parameters) {
    FMythicWarMapStyle Style;

    auto ColorFor = [](uint8 Idx) { return GreyForIndex(Idx); };

    {
        FMythicWarMapCell Unclaimed;
        const FColor Out = MythicWarMap::CellToColor(Unclaimed, ColorFor, Style);
        TestEqual(TEXT("Unclaimed alpha is 0"), (int32)Out.A, 0);
        TestEqual(TEXT("Unclaimed matches UnclaimedColor"), Out, Style.UnclaimedColor);
    }

    {
        const FColor Out = MythicWarMap::CellToColor(Claimed(100), ColorFor, Style);
        TestEqual(TEXT("Claimed routes to ColorForIndex"), Out, GreyForIndex(100));
    }

    {
        const FColor NonPlayer = MythicWarMap::CellToColor(Claimed(100, false), ColorFor, Style);
        const FColor Player = MythicWarMap::CellToColor(Claimed(100, true), ColorFor, Style);
        TestTrue(TEXT("Player-owned differs from non-player same-faction"), Player != NonPlayer);
        TestTrue(TEXT("Player-owned is brighter (toward white)"), Player.R > NonPlayer.R);
        TestEqual(TEXT("Player blend R == 178"), (int32)Player.R, 178);
    }

    {
        FMythicWarMapStyle S0 = Style; S0.PlayerTint = 0.0f;
        const FColor P0 = MythicWarMap::CellToColor(Claimed(100, true), ColorFor, S0);
        TestEqual(TEXT("PlayerTint 0 -> faction color"), P0, GreyForIndex(100));

        FMythicWarMapStyle S1 = Style; S1.PlayerTint = 1.0f;
        const FColor P1 = MythicWarMap::CellToColor(Claimed(100, true), ColorFor, S1);
        TestEqual(TEXT("PlayerTint 1 -> PlayerOwnedColor"), P1, S1.PlayerOwnedColor);
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapBorderTest,
    "Mythic.LivingWorld.WarMap.IsBorderCell",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapBorderTest::RunTest(const FString& Parameters) {
    const int32 W = 3, H = 3;

    {
        auto Uniform = [](int32, int32) -> uint8 { return 5; };
        TestFalse(TEXT("Uniform interior cell is not a border"),
                  MythicWarMap::IsBorderCell(1, 1, W, H, Uniform));
        TestFalse(TEXT("Uniform corner cell is not a border (OOB skipped)"),
                  MythicWarMap::IsBorderCell(0, 0, W, H, Uniform));
    }

    {
        auto Mixed = [](int32 X, int32 Y) -> uint8 { return (X == 1 && Y == 1) ? (uint8)7 : (uint8)5; };
        TestTrue(TEXT("Center (different from neighbors) is a border"),
                 MythicWarMap::IsBorderCell(1, 1, W, H, Mixed));
        TestTrue(TEXT("Edge-mid cell adjacent to differing center is a border"),
                 MythicWarMap::IsBorderCell(1, 0, W, H, Mixed));
        TestFalse(TEXT("Corner not adjacent to the differing cell is not a border"),
                  MythicWarMap::IsBorderCell(0, 0, W, H, Mixed));
    }

    {
        auto OneClaimed = [](int32 X, int32 Y) -> uint8 { return (X == 0 && Y == 0) ? (uint8)5 : (uint8)0xFF; };
        TestFalse(TEXT("Unclaimed cell is never a border"),
                  MythicWarMap::IsBorderCell(1, 0, W, H, OneClaimed));
        TestTrue(TEXT("Claimed cell beside unclaimed neighbors is a border"),
                 MythicWarMap::IsBorderCell(0, 0, W, H, OneClaimed));
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapApplyBorderTest,
    "Mythic.LivingWorld.WarMap.ApplyBorder",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapApplyBorderTest::RunTest(const FString& Parameters) {
    FMythicWarMapStyle Style;
    const FColor Base(200, 100, 50, 128);
    const FColor Out = MythicWarMap::ApplyBorder(Base, Style);

    TestEqual(TEXT("R darkened: round(200*0.45)=90"), (int32)Out.R, 90);
    TestEqual(TEXT("G darkened: round(100*0.45)=45"), (int32)Out.G, 45);
    TestEqual(TEXT("B darkened: round(50*0.45)=23"), (int32)Out.B, 23);
    TestEqual(TEXT("Alpha forced opaque"), (int32)Out.A, 255);

    FMythicWarMapStyle S0 = Style; S0.BorderDarken = 0.0f;
    const FColor Black = MythicWarMap::ApplyBorder(Base, S0);
    TestEqual(TEXT("BorderDarken 0 -> R=0"), (int32)Black.R, 0);
    TestEqual(TEXT("BorderDarken 0 -> alpha 255"), (int32)Black.A, 255);
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapWorldToNormTest,
    "Mythic.LivingWorld.WarMap.WorldToNormalized",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapWorldToNormTest::RunTest(const FString& Parameters) {
    const FVector2D Origin(0, 0);
    const float CellSize = 100.0f;
    const int32 W = 4, H = 4;

    {
        const FVector2D N = MythicWarMap::WorldToNormalized(FVector2D(0, 0), Origin, CellSize, W, H);
        TestTrue(TEXT("Origin X ~ 0"), FMath::IsNearlyEqual(N.X, 0.0f, 1e-4f));
        TestTrue(TEXT("Origin Y ~ 1 (flipped)"), FMath::IsNearlyEqual(N.Y, 1.0f, 1e-4f));
    }
    {
        const FVector2D N = MythicWarMap::WorldToNormalized(FVector2D(400, 400), Origin, CellSize, W, H);
        TestTrue(TEXT("Far X ~ 1"), FMath::IsNearlyEqual(N.X, 1.0f, 1e-4f));
        TestTrue(TEXT("Far Y ~ 0 (flipped)"), FMath::IsNearlyEqual(N.Y, 0.0f, 1e-4f));
    }
    {
        const FVector2D N = MythicWarMap::WorldToNormalized(FVector2D(200, 200), Origin, CellSize, W, H);
        TestTrue(TEXT("Center X ~ 0.5"), FMath::IsNearlyEqual(N.X, 0.5f, 1e-4f));
        TestTrue(TEXT("Center Y ~ 0.5"), FMath::IsNearlyEqual(N.Y, 0.5f, 1e-4f));
    }
    {
        const FVector2D Lo = MythicWarMap::WorldToNormalized(FVector2D(-1000, -1000), Origin, CellSize, W, H);
        TestTrue(TEXT("OOB low X clamps to 0"), FMath::IsNearlyEqual(Lo.X, 0.0f, 1e-4f));
        TestTrue(TEXT("OOB low Y clamps to 1 (flipped)"), FMath::IsNearlyEqual(Lo.Y, 1.0f, 1e-4f));
        const FVector2D Hi = MythicWarMap::WorldToNormalized(FVector2D(9999, 9999), Origin, CellSize, W, H);
        TestTrue(TEXT("OOB high X clamps to 1"), FMath::IsNearlyEqual(Hi.X, 1.0f, 1e-4f));
        TestTrue(TEXT("OOB high Y clamps to 0 (flipped)"), FMath::IsNearlyEqual(Hi.Y, 0.0f, 1e-4f));
    }
    {
        const FVector2D D0 = MythicWarMap::WorldToNormalized(FVector2D(200, 200), Origin, 0.0f, W, H);
        TestEqual(TEXT("CellSize 0 -> zero"), D0, FVector2D::ZeroVector);
        const FVector2D D1 = MythicWarMap::WorldToNormalized(FVector2D(200, 200), Origin, CellSize, 0, H);
        TestEqual(TEXT("GridW 0 -> zero"), D1, FVector2D::ZeroVector);
    }

    {
        const FVector2D Off(1000, 2000);
        const FVector2D N = MythicWarMap::WorldToNormalized(Off, Off, CellSize, W, H);
        TestTrue(TEXT("At origin offset -> X 0"), FMath::IsNearlyEqual(N.X, 0.0f, 1e-4f));
        TestTrue(TEXT("At origin offset -> Y 1"), FMath::IsNearlyEqual(N.Y, 1.0f, 1e-4f));
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapNormToWorldTest,
    "Mythic.LivingWorld.WarMap.NormalizedToWorld",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapNormToWorldTest::RunTest(const FString& Parameters) {
    const FVector2D Origin(0, 0);
    const float CellSize = 100.0f;
    const int32 W = 4, H = 4;

    {
        const FVector2D P = MythicWarMap::NormalizedToWorld(FVector2D(0.0f, 1.0f), Origin, CellSize, W, H);
        TestTrue(TEXT("UMG (0,1) -> world origin X"), FMath::IsNearlyEqual(P.X, 0.0f, 1e-3f));
        TestTrue(TEXT("UMG (0,1) -> world origin Y"), FMath::IsNearlyEqual(P.Y, 0.0f, 1e-3f));
    }
    {
        const FVector2D P = MythicWarMap::NormalizedToWorld(FVector2D(1.0f, 0.0f), Origin, CellSize, W, H);
        TestTrue(TEXT("UMG (1,0) -> far corner X"), FMath::IsNearlyEqual(P.X, 400.0f, 1e-3f));
        TestTrue(TEXT("UMG (1,0) -> far corner Y"), FMath::IsNearlyEqual(P.Y, 400.0f, 1e-3f));
    }

    {
        const FVector2D Cases[] = {
            FVector2D(0.0f, 0.0f), FVector2D(400.0f, 400.0f), FVector2D(200.0f, 200.0f),
            FVector2D(37.5f, 362.5f), FVector2D(399.9f, 0.1f),
        };
        for (const FVector2D& WorldXY : Cases) {
            const FVector2D N = MythicWarMap::WorldToNormalized(WorldXY, Origin, CellSize, W, H);
            const FVector2D Back = MythicWarMap::NormalizedToWorld(N, Origin, CellSize, W, H);
            TestTrue(*FString::Printf(TEXT("Round trip X for %s"), *WorldXY.ToString()),
                     FMath::IsNearlyEqual(Back.X, WorldXY.X, 0.05f));
            TestTrue(*FString::Printf(TEXT("Round trip Y for %s"), *WorldXY.ToString()),
                     FMath::IsNearlyEqual(Back.Y, WorldXY.Y, 0.05f));
        }
    }

    {
        const FVector2D Off(1000.0f, -2000.0f);
        const FVector2D WorldXY(1150.0f, -1725.0f);
        const FVector2D N = MythicWarMap::WorldToNormalized(WorldXY, Off, CellSize, W, H);
        const FVector2D Back = MythicWarMap::NormalizedToWorld(N, Off, CellSize, W, H);
        TestTrue(TEXT("Offset round trip X"), FMath::IsNearlyEqual(Back.X, WorldXY.X, 0.05f));
        TestTrue(TEXT("Offset round trip Y"), FMath::IsNearlyEqual(Back.Y, WorldXY.Y, 0.05f));
    }

    {
        const FVector2D Off(500.0f, 600.0f);
        TestEqual(TEXT("CellSize 0 -> origin"),
                  MythicWarMap::NormalizedToWorld(FVector2D(0.5f, 0.5f), Off, 0.0f, W, H), Off);
        TestEqual(TEXT("GridW 0 -> origin"),
                  MythicWarMap::NormalizedToWorld(FVector2D(0.5f, 0.5f), Off, CellSize, 0, H), Off);
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapCellToNormTest,
    "Mythic.LivingWorld.WarMap.CellToNormalized",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapCellToNormTest::RunTest(const FString& Parameters) {
    const int32 W = 4, H = 4;

    {
        const FVector2D N = MythicWarMap::CellToNormalized(0, 0, W, H);
        TestTrue(TEXT("(0,0) X ~ 0.125"), FMath::IsNearlyEqual(N.X, 0.125f, 1e-4f));
        TestTrue(TEXT("(0,0) Y ~ 0.875 (flipped)"), FMath::IsNearlyEqual(N.Y, 0.875f, 1e-4f));
    }
    {
        const FVector2D N = MythicWarMap::CellToNormalized(3, 3, W, H);
        TestTrue(TEXT("(3,3) X ~ 0.875"), FMath::IsNearlyEqual(N.X, 0.875f, 1e-4f));
        TestTrue(TEXT("(3,3) Y ~ 0.125 (flipped)"), FMath::IsNearlyEqual(N.Y, 0.125f, 1e-4f));
    }
    {
        TestEqual(TEXT("Zero grid -> zero"), MythicWarMap::CellToNormalized(2, 2, 0, 0), FVector2D::ZeroVector);
    }
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapResolveColorTest,
    "Mythic.LivingWorld.WarMap.ResolveFactionColorForId",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapResolveColorTest::RunTest(const FString& Parameters) {
    TArray<FMythicFactionData> Factions;
    Factions.SetNum(2);
    Factions[0].bOverrideFactionColor = true;
    Factions[0].FactionColor = FColor(10, 20, 30, 255);
    Factions[1].bOverrideFactionColor = false;

    TestEqual(TEXT("idx 0 honors override"),
              MythicWarMap::ResolveFactionColorForId(0, Factions), FColor(10, 20, 30, 255));

    TestEqual(TEXT("idx 1 -> deterministic color"),
              MythicWarMap::ResolveFactionColorForId(1, Factions),
              MythicFactionColor::DeterministicColorForId(1));

    TestEqual(TEXT("idx 5 (OOB) -> deterministic color"),
              MythicWarMap::ResolveFactionColorForId(5, Factions),
              MythicFactionColor::DeterministicColorForId(5));

    TArray<FMythicFactionData> Empty;
    TestEqual(TEXT("empty list -> deterministic color"),
              MythicWarMap::ResolveFactionColorForId(3, Empty),
              MythicFactionColor::DeterministicColorForId(3));

    TestEqual(TEXT("deterministic stable"),
              MythicWarMap::ResolveFactionColorForId(7, Empty),
              MythicWarMap::ResolveFactionColorForId(7, Empty));
    return true;
}
