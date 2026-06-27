// Mythic Living World — War-Map pure-helper tests
// Covers the engine-free, deterministic core of the strategic war-map texture pipeline (MythicWarMapTypes.h):
//   - CoordToTexelIndex (Y*W+X)
//   - CellToColor (unclaimed -> transparent; claimed -> ColorForIndex; player-owned tinted)
//   - IsBorderCell (uniform interior false; differing neighbor true; OOB skipped; unclaimed never border)
//   - ApplyBorder (RGB darkened, alpha forced opaque)
//   - WorldToNormalized (corners -> 0/1, center -> 0.5, Y-flip, OOB clamps, degenerate -> 0)
//   - CellToNormalized (centers + Y-flip)
//   - ResolveFactionColorForId (idx<Num override-aware; idx>=Num deterministic; matches DeterministicColorForId)
// Pure helpers only — zero engine/render/replication state.
// Run via: Session Frontend -> Automation -> Mythic.LivingWorld.WarMap

#include "Misc/AutomationTest.h"
#include "UI/WarMap/MythicWarMapTypes.h"
#include "World/LivingWorld/Factions/FactionColor.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"

namespace {
    // A claimed cell helper.
    FMythicWarMapCell Claimed(uint8 Idx, bool bPlayer = false) {
        FMythicWarMapCell C;
        C.FactionIndex = Idx;
        C.bPlayerOwned = bPlayer;
        return C;
    }

    // Identity-ish color seam: faction idx N -> grey (N,N,N,255), so tests can reason about the value cheaply.
    FColor GreyForIndex(uint8 Idx) {
        return FColor(Idx, Idx, Idx, 255);
    }
} // namespace

// ─────────────────────────────────────────────────────────────
// CoordToTexelIndex
// ─────────────────────────────────────────────────────────────

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

// ─────────────────────────────────────────────────────────────
// CellToColor
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapCellToColorTest,
    "Mythic.LivingWorld.WarMap.CellToColor",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapCellToColorTest::RunTest(const FString& Parameters) {
    FMythicWarMapStyle Style; // defaults: UnclaimedColor transparent, PlayerOwnedColor white, PlayerTint 0.5

    auto ColorFor = [](uint8 Idx) { return GreyForIndex(Idx); };

    // Unclaimed -> the style's unclaimed color (transparent by default).
    {
        FMythicWarMapCell Unclaimed; // FactionIndex 0xFF
        const FColor Out = MythicWarMap::CellToColor(Unclaimed, ColorFor, Style);
        TestEqual(TEXT("Unclaimed alpha is 0"), (int32)Out.A, 0);
        TestEqual(TEXT("Unclaimed matches UnclaimedColor"), Out, Style.UnclaimedColor);
    }

    // Claimed, non-player -> exactly ColorForIndex.
    {
        const FColor Out = MythicWarMap::CellToColor(Claimed(100), ColorFor, Style);
        TestEqual(TEXT("Claimed routes to ColorForIndex"), Out, GreyForIndex(100));
    }

    // Player-owned -> blended toward white, so it differs from the same-faction non-player color and is brighter.
    {
        const FColor NonPlayer = MythicWarMap::CellToColor(Claimed(100, false), ColorFor, Style);
        const FColor Player = MythicWarMap::CellToColor(Claimed(100, true), ColorFor, Style);
        TestTrue(TEXT("Player-owned differs from non-player same-faction"), Player != NonPlayer);
        TestTrue(TEXT("Player-owned is brighter (toward white)"), Player.R > NonPlayer.R);
        // PlayerTint 0.5 of (100 -> 255): round(100 + 0.5*155) = 178.
        TestEqual(TEXT("Player blend R == 178"), (int32)Player.R, 178);
    }

    // PlayerTint 0 -> player cell identical to faction color; PlayerTint 1 -> exactly PlayerOwnedColor.
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

// ─────────────────────────────────────────────────────────────
// IsBorderCell
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapBorderTest,
    "Mythic.LivingWorld.WarMap.IsBorderCell",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapBorderTest::RunTest(const FString& Parameters) {
    const int32 W = 3, H = 3;

    // A 3x3 grid, all faction 5 EXCEPT center (1,1). Border = the eight edge cells touching... actually all-uniform.
    // Case 1: fully uniform faction 5 interior cell -> NOT a border.
    {
        auto Uniform = [](int32, int32) -> uint8 { return 5; };
        TestFalse(TEXT("Uniform interior cell is not a border"),
                  MythicWarMap::IsBorderCell(1, 1, W, H, Uniform));
        // A corner of a uniform grid is also not a border (OOB neighbors skipped).
        TestFalse(TEXT("Uniform corner cell is not a border (OOB skipped)"),
                  MythicWarMap::IsBorderCell(0, 0, W, H, Uniform));
    }

    // Case 2: center is faction 7, everything else faction 5 -> center IS a border, and 5-cells adjacent to it are too.
    {
        auto Mixed = [](int32 X, int32 Y) -> uint8 { return (X == 1 && Y == 1) ? (uint8)7 : (uint8)5; };
        TestTrue(TEXT("Center (different from neighbors) is a border"),
                 MythicWarMap::IsBorderCell(1, 1, W, H, Mixed));
        TestTrue(TEXT("Edge-mid cell adjacent to differing center is a border"),
                 MythicWarMap::IsBorderCell(1, 0, W, H, Mixed));
        // A corner like (0,0) only touches (1,0)=5 and (0,1)=5 — same faction -> not a border.
        TestFalse(TEXT("Corner not adjacent to the differing cell is not a border"),
                  MythicWarMap::IsBorderCell(0, 0, W, H, Mixed));
    }

    // Case 3: unclaimed self -> never a border, even next to a claimed neighbor.
    {
        auto OneClaimed = [](int32 X, int32 Y) -> uint8 { return (X == 0 && Y == 0) ? (uint8)5 : (uint8)0xFF; };
        TestFalse(TEXT("Unclaimed cell is never a border"),
                  MythicWarMap::IsBorderCell(1, 0, W, H, OneClaimed));
        // The lone claimed cell next to unclaimed neighbors IS a border (unclaimed counts as different).
        TestTrue(TEXT("Claimed cell beside unclaimed neighbors is a border"),
                 MythicWarMap::IsBorderCell(0, 0, W, H, OneClaimed));
    }
    return true;
}

// ─────────────────────────────────────────────────────────────
// ApplyBorder
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapApplyBorderTest,
    "Mythic.LivingWorld.WarMap.ApplyBorder",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapApplyBorderTest::RunTest(const FString& Parameters) {
    FMythicWarMapStyle Style; // BorderDarken 0.45
    const FColor Base(200, 100, 50, 128);
    const FColor Out = MythicWarMap::ApplyBorder(Base, Style);

    TestEqual(TEXT("R darkened: round(200*0.45)=90"), (int32)Out.R, 90);
    TestEqual(TEXT("G darkened: round(100*0.45)=45"), (int32)Out.G, 45);
    TestEqual(TEXT("B darkened: round(50*0.45)=23"), (int32)Out.B, 23); // 22.5 -> 23 (round-half-to-even/away both 23 here)
    TestEqual(TEXT("Alpha forced opaque"), (int32)Out.A, 255);

    // BorderDarken 0 -> black RGB, opaque.
    FMythicWarMapStyle S0 = Style; S0.BorderDarken = 0.0f;
    const FColor Black = MythicWarMap::ApplyBorder(Base, S0);
    TestEqual(TEXT("BorderDarken 0 -> R=0"), (int32)Black.R, 0);
    TestEqual(TEXT("BorderDarken 0 -> alpha 255"), (int32)Black.A, 255);
    return true;
}

// ─────────────────────────────────────────────────────────────
// WorldToNormalized
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapWorldToNormTest,
    "Mythic.LivingWorld.WarMap.WorldToNormalized",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapWorldToNormTest::RunTest(const FString& Parameters) {
    const FVector2D Origin(0, 0);
    const float CellSize = 100.0f;
    const int32 W = 4, H = 4; // world span 0..400 in each axis

    // Bottom-left world corner (grid origin) -> UMG (0, 1) after Y flip.
    {
        const FVector2D N = MythicWarMap::WorldToNormalized(FVector2D(0, 0), Origin, CellSize, W, H);
        TestTrue(TEXT("Origin X ~ 0"), FMath::IsNearlyEqual(N.X, 0.0f, 1e-4f));
        TestTrue(TEXT("Origin Y ~ 1 (flipped)"), FMath::IsNearlyEqual(N.Y, 1.0f, 1e-4f));
    }
    // Far corner (400,400) -> UMG (1, 0).
    {
        const FVector2D N = MythicWarMap::WorldToNormalized(FVector2D(400, 400), Origin, CellSize, W, H);
        TestTrue(TEXT("Far X ~ 1"), FMath::IsNearlyEqual(N.X, 1.0f, 1e-4f));
        TestTrue(TEXT("Far Y ~ 0 (flipped)"), FMath::IsNearlyEqual(N.Y, 0.0f, 1e-4f));
    }
    // Center (200,200) -> (0.5, 0.5).
    {
        const FVector2D N = MythicWarMap::WorldToNormalized(FVector2D(200, 200), Origin, CellSize, W, H);
        TestTrue(TEXT("Center X ~ 0.5"), FMath::IsNearlyEqual(N.X, 0.5f, 1e-4f));
        TestTrue(TEXT("Center Y ~ 0.5"), FMath::IsNearlyEqual(N.Y, 0.5f, 1e-4f));
    }
    // Out-of-bounds clamps to [0,1].
    {
        const FVector2D Lo = MythicWarMap::WorldToNormalized(FVector2D(-1000, -1000), Origin, CellSize, W, H);
        TestTrue(TEXT("OOB low X clamps to 0"), FMath::IsNearlyEqual(Lo.X, 0.0f, 1e-4f));
        TestTrue(TEXT("OOB low Y clamps to 1 (flipped)"), FMath::IsNearlyEqual(Lo.Y, 1.0f, 1e-4f));
        const FVector2D Hi = MythicWarMap::WorldToNormalized(FVector2D(9999, 9999), Origin, CellSize, W, H);
        TestTrue(TEXT("OOB high X clamps to 1"), FMath::IsNearlyEqual(Hi.X, 1.0f, 1e-4f));
        TestTrue(TEXT("OOB high Y clamps to 0 (flipped)"), FMath::IsNearlyEqual(Hi.Y, 0.0f, 1e-4f));
    }
    // Degenerate inputs -> (0,0).
    {
        const FVector2D D0 = MythicWarMap::WorldToNormalized(FVector2D(200, 200), Origin, 0.0f, W, H);
        TestEqual(TEXT("CellSize 0 -> zero"), D0, FVector2D::ZeroVector);
        const FVector2D D1 = MythicWarMap::WorldToNormalized(FVector2D(200, 200), Origin, CellSize, 0, H);
        TestEqual(TEXT("GridW 0 -> zero"), D1, FVector2D::ZeroVector);
    }

    // Non-zero origin offset is respected.
    {
        const FVector2D Off(1000, 2000);
        const FVector2D N = MythicWarMap::WorldToNormalized(Off, Off, CellSize, W, H);
        TestTrue(TEXT("At origin offset -> X 0"), FMath::IsNearlyEqual(N.X, 0.0f, 1e-4f));
        TestTrue(TEXT("At origin offset -> Y 1"), FMath::IsNearlyEqual(N.Y, 1.0f, 1e-4f));
    }
    return true;
}

// ─────────────────────────────────────────────────────────────
// CellToNormalized
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapCellToNormTest,
    "Mythic.LivingWorld.WarMap.CellToNormalized",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapCellToNormTest::RunTest(const FString& Parameters) {
    const int32 W = 4, H = 4;

    // Cell (0,0) center -> ((0.5)/4, 1-(0.5)/4) = (0.125, 0.875).
    {
        const FVector2D N = MythicWarMap::CellToNormalized(0, 0, W, H);
        TestTrue(TEXT("(0,0) X ~ 0.125"), FMath::IsNearlyEqual(N.X, 0.125f, 1e-4f));
        TestTrue(TEXT("(0,0) Y ~ 0.875 (flipped)"), FMath::IsNearlyEqual(N.Y, 0.875f, 1e-4f));
    }
    // Cell (3,3) center -> (3.5/4, 1-3.5/4) = (0.875, 0.125).
    {
        const FVector2D N = MythicWarMap::CellToNormalized(3, 3, W, H);
        TestTrue(TEXT("(3,3) X ~ 0.875"), FMath::IsNearlyEqual(N.X, 0.875f, 1e-4f));
        TestTrue(TEXT("(3,3) Y ~ 0.125 (flipped)"), FMath::IsNearlyEqual(N.Y, 0.125f, 1e-4f));
    }
    // Degenerate grid -> (0,0).
    {
        TestEqual(TEXT("Zero grid -> zero"), MythicWarMap::CellToNormalized(2, 2, 0, 0), FVector2D::ZeroVector);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────
// ResolveFactionColorForId
// ─────────────────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWarMapResolveColorTest,
    "Mythic.LivingWorld.WarMap.ResolveFactionColorForId",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWarMapResolveColorTest::RunTest(const FString& Parameters) {
    // Build a small InitialFactions list: index 0 with a pinned override color, index 1 with no override.
    TArray<FMythicFactionData> Factions;
    Factions.SetNum(2);
    Factions[0].bOverrideFactionColor = true;
    Factions[0].FactionColor = FColor(10, 20, 30, 255);
    Factions[1].bOverrideFactionColor = false;

    // idx 0: override honored.
    TestEqual(TEXT("idx 0 honors override"),
              MythicWarMap::ResolveFactionColorForId(0, Factions), FColor(10, 20, 30, 255));

    // idx 1: no override -> deterministic-from-id color.
    TestEqual(TEXT("idx 1 -> deterministic color"),
              MythicWarMap::ResolveFactionColorForId(1, Factions),
              MythicFactionColor::DeterministicColorForId(1));

    // idx >= Num -> deterministic-from-id color.
    TestEqual(TEXT("idx 5 (OOB) -> deterministic color"),
              MythicWarMap::ResolveFactionColorForId(5, Factions),
              MythicFactionColor::DeterministicColorForId(5));

    // Empty list -> deterministic for any index.
    TArray<FMythicFactionData> Empty;
    TestEqual(TEXT("empty list -> deterministic color"),
              MythicWarMap::ResolveFactionColorForId(3, Empty),
              MythicFactionColor::DeterministicColorForId(3));

    // Determinism: same index, same color, twice.
    TestEqual(TEXT("deterministic stable"),
              MythicWarMap::ResolveFactionColorForId(7, Empty),
              MythicWarMap::ResolveFactionColorForId(7, Empty));
    return true;
}
