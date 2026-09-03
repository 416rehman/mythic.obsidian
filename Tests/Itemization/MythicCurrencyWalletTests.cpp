#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "Itemization/Inventory/InventoryProfile.h"
#include "Itemization/Inventory/InventorySlotDefinition.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Misc/ScopeExit.h"
#include "Player/MythicPlayerState.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Tests/Itemization/MythicCurrencyWalletTestTypes.h"
#include "UI/HUD/MythicHudNotice.h"
#include "UObject/StrongObjectPtr.h"
#include "World/Entity/MythicEntityId.h"

namespace {

// The wallet is whatever currency stacks the controller's inventories hold, every debit is authority-gated on the
// controller, and the spent event is raised on the player state's ability system. All three must be live.
struct FMythicWalletFixture {
    UGameInstance *GameInstance = nullptr;
    AMythicCurrencyWalletTestController *Controller = nullptr;
    AMythicPlayerState *PlayerState = nullptr;
    UMythicAbilitySystemComponent *ASC = nullptr;
    UItemDefinition *Coin = nullptr;
    // A dynamic delegate holds only a weak reference, so an unrooted listener is dropped by any GC between the
    // bind and the broadcast and the notice silently reaches nobody. Keep it alive for the whole fixture.
    TStrongObjectPtr<UMythicCurrencyWalletNoticeListener> Notices;
    int32 SpentEvents = 0;
    float LastSpentMagnitude = 0.0f;

    UMythicInventoryComponent *FirstPurse() const { return Controller->GetAllInventoryComponents()[0]; }
    UMythicInventoryComponent *SecondPurse() const { return Controller->SecondInventory; }
};

UInventoryProfile *MakeCarriedProfile(UObject *Outer, int32 SlotCount) {
    UInventoryProfile *Profile = NewObject<UInventoryProfile>(Outer);
    FInventoryProfileEntry Entry;
    Entry.SlotDefinition = NewObject<UInventorySlotDefinition>(Profile);
    Entry.Count = SlotCount;
    FInventorySlotGroup Group;
    Group.SlotDomain = EMythicInventorySlotDomain::Carried;
    Group.Slots.Add(Entry);
    Profile->SlotGroups.Add(FGameplayTag(), Group);
    return Profile;
}

bool BuildWalletFixture(FAutomationTestBase &Test, FMythicWalletFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    UWorld *World = Out.GameInstance->GetWorld();
    if (!Test.TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    // AActor::ProcessEvent drops every UFUNCTION call - Client RPCs included - on a world whose actors were never
    // initialised, silently and with no error. Without this the charge's HUD notice never leaves the controller.
    World->InitializeActorsForPlay(FURL());
    if (!Test.TestTrue(TEXT("the world initialised its actors, or every RPC below is swallowed"),
                       World->AreActorsInitialized())) {
        return false;
    }
    Out.PlayerState = World->SpawnActor<AMythicPlayerState>();
    Out.Controller = World->SpawnActor<AMythicCurrencyWalletTestController>();
    if (!Test.TestNotNull(TEXT("the player state spawned"), Out.PlayerState)
        || !Test.TestNotNull(TEXT("the controller spawned"), Out.Controller)) {
        return false;
    }
    if (!Test.TestTrue(TEXT("the controller holds authority"), Out.Controller->HasAuthority())) {
        return false;
    }
    Out.PlayerState->AuthoritySetPersistentEntityId(
        FMythicEntityId::FromAuthorityGuid(EMythicEntityDomain::PlayerCharacter, FGuid::NewGuid()));
    Out.Controller->SetPlayerState(Out.PlayerState);

    Out.ASC = Out.PlayerState->GetMythicAbilitySystemComponent();
    if (!Test.TestNotNull(TEXT("the player state owns an ability system"), Out.ASC)) {
        return false;
    }
    if (!Out.ASC->IsRegistered()) {
        Out.ASC->RegisterComponent();
    }
    Out.ASC->InitAbilityActorInfo(Out.PlayerState, Out.PlayerState);
    if (!Test.TestTrue(TEXT("the controller reaches the player state's ability system"),
                       Out.Controller->GetAbilitySystemComponent() == Out.ASC)) {
        return false;
    }

    Out.Coin = NewObject<UItemDefinition>(Out.GameInstance, TEXT("WalletTestCoin"));
    Out.Coin->Name = FText::FromString(TEXT("Test Coin"));
    Out.Coin->ItemType = ITEMIZATION_TYPE_CURRENCY;
    Out.Coin->StackSizeMax = 100;

    UInventoryProfile *Profile = MakeCarriedProfile(Out.GameInstance, 4);
    for (UMythicInventoryComponent *Purse : Out.Controller->GetAllInventoryComponents()) {
        if (!Test.TestNotNull(TEXT("every purse exists"), Purse)) {
            return false;
        }
        if (!Purse->IsRegistered()) {
            Purse->RegisterComponent();
        }
        Purse->InventoryProfile = Profile;
        Purse->InitializeSlots();
        if (!Test.TestEqual(TEXT("every purse opened its slots"), Purse->GetNumSlots(), 4)) {
            return false;
        }
    }

    Out.Notices.Reset(NewObject<UMythicCurrencyWalletNoticeListener>(Out.GameInstance));
    Out.Notices->Bind(Out.Controller);
    Out.ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_CURRENCY_SPENT).AddLambda(
        [&Out](const FGameplayEventData *Payload) {
            Out.SpentEvents++;
            Out.LastSpentMagnitude = Payload ? Payload->EventMagnitude : 0.0f;
        });
    return true;
}

bool FillPurse(FAutomationTestBase &Test, const FMythicWalletFixture &Fixture, UMythicInventoryComponent *Purse,
               int32 Amount) {
    UMythicItemInstance *Coins = NewObject<UMythicItemInstance>(Fixture.Controller);
    Coins->SetOwner(Fixture.Controller);
    Coins->InitializeFixtureForTests(Fixture.Coin, Amount, 1);
    return Test.TestEqual(TEXT("the purse took the whole stack"), Purse->AddToAnySlot(Coins), Amount)
        && Test.TestEqual(TEXT("the purse reads the stack as currency"), Purse->GetTotalCurrency(), Amount);
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCurrencyWalletChargeSpansPursesTest,
    "Mythic.Itemization.CurrencyWallet.ChargeSpansPurses",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCurrencyWalletChargeSpansPursesTest::RunTest(const FString &Parameters) {
    FMythicWalletFixture Fixture;
    const bool bReady = BuildWalletFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    if (!FillPurse(*this, Fixture, Fixture.FirstPurse(), 60) || !FillPurse(*this, Fixture, Fixture.SecondPurse(), 60)) {
        return false;
    }
    TestEqual(TEXT("the wallet sums both purses"), Fixture.Controller->GetCarriedCurrency(), 120);

    TestTrue(TEXT("a charge the wallet covers succeeds"), Fixture.Controller->TryChargeCurrency(100));
    TestEqual(TEXT("the first purse is emptied first"), Fixture.FirstPurse()->GetTotalCurrency(), 0);
    TestEqual(TEXT("the second purse covers the rest"), Fixture.SecondPurse()->GetTotalCurrency(), 20);
    TestEqual(TEXT("the wallet reads the remainder"), Fixture.Controller->GetCarriedCurrency(), 20);
    TestEqual(TEXT("Currency.Spent fired once"), Fixture.SpentEvents, 1);
    TestEqual(TEXT("Currency.Spent carries the amount charged"), Fixture.LastSpentMagnitude, 100.0f);
    TestEqual(TEXT("the owner saw one feed line"), Fixture.Notices->NoticeCount, 1);
    TestEqual(TEXT("the feed line is a combat beat"), Fixture.Notices->LastNotice.Kind, EMythicNoticeKind::Combat);
    TestEqual(TEXT("the feed line names the amount"), Fixture.Notices->LastNotice.Text.ToString(),
              FString(TEXT("-100 gold")));

    TestTrue(TEXT("a free charge succeeds without touching the wallet"), Fixture.Controller->TryChargeCurrency(0));
    TestEqual(TEXT("a free charge raises no event"), Fixture.SpentEvents, 1);
    TestEqual(TEXT("a free charge raises no feed line"), Fixture.Notices->NoticeCount, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCurrencyWalletShortChargeIsNoOpTest,
    "Mythic.Itemization.CurrencyWallet.ShortChargeIsNoOp",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCurrencyWalletShortChargeIsNoOpTest::RunTest(const FString &Parameters) {
    FMythicWalletFixture Fixture;
    const bool bReady = BuildWalletFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    if (!FillPurse(*this, Fixture, Fixture.FirstPurse(), 60) || !FillPurse(*this, Fixture, Fixture.SecondPurse(), 60)) {
        return false;
    }

    TestFalse(TEXT("one coin short refuses"), Fixture.Controller->TryChargeCurrency(121));
    TestEqual(TEXT("the first purse is untouched"), Fixture.FirstPurse()->GetTotalCurrency(), 60);
    TestEqual(TEXT("the second purse is untouched"), Fixture.SecondPurse()->GetTotalCurrency(), 60);
    TestEqual(TEXT("no Currency.Spent fired"), Fixture.SpentEvents, 0);
    TestEqual(TEXT("no feed line was raised"), Fixture.Notices->NoticeCount, 0);

    TestTrue(TEXT("the exact balance is still affordable"), Fixture.Controller->TryChargeCurrency(120));
    TestEqual(TEXT("the exact charge empties both purses"), Fixture.Controller->GetCarriedCurrency(), 0);
    TestEqual(TEXT("Currency.Spent carries the exact charge"), Fixture.LastSpentMagnitude, 120.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCurrencyWalletGrantMintsStackTest,
    "Mythic.Itemization.CurrencyWallet.GrantMintsStack",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCurrencyWalletGrantMintsStackTest::RunTest(const FString &Parameters) {
    FMythicWalletFixture Fixture;
    const bool bReady = BuildWalletFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    // The grant mints whatever the project settings name as currency; point them at the fixture coin for the test.
    UMythicDeveloperSettings *Settings = GetMutableDefault<UMythicDeveloperSettings>();
    const TSoftObjectPtr<UItemDefinition> AuthoredCurrency = Settings->CurrencyItemDefinition;
    Settings->CurrencyItemDefinition = TSoftObjectPtr<UItemDefinition>(FSoftObjectPath(Fixture.Coin));
    ON_SCOPE_EXIT {
        Settings->CurrencyItemDefinition = AuthoredCurrency;
    };
    if (!TestTrue(TEXT("the settings resolve the fixture coin"), Settings->GetCurrencyItemDefinition() == Fixture.Coin)) {
        return false;
    }

    TestEqual(TEXT("a grant returns the amount minted"), Fixture.Controller->GrantCurrency(75), 75);
    TestEqual(TEXT("the first purse holds the new stack"), Fixture.FirstPurse()->GetTotalCurrency(), 75);
    TestEqual(TEXT("the wallet reads the grant"), Fixture.Controller->GetCarriedCurrency(), 75);
    TestEqual(TEXT("a grant is not a spend"), Fixture.SpentEvents, 0);

    TestEqual(TEXT("a grant past one stack splits across stacks"), Fixture.Controller->GrantCurrency(150), 150);
    TestEqual(TEXT("the wallet reads both grants"), Fixture.Controller->GetCarriedCurrency(), 225);
    TestEqual(TEXT("nothing is minted for zero"), Fixture.Controller->GrantCurrency(0), 0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
