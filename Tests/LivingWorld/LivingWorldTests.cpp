// Mythic Living World — Unit Tests
// Covers: CausalFabric, TerritoryGrid, FactionDatabase, MoralSignature, SettlementRegistry, WorldSimThread,
//         SocialGraph, SchemeEngine
// Run via: Session Frontend → Automation → Mythic.LivingWorld

#include "Misc/AutomationTest.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/LivingWorld/Settlements/SettlementRegistry.h"
#include "World/LivingWorld/Simulation/WorldSimThread.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/Simulation/SchemeEngine.h"
#include "World/LivingWorld/Simulation/SchemeTypes.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"

// ═══════════════════════════════════════════════════════════════
// Helper: Create in-memory FactionDatabaseSettings
// ═══════════════════════════════════════════════════════════════

namespace LivingWorldTestHelpers {
    UMythicFactionDatabaseSettings *CreateFactionSettings(int32 MaxFactions = 20, int32 InitialCount = 3) {
        auto *Settings = NewObject<UMythicFactionDatabaseSettings>();
        Settings->MaxFactions = MaxFactions;
        Settings->InitialFactions.SetNum(InitialCount);

        for (int32 i = 0; i < InitialCount; ++i) {
            FMythicFactionData &F = Settings->InitialFactions[i];
            F.DisplayName = FText::FromString(FString::Printf(TEXT("TestFaction_%d"), i));
            F.bAlive = true;
            F.Population = 100 * (i + 1);
            F.bHasBeenPopulated = true;
            F.bControlsTerritory = true;
            F.bHasEconomy = true;
            F.bHasCivilianPopulation = true;
            F.bParticipatesInTrade = true;
            F.bCanNegotiate = true;
            F.BaseProduction.Food = 1.0f;
            F.BaseProduction.Materials = 0.5f;
        }

        // Give faction 0 territory
        Settings->InitialFactions[0].ControlledCellCount = 10;

        return Settings;
    }

    UMythicTerritoryGridSettings *CreateGridSettings(int32 Width = 16, int32 Height = 16) {
        auto *Settings = NewObject<UMythicTerritoryGridSettings>();
        Settings->GridWidth = Width;
        Settings->GridHeight = Height;
        Settings->CellWorldSize = 5000.0f;
        Settings->WorldOrigin = FVector2D::ZeroVector;
        Settings->InfluenceBleedRate = 0.1f;
        Settings->MinControlThreshold = 0.1f;
        return Settings;
    }

    UMythicLivingWorldSettings *CreateLivingWorldSettings() {
        auto *Settings = NewObject<UMythicLivingWorldSettings>();
        Settings->FabricCapacity = 64;
        Settings->SimTickIntervalSeconds = 1.0f;
        return Settings;
    }

    FMythicFactionId MakeFactionId(uint8 Index) {
        FMythicFactionId Id;
        Id.Index = Index;
        return Id;
    }
}

// ═══════════════════════════════════════════════════════════════
//  CAUSAL FABRIC TESTS
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldCausalFabricInitTest,
    "Mythic.LivingWorld.Phase1.CausalFabric.Initialize",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldCausalFabricInitTest::RunTest(const FString &Parameters) {
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(128);

    TestEqual(TEXT("Capacity should match"), Fabric->GetCapacity(), 128);
    TestEqual(TEXT("Event count starts at zero"), (int32)Fabric->GetTotalEventCount(), 1); // NextEventId starts at 1

    auto RecentEvents = Fabric->GetRecentEvents(10);
    TestEqual(TEXT("No events in fresh fabric"), RecentEvents.Num(), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldCausalFabricAppendReadTest,
    "Mythic.LivingWorld.Phase1.CausalFabric.AppendAndRead",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldCausalFabricAppendReadTest::RunTest(const FString &Parameters) {
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(64);

    FMythicWorldEvent Event;
    Event.WorldTime = 100.0;
    Event.Cell = FMythicCellCoord(5, 5);
    Event.PrimaryFaction = LivingWorldTestHelpers::MakeFactionId(0);
    Event.Significance = 0.8f;
    Event.CategoryFlags = EMythicEventCategory::Combat;

    const uint32 EventId = Fabric->AppendEvent(Event);
    TestTrue(TEXT("EventId should be valid"), EventId > 0);

    Fabric->CommitWrites();

    const FMythicWorldEvent *Retrieved = Fabric->GetEvent(EventId);
    TestNotNull(TEXT("Should retrieve committed event"), Retrieved);

    if (Retrieved) {
        TestEqual(TEXT("Cell X matches"), Retrieved->Cell.X, 5);
        TestEqual(TEXT("Cell Y matches"), Retrieved->Cell.Y, 5);
        TestEqual(TEXT("Faction matches"), Retrieved->PrimaryFaction.Index, (uint8)0);
        TestEqual(TEXT("Category matches"), Retrieved->CategoryFlags, EMythicEventCategory::Combat);
        TestNearlyEqual(TEXT("Significance matches"), Retrieved->Significance, 0.8f);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldCausalFabricRingWrapTest,
    "Mythic.LivingWorld.Phase1.CausalFabric.RingBufferWrap",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldCausalFabricRingWrapTest::RunTest(const FString &Parameters) {
    constexpr int32 Capacity = 8;
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(Capacity);

    // Fill beyond capacity — first events should be overwritten
    TArray<uint32> EventIds;
    for (int32 i = 0; i < Capacity + 4; ++i) {
        FMythicWorldEvent Event;
        Event.WorldTime = static_cast<double>(i + 1); // 1-based: WorldTime 0 now means "unset" (AppendEvent stamps it)
        Event.Cell = FMythicCellCoord(i, 0);
        EventIds.Add(Fabric->AppendEvent(Event));
    }
    Fabric->CommitWrites();

    // Oldest events (0-3) should be overwritten
    for (int32 i = 0; i < 4; ++i) {
        const FMythicWorldEvent *Old = Fabric->GetEvent(EventIds[i]);
        TestNull(TEXT("Overwritten event should be null"), Old);
    }

    // Newest events (4-11) should still be accessible
    for (int32 i = 4; i < Capacity + 4; ++i) {
        const FMythicWorldEvent *Current = Fabric->GetEvent(EventIds[i]);
        TestNotNull(TEXT("Recent event should exist"), Current);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldCausalFabricQueryByCellTest,
    "Mythic.LivingWorld.Phase1.CausalFabric.QueryByCell",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldCausalFabricQueryByCellTest::RunTest(const FString &Parameters) {
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(64);

    // Append events in different cells
    for (int32 i = 0; i < 5; ++i) {
        FMythicWorldEvent Event;
        Event.WorldTime = static_cast<double>(i + 1); // 1-based: WorldTime 0 now means "unset" (AppendEvent stamps it)
        Event.Cell = FMythicCellCoord(0, 0);
        Event.CategoryFlags = EMythicEventCategory::Combat;
        Fabric->AppendEvent(Event);
    }
    for (int32 i = 0; i < 3; ++i) {
        FMythicWorldEvent Event;
        Event.WorldTime = static_cast<double>(i + 5);
        Event.Cell = FMythicCellCoord(1, 1);
        Event.CategoryFlags = EMythicEventCategory::Crime;
        Fabric->AppendEvent(Event);
    }
    Fabric->CommitWrites();

    TArray<FMythicWorldEvent> Results;
    Fabric->QueryEventsByCell(FMythicCellCoord(0, 0), 0.0, 100.0, 64, Results);
    TestEqual(TEXT("Cell(0,0) should have 5 events"), Results.Num(), 5);

    Results.Empty();
    Fabric->QueryEventsByCell(FMythicCellCoord(1, 1), 0.0, 100.0, 64, Results);
    TestEqual(TEXT("Cell(1,1) should have 3 events"), Results.Num(), 3);

    Results.Empty();
    Fabric->QueryEventsByCell(FMythicCellCoord(5, 5), 0.0, 100.0, 64, Results);
    TestEqual(TEXT("Empty cell should have 0 events"), Results.Num(), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldCausalFabricQueryByCategoryTest,
    "Mythic.LivingWorld.Phase1.CausalFabric.QueryByCategory",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldCausalFabricQueryByCategoryTest::RunTest(const FString &Parameters) {
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(64);

    // Combat events
    for (int32 i = 0; i < 4; ++i) {
        FMythicWorldEvent Event;
        Event.WorldTime = static_cast<double>(i + 1); // 1-based: WorldTime 0 now means "unset" (AppendEvent stamps it)
        Event.Cell = FMythicCellCoord(0, 0);
        Event.CategoryFlags = EMythicEventCategory::Combat;
        Fabric->AppendEvent(Event);
    }
    // Crime events
    for (int32 i = 0; i < 2; ++i) {
        FMythicWorldEvent Event;
        Event.WorldTime = static_cast<double>(i + 4);
        Event.Cell = FMythicCellCoord(0, 0);
        Event.CategoryFlags = EMythicEventCategory::Crime;
        Fabric->AppendEvent(Event);
    }
    // Combined category event
    {
        FMythicWorldEvent Event;
        Event.WorldTime = 6.0;
        Event.Cell = FMythicCellCoord(0, 0);
        Event.CategoryFlags = EMythicEventCategory::Combat | EMythicEventCategory::Death;
        Fabric->AppendEvent(Event);
    }
    Fabric->CommitWrites();

    TArray<FMythicWorldEvent> Results;
    Fabric->QueryEventsByCategory(EMythicEventCategory::Combat, 0.0, 100.0, 64, Results);
    TestEqual(TEXT("Combat category should include 4 pure + 1 combined"), Results.Num(), 5);

    Results.Empty();
    Fabric->QueryEventsByCategory(EMythicEventCategory::Crime, 0.0, 100.0, 64, Results);
    TestEqual(TEXT("Crime category should have 2"), Results.Num(), 2);

    Results.Empty();
    Fabric->QueryEventsByCategory(EMythicEventCategory::Death, 0.0, 100.0, 64, Results);
    TestEqual(TEXT("Death category should have 1 combined"), Results.Num(), 1);

    return true;
}

// ═══════════════════════════════════════════════════════════════
//  TERRITORY GRID TESTS
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldTerritoryGridInitTest,
    "Mythic.LivingWorld.Phase1.TerritoryGrid.Initialize",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldTerritoryGridInitTest::RunTest(const FString &Parameters) {
    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings(32, 32));

    TestEqual(TEXT("Width should match"), Grid->GetWidth(), 32);
    TestEqual(TEXT("Height should match"), Grid->GetHeight(), 32);

    // Default cell should be uncontrolled
    FMythicTerritoryCell Cell = Grid->GetCell(FMythicCellCoord(0, 0));
    TestFalse(TEXT("Default cell should have no valid faction"), Cell.DominantFaction.IsValid());
    TestNearlyEqual(TEXT("Default influence should be 0"), Cell.Influence, 0.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldTerritoryGridSetReadTest,
    "Mythic.LivingWorld.Phase1.TerritoryGrid.SetAndRead",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldTerritoryGridSetReadTest::RunTest(const FString &Parameters) {
    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    FMythicFactionId Faction0 = LivingWorldTestHelpers::MakeFactionId(0);
    Grid->SetCellInfluence(FMythicCellCoord(3, 3), Faction0, 0.9f);
    Grid->CommitWrites();

    FMythicTerritoryCell Cell = Grid->GetCell(FMythicCellCoord(3, 3));
    TestEqual(TEXT("Dominant faction should be 0"), Cell.DominantFaction.Index, (uint8)0);
    TestNearlyEqual(TEXT("Influence should be 0.9"), Cell.Influence, 0.9f);

    // Unset cell should be unchanged
    FMythicTerritoryCell EmptyCell = Grid->GetCell(FMythicCellCoord(10, 10));
    TestFalse(TEXT("Unset cell should have no valid faction"), EmptyCell.DominantFaction.IsValid());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldTerritoryGridPropagateTest,
    "Mythic.LivingWorld.Phase1.TerritoryGrid.PropagateInfluence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldTerritoryGridPropagateTest::RunTest(const FString &Parameters) {
    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    FMythicFactionId Faction0 = LivingWorldTestHelpers::MakeFactionId(0);
    FMythicFactionId Faction1 = LivingWorldTestHelpers::MakeFactionId(1);

    // ─── Test 1: Spread to empty neighbors ───
    // A strong cell should bleed influence into adjacent empty cells
    Grid->SetCellInfluence(FMythicCellCoord(8, 8), Faction0, 1.0f);
    Grid->PropagateInfluence();
    Grid->CommitWrites();

    FMythicTerritoryCell NeighborUp = Grid->GetCell(FMythicCellCoord(8, 9));
    FMythicTerritoryCell NeighborDown = Grid->GetCell(FMythicCellCoord(8, 7));
    FMythicTerritoryCell NeighborLeft = Grid->GetCell(FMythicCellCoord(7, 8));
    FMythicTerritoryCell NeighborRight = Grid->GetCell(FMythicCellCoord(9, 8));

    TestTrue(TEXT("Empty neighbor up should gain influence"), NeighborUp.Influence > 0.0f);
    TestTrue(TEXT("Empty neighbor down should gain influence"), NeighborDown.Influence > 0.0f);
    TestTrue(TEXT("Empty neighbor left should gain influence"), NeighborLeft.Influence > 0.0f);
    TestTrue(TEXT("Empty neighbor right should gain influence"), NeighborRight.Influence > 0.0f);

    // Claimed neighbors should belong to Faction 0
    if (NeighborUp.Influence > 0.0f) {
        TestEqual(TEXT("Claimed neighbor should be Faction 0"), NeighborUp.DominantFaction.Index, (uint8)0);
    }

    // Diagonal neighbor should NOT gain influence (4-neighbor, not 8-neighbor)
    FMythicTerritoryCell Diagonal = Grid->GetCell(FMythicCellCoord(9, 9));
    TestNearlyEqual(TEXT("Diagonal should have zero influence"), Diagonal.Influence, 0.0f);

    // ─── Test 2: Erosion from opposing faction ───
    // Place Faction 1 adjacent to Faction 0 — should erode its influence
    auto *Grid2 = NewObject<UMythicTerritoryGrid>();
    Grid2->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    Grid2->SetCellInfluence(FMythicCellCoord(5, 5), Faction0, 0.3f);
    Grid2->SetCellInfluence(FMythicCellCoord(6, 5), Faction1, 1.0f);
    Grid2->SetCellInfluence(FMythicCellCoord(4, 5), Faction1, 1.0f);
    Grid2->SetCellInfluence(FMythicCellCoord(5, 6), Faction1, 1.0f);
    Grid2->SetCellInfluence(FMythicCellCoord(5, 4), Faction1, 1.0f);

    Grid2->PropagateInfluence();
    Grid2->CommitWrites();

    FMythicTerritoryCell ErodedCell = Grid2->GetCell(FMythicCellCoord(5, 5));

    // Faction 0 at 0.3 surrounded by 4 Faction 1 cells at 1.0 — should be heavily eroded
    // Erosion = 4 × (1.0 × bleedRate) / 4 = bleedRate. With bleedRate=0.1, erosion=0.1
    // New influence = 0.3 - 0.1 = 0.2 (still held but weaker)
    // OR if eroded below threshold, flipped to Faction 1
    TestTrue(TEXT("Surrounded cell should be eroded or flipped"),
             ErodedCell.Influence < 0.3f || ErodedCell.DominantFaction == Faction1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldTerritoryGridWorldToCellTest,
    "Mythic.LivingWorld.Phase1.TerritoryGrid.WorldToCell",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldTerritoryGridWorldToCellTest::RunTest(const FString &Parameters) {
    auto *Grid = NewObject<UMythicTerritoryGrid>();
    auto *Settings = LivingWorldTestHelpers::CreateGridSettings(16, 16);
    Settings->CellWorldSize = 5000.0f;
    Settings->WorldOrigin = FVector2D(0.0, 0.0);
    Grid->Initialize(Settings);

    // World position (0, 0, 0) should map to cell (0, 0)
    FMythicCellCoord Cell0 = Grid->WorldToCell(FVector(0, 0, 0));
    TestEqual(TEXT("Origin maps to cell 0,0 X"), Cell0.X, 0);
    TestEqual(TEXT("Origin maps to cell 0,0 Y"), Cell0.Y, 0);

    // World position (5000, 0, 0) should map to cell (1, 0) (CellSize = 5000)
    FMythicCellCoord Cell1 = Grid->WorldToCell(FVector(5000, 0, 0));
    TestEqual(TEXT("5000 units = cell 1,0 X"), Cell1.X, 1);
    TestEqual(TEXT("5000 units = cell 1,0 Y"), Cell1.Y, 0);

    // Verify IsValidCoord
    TestTrue(TEXT("(0,0) should be valid"), Grid->IsValidCoord(FMythicCellCoord(0, 0)));
    TestTrue(TEXT("(15,15) should be valid"), Grid->IsValidCoord(FMythicCellCoord(15, 15)));
    TestFalse(TEXT("(16,0) should be invalid"), Grid->IsValidCoord(FMythicCellCoord(16, 0)));
    TestFalse(TEXT("(-1,0) should be invalid"), Grid->IsValidCoord(FMythicCellCoord(-1, 0)));

    return true;
}

// ═══════════════════════════════════════════════════════════════
//  FACTION DATABASE TESTS
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldFactionDBInitTest,
    "Mythic.LivingWorld.Phase1.FactionDatabase.InitAndRead",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldFactionDBInitTest::RunTest(const FString &Parameters) {
    auto *DB = NewObject<UMythicFactionDatabase>();
    DB->Initialize(LivingWorldTestHelpers::CreateFactionSettings(20, 3));

    TestEqual(TEXT("Max factions = 20"), DB->GetMaxFactions(), 20);
    TestEqual(TEXT("Registered count = 3"), DB->GetRegisteredCount(), 3);
    TestEqual(TEXT("Active factions = 3"), DB->GetActiveFactionCount(), 3);

    FMythicFactionData Data;
    bool bFound = DB->GetFaction(LivingWorldTestHelpers::MakeFactionId(0), Data);
    TestTrue(TEXT("Faction 0 should exist"), bFound);
    TestTrue(TEXT("Faction 0 should be alive"), Data.bAlive);
    TestEqual(TEXT("Faction 0 pop = 100"), Data.Population, 100);
    TestEqual(TEXT("Faction 0 cells = 10"), Data.ControlledCellCount, 10);

    // Invalid ID should fail
    bFound = DB->GetFaction(LivingWorldTestHelpers::MakeFactionId(99), Data);
    TestFalse(TEXT("Invalid faction should not be found"), bFound);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldFactionDBDoubleBufferTest,
    "Mythic.LivingWorld.Phase1.FactionDatabase.DoubleBuffer",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldFactionDBDoubleBufferTest::RunTest(const FString &Parameters) {
    auto *DB = NewObject<UMythicFactionDatabase>();
    DB->Initialize(LivingWorldTestHelpers::CreateFactionSettings(20, 2));

    // Modify write buffer
    FMythicFactionData *WriteFaction = DB->GetFactionMutable(LivingWorldTestHelpers::MakeFactionId(0));
    TestNotNull(TEXT("Mutable faction should exist"), WriteFaction);
    WriteFaction->Population = 999;

    // Read buffer should still have old value (not yet committed)
    FMythicFactionData ReadData;
    DB->GetFaction(LivingWorldTestHelpers::MakeFactionId(0), ReadData);
    TestEqual(TEXT("Pre-commit read should have original pop"), ReadData.Population, 100);

    // After commit, read buffer updates
    DB->CommitWrites();
    DB->GetFaction(LivingWorldTestHelpers::MakeFactionId(0), ReadData);
    TestEqual(TEXT("Post-commit read should have new pop"), ReadData.Population, 999);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldFactionDBRegisterAnnihilateTest,
    "Mythic.LivingWorld.Phase1.FactionDatabase.RegisterAndAnnihilate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldFactionDBRegisterAnnihilateTest::RunTest(const FString &Parameters) {
    auto *DB = NewObject<UMythicFactionDatabase>();
    DB->Initialize(LivingWorldTestHelpers::CreateFactionSettings(20, 2));

    TestEqual(TEXT("Initial registered = 2"), DB->GetRegisteredCount(), 2);

    // Register a new faction
    FMythicFactionData NewFaction;
    NewFaction.DisplayName = FText::FromString(TEXT("NewTestFaction"));
    NewFaction.bAlive = true;
    NewFaction.Population = 50;
    FMythicFactionId NewId = DB->RegisterFaction(NewFaction);

    TestTrue(TEXT("New faction ID should be valid"), NewId.IsValid());
    TestEqual(TEXT("New faction index should be 2"), (int32)NewId.Index, 2);
    TestEqual(TEXT("Registered count = 3"), DB->GetRegisteredCount(), 3);

    DB->CommitWrites();
    TestEqual(TEXT("Active factions = 3 after commit"), DB->GetActiveFactionCount(), 3);

    // Annihilate the new faction
    DB->AnnihilateFaction(NewId);
    DB->CommitWrites();

    TestEqual(TEXT("Registered count stays 3"), DB->GetRegisteredCount(), 3);
    TestEqual(TEXT("Active factions = 2 after annihilation"), DB->GetActiveFactionCount(), 2);

    FMythicFactionData Annihilated;
    DB->GetFaction(NewId, Annihilated);
    TestFalse(TEXT("Annihilated faction should not be alive"), Annihilated.bAlive);
    TestEqual(TEXT("Annihilated faction pop = 0"), Annihilated.Population, 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldFactionDBRelationshipsTest,
    "Mythic.LivingWorld.Phase1.FactionDatabase.Relationships",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldFactionDBRelationshipsTest::RunTest(const FString &Parameters) {
    auto *DB = NewObject<UMythicFactionDatabase>();
    DB->Initialize(LivingWorldTestHelpers::CreateFactionSettings(20, 3));

    FMythicFactionId A = LivingWorldTestHelpers::MakeFactionId(0);
    FMythicFactionId B = LivingWorldTestHelpers::MakeFactionId(1);
    FMythicFactionId C = LivingWorldTestHelpers::MakeFactionId(2);

    // Default relationship should be Neutral
    TestEqual(TEXT("Default A-B = Neutral"), DB->GetRelationship(A, B), EMythicFactionRelation::Neutral);

    // Self-relationship should always be Allied
    TestEqual(TEXT("Self A-A = Allied"), DB->GetRelationship(A, A), EMythicFactionRelation::Allied);

    // Set relationship and verify symmetry
    DB->SetRelationship(A, B, EMythicFactionRelation::Hostile);
    DB->CommitWrites();

    TestEqual(TEXT("A→B = Hostile"), DB->GetRelationship(A, B), EMythicFactionRelation::Hostile);
    TestEqual(TEXT("B→A = Hostile (symmetric)"), DB->GetRelationship(B, A), EMythicFactionRelation::Hostile);

    // C should be unaffected
    TestEqual(TEXT("A-C unchanged = Neutral"), DB->GetRelationship(A, C), EMythicFactionRelation::Neutral);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldFactionDBSerializeBehaviorFlagsTest,
    "Mythic.LivingWorld.Phase1.FactionDatabase.SerializeBehaviorFlags",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldFactionDBSerializeBehaviorFlagsTest::RunTest(const FString &Parameters) {
    // Guards the iter-97 v2 serialization: the 5 runtime-mutated faction behavior flags must round-trip through
    // save/load, so an evolved/devolved faction does NOT snap back to its designer defaults on reload.
    auto *DB = NewObject<UMythicFactionDatabase>();
    DB->Initialize(LivingWorldTestHelpers::CreateFactionSettings(20, 3));

    // Flip all 5 behavior flags OFF their default-true on faction 1, then commit to the read snapshot.
    FMythicFactionData *WF = DB->GetFactionMutable(LivingWorldTestHelpers::MakeFactionId(1));
    TestNotNull(TEXT("Mutable faction 1 should exist"), WF);
    if (!WF) {
        return false;
    }
    WF->bControlsTerritory = false;
    WF->bHasEconomy = false;
    WF->bHasCivilianPopulation = false;
    WF->bParticipatesInTrade = false;
    WF->bCanNegotiate = false;
    DB->CommitWrites();

    // Serialize → load into a fresh database (mirrors SaveLivingWorld/LoadLivingWorld via an in-memory archive).
    TArray<uint8> Blob;
    FMemoryWriter Writer(Blob);
    DB->Serialize(Writer);

    auto *DB2 = NewObject<UMythicFactionDatabase>();
    FMemoryReader Reader(Blob);
    DB2->Serialize(Reader);

    // The flipped flags must survive the round-trip (NOT reset to default true).
    FMythicFactionData Loaded;
    const bool bFound = DB2->GetFaction(LivingWorldTestHelpers::MakeFactionId(1), Loaded);
    TestTrue(TEXT("Faction 1 should exist after load"), bFound);
    TestFalse(TEXT("bControlsTerritory restored to false"), Loaded.bControlsTerritory);
    TestFalse(TEXT("bHasEconomy restored to false"), Loaded.bHasEconomy);
    TestFalse(TEXT("bHasCivilianPopulation restored to false"), Loaded.bHasCivilianPopulation);
    TestFalse(TEXT("bParticipatesInTrade restored to false"), Loaded.bParticipatesInTrade);
    TestFalse(TEXT("bCanNegotiate restored to false"), Loaded.bCanNegotiate);

    // Sanity: an untouched faction keeps its default-true flags across the round-trip.
    FMythicFactionData Loaded0;
    DB2->GetFaction(LivingWorldTestHelpers::MakeFactionId(0), Loaded0);
    TestTrue(TEXT("Faction 0 keeps default-true bControlsTerritory"), Loaded0.bControlsTerritory);

    return true;
}

// ═══════════════════════════════════════════════════════════════
//  MORAL SIGNATURE TESTS
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldMoralWelfordTest,
    "Mythic.LivingWorld.Phase1.MoralSignature.Welford",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldMoralWelfordTest::RunTest(const FString &Parameters) {
    FMythicMoralSignature Sig;

    // Accumulate 3 violent actions: values 0.8, 0.6, 1.0
    constexpr float Values[] = {0.8f, 0.6f, 1.0f};
    for (float V : Values) {
        FMythicMoralAction Action;
        FMemory::Memzero(Action.AxisValues, sizeof(Action.AxisValues));
        Action.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] = V;
        Sig.AccumulateAction(Action);
    }

    TestEqual(TEXT("TotalActions = 3"), Sig.TotalActions, 3);

    // Mean of Violence axis should be (0.8+0.6+1.0)/3 = 0.8
    const float ExpectedMean = (0.8f + 0.6f + 1.0f) / 3.0f;
    TestNearlyEqual(TEXT("Violence mean should be ~0.8"), Sig.Axes[static_cast<int32>(EMythicMoralAxis::Violence)].Mean, ExpectedMean, 0.001f);

    // Variance: population variance of {0.8, 0.6, 1.0}
    // Deviations: (0.8-0.8)=0, (0.6-0.8)=-0.2, (1.0-0.8)=0.2  => sum_sq = 0 + 0.04 + 0.04 = 0.08
    // Pop variance = 0.08 / 3 ≈ 0.02667
    const float ExpectedVariance = 0.08f / 3.0f;
    TestNearlyEqual(TEXT("Violence variance"),
                    Sig.Axes[static_cast<int32>(EMythicMoralAxis::Violence)].GetVariance(),
                    ExpectedVariance, 0.01f);

    // Dominant axis should be Violence (index 0) since other axes are 0
    TestEqual(TEXT("Dominant axis should be Violence"), (int32)Sig.DominantAxis, static_cast<int32>(EMythicMoralAxis::Violence));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldMoralEvaluateAgainstTest,
    "Mythic.LivingWorld.Phase1.MoralSignature.EvaluateAgainst",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldMoralEvaluateAgainstTest::RunTest(const FString &Parameters) {
    FMythicMoralSignature Sig;

    // Create a signature with high Violence mean
    FMythicMoralAction Action;
    FMemory::Memzero(Action.AxisValues, sizeof(Action.AxisValues));
    Action.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] = 0.9f;
    Sig.AccumulateAction(Action);

    // Pacifist faction: Violence = -1.0
    FMythicIdeologyProfile Pacifist;
    Pacifist.Violence = -1.0f;

    // Warlike faction: Violence = +1.0
    FMythicIdeologyProfile Warlike;
    Warlike.Violence = 1.0f;

    float PacifistDot = Sig.EvaluateAgainst(Pacifist);
    float WarlikeDot = Sig.EvaluateAgainst(Warlike);

    // Violent player vs pacifist = negative alignment (0.9 * -1.0 = -0.9)
    TestTrue(TEXT("Pacifist should disapprove (negative dot)"), PacifistDot < 0.0f);
    TestNearlyEqual(TEXT("Pacifist dot = -0.9"), PacifistDot, -0.9f, 0.001f);

    // Violent player vs warlike = positive alignment (0.9 * 1.0 = 0.9)
    TestTrue(TEXT("Warlike should approve (positive dot)"), WarlikeDot > 0.0f);
    TestNearlyEqual(TEXT("Warlike dot = 0.9"), WarlikeDot, 0.9f, 0.001f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldMoralActionSeverityTest,
    "Mythic.LivingWorld.Phase1.MoralSignature.ActionSeverity",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldMoralActionSeverityTest::RunTest(const FString &Parameters) {
    FMythicIdeologyProfile Pacifist;
    Pacifist.Violence = -0.8f;

    // Mild violence action
    FMythicMoralAction MildAction;
    FMemory::Memzero(MildAction.AxisValues, sizeof(MildAction.AxisValues));
    MildAction.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] = 0.2f;

    EMythicMoralSeverity MildResult = FMythicMoralSignature::EvaluateActionSeverity(
        MildAction, Pacifist, 0.1f, 0.3f, 0.6f);
    // Severity = -(0.2 * -0.8) = 0.16. Above Disapprove(0.1) but below Condemn(0.3)
    TestEqual(TEXT("Mild violence vs pacifist = Disapprove"), MildResult, EMythicMoralSeverity::Disapprove);

    // Severe violence action
    FMythicMoralAction SevereAction;
    FMemory::Memzero(SevereAction.AxisValues, sizeof(SevereAction.AxisValues));
    SevereAction.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] = 0.8f;

    EMythicMoralSeverity SevereResult = FMythicMoralSignature::EvaluateActionSeverity(
        SevereAction, Pacifist, 0.1f, 0.3f, 0.6f);
    // Severity = -(0.8 * -0.8) = 0.64. Above Hostile(0.6)
    TestEqual(TEXT("Severe violence vs pacifist = Hostile"), SevereResult, EMythicMoralSeverity::Hostile);

    // Action aligned with faction values (warlike faction, violent action)
    FMythicIdeologyProfile Warlike;
    Warlike.Violence = 0.8f;

    EMythicMoralSeverity AlignedResult = FMythicMoralSignature::EvaluateActionSeverity(
        SevereAction, Warlike, 0.1f, 0.3f, 0.6f);
    // Severity = -(0.8 * 0.8) = -0.64. Negative → Ignore
    TestEqual(TEXT("Violence vs warlike = Ignore (aligned)"), AlignedResult, EMythicMoralSeverity::Ignore);

    return true;
}

// ═══════════════════════════════════════════════════════════════
//  SETTLEMENT REGISTRY TESTS
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSettlementRegisterQueryTest,
    "Mythic.LivingWorld.Phase3.SettlementRegistry.RegisterAndQuery",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSettlementRegisterQueryTest::RunTest(const FString &Parameters) {
    auto *Registry = NewObject<UMythicSettlementRegistry>();

    // Manually create settlement data (normally done by AMythicSettlement actors)
    // Since we can't spawn actors without a world, test the data path directly
    TestEqual(TEXT("Initial settlement count = 0"), Registry->GetSettlementCount(), 0);

    // Query empty registry — should return nullptr
    const FMythicSettlementData *Empty = Registry->GetSettlementAtCell(FMythicCellCoord(5, 5));
    TestNull(TEXT("Empty registry cell query should return null"), Empty);

    TArray<int32> AllIds;
    Registry->GetAllSettlementIds(AllIds);
    TestEqual(TEXT("Empty registry should have 0 IDs"), AllIds.Num(), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSettlementFactionQueryTest,
    "Mythic.LivingWorld.Phase3.SettlementRegistry.FactionQuery",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSettlementFactionQueryTest::RunTest(const FString &Parameters) {
    auto *Registry = NewObject<UMythicSettlementRegistry>();
    FMythicFactionId Faction0 = LivingWorldTestHelpers::MakeFactionId(0);

    TArray<int32> FactionSettlements;
    Registry->GetSettlementsForFaction(Faction0, FactionSettlements);
    TestEqual(TEXT("Empty registry faction query should be empty"), FactionSettlements.Num(), 0);

    return true;
}

// ═══════════════════════════════════════════════════════════════
//  WORLD SIM THREAD TESTS (via friend declaration)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSimEconomyTest,
    "Mythic.LivingWorld.Phase2.WorldSimThread.Economy",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSimEconomyTest::RunTest(const FString &Parameters) {
    // Create shared data objects
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(64);

    auto *DB = NewObject<UMythicFactionDatabase>();
    auto *FactionSettings = LivingWorldTestHelpers::CreateFactionSettings(20, 2);

    // Faction 0: has territory, produces food
    FactionSettings->InitialFactions[0].ControlledCellCount = 10;
    FactionSettings->InitialFactions[0].BaseProduction.Food = 1.0f;
    FactionSettings->InitialFactions[0].BaseProduction.Materials = 0.5f;
    FactionSettings->InitialFactions[0].Population = 100;
    FactionSettings->InitialFactions[0].bHasBeenPopulated = true;

    // Faction 1: no territory (bandit-like)
    FactionSettings->InitialFactions[1].ControlledCellCount = 0;
    FactionSettings->InitialFactions[1].BaseProduction = FMythicResourceStock();
    FactionSettings->InitialFactions[1].Population = 50;
    FactionSettings->InitialFactions[1].bHasBeenPopulated = true;
    FactionSettings->InitialFactions[1].bControlsTerritory = false;

    DB->Initialize(FactionSettings);

    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    auto *Settings = LivingWorldTestHelpers::CreateLivingWorldSettings();

    // Record initial reserves
    FMythicFactionData InitialData0;
    DB->GetFaction(LivingWorldTestHelpers::MakeFactionId(0), InitialData0);
    const float InitialReservesFood = InitialData0.Reserves.Food;

    // Setup and tick the sim thread directly (via friend access)
    FMythicWorldSimThread SimThread;
    FCriticalSection SimLock;
    SimThread.Setup(Fabric, DB, Grid, nullptr, Settings, 1.0f, &SimLock);

    // Run one tick
    SimThread.SimTick();

    // Faction 0 should have generated supply from territory
    FMythicFactionData PostData0;
    DB->GetFaction(LivingWorldTestHelpers::MakeFactionId(0), PostData0);

    // Supply should be non-zero for faction with territory + BaseProduction
    TestTrue(TEXT("Faction 0 supply.Food should be > 0 after tick"), PostData0.Supply.Food > 0.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSimPopulationTest,
    "Mythic.LivingWorld.Phase2.WorldSimThread.Population",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSimPopulationTest::RunTest(const FString &Parameters) {
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(64);

    auto *DB = NewObject<UMythicFactionDatabase>();
    auto *FactionSettings = LivingWorldTestHelpers::CreateFactionSettings(20, 1);

    // Faction with territory, food, and civilian population — should grow
    FactionSettings->InitialFactions[0].ControlledCellCount = 10;
    FactionSettings->InitialFactions[0].BaseProduction.Food = 5.0f;
    FactionSettings->InitialFactions[0].Population = 100;
    FactionSettings->InitialFactions[0].bHasBeenPopulated = true;
    FactionSettings->InitialFactions[0].bHasCivilianPopulation = true;
    FactionSettings->InitialFactions[0].Reserves.Food = 50.0f;

    DB->Initialize(FactionSettings);

    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    auto *Settings = LivingWorldTestHelpers::CreateLivingWorldSettings();

    FMythicWorldSimThread SimThread;
    FCriticalSection SimLock;
    SimThread.Setup(Fabric, DB, Grid, nullptr, Settings, 1.0f, &SimLock);

    // Tick economy first so reserves are populated, then population
    SimThread.TickEconomy();
    SimThread.TickPopulation();
    SimThread.CommitAllSnapshots();

    // With food-rich faction, population should not decrease catastrophically
    FMythicFactionData PostData;
    DB->GetFaction(LivingWorldTestHelpers::MakeFactionId(0), PostData);
    TestTrue(TEXT("Population should remain positive with abundant food"), PostData.Population > 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSimDiplomacyTest,
    "Mythic.LivingWorld.Phase2.WorldSimThread.Diplomacy",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSimDiplomacyTest::RunTest(const FString &Parameters) {
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(64);

    auto *DB = NewObject<UMythicFactionDatabase>();
    auto *FactionSettings = LivingWorldTestHelpers::CreateFactionSettings(20, 3);

    // Faction 0: Pacifist (Violence = -0.8)
    FactionSettings->InitialFactions[0].Ideology.Violence = -0.8f;
    FactionSettings->InitialFactions[0].Ideology.Mercy = 0.7f;
    FactionSettings->InitialFactions[0].Population = 100;
    FactionSettings->InitialFactions[0].bHasBeenPopulated = true;

    // Faction 1: Also pacifist (similar ideology → should trend friendly)
    FactionSettings->InitialFactions[1].Ideology.Violence = -0.7f;
    FactionSettings->InitialFactions[1].Ideology.Mercy = 0.8f;
    FactionSettings->InitialFactions[1].Population = 100;
    FactionSettings->InitialFactions[1].bHasBeenPopulated = true;

    // Faction 2: Warlike (opposed ideology → should trend hostile)
    FactionSettings->InitialFactions[2].Ideology.Violence = 0.9f;
    FactionSettings->InitialFactions[2].Ideology.Mercy = -0.5f;
    FactionSettings->InitialFactions[2].Population = 100;
    FactionSettings->InitialFactions[2].bHasBeenPopulated = true;

    DB->Initialize(FactionSettings);

    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    auto *Settings = LivingWorldTestHelpers::CreateLivingWorldSettings();

    FMythicWorldSimThread SimThread;
    FCriticalSection SimLock;
    SimThread.Setup(Fabric, DB, Grid, nullptr, Settings, 1.0f, &SimLock);

    // Run diplomacy ticks repeatedly to allow relationship drift
    for (int32 i = 0; i < 20; ++i) {
        SimThread.TickDiplomacy();
    }
    SimThread.CommitAllSnapshots();

    // Similar factions should not be hostile
    EMythicFactionRelation SimilarRelation = DB->GetRelationship(
        LivingWorldTestHelpers::MakeFactionId(0), LivingWorldTestHelpers::MakeFactionId(1));
    TestTrue(TEXT("Similar ideology factions should not be Hostile"),
             SimilarRelation != EMythicFactionRelation::Hostile);

    // Opposed factions: verify they are not Allied (with enough ticks they should drift apart)
    EMythicFactionRelation OpposedRelation = DB->GetRelationship(
        LivingWorldTestHelpers::MakeFactionId(0), LivingWorldTestHelpers::MakeFactionId(2));
    TestTrue(TEXT("Opposed ideology factions should not be Allied"),
             OpposedRelation != EMythicFactionRelation::Allied);

    return true;
}

// ═══════════════════════════════════════════════════════════════
//  SOCIAL GRAPH TESTS
// ═══════════════════════════════════════════════════════════════

// ─── Initialization ──────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSocialGraphInitTest,
    "Mythic.LivingWorld.SocialGraph.Initialize",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSocialGraphInitTest::RunTest(const FString &Parameters) {
    auto *Graph = NewObject<UMythicSocialGraph>();
    Graph->Initialize(8, 0.05f, 0.001f);

    TestEqual(TEXT("Empty graph should have 0 entities"), Graph->GetEntityCount(), 0);
    TestEqual(TEXT("Empty graph should have 0 edges"), Graph->GetTotalEdgeCount(), 0);

    return true;
}

// ─── Edge CRUD ───────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSocialGraphAddEdgeTest,
    "Mythic.LivingWorld.SocialGraph.AddAndQueryEdges",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSocialGraphAddEdgeTest::RunTest(const FString &Parameters) {
    auto *Graph = NewObject<UMythicSocialGraph>();
    Graph->Initialize(8, 0.05f, 0.001f);

    FMassEntityHandle EntityA;
    EntityA.Index = 1;
    EntityA.SerialNumber = 1;
    FMassEntityHandle EntityB;
    EntityB.Index = 2;
    EntityB.SerialNumber = 1;
    FMassEntityHandle EntityC;
    EntityC.Index = 3;
    EntityC.SerialNumber = 1;
    FMythicFactionId FactionA = LivingWorldTestHelpers::MakeFactionId(0);

    const double Now = 1000.0;

    // Add edge A → B
    Graph->AddOrStrengthenEdge(EntityA, EntityB, EMythicSocialRelation::Friend, 0.8f, Now, FactionA);
    TestEqual(TEXT("Should have 1 edge"), Graph->GetTotalEdgeCount(), 1);
    TestEqual(TEXT("Should have 1 entity with edges"), Graph->GetEntityCount(), 1);

    // Add edge A → C
    Graph->AddOrStrengthenEdge(EntityA, EntityC, EMythicSocialRelation::Rival, 0.6f, Now, FactionA);
    TestEqual(TEXT("Should have 2 edges"), Graph->GetTotalEdgeCount(), 2);

    // Query edges from A
    TArray<FMythicSocialEdge> Edges;
    int32 Count = Graph->GetEdges(EntityA, Now, Edges);
    TestEqual(TEXT("EntityA should have 2 outgoing edges"), Count, 2);

    // Check specific edge exists
    FMythicSocialEdge FoundEdge;
    bool bHasEdge = Graph->HasEdge(EntityA, EntityB, Now, FoundEdge);
    TestTrue(TEXT("A→B edge should exist"), bHasEdge);
    TestEqual(TEXT("A→B relation should be Friend"),
              FoundEdge.Relation, EMythicSocialRelation::Friend);

    // Directed: B has no outgoing edges
    TArray<FMythicSocialEdge> BEdges;
    Count = Graph->GetEdges(EntityB, Now, BEdges);
    TestEqual(TEXT("EntityB should have 0 outgoing edges"), Count, 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSocialGraphStrengthenTest,
    "Mythic.LivingWorld.SocialGraph.StrengthenExistingEdge",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSocialGraphStrengthenTest::RunTest(const FString &Parameters) {
    auto *Graph = NewObject<UMythicSocialGraph>();
    Graph->Initialize(8, 0.05f, 0.001f);

    FMassEntityHandle A;
    A.Index = 1;
    A.SerialNumber = 1;
    FMassEntityHandle B;
    B.Index = 2;
    B.SerialNumber = 1;
    FMythicFactionId Faction = LivingWorldTestHelpers::MakeFactionId(0);

    Graph->AddOrStrengthenEdge(A, B, EMythicSocialRelation::Friend, 0.3f, 100.0, Faction);
    Graph->AddOrStrengthenEdge(A, B, EMythicSocialRelation::Friend, 0.3f, 200.0, Faction);

    TestEqual(TEXT("Should still be 1 edge after strengthen"), Graph->GetTotalEdgeCount(), 1);

    FMythicSocialEdge Edge;
    Graph->HasEdge(A, B, 200.0, Edge);
    TestTrue(TEXT("Strengthened edge should have higher strength"), Edge.Strength > 0.3f);
    TestEqual(TEXT("Last interaction time should be updated"), Edge.LastInteractionTime, 200.0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSocialGraphRemoveEdgeTest,
    "Mythic.LivingWorld.SocialGraph.RemoveEdge",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSocialGraphRemoveEdgeTest::RunTest(const FString &Parameters) {
    auto *Graph = NewObject<UMythicSocialGraph>();
    Graph->Initialize(8, 0.05f, 0.001f);

    FMassEntityHandle A;
    A.Index = 1;
    A.SerialNumber = 1;
    FMassEntityHandle B;
    B.Index = 2;
    B.SerialNumber = 1;
    FMassEntityHandle C;
    C.Index = 3;
    C.SerialNumber = 1;
    FMythicFactionId Faction = LivingWorldTestHelpers::MakeFactionId(0);

    Graph->AddOrStrengthenEdge(A, B, EMythicSocialRelation::Friend, 0.8f, 100.0, Faction);
    Graph->AddOrStrengthenEdge(A, C, EMythicSocialRelation::Rival, 0.6f, 100.0, Faction);

    bool bRemoved = Graph->RemoveEdge(A, B);
    TestTrue(TEXT("RemoveEdge should return true"), bRemoved);
    TestEqual(TEXT("Should have 1 edge after removal"), Graph->GetTotalEdgeCount(), 1);

    bRemoved = Graph->RemoveEdge(A, B);
    TestFalse(TEXT("RemoveEdge should return false for non-existent"), bRemoved);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSocialGraphRemoveAllTest,
    "Mythic.LivingWorld.SocialGraph.RemoveAllEdgesOnDeath",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSocialGraphRemoveAllTest::RunTest(const FString &Parameters) {
    auto *Graph = NewObject<UMythicSocialGraph>();
    Graph->Initialize(8, 0.05f, 0.001f);

    FMassEntityHandle A;
    A.Index = 1;
    A.SerialNumber = 1;
    FMassEntityHandle B;
    B.Index = 2;
    B.SerialNumber = 1;
    FMassEntityHandle C;
    C.Index = 3;
    C.SerialNumber = 1;
    FMythicFactionId Faction = LivingWorldTestHelpers::MakeFactionId(0);

    Graph->AddOrStrengthenEdge(A, B, EMythicSocialRelation::Friend, 0.8f, 100.0, Faction);
    Graph->AddOrStrengthenEdge(A, C, EMythicSocialRelation::Family, 0.9f, 100.0, Faction);
    Graph->AddOrStrengthenEdge(B, A, EMythicSocialRelation::Friend, 0.7f, 100.0, Faction);
    Graph->AddOrStrengthenEdge(C, A, EMythicSocialRelation::Family, 0.9f, 100.0, Faction);

    TArray<FMassEntityHandle> SeveredConnections;
    Graph->RemoveAllEdges(A, SeveredConnections);

    TestEqual(TEXT("All edges involving A should be removed"), Graph->GetTotalEdgeCount(), 0);
    TestTrue(TEXT("Severed connections should include B and C"), SeveredConnections.Num() >= 2);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSocialGraphRelationFilterTest,
    "Mythic.LivingWorld.SocialGraph.QueryByRelation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSocialGraphRelationFilterTest::RunTest(const FString &Parameters) {
    auto *Graph = NewObject<UMythicSocialGraph>();
    Graph->Initialize(8, 0.05f, 0.001f);

    FMassEntityHandle A;
    A.Index = 1;
    A.SerialNumber = 1;
    FMassEntityHandle B;
    B.Index = 2;
    B.SerialNumber = 1;
    FMassEntityHandle C;
    C.Index = 3;
    C.SerialNumber = 1;
    FMassEntityHandle D;
    D.Index = 4;
    D.SerialNumber = 1;
    FMythicFactionId Faction = LivingWorldTestHelpers::MakeFactionId(0);

    Graph->AddOrStrengthenEdge(A, B, EMythicSocialRelation::Friend, 0.8f, 100.0, Faction);
    Graph->AddOrStrengthenEdge(A, C, EMythicSocialRelation::Rival, 0.7f, 100.0, Faction);
    Graph->AddOrStrengthenEdge(A, D, EMythicSocialRelation::Friend, 0.6f, 100.0, Faction);

    TArray<FMythicSocialEdge> Friends;
    TestEqual(TEXT("A should have 2 Friend edges"),
              Graph->GetEdgesByRelation(A, EMythicSocialRelation::Friend, 100.0, Friends), 2);

    TArray<FMythicSocialEdge> Rivals;
    TestEqual(TEXT("A should have 1 Rival edge"),
              Graph->GetEdgesByRelation(A, EMythicSocialRelation::Rival, 100.0, Rivals), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSocialGraphMaxEdgesTest,
    "Mythic.LivingWorld.SocialGraph.MaxEdgeEviction",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSocialGraphMaxEdgesTest::RunTest(const FString &Parameters) {
    const int32 MaxEdges = 3;
    auto *Graph = NewObject<UMythicSocialGraph>();
    Graph->Initialize(MaxEdges, 0.05f, 0.001f);

    FMassEntityHandle Source;
    Source.Index = 1;
    Source.SerialNumber = 1;
    FMythicFactionId Faction = LivingWorldTestHelpers::MakeFactionId(0);

    for (int32 i = 0; i < MaxEdges; ++i) {
        FMassEntityHandle Target;
        Target.Index = 10 + i;
        Target.SerialNumber = 1;
        Graph->AddOrStrengthenEdge(Source, Target, EMythicSocialRelation::Associate,
                                   0.3f + 0.1f * i, 100.0, Faction);
    }
    TestEqual(TEXT("Should have MaxEdges"), Graph->GetTotalEdgeCount(), MaxEdges);

    FMassEntityHandle NewTarget;
    NewTarget.Index = 99;
    NewTarget.SerialNumber = 1;
    Graph->AddOrStrengthenEdge(Source, NewTarget, EMythicSocialRelation::Friend, 0.9f, 100.0, Faction);
    TestEqual(TEXT("Should still have MaxEdges after eviction"), Graph->GetTotalEdgeCount(), MaxEdges);

    FMythicSocialEdge FoundEdge;
    TestTrue(TEXT("New high-strength edge should exist"),
             Graph->HasEdge(Source, NewTarget, 100.0, FoundEdge));

    FMassEntityHandle WeakestTarget;
    WeakestTarget.Index = 10;
    WeakestTarget.SerialNumber = 1;
    TestFalse(TEXT("Weakest edge should be evicted"),
              Graph->HasEdge(Source, WeakestTarget, 100.0, FoundEdge));

    return true;
}

// ─── Pruning ─────────────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSocialGraphPruneTest,
    "Mythic.LivingWorld.SocialGraph.PruneStaleEdges",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSocialGraphPruneTest::RunTest(const FString &Parameters) {
    const float TestDecayRate = 0.1f;
    const float PruneThreshold = 0.01f;
    auto *Graph = NewObject<UMythicSocialGraph>();
    Graph->Initialize(8, PruneThreshold, TestDecayRate);

    FMassEntityHandle A;
    A.Index = 1;
    A.SerialNumber = 1;
    FMassEntityHandle B;
    B.Index = 2;
    B.SerialNumber = 1;
    FMassEntityHandle C;
    C.Index = 3;
    C.SerialNumber = 1;
    FMythicFactionId Faction = LivingWorldTestHelpers::MakeFactionId(0);

    Graph->AddOrStrengthenEdge(A, B, EMythicSocialRelation::Friend, 0.5f, 0.0, Faction);
    Graph->AddOrStrengthenEdge(A, C, EMythicSocialRelation::Friend, 0.5f, 0.0, Faction);

    int32 Pruned = Graph->PruneStaleEdges(100.0, 100);
    TestEqual(TEXT("Both heavily decayed edges should be pruned"), Pruned, 2);
    TestEqual(TEXT("Graph should be empty"), Graph->GetTotalEdgeCount(), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSocialGraphPruneSelectiveTest,
    "Mythic.LivingWorld.SocialGraph.PruneSelectiveDecay",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSocialGraphPruneSelectiveTest::RunTest(const FString &Parameters) {
    const float TestDecayRate = 0.01f;
    const float PruneThreshold = 0.05f;
    auto *Graph = NewObject<UMythicSocialGraph>();
    Graph->Initialize(8, PruneThreshold, TestDecayRate);

    FMassEntityHandle A;
    A.Index = 1;
    A.SerialNumber = 1;
    FMassEntityHandle B;
    B.Index = 2;
    B.SerialNumber = 1;
    FMassEntityHandle C;
    C.Index = 3;
    C.SerialNumber = 1;
    FMythicFactionId Faction = LivingWorldTestHelpers::MakeFactionId(0);

    Graph->AddOrStrengthenEdge(A, B, EMythicSocialRelation::Friend, 0.5f, 0.0, Faction);
    Graph->AddOrStrengthenEdge(A, C, EMythicSocialRelation::Friend, 0.5f, 290.0, Faction);

    int32 Pruned = Graph->PruneStaleEdges(300.0, 100);
    TestEqual(TEXT("Only old edge should be pruned"), Pruned, 1);
    TestEqual(TEXT("Recent edge should survive"), Graph->GetTotalEdgeCount(), 1);

    FMythicSocialEdge Edge;
    TestTrue(TEXT("A→C should still exist"), Graph->HasEdge(A, C, 300.0, Edge));
    TestFalse(TEXT("A→B should be pruned"), Graph->HasEdge(A, B, 300.0, Edge));

    return true;
}

// ═══════════════════════════════════════════════════════════════
//  SCHEME ENGINE TESTS
// ═══════════════════════════════════════════════════════════════

namespace SchemeTestHelpers {
    UMythicFactionDatabaseSettings *CreateSchemeTestFactionSettings() {
        auto *Settings = NewObject<UMythicFactionDatabaseSettings>();
        Settings->MaxFactions = 20;
        Settings->InitialFactions.SetNum(4);

        for (int32 i = 0; i < 4; ++i) {
            FMythicFactionData &F = Settings->InitialFactions[i];
            F.DisplayName = FText::FromString(FString::Printf(TEXT("SchemeFaction_%d"), i));
            F.bAlive = true;
            F.Population = 100 * (i + 1);
            F.MilitaryStrength = 0.3f + 0.2f * i;
            F.bHasBeenPopulated = true;
            F.bControlsTerritory = true;
            F.bHasEconomy = true;
            F.bHasCivilianPopulation = true;
            F.bParticipatesInTrade = true;
            F.bCanNegotiate = true;
        }
        return Settings;
    }

    UMythicLivingWorldSettings *CreateSchemeTestLivingWorldSettings() {
        auto *Settings = LivingWorldTestHelpers::CreateLivingWorldSettings();
        Settings->SchemeGenerationTickInterval = 1;
        Settings->SchemeBaseProbability = 1.0f;
        Settings->MaxSchemesPerFaction = 5;
        Settings->MaxTotalSchemes = 50;
        return Settings;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSchemeEngineInitTest,
    "Mythic.LivingWorld.SchemeEngine.Initialize",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSchemeEngineInitTest::RunTest(const FString &Parameters) {
    auto *Engine = NewObject<UMythicSchemeEngine>();
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(64);

    auto *FactionDB = NewObject<UMythicFactionDatabase>();
    FactionDB->Initialize(SchemeTestHelpers::CreateSchemeTestFactionSettings());

    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    Engine->Initialize(FactionDB, Fabric, Grid, SchemeTestHelpers::CreateSchemeTestLivingWorldSettings());

    TestEqual(TEXT("No active schemes after init"), Engine->GetActiveSchemeCount(), 0);
    TestEqual(TEXT("GetActiveSchemes returns empty"), Engine->GetActiveSchemes().Num(), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSchemeEngineGenerateTest,
    "Mythic.LivingWorld.SchemeEngine.GenerateSchemes",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSchemeEngineGenerateTest::RunTest(const FString &Parameters) {
    auto *Engine = NewObject<UMythicSchemeEngine>();
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(64);

    auto *FactionSettings = SchemeTestHelpers::CreateSchemeTestFactionSettings();
    FactionSettings->InitialFactions[0].MilitaryStrength = 0.8f;
    FactionSettings->InitialFactions[1].MilitaryStrength = 0.8f;
    auto *FactionDB = NewObject<UMythicFactionDatabase>();
    FactionDB->Initialize(FactionSettings);

    FactionDB->SetRelationship(LivingWorldTestHelpers::MakeFactionId(0),
                               LivingWorldTestHelpers::MakeFactionId(1),
                               EMythicFactionRelation::Hostile);

    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    Engine->Initialize(FactionDB, Fabric, Grid, SchemeTestHelpers::CreateSchemeTestLivingWorldSettings());

    Engine->TickSchemes(1.0f, 0);

    TestTrue(TEXT("Should have generated at least 1 scheme"), Engine->GetActiveSchemeCount() > 0);

    TArray<FMythicScheme> Schemes = Engine->GetActiveSchemes();
    if (Schemes.Num() > 0) {
        const FMythicScheme &First = Schemes[0];
        TestTrue(TEXT("Scheme should be active"), First.IsActive());
        TestTrue(TEXT("SchemeId should be > 0"), First.SchemeId > 0);
        TestTrue(TEXT("Origin != Target"), First.OriginFaction != First.TargetFaction);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSchemeEngineProgressTest,
    "Mythic.LivingWorld.SchemeEngine.ProgressSchemes",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSchemeEngineProgressTest::RunTest(const FString &Parameters) {
    auto *Engine = NewObject<UMythicSchemeEngine>();
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(64);

    auto *FactionSettings = SchemeTestHelpers::CreateSchemeTestFactionSettings();
    FactionSettings->InitialFactions[0].MilitaryStrength = 0.8f;
    auto *FactionDB = NewObject<UMythicFactionDatabase>();
    FactionDB->Initialize(FactionSettings);
    FactionDB->SetRelationship(LivingWorldTestHelpers::MakeFactionId(0),
                               LivingWorldTestHelpers::MakeFactionId(1),
                               EMythicFactionRelation::Hostile);

    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    Engine->Initialize(FactionDB, Fabric, Grid, SchemeTestHelpers::CreateSchemeTestLivingWorldSettings());

    Engine->TickSchemes(1.0f, 0);
    TArray<FMythicScheme> Before = Engine->GetActiveSchemes();
    TestTrue(TEXT("Should have generated schemes"), Before.Num() > 0);

    float InitialProgress = Before.Num() > 0 ? Before[0].Progress : 0.0f;
    uint32 TrackId = Before.Num() > 0 ? Before[0].SchemeId : 0;

    Engine->TickSchemes(10.0f, 1);
    TArray<FMythicScheme> After = Engine->GetActiveSchemes();

    for (const FMythicScheme &S : After) {
        if (S.SchemeId == TrackId) {
            TestTrue(TEXT("Scheme progress should increase"), S.Progress > InitialProgress);
            break;
        }
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSchemeEngineCompletionTest,
    "Mythic.LivingWorld.SchemeEngine.SchemeCompletion",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSchemeEngineCompletionTest::RunTest(const FString &Parameters) {
    auto *Engine = NewObject<UMythicSchemeEngine>();
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(256);

    auto *FactionSettings = SchemeTestHelpers::CreateSchemeTestFactionSettings();
    FactionSettings->InitialFactions[0].MilitaryStrength = 0.8f;
    auto *FactionDB = NewObject<UMythicFactionDatabase>();
    FactionDB->Initialize(FactionSettings);
    FactionDB->SetRelationship(LivingWorldTestHelpers::MakeFactionId(0),
                               LivingWorldTestHelpers::MakeFactionId(1),
                               EMythicFactionRelation::Hostile);

    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    Engine->Initialize(FactionDB, Fabric, Grid, SchemeTestHelpers::CreateSchemeTestLivingWorldSettings());

    Engine->TickSchemes(1.0f, 0);

    // Tick many times with large delta to force completion
    for (int32 i = 1; i < 200; ++i) {
        Engine->TickSchemes(10.0f, static_cast<uint32>(i));
    }

    Fabric->CommitWrites();
    TestTrue(TEXT("Fabric should have scheme events"), Fabric->GetTotalEventCount() > 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSchemeEngineByFactionTest,
    "Mythic.LivingWorld.SchemeEngine.GetSchemesByFaction",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSchemeEngineByFactionTest::RunTest(const FString &Parameters) {
    auto *Engine = NewObject<UMythicSchemeEngine>();
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(64);

    auto *FactionSettings = SchemeTestHelpers::CreateSchemeTestFactionSettings();
    FactionSettings->InitialFactions[0].MilitaryStrength = 0.8f;
    auto *FactionDB = NewObject<UMythicFactionDatabase>();
    FactionDB->Initialize(FactionSettings);
    FactionDB->SetRelationship(LivingWorldTestHelpers::MakeFactionId(0),
                               LivingWorldTestHelpers::MakeFactionId(1),
                               EMythicFactionRelation::Hostile);

    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    Engine->Initialize(FactionDB, Fabric, Grid, SchemeTestHelpers::CreateSchemeTestLivingWorldSettings());
    Engine->TickSchemes(1.0f, 0);

    TArray<FMythicScheme> Faction0 = Engine->GetSchemesByFaction(LivingWorldTestHelpers::MakeFactionId(0));
    for (const FMythicScheme &S : Faction0) {
        TestEqual(TEXT("Origin should match query"), S.OriginFaction.Index, (uint8)0);
    }

    TArray<FMythicScheme> NoSchemes = Engine->GetSchemesByFaction(LivingWorldTestHelpers::MakeFactionId(15));
    TestEqual(TEXT("Non-existent faction should have 0 schemes"), NoSchemes.Num(), 0);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSchemeEngineMaxPerFactionTest,
    "Mythic.LivingWorld.SchemeEngine.MaxSchemesPerFaction",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSchemeEngineMaxPerFactionTest::RunTest(const FString &Parameters) {
    auto *Engine = NewObject<UMythicSchemeEngine>();
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(64);

    auto *FactionSettings = SchemeTestHelpers::CreateSchemeTestFactionSettings();
    FactionSettings->InitialFactions[0].MilitaryStrength = 0.8f;
    auto *FactionDB = NewObject<UMythicFactionDatabase>();
    FactionDB->Initialize(FactionSettings);
    FactionDB->SetRelationship(LivingWorldTestHelpers::MakeFactionId(0),
                               LivingWorldTestHelpers::MakeFactionId(1),
                               EMythicFactionRelation::Hostile);

    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    auto *Settings = SchemeTestHelpers::CreateSchemeTestLivingWorldSettings();
    Settings->MaxSchemesPerFaction = 3;
    Engine->Initialize(FactionDB, Fabric, Grid, Settings);

    for (uint32 i = 0; i < 50; ++i) {
        Engine->TickSchemes(0.001f, i);
    }

    TArray<FMythicScheme> Faction0 = Engine->GetSchemesByFaction(LivingWorldTestHelpers::MakeFactionId(0));
    TestTrue(TEXT("Per-faction cap should be respected"),
             Faction0.Num() <= Settings->MaxSchemesPerFaction);

    return true;
}

// Regression: a faction that holds a Hostile/Unfriendly relation toward a faction that was LATER annihilated must
// not pick that DEAD faction as a scheme target. AnnihilateFaction zeroes population/military/territory and sets
// bAlive=false but intentionally leaves the relationship rows intact (faction indices are never recycled), so without
// a bAlive guard in GenerateSchemes' target-selection loop the schemer wastes its scheme budget plotting against a
// non-existent enemy and emits nonsensical diplomacy/chronicle events. There is no downstream bAlive guard in
// ProgressScheme/ExecuteScheme/ApplySchemeEffects that makes this benign (DiplomaticPressure still worsens the
// relationship toward the corpse, ExecuteScheme still writes a "Scheme Completed" event), so the fix lives at
// generation — and this test pins it there.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSchemeEngineSkipsDeadTargetTest,
    "Mythic.LivingWorld.SchemeEngine.SkipsAnnihilatedTarget",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSchemeEngineSkipsDeadTargetTest::RunTest(const FString &Parameters) {
    auto *Engine = NewObject<UMythicSchemeEngine>();
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(64);

    // Faction 0 is the schemer: strong + populous so it is eligible and (with SchemeBaseProbability=1.0) always rolls
    // to generate a scheme this tick.
    auto *FactionSettings = SchemeTestHelpers::CreateSchemeTestFactionSettings();
    FactionSettings->InitialFactions[0].MilitaryStrength = 0.8f;
    auto *FactionDB = NewObject<UMythicFactionDatabase>();
    FactionDB->Initialize(FactionSettings);

    const FMythicFactionId Schemer = LivingWorldTestHelpers::MakeFactionId(0);
    const FMythicFactionId DeadTarget = LivingWorldTestHelpers::MakeFactionId(1);

    // Faction 0's ONLY Hostile/Unfriendly relationship is toward faction 1. Factions 2 and 3 stay Neutral, so faction
    // 1 is the single candidate the target-selection loop could ever pick for faction 0.
    FactionDB->SetRelationship(Schemer, DeadTarget, EMythicFactionRelation::Hostile);

    // Annihilate the sole hostile target, leaving the relationship rows intact — exactly the production condition that
    // exposes the bug.
    FactionDB->AnnihilateFaction(DeadTarget);
    FactionDB->CommitWrites();

    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings());

    Engine->Initialize(FactionDB, Fabric, Grid, SchemeTestHelpers::CreateSchemeTestLivingWorldSettings());

    Engine->TickSchemes(1.0f, 0);

    // No scheme may target the annihilated faction.
    const TArray<FMythicScheme> Schemes = Engine->GetActiveSchemes();
    int32 SchemesAgainstDead = 0;
    for (const FMythicScheme &S : Schemes) {
        if (S.TargetFaction == DeadTarget) {
            ++SchemesAgainstDead;
        }
    }
    TestEqual(TEXT("No scheme should target the annihilated faction"), SchemesAgainstDead, 0);

    // Faction 1 was faction 0's only valid target and factions 2/3 are Neutral toward everyone, so no scheme should be
    // generated at all this tick.
    TestEqual(TEXT("No schemes generated when the only hostile target is dead"), Engine->GetActiveSchemeCount(), 0);

    return true;
}
