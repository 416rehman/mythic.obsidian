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
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Simulation/WorldSimThread.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/Simulation/SchemeEngine.h"
#include "World/LivingWorld/Simulation/SchemeTypes.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/MythicCellSpatialIndex.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "AI/Party/PartySubsystem.h"
#include "AI/NPCs/MythicAIController.h"
#include "Interaction/MythicInteractionComponent.h"
#include "AI/Cognition/CognitiveTypes.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h" // UObjectiveTracker::ComputeObjectiveProgress
#include "Mass/Processors/PressureProcessor.h"
#include "Mass/Processors/SignificanceProcessor.h"
#include "Mass/Processors/ScheduleTransitionProcessor.h"
#include "Mass/Processors/CreatureEcologyProcessor.h"
#include "Mass/Processors/PopulationSpawnerProcessor.h"
#include "World/LivingWorld/LivingWorldReplication.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "GAS/Executions/MythicDamageApplication.h" // ShouldNegateFriendlyFire
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "Itemization/Conversion/ConversionStationComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "Rewards/LootReward.h"
#include "World/LivingWorld/Dialogue/DialogueSelector.h"
#include "World/LivingWorld/Dialogue/MythicDialogueTypes.h" // FMythicDialogueTemplate + UMythicDialogueDatabase
#include "World/LivingWorld/Chronicle/MythicWorldChronicleSubsystem.h" // EventTagToReadable
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "World/EnvironmentController/MythicEnvironmentController.h"
#include "World/EnvironmentController/EnvironmentTypes.h" // UWeatherType::CanTransitionTo
#include "Subsystem/SaveSystem/Character/SavedInventory.h"
#include "Resources/MythicResourceManagerComponent.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "Player/MythicCharacter.h"
#include "Player/MythicPlayerState.h" // ResolveCanonicalPlayerKey
#include "Player/MythicPlayerRegistrySubsystem.h" // ResolveRegisteredKey
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Itemization/Inventory/MythicEncumbrance.h" // ComputeTier / SpeedMultiplierForTier
#include "Itemization/Inventory/MythicInventoryComponent.h" // ComputeSlotWeight
#include "Itemization/Inventory/MythicCurrency.h" // CanAfford / ComputeBalanceAfterSpend / ComputeSalePrice
#include "Mass/EntityHandle.h"

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

// ═══════════════════════════════════════════════════════════════
// FMythicCellSpatialIndex — broad-phase cell->entity range query
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSpatialIndexQueryTest,
    "Mythic.LivingWorld.SpatialIndex.QueryRange",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSpatialIndexQueryTest::RunTest(const FString &Parameters) {
    FMythicCellSpatialIndex Index;

    // (Index, SerialNumber) are arbitrary identities for the test; only the cell placement matters here.
    const FMassEntityHandle E1(1, 1); // cell (5,5)
    const FMassEntityHandle E2(2, 1); // cell (5,6)
    const FMassEntityHandle E3(3, 1); // cell (6,5)
    const FMassEntityHandle E4(4, 1); // cell (9,9) — outside a radius-1 box around (5,5)
    Index.Insert(FMythicCellCoord(5, 5), E1);
    Index.Insert(FMythicCellCoord(5, 6), E2);
    Index.Insert(FMythicCellCoord(6, 5), E3);
    Index.Insert(FMythicCellCoord(9, 9), E4);

    TestEqual(TEXT("Num indexed"), Index.Num(), 4);
    TestEqual(TEXT("Per-cell count (5,5)"), Index.GetCellCount(FMythicCellCoord(5, 5)), 1);
    TestEqual(TEXT("Per-cell count empty"), Index.GetCellCount(FMythicCellCoord(0, 0)), 0);

    // Radius-1 (Chebyshev) box around (5,5) covers x,y in [4..6] → E1,E2,E3 but NOT the far E4 (9,9).
    TArray<FMassEntityHandle> R1;
    Index.QueryRange(FMythicCellCoord(5, 5), 1, R1);
    TestEqual(TEXT("r=1 returns 3 candidates"), R1.Num(), 3);
    TestTrue(TEXT("r=1 contains E1"), R1.Contains(E1));
    TestTrue(TEXT("r=1 contains E2"), R1.Contains(E2));
    TestTrue(TEXT("r=1 contains E3"), R1.Contains(E3));
    TestFalse(TEXT("r=1 excludes far E4"), R1.Contains(E4));

    // Radius 0 → only the center cell.
    TArray<FMassEntityHandle> R0;
    Index.QueryRange(FMythicCellCoord(5, 5), 0, R0);
    TestEqual(TEXT("r=0 returns 1"), R0.Num(), 1);
    TestTrue(TEXT("r=0 is E1"), R0.Contains(E1));

    // Negative radius → no-op (does not clear/append).
    TArray<FMassEntityHandle> RNeg;
    Index.QueryRange(FMythicCellCoord(5, 5), -1, RNeg);
    TestEqual(TEXT("negative radius no-op"), RNeg.Num(), 0);

    // Large radius pulls in everything including the far entity.
    TArray<FMassEntityHandle> RBig;
    Index.QueryRange(FMythicCellCoord(5, 5), 8, RBig);
    TestEqual(TEXT("r=8 returns all 4"), RBig.Num(), 4);

    // Reset clears counts; the structure stays reusable (next rebuild starts empty).
    Index.Reset();
    TestEqual(TEXT("Num after reset"), Index.Num(), 0);
    TestEqual(TEXT("Per-cell count after reset"), Index.GetCellCount(FMythicCellCoord(5, 5)), 0);
    TArray<FMassEntityHandle> RAfter;
    Index.QueryRange(FMythicCellCoord(5, 5), 2, RAfter);
    TestEqual(TEXT("Query after reset is empty"), RAfter.Num(), 0);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// PersistentNPCRegistry — perma-death set + save/load round-trip (incl. the v2 monotonic spawn serial, R18-M7)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldPersistentNPCRegistryTest,
    "Mythic.LivingWorld.Phase1.PersistentNPCRegistry.RegisterAndSerialize",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldPersistentNPCRegistryTest::RunTest(const FString &Parameters) {
    auto *Registry = NewObject<UMythicPersistentNPCRegistry>();

    // OwningLWS = nullptr is safe by construction: RegisterDeath adds to the death set + records FIRST, then guards
    // the world-event + settlement-succession side effects on (OwningLWS). So the registry's own state is exercisable
    // without a world/subsystem.
    const FMythicFactionId F0 = LivingWorldTestHelpers::MakeFactionId(0);
    const FGameplayTag NoRole = FGameplayTag::EmptyTag;
    Registry->RegisterDeath(1001u, F0, NoRole, FMythicCellCoord(3, 4), 100.0, nullptr);
    Registry->RegisterDeath(1002u, F0, NoRole, FMythicCellCoord(5, 6), 200.0, nullptr);

    TestTrue(TEXT("1001 is perma-dead"), Registry->IsPermaDead(1001u));
    TestTrue(TEXT("1002 is perma-dead"), Registry->IsPermaDead(1002u));
    TestFalse(TEXT("9999 is not perma-dead"), Registry->IsPermaDead(9999u));
    TestEqual(TEXT("death count == 2"), Registry->GetDeathCount(), 2);

    // Duplicate registration is a no-op (the dup-guard prevents double-counting a re-reported death).
    Registry->RegisterDeath(1001u, F0, NoRole, FMythicCellCoord(3, 4), 150.0, nullptr);
    TestEqual(TEXT("duplicate register is a no-op"), Registry->GetDeathCount(), 2);

    // Spawn serial is monotonic + never reused (R18-M7 collision-free identity).
    const int32 Serial0 = Registry->AllocateSpawnSerial();
    const int32 Serial1 = Registry->AllocateSpawnSerial();
    const int32 Serial2 = Registry->AllocateSpawnSerial();
    TestEqual(TEXT("spawn serial increments (1)"), Serial1, Serial0 + 1);
    TestEqual(TEXT("spawn serial increments (2)"), Serial2, Serial1 + 1);

    // Save → load into a FRESH registry → state preserved.
    TArray<uint8> Bytes;
    {
        FMemoryWriter Writer(Bytes);
        Registry->Serialize(Writer);
    }
    auto *Loaded = NewObject<UMythicPersistentNPCRegistry>();
    {
        FMemoryReader Reader(Bytes);
        Loaded->Serialize(Reader);
    }

    TestTrue(TEXT("loaded: 1001 perma-dead"), Loaded->IsPermaDead(1001u));
    TestTrue(TEXT("loaded: 1002 perma-dead"), Loaded->IsPermaDead(1002u));
    TestFalse(TEXT("loaded: 9999 not perma-dead"), Loaded->IsPermaDead(9999u));
    TestEqual(TEXT("loaded: death count == 2"), Loaded->GetDeathCount(), 2);
    TestEqual(TEXT("loaded: 2 death records"), Loaded->GetDeathRecords().Num(), 2);

    // The persisted monotonic spawn serial MUST survive load (a reset to 0 would regenerate NameHashes already in the
    // dead set and wrongly perma-dead fresh NPCs — R18-M7). After 3 allocations NextSpawnSerial==3, so the loaded
    // registry's NEXT allocation must be Serial2+1, NOT 0.
    TestEqual(TEXT("loaded: spawn serial persisted (not reset to 0)"), Loaded->AllocateSpawnSerial(), Serial2 + 1);

    // Field round-trip: locate the 1001 record and verify its non-tag fields survived intact.
    const TArray<FMythicPersistentDeathRecord> &Records = Loaded->GetDeathRecords();
    const FMythicPersistentDeathRecord *Rec1001 = Records.FindByPredicate(
        [](const FMythicPersistentDeathRecord &R) { return R.NameHash == 1001u; });
    TestNotNull(TEXT("loaded: 1001 record exists"), Rec1001);
    if (Rec1001) {
        TestEqual(TEXT("loaded: 1001 cell.X"), Rec1001->DeathCell.X, 3);
        TestEqual(TEXT("loaded: 1001 cell.Y"), Rec1001->DeathCell.Y, 4);
        TestEqual(TEXT("loaded: 1001 faction index"), static_cast<int32>(Rec1001->Faction.Index), 0);
        TestEqual(TEXT("loaded: 1001 death time"), Rec1001->DeathTime, 100.0);
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// CausalFabric — event-log save/load round-trip + corrupted-stream crash guard
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldCausalFabricSerializeTest,
    "Mythic.LivingWorld.Phase1.CausalFabric.SerializeRoundTrip",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldCausalFabricSerializeTest::RunTest(const FString &Parameters) {
    auto *Fabric = NewObject<UMythicCausalFabric>();
    Fabric->Initialize(16);

    FMythicWorldEvent E1;
    E1.Cell = FMythicCellCoord(3, 4);
    E1.PrimaryFaction = LivingWorldTestHelpers::MakeFactionId(2);
    E1.Significance = 0.7f;
    E1.CategoryFlags = EMythicEventCategory::Combat | EMythicEventCategory::Death;
    E1.WorldTime = 123.0;
    E1.MoralVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] = 0.9f;
    const uint32 Id1 = Fabric->AppendEvent(E1);

    FMythicWorldEvent E2;
    E2.Cell = FMythicCellCoord(7, 8);
    E2.PrimaryFaction = LivingWorldTestHelpers::MakeFactionId(1);
    E2.Significance = 0.4f;
    E2.CategoryFlags = EMythicEventCategory::Crime;
    E2.WorldTime = 200.0;
    const uint32 Id2 = Fabric->AppendEvent(E2);

    Fabric->CommitWrites();

    // Save → load into a FRESH fabric.
    TArray<uint8> Bytes;
    {
        FMemoryWriter Writer(Bytes);
        Fabric->Serialize(Writer);
    }
    auto *Loaded = NewObject<UMythicCausalFabric>();
    {
        FMemoryReader Reader(Bytes);
        Loaded->Serialize(Reader);
        TestFalse(TEXT("clean load: no archive error"), Reader.IsError());
    }

    TestEqual(TEXT("loaded capacity preserved"), Loaded->GetCapacity(), 16);

    // Events survive the round-trip (serialized fields), and are resolvable by id post-load (CommitWrites ran on load).
    const FMythicWorldEvent *G1 = Loaded->GetEvent(Id1);
    TestNotNull(TEXT("loaded: event 1 found by id"), G1);
    if (G1) {
        TestEqual(TEXT("e1 cell.X"), G1->Cell.X, 3);
        TestEqual(TEXT("e1 cell.Y"), G1->Cell.Y, 4);
        TestEqual(TEXT("e1 faction"), static_cast<int32>(G1->PrimaryFaction.Index), 2);
        TestNearlyEqual(TEXT("e1 significance"), G1->Significance, 0.7f, 0.001f);
        TestEqual(TEXT("e1 category flags"), static_cast<int32>(G1->CategoryFlags),
                  static_cast<int32>(EMythicEventCategory::Combat | EMythicEventCategory::Death));
        TestNearlyEqual(TEXT("e1 moral violence axis"),
                        G1->MoralVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)], 0.9f, 0.001f);
    }
    const FMythicWorldEvent *G2 = Loaded->GetEvent(Id2);
    TestNotNull(TEXT("loaded: event 2 found by id"), G2);
    if (G2) {
        TestEqual(TEXT("e2 cell.X"), G2->Cell.X, 7);
        TestEqual(TEXT("e2 category flags"), static_cast<int32>(G2->CategoryFlags),
                  static_cast<int32>(EMythicEventCategory::Crime));
    }

    // The spatial index is rebuilt on load → cell queries work against the restored buffer.
    TArray<FMythicWorldEvent> CellEvents;
    Loaded->QueryEventsByCell(FMythicCellCoord(3, 4), 0.0, 1000.0, 16, CellEvents);
    TestEqual(TEXT("loaded: one event in cell (3,4)"), CellEvents.Num(), 1);

    // Corrupted-stream guard: a garbage Capacity must SetError, NOT SetNum(garbage) → OOM/crash.
    TArray<uint8> BadBytes;
    {
        FMemoryWriter Writer(BadBytes);
        int32 Ver = 1;
        int32 GarbageCapacity = 999999999;
        Writer << Ver;
        Writer << GarbageCapacity;
    }
    auto *BadFabric = NewObject<UMythicCausalFabric>();
    {
        FMemoryReader Reader(BadBytes);
        BadFabric->Serialize(Reader);
        TestTrue(TEXT("corrupted capacity → archive error (no OOM/crash)"), Reader.IsError());
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// TerritoryGrid — influence-grid save/load round-trip + corrupted-dimensions crash guard
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldTerritoryGridSerializeTest,
    "Mythic.LivingWorld.Phase1.TerritoryGrid.SerializeRoundTrip",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldTerritoryGridSerializeTest::RunTest(const FString &Parameters) {
    auto *Grid = NewObject<UMythicTerritoryGrid>();
    Grid->Initialize(LivingWorldTestHelpers::CreateGridSettings(16, 16));
    Grid->SetCellInfluence(FMythicCellCoord(3, 3), LivingWorldTestHelpers::MakeFactionId(0), 0.9f);
    Grid->SetCellInfluence(FMythicCellCoord(10, 5), LivingWorldTestHelpers::MakeFactionId(1), 0.6f);
    Grid->CommitWrites();

    // Save → load into a FRESH grid. The fresh grid is Initialize'd first so its per-faction cell lists are sized
    // before the load's CommitWrites rebuilds them (the load restores cell state + dimensions over the top).
    TArray<uint8> Bytes;
    {
        FMemoryWriter Writer(Bytes);
        Grid->Serialize(Writer);
    }
    auto *Loaded = NewObject<UMythicTerritoryGrid>();
    Loaded->Initialize(LivingWorldTestHelpers::CreateGridSettings(16, 16));
    {
        FMemoryReader Reader(Bytes);
        Loaded->Serialize(Reader);
        TestFalse(TEXT("clean load: no archive error"), Reader.IsError());
    }

    TestEqual(TEXT("loaded width"), Loaded->GetWidth(), 16);
    TestEqual(TEXT("loaded height"), Loaded->GetHeight(), 16);

    const FMythicTerritoryCell C33 = Loaded->GetCell(FMythicCellCoord(3, 3));
    TestEqual(TEXT("(3,3) dominant faction"), static_cast<int32>(C33.DominantFaction.Index), 0);
    TestNearlyEqual(TEXT("(3,3) influence"), C33.Influence, 0.9f, 0.001f);

    const FMythicTerritoryCell C105 = Loaded->GetCell(FMythicCellCoord(10, 5));
    TestEqual(TEXT("(10,5) dominant faction"), static_cast<int32>(C105.DominantFaction.Index), 1);

    const FMythicTerritoryCell CEmpty = Loaded->GetCell(FMythicCellCoord(0, 0));
    TestFalse(TEXT("(0,0) uncontrolled"), CEmpty.DominantFaction.IsValid());

    // Corrupted-stream guard: patch the serialized Width (offset 4, just after the int32 Version) to a huge value →
    // SetError, NOT an int32 overflow / SetNum(garbage) → OOM/crash.
    TArray<uint8> BadBytes = Bytes; // copy a valid save
    if (BadBytes.Num() >= 8) {
        BadBytes[4] = 0xFF; // Width = 0x7FFFFFFF (little-endian)
        BadBytes[5] = 0xFF;
        BadBytes[6] = 0xFF;
        BadBytes[7] = 0x7F;
    }
    auto *BadGrid = NewObject<UMythicTerritoryGrid>();
    BadGrid->Initialize(LivingWorldTestHelpers::CreateGridSettings(16, 16));
    {
        FMemoryReader Reader(BadBytes);
        BadGrid->Serialize(Reader);
        TestTrue(TEXT("corrupted width → archive error (no overflow/OOM)"), Reader.IsError());
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Serialize corrupted-stream guards — FactionDatabase + SettlementRegistry
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldFactionDatabaseSerializeGuardTest,
    "Mythic.LivingWorld.Phase1.FactionDatabase.SerializeGuard",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldFactionDatabaseSerializeGuardTest::RunTest(const FString &Parameters) {
    // A corrupted save with a garbage MaxFactions must SetError, NOT SetNum(MaxFactions*MaxFactions) → int32 overflow/OOM.
    TArray<uint8> BadBytes;
    {
        FMemoryWriter Writer(BadBytes);
        int32 Version = 3;
        int32 GarbageMaxFactions = 100000; // 100000^2 overflows int32 and is a ~1e10-element allocation
        int32 RegCount = 0;
        Writer << Version;
        Writer << GarbageMaxFactions;
        Writer << RegCount;
    }
    auto *DB = NewObject<UMythicFactionDatabase>();
    {
        FMemoryReader Reader(BadBytes);
        DB->Serialize(Reader);
        TestTrue(TEXT("corrupted MaxFactions → archive error (no overflow/OOM)"), Reader.IsError());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSettlementRegistrySerializeGuardTest,
    "Mythic.LivingWorld.Phase3.SettlementRegistry.SerializeGuard",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSettlementRegistrySerializeGuardTest::RunTest(const FString &Parameters) {
    // A valid empty-override stream loads cleanly (the guard must not reject valid saves).
    {
        TArray<uint8> Bytes;
        {
            FMemoryWriter Writer(Bytes);
            int32 Version = 2;
            int32 Count = 0;
            Writer << Version;
            Writer << Count;
        }
        auto *Reg = NewObject<UMythicSettlementRegistry>();
        FMemoryReader Reader(Bytes);
        Reg->Serialize(Reader);
        TestFalse(TEXT("valid empty override stream loads without error"), Reader.IsError());
    }

    // A garbage Count must SetError, NOT spin a multi-billion-iteration loop (hang/OOM).
    {
        TArray<uint8> BadBytes;
        {
            FMemoryWriter Writer(BadBytes);
            int32 Version = 2;
            int32 GarbageCount = 2000000000; // 2e9
            Writer << Version;
            Writer << GarbageCount;
        }
        auto *Reg = NewObject<UMythicSettlementRegistry>();
        FMemoryReader Reader(BadBytes);
        Reg->Serialize(Reader);
        TestTrue(TEXT("corrupted Count → archive error (no hang/OOM)"), Reader.IsError());
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════
// Belief propagation hop cap — UMythicPartySubsystem::ShouldShareBelief
// (pure predicate; the gossip loop's single eligibility gate)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldBeliefHopCapTest,
    "Mythic.LivingWorld.Party.BeliefHopCap",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldBeliefHopCapTest::RunTest(const FString &Parameters) {
    const int32 MaxHops = 3;

    // Confidence gate (preserves the prior 0.3 prune): weak beliefs are never shared, regardless of hop count.
    {
        FMythicBelief Weak;
        Weak.Confidence = 0.29f;
        Weak.PropagationHops = 0;
        TestFalse(TEXT("confidence below 0.3 → not shared"),
                  UMythicPartySubsystem::ShouldShareBelief(Weak, MaxHops));
    }

    // A strong, freshly-witnessed belief (hop 0) is shared.
    {
        FMythicBelief Fresh;
        Fresh.Confidence = 0.9f;
        Fresh.PropagationHops = 0;
        TestTrue(TEXT("strong hop-0 belief → shared"),
                 UMythicPartySubsystem::ShouldShareBelief(Fresh, MaxHops));
    }

    // Hop cap: a strong belief still propagates while below MaxHops, and stops once it reaches/exceeds it — this is
    // what makes the designer's MaxBeliefPropagationHops actually bound the spread (the bug being fixed: it was dead).
    {
        FMythicBelief BelowCap;
        BelowCap.Confidence = 0.9f;
        BelowCap.PropagationHops = 2; // < 3 → still shared (becomes hop-3 on the target, which then stops)
        TestTrue(TEXT("hops below MaxHops → shared"),
                 UMythicPartySubsystem::ShouldShareBelief(BelowCap, MaxHops));

        FMythicBelief AtCap;
        AtCap.Confidence = 0.9f;
        AtCap.PropagationHops = 3; // == MaxHops → cap reached, not shared
        TestFalse(TEXT("hops == MaxHops → not shared (cap reached)"),
                  UMythicPartySubsystem::ShouldShareBelief(AtCap, MaxHops));

        FMythicBelief PastCap;
        PastCap.Confidence = 0.9f;
        PastCap.PropagationHops = 5; // > MaxHops → not shared
        TestFalse(TEXT("hops beyond MaxHops → not shared"),
                  UMythicPartySubsystem::ShouldShareBelief(PastCap, MaxHops));
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Belief save/load field round-trip — UMythicPartySubsystem::SerializeBelief
// (the v3 fix: companion beliefs now persist full state incl. PropagationHops + Cell)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldBeliefSerializeTest,
    "Mythic.LivingWorld.Party.BeliefSerializeRoundTrip",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldBeliefSerializeTest::RunTest(const FString &Parameters) {
    FMythicBelief Original;
    Original.Confidence = 0.77f;
    Original.FormationTime = 123.5;
    Original.Cell = FMythicCellCoord(5, 7);
    Original.InvolvedFaction.Index = 3;
    Original.LastDecayTime = 100.0;
    Original.PropagationHops = 2;
    Original.SourceEventId = 42;

    // v3 round-trip: the full semantic state must survive (this is the fix — beliefs used to drop everything but
    // EventTag/Confidence/FormationTime, so the now-live hop cap and threat localization were lost on load).
    {
        TArray<uint8> Bytes;
        {
            FMemoryWriter Writer(Bytes);
            UMythicPartySubsystem::SerializeBelief(Writer, Original, 3);
        }
        FMythicBelief Loaded;
        FMemoryReader Reader(Bytes);
        UMythicPartySubsystem::SerializeBelief(Reader, Loaded, 3);

        TestFalse(TEXT("v3 stream fully consumed (no over-read)"), Reader.IsError());
        TestEqual(TEXT("Confidence round-trips"), Loaded.Confidence, 0.77f);
        TestEqual(TEXT("Cell.X round-trips"), Loaded.Cell.X, 5);
        TestEqual(TEXT("Cell.Y round-trips"), Loaded.Cell.Y, 7);
        TestEqual(TEXT("InvolvedFaction round-trips"), Loaded.InvolvedFaction.Index, 3);
        TestEqual(TEXT("PropagationHops round-trips (hop cap survives save/load)"), (int32)Loaded.PropagationHops, 2);
        TestEqual(TEXT("SourceEventId round-trips"), (int32)Loaded.SourceEventId, 42);
        TestTrue(TEXT("LastDecayTime round-trips"), FMath::IsNearlyEqual(Loaded.LastDecayTime, 100.0));
        TestTrue(TEXT("FormationTime round-trips"), FMath::IsNearlyEqual(Loaded.FormationTime, 123.5));
    }

    // Legacy (v2) layout: only the original 3 fields are written/read; the v3 fields stay at their defaults and there
    // is no over-read. Confirms the version gate keeps old saves byte-aligned.
    {
        TArray<uint8> LegacyBytes;
        {
            FMemoryWriter Writer(LegacyBytes);
            UMythicPartySubsystem::SerializeBelief(Writer, Original, 2);
        }
        FMythicBelief LegacyLoaded;
        FMemoryReader Reader(LegacyBytes);
        UMythicPartySubsystem::SerializeBelief(Reader, LegacyLoaded, 2);

        TestFalse(TEXT("legacy stream fully consumed (no over-read)"), Reader.IsError());
        TestEqual(TEXT("legacy Confidence round-trips"), LegacyLoaded.Confidence, 0.77f);
        TestEqual(TEXT("legacy PropagationHops stays default 0 (absent from v2 stream)"),
                  (int32)LegacyLoaded.PropagationHops, 0);
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Routine-desire utility cap — UMythicCognitiveBrainComponent::ScoreRoutineDesire
// (Rally/Report were unbounded → could outscore the daily schedule)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldRoutineDesireCapTest,
    "Mythic.LivingWorld.Cognition.RoutineDesireCap",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldRoutineDesireCapTest::RunTest(const FString &Parameters) {
    const float Ceiling = UMythicCognitiveBrainComponent::RoutineDesireCeiling;

    // High weight × pressure (the unbounded-Rally/Report bug) must cap at the ceiling, not outscore the schedule.
    TestEqual(TEXT("high-injustice Rally caps at ceiling"),
              UMythicCognitiveBrainComponent::ScoreRoutineDesire(0.8f, 5.0f, 1.5f), Ceiling);
    TestEqual(TEXT("high-injustice Report caps at ceiling"),
              UMythicCognitiveBrainComponent::ScoreRoutineDesire(0.8f, 5.0f, 1.2f), Ceiling);

    // Low routine values pass through unclamped (the desire still varies below the ceiling).
    const float Low = UMythicCognitiveBrainComponent::ScoreRoutineDesire(0.2f, 0.5f, 1.2f); // 0.12
    TestTrue(TEXT("low routine value passes through below the ceiling"),
             Low < Ceiling && FMath::IsNearlyEqual(Low, 0.12f));

    // A value exactly at the ceiling is preserved (boundary).
    TestEqual(TEXT("value at ceiling preserved"),
              UMythicCognitiveBrainComponent::ScoreRoutineDesire(Ceiling, 1.0f, 1.0f), Ceiling);

    // Defensive: a negative product can never yield a negative desire utility.
    TestEqual(TEXT("negative product floored at 0"),
              UMythicCognitiveBrainComponent::ScoreRoutineDesire(-1.0f, 1.0f, 1.0f), 0.0f);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Belief confidence decay — UMythicCognitiveBrainComponent::DecayBeliefConfidence
// (telescoping property: N steps == 1 step over the summed time)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldBeliefDecayTest,
    "Mythic.LivingWorld.Cognition.BeliefDecayTelescoping",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldBeliefDecayTest::RunTest(const FString &Parameters) {
    const float Rate = 0.005f;

    // Telescoping: decaying in two steps (120s + 180s) equals one step over the summed delta (300s). This is the
    // property whose ABSENCE (decaying by the full age each tick) made beliefs evaporate in tens of seconds — guard it.
    {
        const float OneStep = UMythicCognitiveBrainComponent::DecayBeliefConfidence(1.0f, Rate, 300.0);
        const float StepA = UMythicCognitiveBrainComponent::DecayBeliefConfidence(1.0f, Rate, 120.0);
        const float TwoStep = UMythicCognitiveBrainComponent::DecayBeliefConfidence(StepA, Rate, 180.0);
        TestTrue(TEXT("decay telescopes: 120s + 180s == 300s"), FMath::IsNearlyEqual(OneStep, TwoStep, 1e-4f));
    }

    // Negative delta (clock skew / out-of-order LastDecayTime) must NOT amplify confidence — clamped to a no-op.
    {
        const float Result = UMythicCognitiveBrainComponent::DecayBeliefConfidence(0.8f, Rate, -100.0);
        TestEqual(TEXT("negative delta is a no-op (no amplification)"), Result, 0.8f);
    }

    // Zero rate = no decay regardless of elapsed time.
    {
        const float Result = UMythicCognitiveBrainComponent::DecayBeliefConfidence(0.8f, 0.0f, 9999.0);
        TestEqual(TEXT("zero rate preserves confidence"), Result, 0.8f);
    }

    // Positive rate over positive time strictly reduces confidence (but stays positive).
    {
        const float Result = UMythicCognitiveBrainComponent::DecayBeliefConfidence(1.0f, Rate, 100.0);
        TestTrue(TEXT("positive decay reduces confidence below the original"), Result < 1.0f && Result > 0.0f);
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Moral trajectory arc — FMythicMoralSignature::TrajectoryAngle / ComputeMoralTrajectoryAngle
// (was declared + serialized but NEVER computed — always 0)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldMoralTrajectoryTest,
    "Mythic.LivingWorld.Morality.TrajectoryAngle",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldMoralTrajectoryTest::RunTest(const FString &Parameters) {
    // Pure angle helper: aligned → 0, opposite → PI, orthogonal → PI/2, degenerate → 0.
    {
        float A[MoralAxisCount] = {};
        float B[MoralAxisCount] = {};
        A[0] = 1.0f;
        B[0] = 1.0f; // aligned
        TestTrue(TEXT("aligned vectors → angle ~0"),
                 FMath::IsNearlyZero(FMythicMoralSignature::ComputeMoralTrajectoryAngle(A, B), 1e-3f));
        B[0] = -1.0f; // opposite
        TestTrue(TEXT("opposite vectors → angle ~PI"),
                 FMath::IsNearlyEqual(FMythicMoralSignature::ComputeMoralTrajectoryAngle(A, B), PI, 1e-3f));
        if (MoralAxisCount >= 2) {
            float C[MoralAxisCount] = {};
            float D[MoralAxisCount] = {};
            C[0] = 1.0f;
            D[1] = 1.0f; // orthogonal
            TestTrue(TEXT("orthogonal vectors → angle ~PI/2"),
                     FMath::IsNearlyEqual(FMythicMoralSignature::ComputeMoralTrajectoryAngle(C, D), HALF_PI, 1e-3f));
        }
        float Z[MoralAxisCount] = {};
        TestEqual(TEXT("degenerate (zero) vector → angle 0"),
                  FMythicMoralSignature::ComputeMoralTrajectoryAngle(A, Z), 0.0f);
    }

    // Integration: consistent behavior keeps the trajectory ~0; a sharp reversal opens it up (a corruption arc) — this
    // is the field that used to be hardcoded 0 forever.
    {
        FMythicMoralSignature Sig;
        FMythicMoralAction Good;
        Good.AxisValues[0] = 1.0f;
        for (int32 k = 0; k < 12; ++k) {
            Sig.AccumulateAction(Good);
        }
        const float ConsistentAngle = Sig.TrajectoryAngle;
        TestTrue(TEXT("consistent behavior → small trajectory angle"), ConsistentAngle < 0.2f);

        FMythicMoralAction Bad;
        Bad.AxisValues[0] = -1.0f; // reversal
        for (int32 k = 0; k < 6; ++k) {
            Sig.AccumulateAction(Bad);
        }
        const float ReversedAngle = Sig.TrajectoryAngle;
        TestTrue(TEXT("a behavioral reversal widens the trajectory angle"),
                 ReversedAngle > ConsistentAngle + 0.1f);
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Intention-commit hysteresis — UMythicCognitiveBrainComponent::ShouldOverrideIntention
// (DesireHysteresis setting was dead — CommitIntention hardcoded 0.2)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldIntentionHysteresisTest,
    "Mythic.LivingWorld.Cognition.IntentionHysteresis",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldIntentionHysteresisTest::RunTest(const FString &Parameters) {
    const float Hys = 0.2f;

    // A marginal improvement (below the margin) does NOT override — this is the anti-flicker the setting controls.
    TestFalse(TEXT("tiny improvement within margin → keep current"),
              UMythicCognitiveBrainComponent::ShouldOverrideIntention(0.55f, 0.50f, Hys));
    // An improvement at exactly the margin DOES override (>=).
    TestTrue(TEXT("improvement at exactly the margin → override"),
             UMythicCognitiveBrainComponent::ShouldOverrideIntention(0.70f, 0.50f, Hys));
    // A large improvement overrides.
    TestTrue(TEXT("large improvement → override"),
             UMythicCognitiveBrainComponent::ShouldOverrideIntention(0.95f, 0.50f, Hys));
    // A lower-utility desire never overrides.
    TestFalse(TEXT("lower utility → keep current"),
              UMythicCognitiveBrainComponent::ShouldOverrideIntention(0.40f, 0.50f, Hys));
    // Zero hysteresis: an equal-or-better utility overrides (no stickiness).
    TestTrue(TEXT("zero hysteresis: equal utility overrides"),
             UMythicCognitiveBrainComponent::ShouldOverrideIntention(0.50f, 0.50f, 0.0f));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Resistance restoration — UMythicFactionDatabase::RestoreResistanceToFaction (REQ-FAC-004)
// (the Resistance status was terminal — nothing ever restored it to a full faction)
// ═══════════════════════════════════════════════════════════════

// ═══════════════════════════════════════════════════════════════
// Objective callout text selection — UObjectiveDefinition::GetCalloutText
// (CompletedText was declared "shown on completion" but never read)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldObjectiveCalloutTextTest,
    "Mythic.Objectives.ObjectiveDefinition.CalloutText",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldObjectiveCalloutTextTest::RunTest(const FString &Parameters) {
    auto *Def = NewObject<UObjectiveDefinition>();
    Def->DisplayText = FText::FromString(TEXT("Slay 5 wolves"));
    Def->CompletedText = FText::FromString(TEXT("The pack is broken"));

    // In-progress callout uses the progress line.
    TestEqual(TEXT("in-progress → DisplayText"),
              Def->GetCalloutText(false).ToString(), FString(TEXT("Slay 5 wolves")));
    // Completion callout uses the authored completion line (the bug: this was never shown).
    TestEqual(TEXT("completed → CompletedText"),
              Def->GetCalloutText(true).ToString(), FString(TEXT("The pack is broken")));

    // With no authored completion line, completion falls back to DisplayText (prior behaviour preserved).
    Def->CompletedText = FText::GetEmpty();
    TestEqual(TEXT("completed with empty CompletedText → DisplayText fallback"),
              Def->GetCalloutText(true).ToString(), FString(TEXT("Slay 5 wolves")));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Objective progression arithmetic — UObjectiveTracker::ComputeObjectiveProgress
// (advance +1 by occurrence OR by rounded event magnitude floored at 1; completion threshold; overshoot clamp)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicObjectiveProgressTest,
    "Mythic.Objectives.ObjectiveTracker.Progress",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicObjectiveProgressTest::RunTest(const FString &Parameters) {
    int32 NewCount = 0;
    bool bComplete = false;
    auto Step = [&](int32 Cur, bool bByMag, float Mag, int32 Req) {
        UObjectiveTracker::ComputeObjectiveProgress(Cur, bByMag, Mag, Req, NewCount, bComplete);
    };

    // Occurrence count (+1), still in progress.
    Step(0, false, 0.0f, 3);
    TestEqual(TEXT("occurrence: 0→1"), NewCount, 1);
    TestFalse(TEXT("occurrence: not complete at 1/3"), bComplete);

    // Occurrence count completing exactly.
    Step(2, false, 0.0f, 3);
    TestEqual(TEXT("occurrence: 2→3"), NewCount, 3);
    TestTrue(TEXT("occurrence: complete at 3/3"), bComplete);

    // Occurrence count IGNORES magnitude (always +1).
    Step(0, false, 99.0f, 3);
    TestEqual(TEXT("occurrence ignores magnitude → +1"), NewCount, 1);

    // Count-by-magnitude: rounded magnitude added.
    Step(0, true, 5.4f, 10);
    TestEqual(TEXT("magnitude 5.4 → round 5"), NewCount, 5);
    TestFalse(TEXT("magnitude: not complete at 5/10"), bComplete);

    // Magnitude floored at 1 (a tiny magnitude still advances by 1, never 0).
    Step(0, true, 0.2f, 3);
    TestEqual(TEXT("magnitude 0.2 floored to 1"), NewCount, 1);

    // Overshoot clamped to RequiredCount on completion (clean N/N display).
    Step(8, true, 5.0f, 10);
    TestEqual(TEXT("overshoot 13 clamped to 10"), NewCount, 10);
    TestTrue(TEXT("overshoot completes"), bComplete);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Utility reduction-fraction clamp membership — UMythicAttributeSet_Utility::IsReductionFractionAttribute
// (StaminaCostReduction + CooldownReduction clamp to [0,1]; CooldownReduction was previously unclamped)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicObjectiveOfferResultStatesTest,
    "Mythic.Objectives.ObjectiveTracker.OfferResultStates",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicObjectiveOfferResultStatesTest::RunTest(const FString &Parameters) {
    UObjectiveDefinition *Def = NewObject<UObjectiveDefinition>();
    Def->DisplayText = FText::FromString(TEXT("Gather wood"));

    TArray<FObjectiveProgress> Progress;
    FObjectiveProgress OutProgress;

    TestEqual(TEXT("null definition is invalid"),
              UObjectiveTracker::ResolveObjectiveOfferResult(Progress, nullptr, OutProgress),
              EObjectiveOfferResult::Invalid);

    TestEqual(TEXT("untracked definition can be assigned"),
              UObjectiveTracker::ResolveObjectiveOfferResult(Progress, Def, OutProgress),
              EObjectiveOfferResult::Assigned);

    FObjectiveProgress Active;
    Active.Definition = Def;
    Active.CurrentCount = 2;
    Active.bCompleted = false;
    Progress.Add(Active);

    TestEqual(TEXT("tracked incomplete definition is already active"),
              UObjectiveTracker::ResolveObjectiveOfferResult(Progress, Def, OutProgress),
              EObjectiveOfferResult::AlreadyActive);
    TestEqual(TEXT("already-active result returns current progress"), OutProgress.CurrentCount, 2);

    Progress[0].bCompleted = true;
    TestEqual(TEXT("tracked completed definition is already completed"),
              UObjectiveTracker::ResolveObjectiveOfferResult(Progress, Def, OutProgress),
              EObjectiveOfferResult::AlreadyCompleted);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicObjectiveNotificationMessageCategoriesTest,
    "Mythic.Objectives.ObjectiveNotification.MessageCategories",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicObjectiveNotificationMessageCategoriesTest::RunTest(const FString &Parameters) {
    const FText ObjectiveText = FText::FromString(TEXT("Gather wood"));

    TestEqual(TEXT("assignment message"),
              UObjectiveTracker::BuildObjectiveNotificationText(ObjectiveText, EObjectiveNotifyCategory::Assignment,
                                                                EObjectiveOfferResult::Assigned, 0, 5, true, false).ToString(),
              FString(TEXT("Objective Assigned: Gather wood")));

    TestEqual(TEXT("already active message"),
              UObjectiveTracker::BuildObjectiveNotificationText(ObjectiveText, EObjectiveNotifyCategory::Duplicate,
                                                                EObjectiveOfferResult::AlreadyActive, 2, 5, true, false).ToString(),
              FString(TEXT("Objective Already Active: Gather wood 2/5")));

    TestEqual(TEXT("already completed message"),
              UObjectiveTracker::BuildObjectiveNotificationText(ObjectiveText, EObjectiveNotifyCategory::Duplicate,
                                                                EObjectiveOfferResult::AlreadyCompleted, 5, 5, true, false).ToString(),
              FString(TEXT("Objective Already Completed: Gather wood")));

    TestEqual(TEXT("progress message"),
              UObjectiveTracker::BuildObjectiveNotificationText(ObjectiveText, EObjectiveNotifyCategory::Progress,
                                                                EObjectiveOfferResult::Assigned, 2, 5, true, false).ToString(),
              FString(TEXT("Gather wood 2/5")));

    TestEqual(TEXT("completion message"),
              UObjectiveTracker::BuildObjectiveNotificationText(ObjectiveText, EObjectiveNotifyCategory::Completed,
                                                                EObjectiveOfferResult::Assigned, 5, 5, true, false).ToString(),
              FString(TEXT("Objective Complete: Gather wood")));

    TestEqual(TEXT("reward success message"),
              UObjectiveTracker::BuildObjectiveNotificationText(ObjectiveText, EObjectiveNotifyCategory::RewardResult,
                                                                EObjectiveOfferResult::Assigned, 5, 5, true, false).ToString(),
              FString(TEXT("Rewards Granted")));

    TestEqual(TEXT("reward dropped nearby message"),
              UObjectiveTracker::BuildObjectiveNotificationText(ObjectiveText, EObjectiveNotifyCategory::RewardResult,
                                                                EObjectiveOfferResult::Assigned, 5, 5, true, true).ToString(),
              FString(TEXT("Reward Dropped Nearby")));

    TestEqual(TEXT("reward failure message"),
              UObjectiveTracker::BuildObjectiveNotificationText(ObjectiveText, EObjectiveNotifyCategory::RewardResult,
                                                                EObjectiveOfferResult::Assigned, 5, 5, false, false).ToString(),
              FString(TEXT("Reward Delivery Failed")));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicUtilityReductionFractionClampTest,
    "Mythic.GAS.Utility.ReductionFractionClamp",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicUtilityReductionFractionClampTest::RunTest(const FString &Parameters) {
    using U = UMythicAttributeSet_Utility;

    // Reduction fractions → clamped [0,1].
    TestTrue(TEXT("StaminaCostReduction is a reduction fraction"), U::IsReductionFractionAttribute(U::GetStaminaCostReductionAttribute()));
    TestTrue(TEXT("CooldownReduction is a reduction fraction (the fix)"), U::IsReductionFractionAttribute(U::GetCooldownReductionAttribute()));

    // Everything else → NOT clamped to [0,1] (different ranges: stamina pools, regen rate, multiplier, dormant speed).
    TestFalse(TEXT("CurrentStamina is not a reduction fraction"), U::IsReductionFractionAttribute(U::GetCurrentStaminaAttribute()));
    TestFalse(TEXT("MaxStamina is not a reduction fraction"), U::IsReductionFractionAttribute(U::GetMaxStaminaAttribute()));
    TestFalse(TEXT("StaminaRegenRate is not a reduction fraction"), U::IsReductionFractionAttribute(U::GetStaminaRegenRateAttribute()));
    TestFalse(TEXT("Resolve is not a reduction fraction"), U::IsReductionFractionAttribute(U::GetResolveAttribute()));
    TestFalse(TEXT("ProficiencyXPBonus is not a reduction fraction (multiplier)"), U::IsReductionFractionAttribute(U::GetProficiencyXPBonusAttribute()));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Moral signature core — FMythicMoralSignature (Welford online mean/variance + kill-vector sign + severity tiers)
// Welford is a classic subtly-breakable algorithm; the kill vector's sign is comment-guarded as drift-prone.
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicMoralSignatureCoreTest,
    "Mythic.LivingWorld.Morality.SignatureCore",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicMoralSignatureCoreTest::RunTest(const FString &Parameters) {
    // Welford online mean/variance over the known sequence [1, 2, 3] on every axis.
    FMythicMoralSignature Sig;
    for (float V : {1.0f, 2.0f, 3.0f}) {
        FMythicMoralAction A;
        for (int32 i = 0; i < MoralAxisCount; ++i) {
            A.AxisValues[i] = V;
        }
        Sig.AccumulateAction(A);
    }
    TestEqual(TEXT("TotalActions == 3"), Sig.TotalActions, 3);
    TestTrue(TEXT("Welford mean of [1,2,3] == 2"), FMath::IsNearlyEqual(Sig.Axes[0].Mean, 2.0f, 1e-4f));
    // Population variance of [1,2,3] = ((1-2)^2+(2-2)^2+(3-2)^2)/3 = 2/3.
    TestTrue(TEXT("Welford variance == 2/3"), FMath::IsNearlyEqual(Sig.Axes[0].GetVariance(), 2.0f / 3.0f, 1e-4f));
    // ContradictionScore = mean of per-axis variances; all axes saw the same sequence → 2/3.
    TestTrue(TEXT("ContradictionScore == 2/3"), FMath::IsNearlyEqual(Sig.ContradictionScore, 2.0f / 3.0f, 1e-4f));
    // A fresh signature accumulating consistent behavior reads no trajectory arc.
    TestTrue(TEXT("consistent behavior → ~0 trajectory angle"), Sig.TrajectoryAngle < 1e-3f);

    // DominantAxis = the axis with the largest |mean|.
    FMythicMoralSignature Sig2;
    FMythicMoralAction Dom;
    Dom.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] = 0.9f;
    Sig2.AccumulateAction(Dom);
    TestEqual(TEXT("dominant axis = Violence"), static_cast<int32>(Sig2.DominantAxis), static_cast<int32>(EMythicMoralAxis::Violence));

    // Kill-vector sign convention (single-source; comment-guarded against drift): Violence POSITIVE, Mercy NEGATIVE.
    const FMythicMoralAction Kill = FMythicMoralSignature::MakeKillActionMoralVector();
    TestTrue(TEXT("kill vector: Violence > 0"), Kill.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] > 0.0f);
    TestTrue(TEXT("kill vector: Mercy < 0"), Kill.AxisValues[static_cast<int32>(EMythicMoralAxis::Mercy)] < 0.0f);

    // EvaluateActionSeverity: a kill judged by an anti-violence (pacifist) faction → opposed → high severity.
    FMythicIdeologyProfile Pacifist;
    Pacifist.GetAxisMutable(EMythicMoralAxis::Violence) = -1.0f; // glorifies pacifism
    const EMythicMoralSeverity Sev = FMythicMoralSignature::EvaluateActionSeverity(
        Kill, Pacifist, /*Disapprove*/ 0.2f, /*Condemn*/ 0.5f, /*Hostile*/ 0.8f);
    // DotProduct = 0.9 * -1.0 = -0.9 → Severity = +0.9 → >= Hostile(0.8).
    TestEqual(TEXT("kill vs pacifist → Hostile severity"), static_cast<int32>(Sev), static_cast<int32>(EMythicMoralSeverity::Hostile));
    // The same kill judged by a violence-glorifying faction → aligned → not condemned.
    FMythicIdeologyProfile Warlike;
    Warlike.GetAxisMutable(EMythicMoralAxis::Violence) = 1.0f;
    const EMythicMoralSeverity SevW = FMythicMoralSignature::EvaluateActionSeverity(
        Kill, Warlike, 0.2f, 0.5f, 0.8f);
    TestEqual(TEXT("kill vs warlike → Ignore"), static_cast<int32>(SevW), static_cast<int32>(EMythicMoralSeverity::Ignore));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Population spawn density — UMythicPopulationSpawnerProcessor::ComputeTargetDensity
// min(settlement, system) cap × clamp(pop/capacity) fill-ratio, ceil; 0 with no capacity. Governs world NPC density.
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPopulationDensityTest,
    "Mythic.LivingWorld.Population.TargetDensity",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPopulationDensityTest::RunTest(const FString &Parameters) {
    using P = UMythicPopulationSpawnerProcessor;
    // Full faction (pop == capacity) → full effective density.
    TestEqual(TEXT("full faction → min(10,5)=5 density"), P::ComputeTargetDensity(10, 5, 1000, 1000), 5);
    // Half-full faction → half density (ceil): 5 * 0.5 = 2.5 → 3.
    TestEqual(TEXT("half faction → ceil(2.5)=3"), P::ComputeTargetDensity(10, 5, 500, 1000), 3);
    // Settlement density is the tighter cap.
    TestEqual(TEXT("settlement cap dominates (3)"), P::ComputeTargetDensity(3, 5, 1000, 1000), 3);
    // System cap is the tighter cap.
    TestEqual(TEXT("system cap dominates (5)"), P::ComputeTargetDensity(100, 5, 1000, 1000), 5);
    // No capacity (territoryless faction) → 0, never a divide-by-zero.
    TestEqual(TEXT("zero capacity → 0"), P::ComputeTargetDensity(5, 5, 100, 0), 0);
    // Dead faction (0 population) → 0.
    TestEqual(TEXT("zero population → 0"), P::ComputeTargetDensity(5, 5, 0, 1000), 0);
    // A barely-alive faction still seeds at least 1 (ceil of a positive ratio).
    TestEqual(TEXT("tiny pop → ceil ≥ 1"), P::ComputeTargetDensity(5, 5, 1, 1000), 1);
    // Over-capacity population clamps the fill-ratio to 1 (no over-spawn).
    TestEqual(TEXT("over-capacity clamps to full density"), P::ComputeTargetDensity(5, 5, 5000, 1000), 5);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Creature territorial aggression — UMythicCreatureEcologyProcessor::ComputeTerritorialAggression
// Boost near den (clamped to 1), bare base away; recomputed from the authored base (idempotent — no ratchet).
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTerritorialAggressionTest,
    "Mythic.AI.TerritorialAggression",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTerritorialAggressionTest::RunTest(const FString &Parameters) {
    using E = UMythicCreatureEcologyProcessor;
    // Near den → base + boost.
    TestEqual(TEXT("near den: 0.5 + 0.3 = 0.8"), E::ComputeTerritorialAggression(0.5f, true, 0.3f), 0.8f);
    // Near den → clamped to 1.
    TestEqual(TEXT("near den clamps to 1.0"), E::ComputeTerritorialAggression(0.9f, true, 0.3f), 1.0f);
    // Away from den → bare authored base (relaxed).
    TestEqual(TEXT("away: bare base 0.5"), E::ComputeTerritorialAggression(0.5f, false, 0.3f), 0.5f);
    TestEqual(TEXT("away: bare base 0.9"), E::ComputeTerritorialAggression(0.9f, false, 0.3f), 0.9f);
    // THE FIX — IDEMPOTENT: calling repeatedly with the same authored base never accumulates (old code ratcheted).
    const float First = E::ComputeTerritorialAggression(0.5f, true, 0.3f);
    const float Second = E::ComputeTerritorialAggression(0.5f, true, 0.3f); // same base again — must be identical
    TestEqual(TEXT("idempotent near-den (no ratchet)"), Second, First);
    TestEqual(TEXT("idempotent value stays 0.8"), Second, 0.8f);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Interaction focus priority — UMythicInteractionComponent::SelectFocusedInteractable
// In-range → best forward-alignment (highest Dot); else closest out-of-range; in-range always wins.
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicInteractionFocusTest,
    "Mythic.Interaction.FocusPriority",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicInteractionFocusTest::RunTest(const FString &Parameters) {
    using C = FMythicInteractCandidate;
    using I = UMythicInteractionComponent;

    // No candidates → INDEX_NONE.
    TestEqual(TEXT("empty → none"), I::SelectFocusedInteractable({}), (int32)INDEX_NONE);

    // Among in-range, highest Dot (best forward-alignment) wins — regardless of distance.
    {
        TArray<C> Cands = {
            C{/*inRange*/ true, /*dot*/ 0.2f, /*dist*/ 50.0f},
            C{true, 0.9f, 180.0f}, // better aligned, even though farther
            C{true, 0.5f, 30.0f},
        };
        TestEqual(TEXT("in-range: highest dot wins"), I::SelectFocusedInteractable(Cands), 1);
    }
    // In-range always beats out-of-range, even a poorly-aligned in-range vs a close out-of-range.
    {
        TArray<C> Cands = {
            C{false, 1.0f, 250.0f}, // out of range, perfectly aligned, but origin beyond range
            C{true, -0.3f, 199.0f}, // in range, behind-ish — still wins (in-range priority)
        };
        TestEqual(TEXT("in-range beats out-of-range"), I::SelectFocusedInteractable(Cands), 1);
    }
    // Only out-of-range candidates → closest by origin distance.
    {
        TArray<C> Cands = {
            C{false, 0.9f, 400.0f},
            C{false, 0.1f, 220.0f}, // closest origin
            C{false, 0.8f, 300.0f},
        };
        TestEqual(TEXT("all out-of-range: closest wins"), I::SelectFocusedInteractable(Cands), 1);
    }
    // Forward-cone gate (MinDot): default -1 admits the full sphere (unchanged); a positive MinDot excludes
    // candidates the player isn't facing, across BOTH tiers (in-range priority AND out-of-range fallback).
    {
        TArray<C> Cands = {
            C{true, 0.8f, 100.0f},  // in front
            C{true, -0.5f, 40.0f},  // behind, even though closer
        };
        TestEqual(TEXT("cone -1 (full sphere): front wins"), I::SelectFocusedInteractable(Cands, -1.0f), 0);
        TestEqual(TEXT("cone 0 (front hemisphere): behind excluded, front wins"), I::SelectFocusedInteractable(Cands, 0.0f), 0);
    }
    {
        // Everything is behind the cone → nothing focusable.
        TArray<C> Cands = {C{true, -0.3f, 40.0f}, C{true, -0.8f, 30.0f}};
        TestEqual(TEXT("cone 0: all behind → none"), I::SelectFocusedInteractable(Cands, 0.0f), (int32)INDEX_NONE);
    }
    {
        // The cone gates the fallback too: an in-range-but-behind is excluded, an out-of-range-but-in-cone is picked.
        TArray<C> Cands = {
            C{true, -0.9f, 40.0f},   // in range but behind → excluded by the cone
            C{false, 0.5f, 300.0f},  // out of range but within the cone → eligible fallback
        };
        TestEqual(TEXT("cone 0: in-range-behind excluded, out-of-range-in-cone picked"),
                  I::SelectFocusedInteractable(Cands, 0.0f), 1);
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// AI patrol ring — AMythicAIController::GetPatrolCell
// Cardinal ring (E,N,W,S) around the home anchor, leg index wrapping; TickIdleBehavior bounds-checks + skips off-grid.
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPatrolRingTest,
    "Mythic.AI.PatrolRing",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPatrolRingTest::RunTest(const FString &Parameters) {
    const FMythicCellCoord A(10, 10);
    auto Leg = [&](int32 i) { return AMythicAIController::GetPatrolCell(A, i); };

    // Cardinal ring: E, N, W, S.
    TestEqual(TEXT("leg 0 = East (X+1)"), Leg(0).X, 11);  TestEqual(TEXT("leg 0 Y"), Leg(0).Y, 10);
    TestEqual(TEXT("leg 1 = North (Y+1)"), Leg(1).Y, 11); TestEqual(TEXT("leg 1 X"), Leg(1).X, 10);
    TestEqual(TEXT("leg 2 = West (X-1)"), Leg(2).X, 9);   TestEqual(TEXT("leg 2 Y"), Leg(2).Y, 10);
    TestEqual(TEXT("leg 3 = South (Y-1)"), Leg(3).Y, 9);  TestEqual(TEXT("leg 3 X"), Leg(3).X, 10);
    // Wrap: leg 4 == leg 0.
    TestEqual(TEXT("leg 4 wraps to East X"), Leg(4).X, 11); TestEqual(TEXT("leg 4 wraps to East Y"), Leg(4).Y, 10);
    // Defensive negative leg: -1 → leg 3 (South).
    TestEqual(TEXT("leg -1 → South X"), Leg(-1).X, 10); TestEqual(TEXT("leg -1 → South Y"), Leg(-1).Y, 9);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Ideology drift/bleed — FMythicWorldSimThread::DriftTowardClamped
// Exponential approach toward a target, clamped to the [-1,1] axis range (shared by event-drift + faction bleed).
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicIdeologyDriftTest,
    "Mythic.LivingWorld.Factions.IdeologyDrift",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicIdeologyDriftTest::RunTest(const FString &Parameters) {
    using W = FMythicWorldSimThread;
    // Halfway approach.
    TestEqual(TEXT("0 → 1 @ rate 0.5 = 0.5"), W::DriftTowardClamped(0.0f, 1.0f, 0.5f), 0.5f);
    // Rate 0 → no change.
    TestEqual(TEXT("rate 0 → unchanged"), W::DriftTowardClamped(0.3f, 1.0f, 0.0f), 0.3f);
    // Rate 1 → jumps to (in-range) target.
    TestEqual(TEXT("rate 1 → jumps to target"), W::DriftTowardClamped(0.3f, 0.9f, 1.0f), 0.9f);
    // Drift toward a negative target.
    TestEqual(TEXT("0.5 → -0.5 @ 0.5 = 0.0"), W::DriftTowardClamped(0.5f, -0.5f, 0.5f), 0.0f);
    // Already at target → unchanged.
    TestEqual(TEXT("at target → unchanged"), W::DriftTowardClamped(0.4f, 0.4f, 0.5f), 0.4f);
    // CLAMP: an out-of-range target (or rate) can never push the axis past [-1, 1].
    TestEqual(TEXT("clamp high: target 2.0 @ rate 1 → 1.0"), W::DriftTowardClamped(0.9f, 2.0f, 1.0f), 1.0f);
    TestEqual(TEXT("clamp low: target -2.0 @ rate 1 → -1.0"), W::DriftTowardClamped(-0.9f, -2.0f, 1.0f), -1.0f);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Faction schism trigger — FMythicWorldSimThread::ShouldFactionSchism
// (ideological divergence OR geographic fragmentation) AND population >= 2× MinSchismPopulation (both halves viable).
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFactionSchismTriggerTest,
    "Mythic.LivingWorld.Factions.SchismTrigger",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFactionSchismTriggerTest::RunTest(const FString &Parameters) {
    using W = FMythicWorldSimThread;
    // High ideological divergence + ample population → schism.
    TestTrue(TEXT("divergence > threshold, pop ok → schism"), W::ShouldFactionSchism(0.9f, 0.5f, false, 100, 30));
    // Geographic fragmentation alone (low divergence) + ample population → schism.
    TestTrue(TEXT("geographic fragmentation, pop ok → schism"), W::ShouldFactionSchism(0.1f, 0.5f, true, 100, 30));
    // Neither destabilizing signal → no schism regardless of population.
    TestFalse(TEXT("no signal → no schism"), W::ShouldFactionSchism(0.1f, 0.5f, false, 100, 30));
    // THE VIABILITY GATE: a destabilized but too-small faction must NOT schism (a half would be < MinSchismPopulation).
    TestFalse(TEXT("destabilized but pop < 2x → no stillborn splinter"), W::ShouldFactionSchism(0.9f, 0.5f, false, 50, 30));
    // Exactly 2× MinSchismPopulation → viable, schisms.
    TestTrue(TEXT("pop exactly 2x threshold → schism"), W::ShouldFactionSchism(0.9f, 0.5f, false, 60, 30));
    // Divergence exactly AT threshold → not destabilized (strict >), no geographic → no schism.
    TestFalse(TEXT("divergence == threshold (strict >) → no schism"), W::ShouldFactionSchism(0.5f, 0.5f, false, 100, 30));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Creature spawn-population cap — FMythicWorldSimThread::ComputeCappedSpawnPopulation
// Bounds the per-tick spawn growth at carrying capacity (cells*PopPerCell) without forcing decline below current.
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSpawnPopulationCapTest,
    "Mythic.LivingWorld.Economy.SpawnPopulationCap",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSpawnPopulationCapTest::RunTest(const FString &Parameters) {
    using W = FMythicWorldSimThread;
    // Capacity = cells*PopPerCell = 5*100 = 500; growth = cells*spawnRate = 5*2 = 10.
    // Under capacity → grows by the full step.
    TestEqual(TEXT("under capacity grows by step"), W::ComputeCappedSpawnPopulation(10, 5, 2, 100), 20);
    // Near capacity → clamps to capacity (no overshoot).
    TestEqual(TEXT("near capacity clamps to 500"), W::ComputeCappedSpawnPopulation(495, 5, 2, 100), 500);
    // At capacity → no growth.
    TestEqual(TEXT("at capacity stays"), W::ComputeCappedSpawnPopulation(500, 5, 2, 100), 500);
    // ABOVE capacity (legacy over-grown save, or shrunk territory) → no growth, but NOT forced to decline.
    TestEqual(TEXT("above capacity: no growth, no forced decline"), W::ComputeCappedSpawnPopulation(800, 5, 2, 100), 800);
    // Zero cells (lost all territory) → capacity 0, growth 0; population is LEFT ALONE (not zeroed → no wrong annihilation).
    TestEqual(TEXT("zero cells leaves population untouched"), W::ComputeCappedSpawnPopulation(50, 0, 2, 100), 50);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Regen step — UMythicLifeComponent::ComputeRegenTarget (shared by Health/Shield/Stamina regen)
// Cur + Rate*Delta clamped to Max; rate<=0 or already-full → unchanged (so caller's `result > Cur` skips a redundant set).
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRegenStepTest,
    "Mythic.GAS.Regen.Step",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRegenStepTest::RunTest(const FString &Parameters) {
    using L = UMythicLifeComponent;
    // Partial regen toward max.
    TestEqual(TEXT("50 + 10/s*1s → 60"), L::ComputeRegenTarget(50.0f, 100.0f, 10.0f, 1.0f), 60.0f);
    // Sub-interval delta scales.
    TestEqual(TEXT("50 + 10/s*0.5s → 55"), L::ComputeRegenTarget(50.0f, 100.0f, 10.0f, 0.5f), 55.0f);
    // Recharge from 0 (shield/stamina).
    TestEqual(TEXT("0 + 10 → 10"), L::ComputeRegenTarget(0.0f, 100.0f, 10.0f, 1.0f), 10.0f);
    // CLAMP: a big step never overshoots max.
    TestEqual(TEXT("95 + 10 clamps to 100 (no overshoot)"), L::ComputeRegenTarget(95.0f, 100.0f, 10.0f, 1.0f), 100.0f);
    // Already at max → unchanged (caller skips the set).
    TestEqual(TEXT("at max → unchanged"), L::ComputeRegenTarget(100.0f, 100.0f, 10.0f, 1.0f), 100.0f);
    // Above max (defensive) → unchanged, never pulled DOWN.
    TestEqual(TEXT("above max → unchanged (no down-pull)"), L::ComputeRegenTarget(110.0f, 100.0f, 10.0f, 1.0f), 110.0f);
    // Zero/negative rate → unchanged (a non-positive regen rate never DRAINS).
    TestEqual(TEXT("zero rate → unchanged"), L::ComputeRegenTarget(50.0f, 100.0f, 0.0f, 1.0f), 50.0f);
    TestEqual(TEXT("negative rate → unchanged (no drain)"), L::ComputeRegenTarget(50.0f, 100.0f, -5.0f, 1.0f), 50.0f);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Stagger trigger — UMythicLifeComponent::IsHeavyHit + IsStaggerImmune
// Heavy-hit threshold scales with MaxHealth; immunity window prevents stun-lock but never suppresses the FIRST hit.
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStaggerTriggerTest,
    "Mythic.GAS.Stagger.Trigger",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStaggerTriggerTest::RunTest(const FString &Parameters) {
    using L = UMythicLifeComponent;
    // Heavy-hit threshold = 15% of MaxHealth. Use values clearly above/below the round number to avoid float-boundary
    // ambiguity (0.15f * MaxHealth is NOT exactly the round value — 0.15f*1000 ≈ 150.000006).
    TestTrue(TEXT("200 dmg vs 1000 HP @ 0.15 → heavy"), L::IsHeavyHit(200.0f, 1000.0f, 0.15f));
    TestFalse(TEXT("100 dmg vs 1000 HP → chip, no stagger"), L::IsHeavyHit(100.0f, 1000.0f, 0.15f));
    // Boundary: a magnitude exactly equal to the COMPUTED threshold staggers (>=), tested with the same float math.
    TestTrue(TEXT("exactly the computed threshold → heavy (>=)"), L::IsHeavyHit(0.15f * 1000.0f, 1000.0f, 0.15f));
    // Scales with the entity: a small mob staggers at a proportionally small hit.
    TestTrue(TEXT("scales down: 20 dmg vs 100 HP → heavy"), L::IsHeavyHit(20.0f, 100.0f, 0.15f));
    TestFalse(TEXT("10 dmg vs 100 HP → chip"), L::IsHeavyHit(10.0f, 100.0f, 0.15f));
    TestFalse(TEXT("zero MaxHealth never staggers"), L::IsHeavyHit(500.0f, 0.0f, 0.15f));
    TestFalse(TEXT("negative MaxHealth never staggers"), L::IsHeavyHit(500.0f, -100.0f, 0.15f));

    // Immunity: a prior stagger within the window blocks re-stagger; outside it allows.
    TestTrue(TEXT("within window → immune"), L::IsStaggerImmune(/*Now*/ 5.5, /*Last*/ 5.0, /*Window*/ 1.5f));
    TestFalse(TEXT("exactly at window edge → not immune"), L::IsStaggerImmune(6.5, 5.0, 1.5f));
    TestFalse(TEXT("past window → not immune"), L::IsStaggerImmune(7.0, 5.0, 1.5f));
    // THE FIX: never-staggered sentinel (Last<=0) is NOT immune even when Now < Window (load-into-combat arena).
    TestFalse(TEXT("first hit early (Now=1.0 < Window, Last=0) → NOT immune"), L::IsStaggerImmune(1.0, 0.0, 1.5f));
    TestFalse(TEXT("never-staggered at Now=0.2 → NOT immune"), L::IsStaggerImmune(0.2, 0.0, 1.5f));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Stamina cost reduction — UMythicLifeComponent::EffectiveStaminaCost
// Single source for CanSpendStamina (affordability) + TrySpendStamina (deduction): RawCost*(1-Clamp(Reduction,0,1)).
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEffectiveStaminaCostTest,
    "Mythic.GAS.Stamina.EffectiveCost",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEffectiveStaminaCostTest::RunTest(const FString &Parameters) {
    using L = UMythicLifeComponent;
    // No reduction → full cost.
    TestEqual(TEXT("0% reduction → full cost"), L::EffectiveStaminaCost(10.0f, 0.0f), 10.0f);
    // Half reduction → half cost.
    TestEqual(TEXT("50% reduction → half cost"), L::EffectiveStaminaCost(10.0f, 0.5f), 5.0f);
    // Full reduction → free.
    TestEqual(TEXT("100% reduction → free"), L::EffectiveStaminaCost(10.0f, 1.0f), 0.0f);
    // Over-range reduction clamps to 1.0 → free, NOT negative (would otherwise REFUND stamina).
    TestEqual(TEXT(">100% reduction clamps to free (no negative cost)"), L::EffectiveStaminaCost(10.0f, 1.5f), 0.0f);
    // Negative reduction clamps to 0 → full cost, NOT amplified.
    TestEqual(TEXT("negative reduction clamps to full cost"), L::EffectiveStaminaCost(10.0f, -0.5f), 10.0f);
    // Zero raw cost stays zero regardless of reduction.
    TestEqual(TEXT("zero raw cost stays zero"), L::EffectiveStaminaCost(0.0f, 0.5f), 0.0f);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Resource respawn gate — UMythicResourceManagerComponent::ShouldRespawnDestructible
// Destroyed (hits<=0) + real respawn time (>0) + delay elapsed (now>=RespawnTime). Core of the harvest respawn loop.
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicResourceRespawnGateTest,
    "Mythic.Resources.RespawnGate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicResourceRespawnGateTest::RunTest(const FString &Parameters) {
    using R = UMythicResourceManagerComponent;
    // Destroyed + valid respawn time + delay elapsed → respawn.
    TestTrue(TEXT("destroyed, elapsed → respawn"), R::ShouldRespawnDestructible(0, 100.0f, 150.0f));
    // Exactly at the respawn time (boundary) → respawn.
    TestTrue(TEXT("destroyed, exactly at time → respawn"), R::ShouldRespawnDestructible(0, 100.0f, 100.0f));
    // Destroyed but delay NOT elapsed → wait.
    TestFalse(TEXT("destroyed, not elapsed → wait"), R::ShouldRespawnDestructible(0, 100.0f, 50.0f));
    // Not destroyed (still has hits) → never respawn (shouldn't be in the destroyed list anyway; defensive).
    TestFalse(TEXT("not destroyed → no respawn"), R::ShouldRespawnDestructible(5, 100.0f, 150.0f));
    // Uninitialized respawn time (0) → never respawn at world-time 0 (the >0 guard).
    TestFalse(TEXT("zero respawn time → no respawn"), R::ShouldRespawnDestructible(0, 0.0f, 150.0f));
    // Negative respawn time → never respawn.
    TestFalse(TEXT("negative respawn time → no respawn"), R::ShouldRespawnDestructible(0, -1.0f, 150.0f));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Significance LOD tier hysteresis — QualifiesForPromotion / QualifiesForDemotion
// The promote (>= Thr+H) / demote (<= Thr-H) gates form the dead-band that prevents tier oscillation.
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSignificanceHysteresisTest,
    "Mythic.LivingWorld.Significance.TierHysteresis",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSignificanceHysteresisTest::RunTest(const FString &Parameters) {
    using S = UMythicSignificanceProcessor;
    // Promotion gate: score must clear Threshold by the full Hysteresis margin (Thr=0.7, H=0.1 → gate at 0.8).
    TestTrue(TEXT("promote: 0.85 >= 0.8"), S::QualifiesForPromotion(0.85f, 0.7f, 0.1f));
    TestTrue(TEXT("promote: exact 0.80 boundary promotes"), S::QualifiesForPromotion(0.80f, 0.7f, 0.1f));
    TestFalse(TEXT("promote: 0.75 < 0.8 does not"), S::QualifiesForPromotion(0.75f, 0.7f, 0.1f));
    // Zero-hysteresis gate (Tier2 spawn threshold) promotes exactly at the threshold.
    TestTrue(TEXT("promote H=0: 0.80 >= 0.80"), S::QualifiesForPromotion(0.80f, 0.80f, 0.0f));
    TestFalse(TEXT("promote H=0: 0.79 < 0.80"), S::QualifiesForPromotion(0.79f, 0.80f, 0.0f));

    // Demotion gate: score must fall the full Hysteresis margin below Threshold (Thr=0.3, H=0.1 → gate at 0.2).
    TestTrue(TEXT("demote: 0.15 <= 0.2"), S::QualifiesForDemotion(0.15f, 0.3f, 0.1f));
    TestTrue(TEXT("demote: exact 0.20 boundary demotes"), S::QualifiesForDemotion(0.20f, 0.3f, 0.1f));
    TestFalse(TEXT("demote: 0.25 > 0.2 does not"), S::QualifiesForDemotion(0.25f, 0.3f, 0.1f));

    // THE ANTI-OSCILLATION CONTRACT: a score inside the dead-band (Thr_demote-H, Thr_promote+H) = (0.2, 0.8)
    // qualifies for NEITHER promotion nor demotion, so the entity holds its tier instead of flickering.
    const float Mid = 0.5f;
    TestFalse(TEXT("dead-band: no promote"), S::QualifiesForPromotion(Mid, 0.7f, 0.1f));
    TestFalse(TEXT("dead-band: no demote"), S::QualifiesForDemotion(Mid, 0.3f, 0.1f));
    // Just inside each edge of the band also holds.
    TestFalse(TEXT("dead-band edge 0.79: no promote"), S::QualifiesForPromotion(0.79f, 0.7f, 0.1f));
    TestFalse(TEXT("dead-band edge 0.21: no demote"), S::QualifiesForDemotion(0.21f, 0.3f, 0.1f));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Save/load inventory slot-restore mapping — FSerializedInventoryData::ComputeSlotRestoreMapping
// Index-stable match → definition fallback → no double-map → unmatched = INDEX_NONE (layout-change migration).
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSlotRestoreMappingTest,
    "Mythic.SaveLoad.Inventory.SlotRestoreMapping",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSlotRestoreMappingTest::RunTest(const FString &Parameters) {
    const FSoftObjectPath A(TEXT("/Game/Slots/Weapon.Weapon"));
    const FSoftObjectPath B(TEXT("/Game/Slots/Armor.Armor"));
    const FSoftObjectPath C(TEXT("/Game/Slots/Ring.Ring"));
    using F = FSerializedInventoryData;

    // Identical layout → identity mapping.
    {
        TArray<int32> M = F::ComputeSlotRestoreMapping({A, B, C}, {A, B, C});
        TestEqual(TEXT("identical: 0->0"), M[0], 0);
        TestEqual(TEXT("identical: 1->1"), M[1], 1);
        TestEqual(TEXT("identical: 2->2"), M[2], 2);
    }
    // Reordered same defs → fallback finds each by definition, no double-map.
    {
        TArray<int32> M = F::ComputeSlotRestoreMapping({A, B}, {B, A});
        TestEqual(TEXT("reordered: A->target1"), M[0], 1);
        TestEqual(TEXT("reordered: B->target0"), M[1], 0);
    }
    // A slot inserted at the front (layout change) → index-stable fails for shifted slots, fallback recovers them.
    {
        TArray<int32> M = F::ComputeSlotRestoreMapping({A, B, C}, {B, A, C});
        TestEqual(TEXT("insert: A->1 (fallback)"), M[0], 1);
        TestEqual(TEXT("insert: B->0 (fallback)"), M[1], 0);
        TestEqual(TEXT("insert: C->2 (index-stable)"), M[2], 2);
    }
    // Save has MORE slots than the target layout (slots removed) → extras map to INDEX_NONE, never OOB.
    {
        TArray<int32> M = F::ComputeSlotRestoreMapping({A, B, C, A}, {A, B});
        TestEqual(TEXT("more-saved: A->0"), M[0], 0);
        TestEqual(TEXT("more-saved: B->1"), M[1], 1);
        TestEqual(TEXT("more-saved: C unmatched"), M[2], (int32)INDEX_NONE);
        TestEqual(TEXT("more-saved: 2nd A unmatched (target claimed)"), M[3], (int32)INDEX_NONE);
    }
    // No double-map: two saved slots of the same def, one target of that def → first claims, second unmatched.
    {
        TArray<int32> M = F::ComputeSlotRestoreMapping({A, A}, {A});
        TestEqual(TEXT("dup: first A->0"), M[0], 0);
        TestEqual(TEXT("dup: second A unmatched"), M[1], (int32)INDEX_NONE);
    }
    // Empty target layout → everything unmatched, no crash.
    {
        TArray<int32> M = F::ComputeSlotRestoreMapping({A}, {});
        TestEqual(TEXT("empty-target: unmatched"), M[0], (int32)INDEX_NONE);
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Weather transition progress — AMythicEnvironmentController::ComputeWeatherTransitionProgress
// Clamped [0,1] (the consuming FMath::Lerp is UNCLAMPED) + div-by-zero guard for non-positive duration.
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeatherTransitionProgressTest,
    "Mythic.Environment.Weather.TransitionProgress",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWeatherTransitionProgressTest::RunTest(const FString &Parameters) {
    using E = AMythicEnvironmentController;

    // Instant transition → fully complete regardless of elapsed/duration.
    TestEqual(TEXT("instant = 1.0"), E::ComputeWeatherTransitionProgress(true, 0.0, 30.0f), 1.0f);
    // Midpoint.
    TestEqual(TEXT("half elapsed = 0.5"), E::ComputeWeatherTransitionProgress(false, 15.0, 30.0f), 0.5f);
    // Start.
    TestEqual(TEXT("zero elapsed = 0.0"), E::ComputeWeatherTransitionProgress(false, 0.0, 30.0f), 0.0f);
    // THE BUG: over-duration must CLAMP to 1.0, not overshoot (FMath::Lerp is unclamped).
    TestEqual(TEXT("over-duration clamps to 1.0 (no overshoot)"), E::ComputeWeatherTransitionProgress(false, 45.0, 30.0f), 1.0f);
    // Clock rewind (Time < TransitionStartedAt → negative elapsed) clamps to 0, not a negative undershoot.
    TestEqual(TEXT("negative elapsed clamps to 0.0"), E::ComputeWeatherTransitionProgress(false, -5.0, 30.0f), 0.0f);
    // Div-by-zero guard: zero / negative duration → 1.0 (no inf/NaN), not a crashy lerp alpha.
    TestEqual(TEXT("zero duration = 1.0 (no div-by-zero)"), E::ComputeWeatherTransitionProgress(false, 5.0, 0.0f), 1.0f);
    TestEqual(TEXT("negative duration = 1.0"), E::ComputeWeatherTransitionProgress(false, 5.0, -10.0f), 1.0f);
    // And the result is always finite in the guarded cases.
    TestTrue(TEXT("zero-duration result is finite"), FMath::IsFinite(E::ComputeWeatherTransitionProgress(false, 5.0, 0.0f)));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Weather transition VALIDITY gate — UWeatherType::CanTransitionTo
// The per-edge gate of the weather state machine (the BFS pathfinder relies on it): a post-weather lock
// (OnlyAllowTransitionToWeather) pins the only legal next state and takes precedence over a target's pre-weather
// lock (OnlyAllowTransitionFromWeather); with neither lock, any transition is allowed.
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeatherCanTransitionTest,
    "Mythic.Environment.Weather.CanTransition",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWeatherCanTransitionTest::RunTest(const FString &Parameters) {
    // CanTransitionTo only compares tag IDENTITY, so any two distinct registered tags stand in for weather states.
    const FGameplayTag TagA = FGameplayTag::RequestGameplayTag(FName("GAS.State.Dead"), /*ErrorIfNotFound*/ false);
    const FGameplayTag TagB = FGameplayTag::RequestGameplayTag(FName("GAS.State.Downed"), /*ErrorIfNotFound*/ false);
    TestTrue(TEXT("test tags resolve + differ (prerequisite)"), TagA.IsValid() && TagB.IsValid() && TagA != TagB);
    if (!(TagA.IsValid() && TagB.IsValid() && TagA != TagB)) {
        return false;
    }

    UWeatherType *A = NewObject<UWeatherType>();
    UWeatherType *B = NewObject<UWeatherType>();
    A->Tag = TagA;
    B->Tag = TagB;

    // 1. Neither lock → any transition allowed.
    TestTrue(TEXT("unrestricted → allowed"), A->CanTransitionTo(*B));

    // 2. Post-weather lock on A: only the matching target is legal.
    A->OnlyAllowTransitionToWeather = TagB;
    TestTrue(TEXT("post-lock: matching target allowed"), A->CanTransitionTo(*B));
    A->OnlyAllowTransitionToWeather = TagA; // B.Tag (TagB) != TagA
    TestFalse(TEXT("post-lock: non-matching target blocked"), A->CanTransitionTo(*B));
    A->OnlyAllowTransitionToWeather = FGameplayTag(); // clear

    // 3. Pre-weather lock on the target B: only the matching current is legal.
    B->OnlyAllowTransitionFromWeather = TagA; // A.Tag == TagA
    TestTrue(TEXT("pre-lock: matching current allowed"), A->CanTransitionTo(*B));
    B->OnlyAllowTransitionFromWeather = TagB; // A.Tag (TagA) != TagB
    TestFalse(TEXT("pre-lock: non-matching current blocked"), A->CanTransitionTo(*B));

    // 4. A's post-lock takes PRECEDENCE over B's pre-lock (checked first).
    A->OnlyAllowTransitionToWeather = TagB;     // would allow (B.Tag == TagB)
    B->OnlyAllowTransitionFromWeather = TagB;   // would block (A.Tag TagA != TagB)
    TestTrue(TEXT("post-lock precedence over pre-lock"), A->CanTransitionTo(*B));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Attack fragment activation plan — UAttackFragment::PlanAttackActivation
// Damage application and ability grant are INDEPENDENTLY idempotent. The key case: damage already applied but NO
// live ability (a failed/lost first grant) must STILL grant — the old single early-return on bIsApplied skipped it.
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAttackActivationPlanTest,
    "Mythic.Itemization.Attack.ActivationPlan",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAttackActivationPlanTest::RunTest(const FString &Parameters) {
    // Fresh activation: apply damage + grant ability.
    FAttackActivationPlan Plan = UAttackFragment::PlanAttackActivation(/*bDamageApplied*/ false, /*bAbilityHandleValid*/ false);
    TestTrue(TEXT("fresh: apply damage"), Plan.bApplyDamage);
    TestTrue(TEXT("fresh: grant ability"), Plan.bGrantAbility);

    // THE BUG CASE: damage already applied but ability not live → must still grant (old code skipped it).
    Plan = UAttackFragment::PlanAttackActivation(true, false);
    TestFalse(TEXT("damage applied: do NOT re-apply"), Plan.bApplyDamage);
    TestTrue(TEXT("damage applied but no ability: still GRANT"), Plan.bGrantAbility);

    // Both already done (steady equipped state): no-op, matches the old early-return.
    Plan = UAttackFragment::PlanAttackActivation(true, true);
    TestFalse(TEXT("both done: no apply"), Plan.bApplyDamage);
    TestFalse(TEXT("both done: no grant"), Plan.bGrantAbility);

    // Ability live but damage not applied: apply damage, do NOT double-grant.
    Plan = UAttackFragment::PlanAttackActivation(false, true);
    TestTrue(TEXT("ability live, damage missing: apply damage"), Plan.bApplyDamage);
    TestFalse(TEXT("ability live: do NOT double-grant"), Plan.bGrantAbility);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Time-of-day mapping — HourAsDayTime (drives GetDayTime + GetDayTimeTag, consumed by hazards + encounter gating)
// Locks the ACTUAL boundaries (Morning 7-11, Afternoon 12-16, Evening 17-19, Night 20-6) so the EnvironmentTags
// annotations stay accurate and any future shift (e.g. aligning Morning to the 06:00 schedule day-start) is deliberate.
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHourAsDayTimeTest,
    "Mythic.Environment.Time.HourAsDayTime",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHourAsDayTimeTest::RunTest(const FString &Parameters) {
    // Night runs 20:00 through 06:59 (wraps midnight) — the pre-dawn hours and 6am are Night, NOT Morning.
    TestEqual(TEXT("00:00 = Night"), HourAsDayTime(0), Night);
    TestEqual(TEXT("06:00 = Night (Morning starts at 07:00)"), HourAsDayTime(6), Night);
    // Morning 07:00-11:59.
    TestEqual(TEXT("07:00 = Morning (lower boundary)"), HourAsDayTime(7), Morning);
    TestEqual(TEXT("11:00 = Morning (upper boundary)"), HourAsDayTime(11), Morning);
    // Afternoon 12:00-16:59.
    TestEqual(TEXT("12:00 = Afternoon (lower boundary)"), HourAsDayTime(12), Afternoon);
    TestEqual(TEXT("16:00 = Afternoon (upper boundary)"), HourAsDayTime(16), Afternoon);
    // Evening 17:00-19:59.
    TestEqual(TEXT("17:00 = Evening (lower boundary)"), HourAsDayTime(17), Evening);
    TestEqual(TEXT("19:00 = Evening (upper boundary)"), HourAsDayTime(19), Evening);
    // Night 20:00 onward.
    TestEqual(TEXT("20:00 = Night (lower boundary)"), HourAsDayTime(20), Night);
    TestEqual(TEXT("23:00 = Night"), HourAsDayTime(23), Night);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Affix modifier reversal — UAffixesFragment::ComputeReversedModValue
// (Additive→negate, Mult/Div→reciprocal, zero mult/div + Override → not reversible)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixReversalTest,
    "Mythic.Itemization.Affixes.ModReversal",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixReversalTest::RunTest(const FString &Parameters) {
    using A = UAffixesFragment;
    float Out = 0.0f;

    // Additive → negate (apply +50 reversed by -50).
    TestTrue(TEXT("additive reversible"), A::ComputeReversedModValue(EGameplayModOp::Additive, 50.0f, Out));
    TestEqual(TEXT("additive reversal = -value"), Out, -50.0f);
    // Multiplicitive → reciprocal (×2 reversed by ×0.5).
    TestTrue(TEXT("multiplicitive reversible"), A::ComputeReversedModValue(EGameplayModOp::Multiplicitive, 2.0f, Out));
    TestEqual(TEXT("mult reversal = 1/value"), Out, 0.5f);
    // Division → reciprocal (÷4 reversed by ÷0.25 == ×4).
    TestTrue(TEXT("division reversible"), A::ComputeReversedModValue(EGameplayModOp::Division, 4.0f, Out));
    TestEqual(TEXT("div reversal = 1/value"), Out, 0.25f);
    // Zero mult/div → NOT reversible (1/0 = inf would poison the attribute).
    TestFalse(TEXT("zero multiplicitive not reversible"), A::ComputeReversedModValue(EGameplayModOp::Multiplicitive, 0.0f, Out));
    TestFalse(TEXT("zero division not reversible"), A::ComputeReversedModValue(EGameplayModOp::Division, 0.0f, Out));
    // Override → not reversible by a single mod.
    TestFalse(TEXT("override not reversible"), A::ComputeReversedModValue(EGameplayModOp::Override, 5.0f, Out));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Party cross-party duplicate guard — UMythicPartySubsystem::AnyPartyContainsNameHash
// (co-op: an NPC already in ANY player's party can't be recruited again)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPartyCrossPartyDuplicateTest,
    "Mythic.AI.Party.CrossPartyDuplicate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPartyCrossPartyDuplicateTest::RunTest(const FString &Parameters) {
    using P = UMythicPartySubsystem;
    TMap<FString, TArray<FMythicPartyMember>> Parties; // keyed by canonical player key (FString)

    // Empty → never matches.
    TestFalse(TEXT("empty parties → no match"), P::AnyPartyContainsNameHash(Parties, 12345u));

    // Player 1 has a member with NameHash 100.
    FMythicPartyMember M1;
    M1.PersistedNameHash = 100;
    Parties.Add(TEXT("char-p1"), {M1});

    TestTrue(TEXT("hash 100 in P1 → match (re-recruit blocked)"), P::AnyPartyContainsNameHash(Parties, 100u));
    TestFalse(TEXT("hash 200 absent → no match"), P::AnyPartyContainsNameHash(Parties, 200u));
    // NameHash 0 (no captured identity) never matches, even with members present.
    TestFalse(TEXT("hash 0 → never matches"), P::AnyPartyContainsNameHash(Parties, 0u));

    // Player 2 recruits a different NPC (hash 200); querying 200 now matches in ANOTHER player's party.
    FMythicPartyMember M2;
    M2.PersistedNameHash = 200;
    Parties.Add(TEXT("char-p2"), {M2});
    TestTrue(TEXT("hash 200 now in P2 → cross-party match (co-op double-recruit blocked)"),
             P::AnyPartyContainsNameHash(Parties, 200u));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Party save-key migration — UMythicPartySubsystem::MakeLegacyPartyKey
// (a pre-v4 int32 party key migrates to the canonical session-fallback key form — single source)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPartyLegacyKeyMigrationTest,
    "Mythic.AI.Party.LegacyKeyMigration",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPartyLegacyKeyMigrationTest::RunTest(const FString &Parameters) {
    // The migrated key uses the SAME form as the canonical session fallback (single source of truth), so a legacy
    // slice-1 key (0) migrates to "session:0" rather than colliding with a real persistent CharacterID.
    TestEqual(TEXT("legacy 0 → session:0"), UMythicPartySubsystem::MakeLegacyPartyKey(0), FString(TEXT("session:0")));
    TestEqual(TEXT("legacy key == canonical session fallback (same rule)"),
              UMythicPartySubsystem::MakeLegacyPartyKey(5),
              AMythicPlayerState::ResolveCanonicalPlayerKey(FString(), 5));
    // A migrated legacy key never equals a persistent CharacterID (so legacy data can't hijack a real save slot).
    TestNotEqual(TEXT("legacy key ≠ a persistent CharacterID"),
                 UMythicPartySubsystem::MakeLegacyPartyKey(0), FString(TEXT("char-guid-abc")));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Party save-KEY byte round-trip — UMythicPartySubsystem::SerializePartyKey (through a real FArchive)
// (v4 FString key round-trips; a legacy v3 int32 key field is consumed byte-aligned + migrated)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPartySaveKeyRoundTripTest,
    "Mythic.AI.Party.SaveKeyRoundTrip",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPartySaveKeyRoundTripTest::RunTest(const FString &Parameters) {
    using P = UMythicPartySubsystem;

    // v4 round-trip through a real FArchive: write the canonical key, read it back identically + consume all bytes.
    {
        TArray<uint8> Bytes;
        FString Key = TEXT("char-guid-xyz");
        FMemoryWriter Writer(Bytes);
        P::SerializePartyKey(Writer, Key, 4);
        FString Out;
        FMemoryReader Reader(Bytes);
        P::SerializePartyKey(Reader, Out, 4);
        TestEqual(TEXT("v4 key round-trips"), Out, FString(TEXT("char-guid-xyz")));
        TestTrue(TEXT("v4 read consumed exactly the written bytes"), Reader.Tell() == Bytes.Num() && !Reader.IsError());
    }

    // Legacy (v3) read path: a v3 save wrote an int32 key field (slice-1 always 0). Writing that int32 then reading via
    // the v<4 branch must consume the 4-byte int32 (byte-alignment, so later fields don't desync) + migrate the key.
    {
        TArray<uint8> Bytes;
        int32 LegacyId = 0;
        FMemoryWriter Writer(Bytes);
        Writer << LegacyId;
        FString Out;
        FMemoryReader Reader(Bytes);
        P::SerializePartyKey(Reader, Out, 3);
        TestEqual(TEXT("legacy int32 key migrates to session:0"), Out, FString(TEXT("session:0")));
        TestTrue(TEXT("legacy read consumed the 4-byte int32 (byte-aligned)"), Reader.Tell() == 4 && !Reader.IsError());
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Death penalty — UMythicAttributeSet_Exp::ComputeXpAfterDeathPenalty
// (on full death, lose a fraction of current-LEVEL XP progress; default 0 = off; never de-levels)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDeathXpPenaltyTest,
    "Mythic.Progression.DeathXpPenalty",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDeathXpPenaltyTest::RunTest(const FString &Parameters) {
    using E = UProficiencyComponent;

    // Default (fraction 0) = NO penalty: XP unchanged (preserves today's consequence-free respawn).
    TestEqual(TEXT("fraction 0 → XP unchanged"), E::ComputeXpAfterDeathPenalty(800.0f, 0.0f), 800.0f);
    // A 25% penalty removes a quarter of current-level progress.
    TestEqual(TEXT("25% penalty"), E::ComputeXpAfterDeathPenalty(800.0f, 0.25f), 600.0f);
    // Full (1.0) wipes current-level progress back to the level floor (0) — never below, never de-levels.
    TestEqual(TEXT("100% penalty → 0 progress"), E::ComputeXpAfterDeathPenalty(800.0f, 1.0f), 0.0f);
    // Just-leveled (0 progress) → nothing to lose.
    TestEqual(TEXT("0 XP → stays 0"), E::ComputeXpAfterDeathPenalty(0.0f, 0.5f), 0.0f);
    // Fraction clamps: >1 behaves as full, <0 behaves as none — never amplifies loss or GRANTS xp.
    TestEqual(TEXT("fraction >1 clamps to full"), E::ComputeXpAfterDeathPenalty(500.0f, 2.0f), 0.0f);
    TestEqual(TEXT("fraction <0 clamps to none"), E::ComputeXpAfterDeathPenalty(500.0f, -1.0f), 500.0f);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Encumbrance — MythicEncumbrance::ComputeTier / SpeedMultiplierForTier
// (carried weight vs soft/hard capacity → tier → move-speed multiplier; default-off via zero caps)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEncumbranceTest,
    "Mythic.Itemization.Encumbrance",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEncumbranceTest::RunTest(const FString &Parameters) {
    using ET = EMythicEncumbranceTier;

    // Tiers against soft=100, hard=150.
    TestTrue(TEXT("under soft → Unencumbered"), MythicEncumbrance::ComputeTier(50.0f, 100.0f, 150.0f) == ET::Unencumbered);
    TestTrue(TEXT("at soft → Unencumbered (boundary inclusive)"), MythicEncumbrance::ComputeTier(100.0f, 100.0f, 150.0f) == ET::Unencumbered);
    TestTrue(TEXT("over soft, under hard → Heavy"), MythicEncumbrance::ComputeTier(120.0f, 100.0f, 150.0f) == ET::Heavy);
    TestTrue(TEXT("at hard → Heavy (boundary inclusive)"), MythicEncumbrance::ComputeTier(150.0f, 100.0f, 150.0f) == ET::Heavy);
    TestTrue(TEXT("over hard → Overloaded"), MythicEncumbrance::ComputeTier(151.0f, 100.0f, 150.0f) == ET::Overloaded);

    // Default-off: non-positive caps disable that threshold (a huge load is still Unencumbered with caps unset).
    TestTrue(TEXT("zero caps → always Unencumbered"), MythicEncumbrance::ComputeTier(9999.0f, 0.0f, 0.0f) == ET::Unencumbered);
    // Hard disabled but soft set → can reach Heavy, never Overloaded.
    TestTrue(TEXT("soft set, hard off → Heavy not Overloaded"), MythicEncumbrance::ComputeTier(9999.0f, 100.0f, 0.0f) == ET::Heavy);

    // Speed multipliers per tier, with clamping.
    TestEqual(TEXT("Unencumbered → full speed"), MythicEncumbrance::SpeedMultiplierForTier(ET::Unencumbered, 0.7f, 0.3f), 1.0f);
    TestEqual(TEXT("Heavy → HeavyMult"), MythicEncumbrance::SpeedMultiplierForTier(ET::Heavy, 0.7f, 0.3f), 0.7f);
    TestEqual(TEXT("Overloaded → OverloadedMult"), MythicEncumbrance::SpeedMultiplierForTier(ET::Overloaded, 0.7f, 0.3f), 0.3f);
    // A multiplier >1 is clamped (encumbrance never speeds you up); a negative one floors at 0.
    TestEqual(TEXT("Heavy mult >1 clamps to 1"), MythicEncumbrance::SpeedMultiplierForTier(ET::Heavy, 1.5f, 0.3f), 1.0f);
    TestEqual(TEXT("Overloaded mult <0 floors at 0"), MythicEncumbrance::SpeedMultiplierForTier(ET::Overloaded, 0.7f, -1.0f), 0.0f);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Carry-weight aggregation — UMythicInventoryComponent::ComputeSlotWeight (per-slot weight contribution)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCarriedWeightTest,
    "Mythic.Itemization.CarriedWeight",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCarriedWeightTest::RunTest(const FString &Parameters) {
    using I = UMythicInventoryComponent;

    // Normal: weight × stack.
    TestEqual(TEXT("2.5 × 4 = 10"), I::ComputeSlotWeight(2.5f, 4), 10.0f);
    // A single unit.
    TestEqual(TEXT("3 × 1 = 3"), I::ComputeSlotWeight(3.0f, 1), 3.0f);
    // Weightless item contributes 0 regardless of stack (the default → encumbrance-neutral world).
    TestEqual(TEXT("0 weight → 0"), I::ComputeSlotWeight(0.0f, 99), 0.0f);
    // Empty stack contributes 0.
    TestEqual(TEXT("0 stack → 0"), I::ComputeSlotWeight(5.0f, 0), 0.0f);
    // Malformed negatives never produce a negative (which would WRONGLY lighten the load).
    TestEqual(TEXT("negative weight → 0"), I::ComputeSlotWeight(-5.0f, 3), 0.0f);
    TestEqual(TEXT("negative stack → 0"), I::ComputeSlotWeight(5.0f, -3), 0.0f);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Aggro / threat targeting — AMythicAIController::SelectHighestThreatIndex / ComputeThreatDelta
// (highest-threat target selection + per-action threat accrual; INDEX_NONE = no threat → caller uses closest)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicThreatTargetingTest,
    "Mythic.AI.ThreatTargeting",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicThreatTargetingTest::RunTest(const FString &Parameters) {
    using A = AMythicAIController;

    // SelectHighestThreatIndex
    { TArray<float> T = {1.0f, 5.0f, 3.0f}; TestEqual(TEXT("picks the max threat"), A::SelectHighestThreatIndex(T), 1); }
    // All-zero (or empty) threat → INDEX_NONE so the caller falls back to the closest-target policy.
    { TArray<float> T = {0.0f, 0.0f}; TestEqual(TEXT("all-zero threat → NONE (fall back to closest)"), A::SelectHighestThreatIndex(T), (int32)INDEX_NONE); }
    TestEqual(TEXT("empty → NONE"), A::SelectHighestThreatIndex(TArray<float>()), (int32)INDEX_NONE);
    // Ties resolve to the first (lowest-index) max, matching SelectClosestHostileIndex.
    { TArray<float> T = {4.0f, 4.0f, 2.0f}; TestEqual(TEXT("tie → first max"), A::SelectHighestThreatIndex(T), 0); }
    // A negative entry never wins; a positive one does.
    { TArray<float> T = {-1.0f, 0.5f}; TestEqual(TEXT("negative ignored, positive wins"), A::SelectHighestThreatIndex(T), 1); }

    // ComputeThreatDelta = Damage × ThreatPerDamage + BonusThreat, all inputs clamped non-negative.
    TestEqual(TEXT("damage × multiplier"), A::ComputeThreatDelta(10.0f, 1.5f, 0.0f), 15.0f);
    TestEqual(TEXT("+ flat bonus (taunt)"), A::ComputeThreatDelta(10.0f, 1.0f, 100.0f), 110.0f);
    TestEqual(TEXT("bonus-only (zero damage)"), A::ComputeThreatDelta(0.0f, 2.0f, 50.0f), 50.0f);
    // Negative inputs never SUBTRACT threat (a heal mis-tagged negative, or a bad multiplier, can't lower aggro).
    TestEqual(TEXT("negative damage → no contribution"), A::ComputeThreatDelta(-10.0f, 2.0f, 5.0f), 5.0f);
    TestEqual(TEXT("negative multiplier → no contribution"), A::ComputeThreatDelta(10.0f, -2.0f, 5.0f), 5.0f);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Currency / wallet — MythicCurrency::CanAfford / ComputeBalanceAfterSpend / ComputeSalePrice
// (currency = stackable currency-type items; these are the pure money-transaction rules)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCurrencyTest,
    "Mythic.Itemization.Currency",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCurrencyTest::RunTest(const FString &Parameters) {
    // CanAfford
    TestTrue(TEXT("exact balance affords"), MythicCurrency::CanAfford(100, 100));
    TestTrue(TEXT("surplus affords"), MythicCurrency::CanAfford(150, 100));
    TestFalse(TEXT("short cannot afford"), MythicCurrency::CanAfford(50, 100));
    TestTrue(TEXT("free (price 0) always affordable"), MythicCurrency::CanAfford(0, 0));
    TestTrue(TEXT("negative price → free"), MythicCurrency::CanAfford(0, -10));

    // ComputeBalanceAfterSpend — never negative, no-op when unaffordable / non-positive.
    TestEqual(TEXT("spend deducts"), MythicCurrency::ComputeBalanceAfterSpend(100, 30), 70);
    TestEqual(TEXT("exact spend → 0"), MythicCurrency::ComputeBalanceAfterSpend(100, 100), 0);
    TestEqual(TEXT("unaffordable → unchanged (caller must gate on CanAfford)"), MythicCurrency::ComputeBalanceAfterSpend(20, 100), 20);
    TestEqual(TEXT("non-positive price → no-op (no free money)"), MythicCurrency::ComputeBalanceAfterSpend(100, 0), 100);

    // ComputeSalePrice = floor(UnitValue × Quantity × clamp(SellRate,0,1)).
    TestEqual(TEXT("value×qty×rate"), MythicCurrency::ComputeSalePrice(10, 3, 0.5f), 15);
    TestEqual(TEXT("floors fractional coins"), MythicCurrency::ComputeSalePrice(10, 1, 0.55f), 5);
    TestEqual(TEXT("rate>1 clamps to full value"), MythicCurrency::ComputeSalePrice(10, 2, 2.0f), 20);
    TestEqual(TEXT("zero value → 0 (unsellable)"), MythicCurrency::ComputeSalePrice(0, 5, 0.5f), 0);
    TestEqual(TEXT("zero quantity → 0"), MythicCurrency::ComputeSalePrice(10, 0, 0.5f), 0);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Inventory stack-merge gate — UMythicInventoryComponent::ShouldAttemptStackMerge
// (merge into partials for any STACKABLE type, regardless of the incoming's fullness — was `Max > IncomingStacks`)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicInventoryStackMergeGateTest,
    "Mythic.Itemization.Inventory.StackMergeGate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicInventoryStackMergeGateTest::RunTest(const FString &Parameters) {
    using I = UMythicInventoryComponent;
    // A stackable type (StackSizeMax > 1) always attempts merge — including a FULL incoming stack (the fix: the old
    // `Max > IncomingStacks` gate skipped a full incoming, so it never topped off existing partial stacks).
    TestTrue(TEXT("stackable (max 100) → attempt merge"), I::ShouldAttemptStackMerge(100));
    TestTrue(TEXT("stackable (max 2, the minimum stackable) → attempt merge"), I::ShouldAttemptStackMerge(2));
    // A non-stackable type (max 1) never merges; degenerate 0 also skips.
    TestFalse(TEXT("non-stackable (max 1) → no merge"), I::ShouldAttemptStackMerge(1));
    TestFalse(TEXT("degenerate (max 0) → no merge"), I::ShouldAttemptStackMerge(0));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Initial stack-quantity clamp — UMythicItemInstance::ClampInitialStackQuantity
// (a created stack is always >= 1 and <= StackSizeMax; non-stackable / degenerate max <= 1 → a single unit)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicItemInitialStackClampTest,
    "Mythic.Itemization.Inventory.InitialStackClamp",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicItemInitialStackClampTest::RunTest(const FString &Parameters) {
    using I = UMythicItemInstance;

    // Non-stackable (max 1) → always a single unit, whatever was requested.
    TestEqual(TEXT("non-stackable: 5 requested → 1"), I::ClampInitialStackQuantity(5, 1), 1);
    TestEqual(TEXT("non-stackable: 0 requested → 1"), I::ClampInitialStackQuantity(0, 1), 1);

    // Degenerate stack max (0 / negative) → treated as non-stackable, a single unit (never 0 or negative).
    TestEqual(TEXT("degenerate max 0 → 1"), I::ClampInitialStackQuantity(5, 0), 1);
    TestEqual(TEXT("degenerate max -2 → 1"), I::ClampInitialStackQuantity(3, -2), 1);

    // Stackable normal range → requested kept.
    TestEqual(TEXT("stackable: 5 of 100 → 5"), I::ClampInitialStackQuantity(5, 100), 5);
    TestEqual(TEXT("stackable: exactly max → max"), I::ClampInitialStackQuantity(100, 100), 100);

    // Over-stack request clamped down to the cap.
    TestEqual(TEXT("over-stack 150 of 100 → 100"), I::ClampInitialStackQuantity(150, 100), 100);

    // Zero / negative request floored to 1 (never a zero/negative created stack).
    TestEqual(TEXT("zero request → 1"), I::ClampInitialStackQuantity(0, 100), 1);
    TestEqual(TEXT("negative request → 1"), I::ClampInitialStackQuantity(-3, 100), 1);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Dialogue severity constraint — FMythicDialogueSelector::TemplateConstrainsSeverity
// (a template hard-filters by severity iff it tightened EITHER bound; default Min=0/Max=0xFF = unconstrained)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDialogueSeverityConstraintTest,
    "Mythic.LivingWorld.Dialogue.SeverityConstraint",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDialogueSeverityConstraintTest::RunTest(const FString &Parameters) {
    using S = FMythicDialogueSelector;
    // Defaults (Min=0, Max=0xFF) → unconstrained → never hard-filters on severity.
    TestFalse(TEXT("default Min0/Max255 → unconstrained"), S::TemplateConstrainsSeverity(0, 0xFF));
    // Min raised → constrained (already worked before).
    TestTrue(TEXT("Min raised → constrained"), S::TemplateConstrainsSeverity(2, 0xFF));
    // Max lowered with Min at default 0 → constrained (THE FIX — previously treated as unconstrained, ignoring Max).
    TestTrue(TEXT("Max lowered (Min default 0) → constrained (the fix)"), S::TemplateConstrainsSeverity(0, 2));
    // Both bounds tightened → constrained.
    TestTrue(TEXT("both bounds tightened → constrained"), S::TemplateConstrainsSeverity(1, 3));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Dialogue variable substitution — FMythicDialogueSelector::ResolveVariables
// (named {placeholders} → live values; replace-all; unknown placeholders left untouched; empty value blanks out)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDialogueResolveVariablesTest,
    "Mythic.LivingWorld.Dialogue.ResolveVariables",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDialogueResolveVariablesTest::RunTest(const FString &Parameters) {
    using DS = FMythicDialogueSelector;
    FMythicDialogueVariables V;
    V.NPCName = TEXT("Garrick");
    V.FactionName = TEXT("Ironhold");
    // V.TargetName left empty (default) to exercise the blank-out case.

    auto R = [&](const TCHAR *Template) {
        return DS::ResolveVariables(FText::FromString(Template), V).ToString();
    };

    TestEqual(TEXT("single placeholder"), R(TEXT("Hello {npc_name}")), FString(TEXT("Hello Garrick")));
    TestEqual(TEXT("two distinct placeholders"), R(TEXT("{npc_name} of {faction_name}")), FString(TEXT("Garrick of Ironhold")));
    // ReplaceInline replaces ALL occurrences.
    TestEqual(TEXT("repeated placeholder → replace-all"), R(TEXT("{npc_name} & {npc_name}")), FString(TEXT("Garrick & Garrick")));
    // An unrecognised placeholder is left verbatim (no substitution table entry).
    TestEqual(TEXT("unknown placeholder untouched"), R(TEXT("Hi {unknown_var}")), FString(TEXT("Hi {unknown_var}")));
    // No placeholders → text unchanged.
    TestEqual(TEXT("plain text unchanged"), R(TEXT("nothing to fill")), FString(TEXT("nothing to fill")));
    // An empty live value blanks the placeholder out.
    TestEqual(TEXT("empty value blanks out"), R(TEXT("end{target_name}.")), FString(TEXT("end.")));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Dialogue template selection — FMythicDialogueSelector::SelectTemplate
// (highest total score wins; role/faction/situation are hard-filters; role match +20; priority added)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDialogueSelectTemplateTest,
    "Mythic.LivingWorld.Dialogue.SelectTemplate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDialogueSelectTemplateTest::RunTest(const FString &Parameters) {
    using DS = FMythicDialogueSelector;

    // Null / empty database → no template selected.
    {
        FMythicDialogueContext Ctx;
        TestTrue(TEXT("null DB → no template"), DS::SelectTemplate(nullptr, Ctx).Template == nullptr);
        UMythicDialogueDatabase *Empty = NewObject<UMythicDialogueDatabase>();
        TestTrue(TEXT("empty DB → no template"), DS::SelectTemplate(Empty, Ctx).Template == nullptr);
    }

    // Highest total score wins — here by Priority, all else equal (both bare templates also get +5 in-range severity).
    {
        UMythicDialogueDatabase *DB = NewObject<UMythicDialogueDatabase>();
        FMythicDialogueTemplate Lo;
        Lo.Priority = 10;
        Lo.DialogueText = FText::FromString(TEXT("lo"));
        FMythicDialogueTemplate Hi;
        Hi.Priority = 90;
        Hi.DialogueText = FText::FromString(TEXT("hi"));
        DB->Templates.Add(Lo);
        DB->Templates.Add(Hi);
        FMythicDialogueContext Ctx;
        const FMythicDialogueResult R = DS::SelectTemplate(DB, Ctx);
        TestTrue(TEXT("a template is selected"), R.Template != nullptr);
        TestEqual(TEXT("higher priority wins"), R.ResolvedText.ToString(), FString(TEXT("hi")));
    }

    // Two distinct REGISTERED tags stand in for role tags (MatchesTag compares hierarchy, not the editor category).
    const FGameplayTag RoleX = FGameplayTag::RequestGameplayTag(FName("GAS.State.Dead"), /*ErrorIfNotFound*/ false);
    const FGameplayTag RoleY = FGameplayTag::RequestGameplayTag(FName("GAS.State.Downed"), /*ErrorIfNotFound*/ false);
    TestTrue(TEXT("stand-in role tags resolve + differ"), RoleX.IsValid() && RoleY.IsValid() && RoleX != RoleY);
    if (RoleX.IsValid() && RoleY.IsValid() && RoleX != RoleY) {
        // Role hard-filter: a role-gated template is SKIPPED when the context role doesn't match → nothing selected.
        {
            UMythicDialogueDatabase *DB = NewObject<UMythicDialogueDatabase>();
            FMythicDialogueTemplate Gated;
            Gated.RequiredRole = RoleX;
            Gated.Priority = 90;
            Gated.DialogueText = FText::FromString(TEXT("gated"));
            DB->Templates.Add(Gated);
            FMythicDialogueContext Ctx;
            Ctx.RoleTag = RoleY; // mismatch
            TestTrue(TEXT("role mismatch → template skipped"), DS::SelectTemplate(DB, Ctx).Template == nullptr);
        }
        // Role match (+20) beats a generic template of EQUAL priority.
        {
            UMythicDialogueDatabase *DB = NewObject<UMythicDialogueDatabase>();
            FMythicDialogueTemplate Generic;
            Generic.Priority = 50;
            Generic.DialogueText = FText::FromString(TEXT("generic"));
            FMythicDialogueTemplate RoleMatch;
            RoleMatch.RequiredRole = RoleX;
            RoleMatch.Priority = 50;
            RoleMatch.DialogueText = FText::FromString(TEXT("role"));
            DB->Templates.Add(Generic);
            DB->Templates.Add(RoleMatch);
            FMythicDialogueContext Ctx;
            Ctx.RoleTag = RoleX;
            const FMythicDialogueResult R = DS::SelectTemplate(DB, Ctx);
            TestEqual(TEXT("role-matched (+20) beats generic of equal priority"), R.ResolvedText.ToString(), FString(TEXT("role")));
        }
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// World Chronicle event-tag formatting — UMythicWorldChronicleSubsystem::EventTagToReadable
// (player-facing news + NPC {recent_event}: last two tag segments as "Parent Leaf"; invalid → safe default)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicChronicleEventReadableTest,
    "Mythic.LivingWorld.Chronicle.EventTagToReadable",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicChronicleEventReadableTest::RunTest(const FString &Parameters) {
    using C = UMythicWorldChronicleSubsystem;

    // Invalid tag → safe default (never an empty news line).
    TestEqual(TEXT("invalid tag → 'World Event'"), C::EventTagToReadable(FGameplayTag()), FString(TEXT("World Event")));

    // Dominant path: render the last TWO segments ("Parent Leaf") so the category survives. Any registered
    // multi-segment tag exercises the formatting — semantics aside, GAS.State.Dead → "State Dead".
    const FGameplayTag Dead = FGameplayTag::RequestGameplayTag(FName("GAS.State.Dead"), /*ErrorIfNotFound*/ false);
    const FGameplayTag Downed = FGameplayTag::RequestGameplayTag(FName("GAS.State.Downed"), /*ErrorIfNotFound*/ false);
    TestTrue(TEXT("stand-in tags registered (prereq)"), Dead.IsValid() && Downed.IsValid());
    if (Dead.IsValid()) {
        TestEqual(TEXT("GAS.State.Dead → 'State Dead'"), C::EventTagToReadable(Dead), FString(TEXT("State Dead")));
    }
    if (Downed.IsValid()) {
        TestEqual(TEXT("GAS.State.Downed → 'State Downed'"), C::EventTagToReadable(Downed), FString(TEXT("State Downed")));
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Loot entry drop-chance resolution — ULootReward::ResolveEntryDropChance
// (override-wins / per-rarity weight / SKIP on out-of-range rarity — no OOB read of the fixed weights array)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicLootDropChanceResolveTest,
    "Mythic.Itemization.Loot.DropChanceResolve",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicLootDropChanceResolveTest::RunTest(const FString &Parameters) {
    using L = ULootReward;
    const float Weights[] = {0.5f, 0.3f, 0.2f, 0.1f, 0.05f}; // Common..Mythic
    const TConstArrayView<float> W(Weights, 5);
    float Out = -1.0f;

    // Override > 0 wins (ignores rarity/weight), even for an out-of-range rarity.
    TestTrue(TEXT("override wins"), L::ResolveEntryDropChance(0.8f, 2, W, Out));
    TestEqual(TEXT("override value used"), Out, 0.8f);
    TestTrue(TEXT("override wins even for out-of-range rarity"), L::ResolveEntryDropChance(0.8f, 99, W, Out));
    TestEqual(TEXT("override value used (oor rarity)"), Out, 0.8f);

    // No override → per-rarity weight by index.
    TestTrue(TEXT("rarity 0 → weight"), L::ResolveEntryDropChance(0.0f, 0, W, Out));
    TestEqual(TEXT("rarity 0 weight"), Out, 0.5f);
    TestTrue(TEXT("rarity 4 (Mythic) → weight"), L::ResolveEntryDropChance(0.0f, 4, W, Out));
    TestEqual(TEXT("rarity 4 weight"), Out, 0.05f);

    // Out-of-range rarity with NO override → SKIP (false), no OOB read. (The bug fix.)
    TestFalse(TEXT("rarity 5 (out of range), no override → skip"), L::ResolveEntryDropChance(0.0f, 5, W, Out));
    TestFalse(TEXT("negative rarity, no override → skip"), L::ResolveEntryDropChance(0.0f, -1, W, Out));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Proficiency level-at-XP — UProficiencyDefinition::CalcLevelAtXP
// (robust capped walk: exact at boundaries, clamps to MaxLevel — was a float-fragile log inversion w/ no cap)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicProficiencyLevelAtXPTest,
    "Mythic.Player.Proficiency.LevelAtXP",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicProficiencyLevelAtXPTest::RunTest(const FString &Parameters) {
    using D = UProficiencyDefinition;

    // null def → level 1 (safe default). The null path logs an Error by design; whitelist it so the harness doesn't
    // treat the deliberate diagnostic as a test failure.
    AddExpectedError(TEXT("CalcLevelAtXP: Def is null"), EAutomationExpectedErrorFlags::Contains, 1);
    TestEqual(TEXT("null def → 1"), D::CalcLevelAtXP(500.0f, nullptr), 1);

    // Geometric track: STARTING_XP=100, GrowthRate=2 → Cumulative(L)=100*(2^(L-1)-1): 0,100,300,700,1500.
    UProficiencyDefinition *Def = NewObject<UProficiencyDefinition>();
    Def->GrowthRate = 2.0f;
    Def->MaxLevel = 5;
    TestEqual(TEXT("XP<0 → 1"), D::CalcLevelAtXP(-50.0f, Def), 1);
    TestEqual(TEXT("0 XP → 1"), D::CalcLevelAtXP(0.0f, Def), 1);
    TestEqual(TEXT("99 XP → 1"), D::CalcLevelAtXP(99.0f, Def), 1);
    TestEqual(TEXT("100 XP → 2 (exact boundary)"), D::CalcLevelAtXP(100.0f, Def), 2);
    TestEqual(TEXT("299 XP → 2"), D::CalcLevelAtXP(299.0f, Def), 2);
    TestEqual(TEXT("300 XP → 3 (exact boundary)"), D::CalcLevelAtXP(300.0f, Def), 3);
    TestEqual(TEXT("1500 XP → 5 (max boundary)"), D::CalcLevelAtXP(1500.0f, Def), 5);
    TestEqual(TEXT("huge overshoot → clamped to MaxLevel"), D::CalcLevelAtXP(1.0e6f, Def), 5);

    // Round-trip: CalcLevelAtXP(Cumulative(L)) == L for every level (would FAIL under the old float-fragile inversion
    // at a boundary that rounded down).
    for (int32 L = 1; L <= Def->MaxLevel; ++L) {
        const float CumXP = D::CalcCumulativeXPForLevel(L, Def);
        TestEqual(*FString::Printf(TEXT("round-trip level %d"), L), D::CalcLevelAtXP(CumXP, Def), L);
    }

    // Linear track (GrowthRate≈1): Cumulative(L)=100*(L-1).
    UProficiencyDefinition *Lin = NewObject<UProficiencyDefinition>();
    Lin->GrowthRate = 1.0f;
    Lin->MaxLevel = 10;
    TestEqual(TEXT("linear: 250 XP → 3"), D::CalcLevelAtXP(250.0f, Lin), 3);
    TestEqual(TEXT("linear: overshoot → clamped to MaxLevel"), D::CalcLevelAtXP(1.0e6f, Lin), 10);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Proficiency XP curve — UProficiencyDefinition::CalcXPCostForLevelUp + CalcXPRemainingForLevel
// (the two siblings of CalcLevelAtXP/CalcCumulativeXPForLevel that were untested: per-level cost + XP-to-target)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicProficiencyXPCurveTest,
    "Mythic.Player.Proficiency.XPCurve",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicProficiencyXPCurveTest::RunTest(const FString &Parameters) {
    using D = UProficiencyDefinition;

    // Same geometric track as the LevelAtXP test: STARTING_XP=100, GrowthRate=2 → per-level cost = 100*2^(L-1);
    // cumulative(L) = 100*(2^(L-1)-1) → L2=100, L3=300, L4=700, L5=1500.
    UProficiencyDefinition *Def = NewObject<UProficiencyDefinition>();
    Def->GrowthRate = 2.0f;
    Def->MaxLevel = 5;

    // CalcXPCostForLevelUp: cost from L→L+1 = 100*2^(L-1) (geometric; not MaxLevel-clamped in this function).
    TestEqual(TEXT("cost L1→L2 = 100"), D::CalcXPCostForLevelUp(1, Def), 100.0f);
    TestEqual(TEXT("cost L2→L3 = 200"), D::CalcXPCostForLevelUp(2, Def), 200.0f);
    TestEqual(TEXT("cost L3→L4 = 400"), D::CalcXPCostForLevelUp(3, Def), 400.0f);
    TestEqual(TEXT("cost L4→L5 = 800"), D::CalcXPCostForLevelUp(4, Def), 800.0f);
    TestEqual(TEXT("cost level<1 → 0"), D::CalcXPCostForLevelUp(0, Def), 0.0f);

    // CalcXPRemainingForLevel: max(cumulative(target) - currentXP, 0); 0 once already at/past the target level.
    TestEqual(TEXT("remaining 0 XP → L3 = 300"), D::CalcXPRemainingForLevel(0.0f, 3, Def), 300.0f);
    TestEqual(TEXT("remaining 150 XP → L3 = 150"), D::CalcXPRemainingForLevel(150.0f, 3, Def), 150.0f);
    TestEqual(TEXT("remaining 0 XP → L2 = 100"), D::CalcXPRemainingForLevel(0.0f, 2, Def), 100.0f);
    TestEqual(TEXT("remaining at target (300 XP → L3) = 0"), D::CalcXPRemainingForLevel(300.0f, 3, Def), 0.0f);
    TestEqual(TEXT("remaining past target (400 XP → L3) = 0"), D::CalcXPRemainingForLevel(400.0f, 3, Def), 0.0f);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Conversion product level — UConversionStationComponent::ResolveProductLevel
// (InheritStationLevel was hard-stubbed to 1; now driven by the station's configured StationLevel)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicConversionProductLevelTest,
    "Mythic.Itemization.Conversion.ProductLevel",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicConversionProductLevelTest::RunTest(const FString &Parameters) {
    using C = UConversionStationComponent;
    // Args: (LevelMode, InputLevel, StationLevel, FixedLevel)
    // FixedLevel → the recipe's fixed level (ignores input/station).
    TestEqual(TEXT("FixedLevel → FixedLevel"), C::ResolveProductLevel(EProductLevelMode::FixedLevel, 7, 4, 9), 9);
    // InheritInputLevel → the snapshotted input level.
    TestEqual(TEXT("InheritInputLevel → input level"), C::ResolveProductLevel(EProductLevelMode::InheritInputLevel, 7, 4, 9), 7);
    // InheritStationLevel → the station level (the fix — was hard-stubbed to 1).
    TestEqual(TEXT("InheritStationLevel → station level (the fix)"), C::ResolveProductLevel(EProductLevelMode::InheritStationLevel, 7, 4, 9), 4);
    // Regression guard: a station level above 1 is honored, not collapsed to 1.
    TestEqual(TEXT("InheritStationLevel honors station level > 1"), C::ResolveProductLevel(EProductLevelMode::InheritStationLevel, 2, 5, 1), 5);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Settlement center cell — AMythicSettlement::ComputeCenterCell
// (the Socialize gather-point — was never assigned → always (0,0) → NPCs socialized at the grid origin corner)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSettlementCenterCellTest,
    "Mythic.LivingWorld.Settlement.CenterCell",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSettlementCenterCellTest::RunTest(const FString &Parameters) {
    using S = AMythicSettlement;

    // Empty → (0,0) fallback (degenerate; MinSettlementCells should prevent it).
    {
        const TArray<FMythicCellCoord> Empty;
        const FMythicCellCoord C = S::ComputeCenterCell(Empty);
        TestTrue(TEXT("empty → (0,0)"), C.X == 0 && C.Y == 0);
    }
    // Single cell → that cell.
    {
        const TArray<FMythicCellCoord> One = {FMythicCellCoord(40, 50)};
        const FMythicCellCoord C = S::ComputeCenterCell(One);
        TestTrue(TEXT("single cell → itself"), C.X == 40 && C.Y == 50);
    }
    // 3×3 block at 40..42 → centroid (41,41), which IS a member → (41,41); crucially NOT the (0,0) default (the bug).
    {
        TArray<FMythicCellCoord> Block;
        for (int32 Y = 40; Y <= 42; ++Y) {
            for (int32 X = 40; X <= 42; ++X) {
                Block.Add(FMythicCellCoord(X, Y));
            }
        }
        const FMythicCellCoord C = S::ComputeCenterCell(Block);
        TestTrue(TEXT("3x3 block → center (41,41)"), C.X == 41 && C.Y == 41);
        TestFalse(TEXT("center is NOT the (0,0) default"), C.X == 0 && C.Y == 0);
    }
    // The result is always an ACTUAL input cell (nearest-centroid guarantee), incl. for an offset/sparse set.
    {
        const TArray<FMythicCellCoord> Cells = {
            FMythicCellCoord(10, 10), FMythicCellCoord(10, 11), FMythicCellCoord(11, 10), FMythicCellCoord(100, 100)};
        const FMythicCellCoord C = S::ComputeCenterCell(Cells);
        const bool bIsInput = Cells.ContainsByPredicate([&](const FMythicCellCoord &E) { return E.X == C.X && E.Y == C.Y; });
        TestTrue(TEXT("center is an actual input cell"), bIsInput);
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════
// Shop claim eligibility — UMythicSettlementRegistry::CanClaimShop
// (the PRODUCER half of the shop-ownership lifecycle: which vacant slots a role-bearing NPC may claim)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicShopClaimEligibilityTest,
    "Mythic.LivingWorld.Settlement.ShopClaimEligibility",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicShopClaimEligibilityTest::RunTest(const FString &Parameters) {
    // Two distinct, guaranteed-registered native tags stand in for NPC role tags (the matcher is role-agnostic).
    const FGameplayTag RoleA = TAG_LIVINGWORLD_EVENT_FACTION_RESTORATION;
    const FGameplayTag RoleB = TAG_LIVINGWORLD_EVENT_FACTION_SCHISM;

    auto MakeShop = [](const FGameplayTag &Required) {
        FMythicShopSlot S;
        S.RequiredRole = Required;
        S.OwnerEntityId = 0;
        S.VacatedTime = 0.0;
        S.bPlayerOwned = false;
        return S;
    };
    using Reg = UMythicSettlementRegistry;

    // Claimable: unowned + ready (VacatedTime 0) + role matches.
    TestTrue(TEXT("matching role on a ready slot is claimable"), Reg::CanClaimShop(MakeShop(RoleA), RoleA));
    // Role mismatch → not claimable.
    TestFalse(TEXT("non-matching role cannot claim"), Reg::CanClaimShop(MakeShop(RoleA), RoleB));
    // Already owned → not claimable.
    {
        FMythicShopSlot S = MakeShop(RoleA);
        S.OwnerEntityId = 42;
        TestFalse(TEXT("owned slot not claimable"), Reg::CanClaimShop(S, RoleA));
    }
    // Player-owned → not claimable (player ownership overrides NPC succession).
    {
        FMythicShopSlot S = MakeShop(RoleA);
        S.bPlayerOwned = true;
        TestFalse(TEXT("player-owned slot not claimable"), Reg::CanClaimShop(S, RoleA));
    }
    // Mid-succession (VacatedTime > 0, delay not yet elapsed) → not claimable.
    {
        FMythicShopSlot S = MakeShop(RoleA);
        S.VacatedTime = 123.0;
        TestFalse(TEXT("mid-succession slot not claimable"), Reg::CanClaimShop(S, RoleA));
    }
    // Invalid claimant / shop role → not claimable.
    TestFalse(TEXT("invalid claimant role cannot claim"), Reg::CanClaimShop(MakeShop(RoleA), FGameplayTag()));
    TestFalse(TEXT("slot with no required role is not claimable"), Reg::CanClaimShop(MakeShop(FGameplayTag()), RoleA));

    // Hierarchy: a more-specific claimant satisfies a parent requirement; a parent claimant does NOT satisfy a child req.
    const FGameplayTag ParentRole = RoleA.RequestDirectParent();
    if (ParentRole.IsValid()) {
        TestTrue(TEXT("child role satisfies parent requirement"), Reg::CanClaimShop(MakeShop(ParentRole), RoleA));
        TestFalse(TEXT("parent role does NOT satisfy child requirement"), Reg::CanClaimShop(MakeShop(RoleA), ParentRole));
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Offense probability-clamp membership — UMythicAttributeSet_Offense::IsProbabilityAttribute
// (crit chance + 8 on-hit proc chances clamp to [0,1]; multipliers/base values do NOT)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicOffenseProbabilityClampTest,
    "Mythic.GAS.Offense.ProbabilityClamp",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicOffenseProbabilityClampTest::RunTest(const FString &Parameters) {
    using O = UMythicAttributeSet_Offense;

    // Probabilities → clamped (member of the [0,1] set).
    TestTrue(TEXT("CriticalHitChance is a probability"), O::IsProbabilityAttribute(O::GetCriticalHitChanceAttribute()));
    TestTrue(TEXT("ApplyBurnOnHitChance is a probability"), O::IsProbabilityAttribute(O::GetApplyBurnOnHitChanceAttribute()));
    TestTrue(TEXT("ApplyBleedOnHitChance is a probability"), O::IsProbabilityAttribute(O::GetApplyBleedOnHitChanceAttribute()));
    TestTrue(TEXT("ApplyPoisonOnHitChance is a probability"), O::IsProbabilityAttribute(O::GetApplyPoisonOnHitChanceAttribute()));
    TestTrue(TEXT("ApplySlowOnHitChance is a probability"), O::IsProbabilityAttribute(O::GetApplySlowOnHitChanceAttribute()));
    TestTrue(TEXT("ApplyFreezeOnHitChance is a probability"), O::IsProbabilityAttribute(O::GetApplyFreezeOnHitChanceAttribute()));
    TestTrue(TEXT("ApplyStunOnHitChance is a probability"), O::IsProbabilityAttribute(O::GetApplyStunOnHitChanceAttribute()));
    TestTrue(TEXT("ApplyWeakenOnHitChance is a probability"), O::IsProbabilityAttribute(O::GetApplyWeakenOnHitChanceAttribute()));
    TestTrue(TEXT("ApplyTerrifyOnHitChance is a probability"), O::IsProbabilityAttribute(O::GetApplyTerrifyOnHitChanceAttribute()));

    // Multipliers / base values → NOT clamped (must keep their full range; clamping would remove design headroom).
    TestFalse(TEXT("CriticalHitDamage is NOT a probability (multiplier)"), O::IsProbabilityAttribute(O::GetCriticalHitDamageAttribute()));
    TestFalse(TEXT("Power is NOT a probability"), O::IsProbabilityAttribute(O::GetPowerAttribute()));
    TestFalse(TEXT("DamagePerHit is NOT a probability"), O::IsProbabilityAttribute(O::GetDamagePerHitAttribute()));
    TestFalse(TEXT("BonusSwordDamage is NOT a probability (multiplier)"), O::IsProbabilityAttribute(O::GetBonusSwordDamageAttribute()));
    TestFalse(TEXT("BonusSkillDamage is NOT a probability (multiplier)"), O::IsProbabilityAttribute(O::GetBonusSkillDamageAttribute()));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Death-latch rule — UMythicAttributeSet_Life::ComputeOutOfHealthLatch
// (the latch must track health after EVERY mutation; a restoring heal clears it)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicLifeOutOfHealthLatchTest,
    "Mythic.GAS.Life.OutOfHealthLatch",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicLifeOutOfHealthLatchTest::RunTest(const FString &Parameters) {
    // At/below zero → out of health (death latch SET).
    TestTrue(TEXT("health 0 → out of health"), UMythicAttributeSet_Life::ComputeOutOfHealthLatch(0.0f));
    TestTrue(TEXT("health negative → out of health"), UMythicAttributeSet_Life::ComputeOutOfHealthLatch(-5.0f));
    // Above zero → NOT out of health — the case the Healing-path fix relies on (a restoring heal clears the latch),
    // and the recovery transition the direct-Health path already used.
    TestFalse(TEXT("health 50 → not out of health"), UMythicAttributeSet_Life::ComputeOutOfHealthLatch(50.0f));
    TestFalse(TEXT("health 0.01 → not out of health (heal-from-0 clears latch)"),
              UMythicAttributeSet_Life::ComputeOutOfHealthLatch(0.01f));
    return true;
}

// ═══════════════════════════════════════════════════════════════
// Lethal-outcome rule — UMythicAttributeSet_Life::ResolveLethalOutcome
// (foundation of co-op down/revive: down-vs-die gate, default-off so behavior is unchanged)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicLethalOutcomeTest,
    "Mythic.GAS.Life.LethalOutcome",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicLethalOutcomeTest::RunTest(const FString &Parameters) {
    using EOut = EMythicLethalOutcome;
    auto Resolve = &UMythicAttributeSet_Life::ResolveLethalOutcome;

    // Non-lethal always survives, regardless of policy/state flags.
    TestEqual(TEXT("non-lethal survives (down off)"), Resolve(false, false, false, false), EOut::Survive);
    TestEqual(TEXT("non-lethal survives (down on, revivable)"), Resolve(false, true, false, true), EOut::Survive);

    // Lethal + down DISABLED → dies (current behavior preserved — the default).
    TestEqual(TEXT("lethal + down disabled → die"), Resolve(true, false, false, true), EOut::Die);

    // Lethal + down enabled, but NOT a revivable pawn (NPC/creature) → dies outright.
    TestEqual(TEXT("lethal + down on + non-revivable → die"), Resolve(true, true, false, false), EOut::Die);

    // Lethal + down enabled + revivable + ALREADY downed → finished off (dies).
    TestEqual(TEXT("lethal + down on + already downed → die (finished off)"), Resolve(true, true, true, true), EOut::Die);

    // Lethal + down enabled + revivable + not yet downed → enters the downed state (the new co-op path).
    TestEqual(TEXT("lethal + down on + revivable + first hit → downed"), Resolve(true, true, false, true), EOut::EnterDownState);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Revive-eligibility gate — UMythicLifeComponent::CanReviveTarget
// (only a non-downed reviver can revive a downed target)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicReviveGateTest,
    "Mythic.GAS.Life.CanReviveTarget",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicReviveGateTest::RunTest(const FString &Parameters) {
    auto CanRevive = &UMythicLifeComponent::CanReviveTarget; // (bTargetDowned, bReviverDowned)

    // The only valid case: a downed target + a reviver who isn't downed.
    TestTrue(TEXT("downed target + healthy reviver → can revive"), CanRevive(true, false));
    // Target not downed → nothing to revive.
    TestFalse(TEXT("non-downed target → cannot revive"), CanRevive(false, false));
    // A downed reviver can't revive anyone (can't act).
    TestFalse(TEXT("downed reviver → cannot revive"), CanRevive(true, true));
    // Both invalid.
    TestFalse(TEXT("non-downed target + downed reviver → cannot revive"), CanRevive(false, true));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Equip-requirement gate — UMythicInventoryComponent::MeetsEquipRequirement
// (an item with a RequiredEquipTag may only be equipped by an owner who owns it; empty = no requirement)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEquipRequirementTest,
    "Mythic.Itemization.EquipRequirement",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEquipRequirementTest::RunTest(const FString &Parameters) {
    auto Meets = &UMythicInventoryComponent::MeetsEquipRequirement;

    // No requirement (invalid tag) → always equippable, regardless of owner tags (the default — non-breaking).
    FGameplayTagContainer NoTags;
    TestTrue(TEXT("empty requirement passes with no owner tags"), Meets(FGameplayTag(), NoTags));

    const FGameplayTag Req = FGameplayTag::RequestGameplayTag(FName("GAS.State.Dead"), /*ErrorIfNotFound*/ false);
    TestTrue(TEXT("native tag resolves (test prerequisite)"), Req.IsValid());
    if (Req.IsValid()) {
        FGameplayTagContainer HasReq;
        HasReq.AddTag(Req);
        // Empty requirement still passes even when the owner has tags.
        TestTrue(TEXT("empty requirement passes even with owner tags"), Meets(FGameplayTag(), HasReq));
        // Owner owns the required tag → equippable.
        TestTrue(TEXT("owner has required tag → equippable"), Meets(Req, HasReq));
        // Owner lacks the required tag → blocked.
        TestFalse(TEXT("owner lacks required tag → blocked"), Meets(Req, NoTags));
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// AI sight sanitiser — AMythicAIController::SanitizePerception
// (lose-sight must be ≥ sight else instant target loss; angle clamped [0,180]; radii ≥ 0)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAISightSanitizeTest,
    "Mythic.AI.SanitizePerception",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAISightSanitizeTest::RunTest(const FString &Parameters) {
    // A sane config is left unchanged.
    {
        float S = 1500.0f, L = 2000.0f, A = 90.0f;
        AMythicAIController::SanitizePerception(S, L, A);
        TestEqual(TEXT("sane sight kept"), S, 1500.0f);
        TestEqual(TEXT("sane lose-sight kept"), L, 2000.0f);
        TestEqual(TEXT("sane angle kept"), A, 90.0f);
    }
    // Lose-sight below sight is bumped up to the sight radius (prevents sensed-then-instantly-lost).
    {
        float S = 1500.0f, L = 1000.0f, A = 90.0f;
        AMythicAIController::SanitizePerception(S, L, A);
        TestEqual(TEXT("lose-sight clamped up to sight"), L, 1500.0f);
    }
    // Peripheral angle clamped to [0,180].
    {
        float S = 1000.0f, L = 1000.0f, A = 270.0f;
        AMythicAIController::SanitizePerception(S, L, A);
        TestEqual(TEXT("over-wide angle clamped to 180"), A, 180.0f);
    }
    {
        float S = 1000.0f, L = 1000.0f, A = -30.0f;
        AMythicAIController::SanitizePerception(S, L, A);
        TestEqual(TEXT("negative angle clamped to 0"), A, 0.0f);
    }
    // Negative sight clamped to 0, and lose-sight stays ≥ sight.
    {
        float S = -100.0f, L = -50.0f, A = 90.0f;
        AMythicAIController::SanitizePerception(S, L, A);
        TestEqual(TEXT("negative sight clamped to 0"), S, 0.0f);
        TestTrue(TEXT("lose-sight ≥ sight after sanitise"), L >= S);
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// AI closest-hostile acquisition — AMythicAIController::SelectClosestHostileIndex
// (engage the nearest perceived hostile by squared distance; empty → none; ties → first)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAIClosestHostileTest,
    "Mythic.AI.ClosestHostile",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAIClosestHostileTest::RunTest(const FString &Parameters) {
    using A = AMythicAIController;

    // No candidates → INDEX_NONE.
    TestEqual(TEXT("empty → none"), A::SelectClosestHostileIndex({}), (int32)INDEX_NONE);

    // Single candidate → index 0.
    {
        const float D[] = {400.0f};
        TestEqual(TEXT("single → 0"), A::SelectClosestHostileIndex(D), 0);
    }
    // Nearest (smallest squared distance) wins, wherever it sits.
    {
        const float D[] = {900.0f, 100.0f, 625.0f}; // index 1 is closest
        TestEqual(TEXT("nearest wins"), A::SelectClosestHostileIndex(D), 1);
    }
    // Ties resolve to the first.
    {
        const float D[] = {250.0f, 250.0f, 800.0f};
        TestEqual(TEXT("tie → first"), A::SelectClosestHostileIndex(D), 0);
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// AI pursuit leash — AMythicAIController::ShouldReleaseLeash
// (drop the target when pulled beyond the leash range from the engage anchor; range <= 0 disables — infinite pursuit)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAILeashTest,
    "Mythic.AI.PursuitLeash",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAILeashTest::RunTest(const FString &Parameters) {
    using A = AMythicAIController;

    // Disabled (range 0 / negative) → never leashes, however far the NPC is pulled (prior infinite-pursuit default).
    TestFalse(TEXT("range 0 → no leash even far away"), A::ShouldReleaseLeash(/*distSq*/ 9.9e8f, /*rangeSq*/ 0.0f));
    TestFalse(TEXT("range negative → no leash"), A::ShouldReleaseLeash(9.9e8f, -1.0f));

    // Within the leash → stays engaged.
    TestFalse(TEXT("within leash → stay"), A::ShouldReleaseLeash(/*distSq*/ 10000.0f, /*rangeSq*/ 40000.0f));
    // Exactly at the boundary → stays (strict >).
    TestFalse(TEXT("exactly at leash → stay"), A::ShouldReleaseLeash(40000.0f, 40000.0f));
    // Beyond the leash → release.
    TestTrue(TEXT("beyond leash → release"), A::ShouldReleaseLeash(50000.0f, 40000.0f));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Fall-damage curve — AMythicCharacter::ComputeFallDamage
// (0 at/below the safe impact speed; linear above; capped at max; max<=0 = uncapped)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFallDamageTest,
    "Mythic.Traversal.FallDamage",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFallDamageTest::RunTest(const FString &Parameters) {
    auto Compute = &AMythicCharacter::ComputeFallDamage;

    // Below and at the safe impact speed → no damage.
    TestEqual(TEXT("below safe speed → 0"), Compute(1000.0f, 1200.0f, 0.05f, 100.0f), 0.0f);
    TestEqual(TEXT("exactly safe speed → 0"), Compute(1200.0f, 1200.0f, 0.05f, 100.0f), 0.0f);

    // Above safe → linear in the excess: (2000-1200)*0.05 = 40.
    TestEqual(TEXT("above safe → linear"), Compute(2000.0f, 1200.0f, 0.05f, 100.0f), 40.0f);

    // Capped: (5000-1200)*0.05 = 190 → clamped to MaxDamage 100.
    TestEqual(TEXT("damage capped at max"), Compute(5000.0f, 1200.0f, 0.05f, 100.0f), 100.0f);

    // MaxDamage <= 0 means uncapped → full 190.
    TestEqual(TEXT("max<=0 → uncapped"), Compute(5000.0f, 1200.0f, 0.05f, 0.0f), 190.0f);

    // A non-positive damage-per-speed is inert (a safety guard, never negative damage).
    TestEqual(TEXT("zero scale → 0"), Compute(5000.0f, 1200.0f, 0.0f, 100.0f), 0.0f);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Friendly-fire gate — UMythicDamageApplication::ShouldNegateFriendlyFire
// (negate ONLY a distinct player→player hit while FF is off; never self-damage; never any hit involving an NPC)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFriendlyFireGateTest,
    "Mythic.GAS.FriendlyFireGate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFriendlyFireGateTest::RunTest(const FString &Parameters) {
    auto FF = &UMythicDamageApplication::ShouldNegateFriendlyFire;
    // args: (bSourceIsPlayer, bTargetIsPlayer, bSameActor, bFriendlyFireEnabled)

    // Distinct player → player, FF OFF → NEGATE (the default co-op behaviour).
    TestTrue(TEXT("player→other-player, FF off → negate"), FF(true, true, false, false));
    // Same hit but FF ON → allowed.
    TestFalse(TEXT("player→other-player, FF on → allowed"), FF(true, true, false, true));
    // Self-damage (same actor) is NEVER negated, even FF off (fall damage / env hazard / self-DoT).
    TestFalse(TEXT("self-damage (same actor) → never negated"), FF(true, true, true, false));
    // PvE: any hit involving an NPC is never negated.
    TestFalse(TEXT("player→NPC → not negated"), FF(true, false, false, false));
    TestFalse(TEXT("NPC→player → not negated"), FF(false, true, false, false));
    TestFalse(TEXT("NPC→NPC → not negated"), FF(false, false, false, false));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Canonical player key — AMythicPlayerState::ResolveCanonicalPlayerKey
// (persistent CharacterID wins when set; else a session-stable "session:<id>" fallback — the cross-session key the
//  party/companion + player-registry systems address a player by)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCanonicalPlayerKeyTest,
    "Mythic.Player.CanonicalPlayerKey",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCanonicalPlayerKeyTest::RunTest(const FString &Parameters) {
    using PS = AMythicPlayerState;

    // A loaded character → its persistent save-slot CharacterID is the key (stable across sessions; session id ignored).
    TestEqual(TEXT("persistent id wins"), PS::ResolveCanonicalPlayerKey(TEXT("char-guid-abc"), 256), FString(TEXT("char-guid-abc")));
    // No persistent id yet → session-stable fallback, distinct per session player id.
    TestEqual(TEXT("empty → session fallback"), PS::ResolveCanonicalPlayerKey(TEXT(""), 256), FString(TEXT("session:256")));
    TestEqual(TEXT("empty → session fallback (other id)"), PS::ResolveCanonicalPlayerKey(TEXT(""), 7), FString(TEXT("session:7")));
    // Two unsaved players get DIFFERENT keys (no collision within a session).
    TestNotEqual(TEXT("distinct sessions → distinct keys"),
                 PS::ResolveCanonicalPlayerKey(TEXT(""), 1), PS::ResolveCanonicalPlayerKey(TEXT(""), 2));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Player registry resolution — UMythicPlayerRegistrySubsystem::ResolveRegisteredKey
// (canonical key → live PlayerState; empty key / no entry / stale weak ref → nullptr)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPlayerRegistryResolveTest,
    "Mythic.Player.PlayerRegistryResolve",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPlayerRegistryResolveTest::RunTest(const FString &Parameters) {
    using R = UMythicPlayerRegistrySubsystem;

    AMythicPlayerState *PS = NewObject<AMythicPlayerState>();
    TestNotNull(TEXT("constructed a PlayerState (test prerequisite)"), PS);

    TMap<FString, TWeakObjectPtr<AMythicPlayerState>> Map;
    Map.Add(TEXT("char-guid-1"), PS);

    // Registered key resolves to its PlayerState.
    TestTrue(TEXT("registered key resolves"), R::ResolveRegisteredKey(Map, TEXT("char-guid-1")) == PS);
    // Unregistered key → nullptr.
    TestTrue(TEXT("unregistered key → null"), R::ResolveRegisteredKey(Map, TEXT("char-guid-2")) == nullptr);
    // Empty key → nullptr (never resolves the empty string).
    TestTrue(TEXT("empty key → null"), R::ResolveRegisteredKey(Map, TEXT("")) == nullptr);
    // A stale / null weak entry → nullptr (departed player without unregister).
    Map.Add(TEXT("stale"), TWeakObjectPtr<AMythicPlayerState>(nullptr));
    TestTrue(TEXT("stale weak entry → null"), R::ResolveRegisteredKey(Map, TEXT("stale")) == nullptr);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// NPC schedule — UMythicScheduleTransitionProcessor::GetPhaseForHour + ComputeStaggeredHour
// (phase-by-hour boundaries + the per-NPC stagger that breaks lockstep town routines)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSchedulePhaseStaggerTest,
    "Mythic.LivingWorld.Schedule.PhaseAndStagger",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSchedulePhaseStaggerTest::RunTest(const FString &Parameters) {
    using P = UMythicScheduleTransitionProcessor;

    // Null settings → Rest (safe default).
    TestTrue(TEXT("null settings → Rest"), P::GetPhaseForHour(10.0f, nullptr) == EMythicSchedulePhase::Rest);

    UMythicLivingWorldSettings *Settings = NewObject<UMythicLivingWorldSettings>();
    Settings->ScheduleDayStartHour = 6.0f;
    Settings->ScheduleWorkStartHour = 8.0f;
    Settings->ScheduleWorkEndHour = 14.0f;
    Settings->ScheduleSocialStartHour = 15.0f;
    Settings->ScheduleSocialEndHour = 18.0f;
    Settings->ScheduleDayEndHour = 19.0f;

    TestTrue(TEXT("hour 3 (pre-dawn) → Rest"), P::GetPhaseForHour(3.0f, Settings) == EMythicSchedulePhase::Rest);
    TestTrue(TEXT("hour 7 (dawn commute) → Travel"), P::GetPhaseForHour(7.0f, Settings) == EMythicSchedulePhase::Travel);
    TestTrue(TEXT("hour 8 (work start, inclusive) → Work"), P::GetPhaseForHour(8.0f, Settings) == EMythicSchedulePhase::Work);
    TestTrue(TEXT("hour 10 → Work"), P::GetPhaseForHour(10.0f, Settings) == EMythicSchedulePhase::Work);
    TestTrue(TEXT("hour 14.5 (work→social) → Travel"), P::GetPhaseForHour(14.5f, Settings) == EMythicSchedulePhase::Travel);
    TestTrue(TEXT("hour 16 → Social"), P::GetPhaseForHour(16.0f, Settings) == EMythicSchedulePhase::Social);
    TestTrue(TEXT("hour 18.5 (social→home) → Travel"), P::GetPhaseForHour(18.5f, Settings) == EMythicSchedulePhase::Travel);
    TestTrue(TEXT("hour 22 (night) → Rest"), P::GetPhaseForHour(22.0f, Settings) == EMythicSchedulePhase::Rest);

    // ComputeStaggeredHour: disabled (0) → unchanged.
    TestEqual(TEXT("stagger 0 → unchanged"), P::ComputeStaggeredHour(12.0f, 12345u, 0.0f), 12.0f);
    // Deterministic for a given NameHash.
    TestEqual(TEXT("deterministic per NameHash"),
              P::ComputeStaggeredHour(12.0f, 777u, 2.0f), P::ComputeStaggeredHour(12.0f, 777u, 2.0f));
    // Always wraps into [0,24), including near the day boundaries.
    const uint32 Hashes[] = {0u, 1u, 7u, 99u, 4242u, 100002u, 999999u};
    const float Hours[] = {0.0f, 0.5f, 12.0f, 23.5f};
    for (uint32 H : Hashes) {
        for (float Hour : Hours) {
            const float R = P::ComputeStaggeredHour(Hour, H, 2.0f);
            TestTrue(TEXT("staggered hour stays in [0,24)"), R >= 0.0f && R < 24.0f);
        }
    }
    // A mid-day hour (away from wrap) stays within ±MaxStagger.
    TestTrue(TEXT("mid-day stagger within ±2h"),
             FMath::Abs(P::ComputeStaggeredHour(12.0f, 4242u, 2.0f) - 12.0f) <= 2.0f + KINDA_SMALL_NUMBER);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Combat probability roll — MythicCombat::RollSucceeds
// (shared boundary rule for crit/status procs, dodge, and resistance survival in the damage executions)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCombatRollSucceedsTest,
    "Mythic.GAS.Damage.RollSucceeds",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCombatRollSucceedsTest::RunTest(const FString &Parameters) {
    using MythicCombat::RollSucceeds;

    // 0% (and negative) probability NEVER succeeds — even on an exact-0.0 roll (the bug the lower-bound guard fixes).
    TestFalse(TEXT("0% never procs at roll 0.0"), RollSucceeds(0.0f, 0.0f));
    TestFalse(TEXT("0% never procs mid-roll"), RollSucceeds(0.0f, 0.5f));
    TestFalse(TEXT("negative probability never procs"), RollSucceeds(-0.25f, 0.0f));

    // 100% ALWAYS succeeds — even on an exact-1.0 roll (FRand() can return 1.0, so the comparison is `<=`, not `<`).
    TestTrue(TEXT("100% procs at roll 1.0"), RollSucceeds(1.0f, 1.0f));
    TestTrue(TEXT("100% procs at roll 0.0"), RollSucceeds(1.0f, 0.0f));

    // Boundary: roll == probability succeeds (inclusive); just above fails; just below succeeds.
    TestTrue(TEXT("roll == chance succeeds"), RollSucceeds(0.5f, 0.5f));
    TestFalse(TEXT("roll just above chance fails"), RollSucceeds(0.5f, 0.5001f));
    TestTrue(TEXT("roll just below chance succeeds"), RollSucceeds(0.5f, 0.4999f));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Territory proxy delta gate — AMythicLivingWorldReplicator::TerritoryProxyNeedsUpdate
// (only re-replicate a cell when its client-visible fields change, not on every influence shift)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldTerritoryProxyDeltaTest,
    "Mythic.LivingWorld.Replication.TerritoryProxyDelta",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldTerritoryProxyDeltaTest::RunTest(const FString &Parameters) {
    FMythicTerritoryProxyItem Proxy;
    Proxy.ControllingFaction.Index = 2;
    Proxy.ContestedLevel = 0;

    FMythicFactionId Faction2;
    Faction2.Index = 2;
    FMythicFactionId Faction3;
    Faction3.Index = 3;

    // Same dominant faction + contested → NO re-replication (the influence-shift-only case the fix now skips).
    TestFalse(TEXT("unchanged dominant + contested → no update (delta skip)"),
              AMythicLivingWorldReplicator::TerritoryProxyNeedsUpdate(Proxy, Faction2, 0));
    // Dominant faction flipped → must replicate.
    TestTrue(TEXT("dominant-faction flip → update"),
             AMythicLivingWorldReplicator::TerritoryProxyNeedsUpdate(Proxy, Faction3, 0));
    // Contested level changed (same faction) → must replicate (future-proofs the contested field).
    TestTrue(TEXT("contested-level change → update"),
             AMythicLivingWorldReplicator::TerritoryProxyNeedsUpdate(Proxy, Faction2, 5));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Significance proximity score — UMythicSignificanceProcessor::ComputeProximityScore
// (core LOD-promotion proximity math; nearest-player, normalized, clamped)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldProximityScoreTest,
    "Mythic.LivingWorld.Significance.ProximityScore",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldProximityScoreTest::RunTest(const FString &Parameters) {
    const float R = 10.0f;
    auto Prox = &UMythicSignificanceProcessor::ComputeProximityScore;

    // No players → 0 (nothing to be near).
    {
        TArray<FMythicCellCoord> NoPlayers;
        TestEqual(TEXT("no players → 0"), Prox(FMythicCellCoord(5, 5), NoPlayers, R), 0.0f);
    }

    TArray<FMythicCellCoord> P1;
    P1.Add(FMythicCellCoord(0, 0));
    TestEqual(TEXT("at the player's cell → 1.0"), Prox(FMythicCellCoord(0, 0), P1, R), 1.0f);
    TestEqual(TEXT("Manhattan dist 5 of radius 10 → 0.5"), Prox(FMythicCellCoord(5, 0), P1, R), 0.5f);
    TestEqual(TEXT("at the radius → 0"), Prox(FMythicCellCoord(10, 0), P1, R), 0.0f);
    TestEqual(TEXT("beyond the radius → clamped to 0"), Prox(FMythicCellCoord(20, 0), P1, R), 0.0f);

    // Multiple players → the NEAREST wins (max over players).
    {
        TArray<FMythicCellCoord> P2;
        P2.Add(FMythicCellCoord(100, 100)); // far
        P2.Add(FMythicCellCoord(0, 0));     // near (dist 3 → 0.7)
        TestEqual(TEXT("nearest player wins (max over players)"),
                  Prox(FMythicCellCoord(3, 0), P2, R), 0.7f);
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Significance rescore gate — UMythicSignificanceProcessor::ShouldRescore
// (LOD: hot-set always re-evaluated so embodied NPCs demote → no cognitive-slot leak)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldRescoreGateTest,
    "Mythic.LivingWorld.Significance.RescoreGate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldRescoreGateTest::RunTest(const FString &Parameters) {
    using Tier = EMythicSignificanceTier;

    // Ambient (Tier0) is event-driven: rescored only when dirty.
    TestFalse(TEXT("ambient + clean → skip"),
              UMythicSignificanceProcessor::ShouldRescore(false, Tier::Tier0_Ambient));
    TestTrue(TEXT("ambient + dirty → rescore"),
             UMythicSignificanceProcessor::ShouldRescore(true, Tier::Tier0_Ambient));

    // Hydrated hot-set is ALWAYS re-evaluated (even when clean) — this is the slot-leak fix: a stationary embodied
    // NPC re-scores each tick so it demotes when players leave.
    TestTrue(TEXT("Tier1 clean → always rescore"),
             UMythicSignificanceProcessor::ShouldRescore(false, Tier::Tier1_Reactive));
    TestTrue(TEXT("Tier2 clean → always rescore"),
             UMythicSignificanceProcessor::ShouldRescore(false, Tier::Tier2_Cognitive));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Despair state machine — UMythicPressureProcessor::ComputeDespairState
// (bDespaired was set-once-never-reset → permanent despair; now recoverable w/ hysteresis)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldDespairStateTest,
    "Mythic.LivingWorld.Pressure.DespairState",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldDespairStateTest::RunTest(const FString &Parameters) {
    const float Thr = 10.0f; // recovery band = 7.5 (0.75 * threshold)

    // Enter despair at/above the threshold.
    TestTrue(TEXT("enters despair at the threshold"),
             UMythicPressureProcessor::ComputeDespairState(10.0f, Thr, false));
    TestFalse(TEXT("no despair below the threshold (not previously despaired)"),
              UMythicPressureProcessor::ComputeDespairState(9.9f, Thr, false));

    // Hysteresis: once despaired, stays despaired within the band [7.5, threshold)...
    TestTrue(TEXT("stays despaired within the hysteresis band"),
             UMythicPressureProcessor::ComputeDespairState(8.0f, Thr, true));
    // ...and recovers once pressure drops below the band (the fix: previously it could NEVER recover).
    TestFalse(TEXT("recovers once pressure drops below the band"),
              UMythicPressureProcessor::ComputeDespairState(7.0f, Thr, true));
    // In the band but NOT previously despaired → does not spuriously enter (asymmetric enter/stay).
    TestFalse(TEXT("in-band but not previously despaired → not despaired"),
              UMythicPressureProcessor::ComputeDespairState(8.0f, Thr, false));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Diplomacy score→relation hysteresis mapping + shift significance — FMythicWorldSimThread
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldDiplomacyMappingTest,
    "Mythic.LivingWorld.Simulation.DiplomacyMapping",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldDiplomacyMappingTest::RunTest(const FString &Parameters) {
    using R = EMythicFactionRelation;
    const float Ally = 0.5f, Friendly = 0.2f, Unfriendly = -0.2f, Hostile = -0.5f, Hyst = 0.1f;
    auto Map = [&](float S, R C) {
        return FMythicWorldSimThread::MapDiplomacyScoreToRelation(S, C, Ally, Friendly, Unfriendly, Hostile, Hyst);
    };

    // Entering a tier requires overshooting its threshold by Hyst.
    TestEqual(TEXT("score above ally+hyst → Allied"), (int32)Map(0.65f, R::Neutral), (int32)R::Allied);
    TestEqual(TEXT("score below hostile-hyst → Hostile"), (int32)Map(-0.65f, R::Neutral), (int32)R::Hostile);
    TestEqual(TEXT("score ~0 → Neutral"), (int32)Map(0.0f, R::Neutral), (int32)R::Neutral);

    // In the [base, base+hyst) band a NON-current tier is NOT entered (steps down one tier instead)...
    TestEqual(TEXT("score in [ally, ally+hyst), not Allied → Friendly"), (int32)Map(0.55f, R::Neutral), (int32)R::Friendly);
    TestEqual(TEXT("score in (hostile-hyst, ...], not Hostile → Unfriendly"),
              (int32)Map(-0.55f, R::Neutral), (int32)R::Unfriendly);
    // ...but a faction already in that tier STAYS (sticky — hysteresis prevents flicker).
    TestEqual(TEXT("score in [ally, ally+hyst), currently Allied → stays Allied"),
              (int32)Map(0.55f, R::Allied), (int32)R::Allied);
    TestEqual(TEXT("score below hostile (base), currently Hostile → stays Hostile"),
              (int32)Map(-0.55f, R::Hostile), (int32)R::Hostile);

    // Shift significance scales with extremity: war/alliance >> minor shift > neutral.
    TestTrue(TEXT("war (Hostile) is a major shift"),
             FMythicWorldSimThread::DiplomacyShiftSignificance(R::Hostile) >= 0.8f);
    TestTrue(TEXT("alliance (Allied) is a major shift"),
             FMythicWorldSimThread::DiplomacyShiftSignificance(R::Allied) >= 0.8f);
    TestTrue(TEXT("war outranks a friendly shift"),
             FMythicWorldSimThread::DiplomacyShiftSignificance(R::Hostile) >
                 FMythicWorldSimThread::DiplomacyShiftSignificance(R::Friendly));
    TestTrue(TEXT("a friendly shift outranks a return-to-neutral"),
             FMythicWorldSimThread::DiplomacyShiftSignificance(R::Friendly) >
                 FMythicWorldSimThread::DiplomacyShiftSignificance(R::Neutral));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Item stack-quantity creation clamp — UMythicItemInstance::ClampInitialStackQuantity
// (Initialize assigned quantity raw — could over-stack past the cap or create zero/negative)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldItemStackClampTest,
    "Mythic.Itemization.ItemInstance.ClampInitialStackQuantity",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldItemStackClampTest::RunTest(const FString &Parameters) {
    // Over the stack cap → clamped to the cap (the bug: Initialize created over-cap stacks).
    TestEqual(TEXT("over-max clamps to the stack cap"),
              UMythicItemInstance::ClampInitialStackQuantity(999, 10), 10);
    // Zero / negative → clamped to 1 (Quantity ClampMin contract).
    TestEqual(TEXT("zero clamps to 1"), UMythicItemInstance::ClampInitialStackQuantity(0, 10), 1);
    TestEqual(TEXT("negative clamps to 1"), UMythicItemInstance::ClampInitialStackQuantity(-5, 10), 1);
    // Valid in-range quantity passes through.
    TestEqual(TEXT("valid quantity passes through"), UMythicItemInstance::ClampInitialStackQuantity(5, 10), 5);
    // Boundary at the cap is preserved.
    TestEqual(TEXT("exact cap preserved"), UMythicItemInstance::ClampInitialStackQuantity(10, 10), 10);
    // Non-stackable item (StackSizeMax <= 1) is always a single unit, regardless of requested quantity.
    TestEqual(TEXT("non-stackable → 1"), UMythicItemInstance::ClampInitialStackQuantity(7, 1), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldResistanceRestorationTest,
    "Mythic.LivingWorld.Phase1.FactionDatabase.ResistanceRestoration",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldResistanceRestorationTest::RunTest(const FString &Parameters) {
    auto *DB = NewObject<UMythicFactionDatabase>();
    DB->Initialize(LivingWorldTestHelpers::CreateFactionSettings(20, 3));

    // Form a resistance from faction 0's survivors (writes to the write buffer — read it back the same way).
    const FMythicFactionId ResistanceId = DB->CreateFactionFromConquest(LivingWorldTestHelpers::MakeFactionId(0), 25);
    TestTrue(TEXT("resistance faction created"), ResistanceId.IsValid());

    FMythicFactionData *Res = DB->GetFactionMutableByIndex(ResistanceId.Index);
    TestNotNull(TEXT("resistance retrievable"), Res);
    if (Res) {
        TestEqual(TEXT("new faction starts in Resistance status"),
                  static_cast<int32>(Res->Status), static_cast<int32>(EMythicFactionStatus::Resistance));
    }

    // Restore it → should become a full, Active faction (the previously-impossible transition).
    DB->RestoreResistanceToFaction(ResistanceId);
    Res = DB->GetFactionMutableByIndex(ResistanceId.Index);
    if (Res) {
        TestEqual(TEXT("restored resistance is now Active"),
                  static_cast<int32>(Res->Status), static_cast<int32>(EMythicFactionStatus::Active));
    }

    // No-op guard: restoring a non-resistance faction must not change its status.
    if (FMythicFactionData *Active = DB->GetFactionMutableByIndex(1)) {
        const EMythicFactionStatus Before = Active->Status;
        DB->RestoreResistanceToFaction(LivingWorldTestHelpers::MakeFactionId(1));
        TestEqual(TEXT("restoring a non-resistance faction is a no-op"),
                  static_cast<int32>(Active->Status), static_cast<int32>(Before));
    }

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

// ─── Edge decay (the relationship-aging keystone — exponential, time-based) ──────────────────────────────────────────

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLivingWorldSocialGraphDecayTest,
    "Mythic.LivingWorld.SocialGraph.ApplyDecay",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLivingWorldSocialGraphDecayTest::RunTest(const FString &Parameters) {
    using SG = UMythicSocialGraph;

    // No decay when the rate is non-positive → strength unchanged.
    {
        FMythicSocialEdge E;
        E.Strength = 0.8f;
        E.LastInteractionTime = 0.0;
        TestEqual(TEXT("rate 0 → unchanged"), SG::ApplyDecay(E, 100.0, 0.0f), 0.8f);
    }
    // No decay when no time has elapsed (WorldTime <= LastInteractionTime) — equal or past clock.
    {
        FMythicSocialEdge E;
        E.Strength = 0.8f;
        E.LastInteractionTime = 100.0;
        TestEqual(TEXT("equal time → unchanged"), SG::ApplyDecay(E, 100.0, 0.01f), 0.8f);
        TestEqual(TEXT("worldtime before last → unchanged"), SG::ApplyDecay(E, 50.0, 0.01f), 0.8f);
    }
    // Exponential decay over elapsed time: S × e^(-rate·Δt).
    {
        FMythicSocialEdge E;
        E.Strength = 1.0f;
        E.LastInteractionTime = 0.0;
        // rate 0.01 × 100 = 1.0 → e^-1 ≈ 0.367879.
        TestEqual(TEXT("rate·Δt=1 → S·e^-1"), SG::ApplyDecay(E, 100.0, 0.01f), 0.367879f, 0.001f);
        // Decay never increases strength.
        TestTrue(TEXT("decayed ≤ original"), SG::ApplyDecay(E, 100.0, 0.01f) <= E.Strength);
    }
    {
        FMythicSocialEdge E;
        E.Strength = 0.5f;
        E.LastInteractionTime = 0.0;
        // rate 0.001 × 100 = 0.1 → 0.5 × e^-0.1 ≈ 0.452419.
        TestEqual(TEXT("rate·Δt=0.1 → S·e^-0.1"), SG::ApplyDecay(E, 100.0, 0.001f), 0.452419f, 0.001f);
    }

    return true;
}

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

    // Tick with SimDeltaTime = 0 to isolate GENERATION: TickSchemes generates AND then progresses in the same call,
    // and ProgressScheme's detection roll (FRand() < DetectionRisk * SimDeltaTime) could otherwise flip a freshly
    // generated scheme to Discovered on its own birth tick (~1%), making the IsActive() assertion below flaky.
    // Generation is SimDeltaTime-independent (gate uses SchemeBaseProbability, not dt), so dt=0 generates normally
    // while the detection roll (× 0) can never fire — deterministic generation, no incidental same-tick discovery.
    Engine->TickSchemes(0.0f, 0);

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
