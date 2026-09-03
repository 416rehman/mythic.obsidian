#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/ScopeExit.h"
#include "UObject/UObjectIterator.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Rewards/LootReward.h"
#include "Rewards/LootScaling.h"
#include "Tests/Itemization/MythicPreLootRollTestTypes.h"

namespace {

// Kill loot is rolled per crediting controller through ULootReward::Give, which reads the controller's ability
// system for level and find, the game state for the rarity curves and the loot manager for the drops. The Mythic
// controller and player state are what the life component credits, so spawning them is the honest fixture.
struct FMythicPreLootRollFixture {
    UGameInstance *GameInstance = nullptr;
    UWorld *World = nullptr;
    UMythicLootManagerSubsystem *LootManager = nullptr;
};

bool BuildPreLootRollFixture(FAutomationTestBase &Test, FMythicPreLootRollFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    Out.World = Out.GameInstance->GetWorld();
    if (!Test.TestNotNull(TEXT("standalone world exists"), Out.World)) {
        return false;
    }
    AMythicGameState *GameState = Out.World->SpawnActor<AMythicPreLootRollTestGameState>();
    if (!Test.TestNotNull(TEXT("the game state spawned"), GameState)) {
        return false;
    }
    // The standalone world never initialises its actors, so the game state is installed by hand.
    Out.World->SetGameState(GameState);
    Out.LootManager = Out.GameInstance->GetSubsystem<UMythicLootManagerSubsystem>();
    return Test.TestNotNull(TEXT("the loot manager exists on the authority game instance"), Out.LootManager);
}

// A crediting player: a Mythic controller with its own player state, whose ability system carries every set the
// loot roll reads. Find is pinned to zero so the expected drop counts are exact.
AMythicPlayerController *SpawnPreLootRollPlayer(FAutomationTestBase &Test, UWorld *World) {
    AMythicPlayerController *PC = World->SpawnActor<AMythicPlayerController>();
    AMythicPlayerState *PS = World->SpawnActor<AMythicPlayerState>();
    if (!Test.TestNotNull(TEXT("the crediting controller spawned"), PC)
        || !Test.TestNotNull(TEXT("the crediting player state spawned"), PS)) {
        return nullptr;
    }
    PC->PlayerState = PS;
    UMythicAbilitySystemComponent *ASC = PS->GetMythicAbilitySystemComponent();
    if (!Test.TestNotNull(TEXT("the player state owns an ability system"), ASC)) {
        return nullptr;
    }
    if (!ASC->IsRegistered()) {
        ASC->RegisterComponent();
    }
    // A live world registers the default-subobject attribute sets in InitializeComponent; the test world never calls it.
    if (!ASC->HasBeenInitialized()) {
        ASC->InitializeComponent();
    }
    ASC->InitAbilityActorInfo(PS, PS);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetItemRarityFindAttribute(), 0.0f);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Utility::GetItemQuantityFindAttribute(), 0.0f);
    return PC;
}

// A table whose every row passes and whose own roll is always one, so the count paid is the count the bonus asked for.
UMythicLootTable *MakePreLootRollTable(UObject *Outer, TArray<UItemDefinition *> &OutItems, const int32 Rows) {
    UMythicLootTable *Table = NewObject<UMythicLootTable>(Outer);
    Table->DropChance = 1.0f;
    Table->MaxItems = 1;
    for (int32 Row = 0; Row < Rows; Row++) {
        UItemDefinition *Item = NewObject<UItemDefinition>(Outer);
        Item->StackSizeMax = 1;
        FLootTableEntry &Entry = Table->Entries.AddDefaulted_GetRef();
        Entry.Item = Item;
        Entry.OverrideDropChance = 1.0f;
        OutItems.Add(Item);
    }
    return Table;
}

// Every drop the roll paid became an item instance of one of the table's definitions, whatever became of the actor.
int32 CountPreLootRollItems(const TArray<UItemDefinition *> &Items) {
    int32 Count = 0;
    for (TObjectIterator<UMythicItemInstance> It; It; ++It) {
        if (IsValid(*It) && Items.Contains(It->GetItemDefinition())) {
            Count++;
        }
    }
    return Count;
}

// What the life component does for one crediting controller of a slain enemy: a private reward at the corpse.
void GivePreLootRollLoot(UObject *Outer, APlayerController *PC, const TArray<UMythicLootTable *> &Tables, const int32 EnemyTierInt) {
    ULootReward *Reward = NewObject<ULootReward>(Outer);
    Reward->OverridenLootSource.LootTables = Tables;
    Reward->OverridenLootSource.IsPrivate = true;
    Reward->OverridenLootSource.bSkipGlobal = true;
    FLootRewardContext Ctx;
    Ctx.PlayerController = PC;
    Ctx.PutInInventory = nullptr;
    Ctx.SpawnLocation = FVector(0.0, 0.0, 200.0);
    Ctx.EnemyTierInt = EnemyTierInt;
    Ctx.ItemLevel = 1;
    Reward->Give(Ctx);
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPreLootRollDropCountScaleTest,
    "Mythic.Itemization.PreLootRoll.DropCountScale",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPreLootRollDropCountScaleTest::RunTest(const FString &Parameters) {
    FLootTierBonus Bonus;
    TestEqual(TEXT("a fresh bonus scales by one"), Bonus.DropCountScale, 1.0f);
    TestEqual(TEXT("the default scale leaves the table's roll alone"), FMythicLootScaling::ResolveDropCount(2, Bonus, 1.0f), 2);

    Bonus.ExtraDropCount = 3;
    TestEqual(TEXT("extra drops add to the roll"), FMythicLootScaling::ResolveDropCount(1, Bonus, 1.0f), 4);

    Bonus.FractionalDropChance = 0.5f;
    TestEqual(TEXT("a fractional roll under the chance adds one more"), FMythicLootScaling::ResolveDropCount(1, Bonus, 0.25f), 5);
    TestEqual(TEXT("a fractional roll over the chance adds none"), FMythicLootScaling::ResolveDropCount(1, Bonus, 0.75f), 4);

    Bonus.DropCountScale = 0.0f;
    TestEqual(TEXT("DropCountScale 0 pays nothing, extras included"), FMythicLootScaling::ResolveDropCount(1, Bonus, 0.25f), 0);

    Bonus.DropCountScale = 0.5f;
    TestEqual(TEXT("a partial scale floors"), FMythicLootScaling::ResolveDropCount(1, Bonus, 0.75f), 2);

    Bonus.DropCountScale = -1.0f;
    TestEqual(TEXT("a negative scale is a zero scale"), FMythicLootScaling::ResolveDropCount(1, Bonus, 0.75f), 0);

    const FLootTierBonus Boss = FMythicLootScaling::ComputeTierLootBonus(5, 0.0f);
    TestEqual(TEXT("the tier bonus starts every credit at scale one"), Boss.DropCountScale, 1.0f);
    TestEqual(TEXT("a boss credit keeps its extra drops through the scale"), FMythicLootScaling::ResolveDropCount(1, Boss, 1.0f), 4);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPreLootRollOncePerControllerTest,
    "Mythic.Itemization.PreLootRoll.OncePerCreditingController",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPreLootRollOncePerControllerTest::RunTest(const FString &Parameters) {
    FMythicPreLootRollFixture Fixture;
    const bool bReady = BuildPreLootRollFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    AMythicPlayerController *Killer = SpawnPreLootRollPlayer(*this, Fixture.World);
    AMythicPlayerController *Partner = SpawnPreLootRollPlayer(*this, Fixture.World);
    if (!Killer || !Partner) {
        return false;
    }

    TMap<APlayerController *, int32> Raised;
    const FDelegateHandle Listener = Fixture.LootManager->OnPreLootRoll.AddLambda(
        [&Raised](APlayerController *PC, FLootTierBonus &) { Raised.FindOrAdd(PC)++; });
    ON_SCOPE_EXIT {
        Fixture.LootManager->OnPreLootRoll.Remove(Listener);
    };

    TArray<UItemDefinition *> Items;
    UMythicLootTable *First = MakePreLootRollTable(Fixture.GameInstance, Items, 2);
    UMythicLootTable *Second = MakePreLootRollTable(Fixture.GameInstance, Items, 2);

    // A shared kill credits each eligible controller with its own private roll; the killer's victim owns two tables.
    GivePreLootRollLoot(Fixture.GameInstance, Killer, {First, Second}, 1);
    GivePreLootRollLoot(Fixture.GameInstance, Partner, {First}, 1);
    TestEqual(TEXT("the killer's roll fired once even though the victim rolls two tables"), Raised.FindRef(Killer), 1);
    TestEqual(TEXT("the partner's roll fired once"), Raised.FindRef(Partner), 1);
    TestEqual(TEXT("nobody else was asked"), Raised.Num(), 2);

    // A quest, chest or bounty reward carries no slain enemy and never asks.
    GivePreLootRollLoot(Fixture.GameInstance, Killer, {First}, 0);
    TestEqual(TEXT("a reward without a slain enemy raises no pre-loot roll"), Raised.FindRef(Killer), 1);

    // The seam itself: no manager means no listener, and the tier bonus still comes back whole.
    const FLootTierBonus Plain = ULootReward::PrepareLootRoll(nullptr, Killer, 5, 0.0f);
    TestEqual(TEXT("a boss credit without a manager still carries the tier's extra drops"), Plain.ExtraDropCount, 3);
    TestEqual(TEXT("and starts at scale one"), Plain.DropCountScale, 1.0f);

    AddInfo(FString::Printf(TEXT("pre-loot rolls raised: killer %d, partner %d over 3 rewards (2 kills, 1 quest-shaped)"),
                            Raised.FindRef(Killer), Raised.FindRef(Partner)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPreLootRollShapesDropsTest,
    "Mythic.Itemization.PreLootRoll.ShapesTheDropCount",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPreLootRollShapesDropsTest::RunTest(const FString &Parameters) {
    FMythicPreLootRollFixture Fixture;
    const bool bReady = BuildPreLootRollFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    AMythicPlayerController *Killer = SpawnPreLootRollPlayer(*this, Fixture.World);
    if (!Killer) {
        return false;
    }

    TArray<UItemDefinition *> Items;
    UMythicLootTable *Table = MakePreLootRollTable(Fixture.GameInstance, Items, 4);

    // The denominator: with nobody listening a normal kill pays the table's single roll.
    GivePreLootRollLoot(Fixture.GameInstance, Killer, {Table}, 1);
    const int32 Baseline = CountPreLootRollItems(Items);
    if (!TestEqual(TEXT("a normal kill with nobody listening pays the table's one roll"), Baseline, 1)) {
        return false;
    }

    // Extra drops asked for in the delegate reach the count.
    FDelegateHandle Listener = Fixture.LootManager->OnPreLootRoll.AddLambda(
        [](APlayerController *, FLootTierBonus &Bonus) { Bonus.ExtraDropCount += 2; });
    GivePreLootRollLoot(Fixture.GameInstance, Killer, {Table}, 1);
    const int32 AfterExtra = CountPreLootRollItems(Items);
    TestEqual(TEXT("two extra drops added in the delegate raise the kill's count to three"), AfterExtra - Baseline, 3);
    Fixture.LootManager->OnPreLootRoll.Remove(Listener);

    // DropCountScale 0 pays nothing, whatever the tier and table would have paid.
    Listener = Fixture.LootManager->OnPreLootRoll.AddLambda(
        [](APlayerController *, FLootTierBonus &Bonus) { Bonus.DropCountScale = 0.0f; });
    GivePreLootRollLoot(Fixture.GameInstance, Killer, {Table}, 5);
    const int32 AfterSuppressed = CountPreLootRollItems(Items);
    TestEqual(TEXT("DropCountScale 0 yields no drops, even from a boss"), AfterSuppressed - AfterExtra, 0);
    Fixture.LootManager->OnPreLootRoll.Remove(Listener);

    // The scale lives on one credit's bonus, so the next kill with nobody listening pays again.
    GivePreLootRollLoot(Fixture.GameInstance, Killer, {Table}, 1);
    const int32 AfterPlain = CountPreLootRollItems(Items);
    TestEqual(TEXT("the suppression did not outlive its credit"), AfterPlain - AfterSuppressed, 1);

    AddInfo(FString::Printf(TEXT("drops paid: baseline %d, +2 extra -> %d, scale 0 -> %d, plain again -> %d"),
                            Baseline, AfterExtra - Baseline, AfterSuppressed - AfterExtra, AfterPlain - AfterSuppressed));
    return true;
}
