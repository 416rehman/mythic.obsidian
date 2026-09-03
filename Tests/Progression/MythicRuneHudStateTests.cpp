
#include "Misc/AutomationTest.h"

#include "Abilities/GameplayAbility.h"
#include "CoreGlobals.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Misc/ScopeExit.h"
#include "Player/MythicPlayerState.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "Progression/Runes/MythicRuneDefinition.h"
#include "Tests/GAS/MythicGA_RuneTestTypes.h"
#include "Tests/Progression/MythicRuneTestTypes.h"
#include "TimerManager.h"
#include "UI/ViewModels/MythicPlayerStatusViewModel.h"
#include "World/Entity/MythicEntityId.h"

namespace {

// The HUD state setter is authority-gated and addressed by socket, so the fixture is the same live player state the
// rune verbs already need: it carries the ability system, the sockets and authority.
struct FMythicRuneHudFixture {
    UGameInstance *GameInstance = nullptr;
    AMythicPlayerState *PlayerState = nullptr;
    UMythicRuneComponent *Runes = nullptr;
    UMythicAbilitySystemComponent *ASC = nullptr;
    UWorld *World = nullptr;
};

bool BuildRuneHudFixture(FAutomationTestBase &Test, FMythicRuneHudFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    Out.World = Out.GameInstance->GetWorld();
    if (!Test.TestNotNull(TEXT("standalone world exists"), Out.World)) {
        return false;
    }
    Out.PlayerState = Out.World->SpawnActor<AMythicPlayerState>();
    if (!Test.TestNotNull(TEXT("the player state spawned"), Out.PlayerState)) {
        return false;
    }
    if (!Test.TestTrue(TEXT("the owner holds authority"), Out.PlayerState->HasAuthority())) {
        return false;
    }
    Out.PlayerState->AuthoritySetPersistentEntityId(
        FMythicEntityId::FromAuthorityGuid(EMythicEntityDomain::PlayerCharacter, FGuid::NewGuid()));
    Out.Runes = Out.PlayerState->GetRuneComponent();
    if (!Test.TestNotNull(TEXT("the player state owns a rune component"), Out.Runes)) {
        return false;
    }
    Out.ASC = Out.PlayerState->GetMythicAbilitySystemComponent();
    if (!Test.TestNotNull(TEXT("the player state owns an ability system"), Out.ASC)) {
        return false;
    }
    if (!Out.Runes->IsRegistered()) {
        Out.Runes->RegisterComponent();
    }
    if (!Out.ASC->IsRegistered()) {
        Out.ASC->RegisterComponent();
    }
    Out.ASC->InitAbilityActorInfo(Out.PlayerState, Out.PlayerState);
    return Test.TestTrue(TEXT("the ability system answers as authority"), Out.ASC->IsOwnerActorAuthoritative());
}

UMythicRuneDefinition *MakeHudTestRune(const TCHAR *IconPath) {
    UMythicRuneDefinition *Rune = NewObject<UMythicRuneDefinition>();
    Rune->Ability = UMythicRuneTestAbility::StaticClass();
    Rune->Icon = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconPath));
    return Rune;
}

// A whole test body runs inside one engine frame, where neither world time nor FTimerManager moves on its own.
// Advancing the clocks the component reads, and the frame counter the timer manager gates on, is what makes a
// "the window has passed" assertion able to fail.
void AdvanceHudTestWorld(UWorld &World, const float Seconds) {
    ++GFrameCounter;
    World.TimeSeconds += Seconds;
    World.UnpausedTimeSeconds += Seconds;
    World.RealTimeSeconds += Seconds;
    World.GetTimerManager().Tick(Seconds);
}

// The state channel's own delegate has no parameters, so the rune listener's no-parameter handler counts it.
UMythicRuneTestListener *ListenToHudState(UMythicRuneComponent *Runes) {
    UMythicRuneTestListener *Listener = NewObject<UMythicRuneTestListener>();
    Runes->OnRuneHudStateChanged.AddUniqueDynamic(Listener, &UMythicRuneTestListener::HandleRunesChanged);
    return Listener;
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneHudStateSetAndReadTest,
    "Mythic.Progression.Runes.HudState.SetAndRead",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneHudStateSetAndReadTest::RunTest(const FString &Parameters) {
    FMythicRuneHudFixture Fixture;
    const bool bReady = BuildRuneHudFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    UMythicRuneDefinition *Rune = MakeHudTestRune(TEXT("/Game/Mythic/UI/Test/T_RuneA.T_RuneA"));
    Runes->ServerEquipRune(0, Rune);
    if (!TestTrue(TEXT("the rune is worn"), Runes->GetRuneInSlot(0) == Rune)) {
        return false;
    }
    UMythicRuneTestListener *Listener = ListenToHudState(Runes);

    TestEqual(TEXT("a fresh socket is Hidden"), Runes->GetRuneHudState(0), EMythicRuneHudState::Hidden);
    TestEqual(TEXT("the rune is found in its socket"), Runes->FindSlotOfRune(Rune), 0);
    TestEqual(TEXT("a rune nobody wears has no socket"), Runes->FindSlotOfRune(MakeHudTestRune(TEXT(""))), static_cast<int32>(INDEX_NONE));

    Runes->SetRuneHudState(0, EMythicRuneHudState::Ready);
    TestEqual(TEXT("Ready reads back"), Runes->GetRuneHudState(0), EMythicRuneHudState::Ready);
    TestEqual(TEXT("the change fired once"), Listener->ChangedCount, 1);
    TestEqual(TEXT("Ready with no duration has nothing left to count"), Runes->GetRuneHudRemainingSeconds(0), 0.0f);
    TestEqual(TEXT("one socket, one row"), Runes->GetRuneHudStates().Num(), 1);

    // The same answer again is not a change; a listener that rebuilt on it would rebuild for nothing.
    Runes->SetRuneHudState(0, EMythicRuneHudState::Ready);
    TestEqual(TEXT("repeating the state fires nothing"), Listener->ChangedCount, 1);

    Runes->SetRuneHudStateForRune(Rune, EMythicRuneHudState::Cooldown, 0.0f, 3);
    TestEqual(TEXT("addressing by rune reaches the same socket"), Runes->GetRuneHudState(0), EMythicRuneHudState::Cooldown);
    TestEqual(TEXT("stacks ride along"), static_cast<int32>(Runes->GetRuneHudStates()[0].Stacks), 3);
    TestEqual(TEXT("a real change fires"), Listener->ChangedCount, 2);

    // The denominator: a socket outside the strip and an unworn rune are refused without touching the channel.
    Runes->SetRuneHudState(Runes->MaxSlots, EMythicRuneHudState::Ready);
    Runes->SetRuneHudStateForRune(MakeHudTestRune(TEXT("")), EMythicRuneHudState::Ready);
    TestEqual(TEXT("out-of-range and unworn sets fire nothing"), Listener->ChangedCount, 2);
    TestEqual(TEXT("and add no row"), Runes->GetRuneHudStates().Num(), 1);

    Runes->SetRuneHudState(0, EMythicRuneHudState::Hidden);
    TestEqual(TEXT("Hidden drops the row"), Runes->GetRuneHudStates().Num(), 0);
    TestEqual(TEXT("and reads back as Hidden"), Runes->GetRuneHudState(0), EMythicRuneHudState::Hidden);
    TestEqual(TEXT("dropping a row fires"), Listener->ChangedCount, 3);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneHudStateTimedWindowTest,
    "Mythic.Progression.Runes.HudState.TimedWindow",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneHudStateTimedWindowTest::RunTest(const FString &Parameters) {
    FMythicRuneHudFixture Fixture;
    const bool bReady = BuildRuneHudFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    Runes->ServerEquipRune(0, MakeHudTestRune(TEXT("/Game/Mythic/UI/Test/T_RuneA.T_RuneA")));

    Runes->SetRuneHudState(0, EMythicRuneHudState::Active, 2.0f);
    TestEqual(TEXT("Active reads back"), Runes->GetRuneHudState(0), EMythicRuneHudState::Active);
    TestEqual(TEXT("the whole window is left the moment it opens"), Runes->GetRuneHudRemainingSeconds(0), 2.0f, 0.01f);
    const FMythicRuneHudStateItem &Row = Runes->GetRuneHudStates()[0];
    TestTrue(TEXT("the row carries an end after its start"), Row.ServerEndTimeSeconds > Row.ServerStartTimeSeconds);

    AdvanceHudTestWorld(*Fixture.World, 1.0f);
    TestEqual(TEXT("a second later a second is left"), Runes->GetRuneHudRemainingSeconds(0), 1.0f, 0.01f);

    AdvanceHudTestWorld(*Fixture.World, 1.5f);
    TestEqual(TEXT("past the end nothing is left"), Runes->GetRuneHudRemainingSeconds(0), 0.0f);
    // The clock only counts; what comes after the window is the server's call, not the reader's.
    TestEqual(TEXT("the state itself does not expire"), Runes->GetRuneHudState(0), EMythicRuneHudState::Active);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneHudStateUnequipClearsTest,
    "Mythic.Progression.Runes.HudState.UnequipClears",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneHudStateUnequipClearsTest::RunTest(const FString &Parameters) {
    FMythicRuneHudFixture Fixture;
    const bool bReady = BuildRuneHudFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    UMythicRuneDefinition *Rune = MakeHudTestRune(TEXT("/Game/Mythic/UI/Test/T_RuneA.T_RuneA"));
    Runes->ServerEquipRune(0, Rune);
    Runes->SetRuneHudState(0, EMythicRuneHudState::Ready);
    if (!TestEqual(TEXT("the socket is Ready before the rune leaves"), Runes->GetRuneHudState(0), EMythicRuneHudState::Ready)) {
        return false;
    }
    UMythicRuneTestListener *Listener = ListenToHudState(Runes);

    Runes->ServerUnequipRune(0);
    TestNull(TEXT("the socket is empty"), Runes->GetRuneInSlot(0));
    TestEqual(TEXT("an empty socket is Hidden"), Runes->GetRuneHudState(0), EMythicRuneHudState::Hidden);
    TestEqual(TEXT("its row is gone"), Runes->GetRuneHudStates().Num(), 0);
    TestEqual(TEXT("the HUD heard the clear once"), Listener->ChangedCount, 1);

    // The denominator: the next rune in that socket starts from nothing rather than inheriting a lit badge.
    Runes->ServerEquipRune(0, MakeHudTestRune(TEXT("/Game/Mythic/UI/Test/T_RuneB.T_RuneB")));
    TestEqual(TEXT("a new rune in the socket starts Hidden"), Runes->GetRuneHudState(0), EMythicRuneHudState::Hidden);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneHudStateMoveCarriesTest,
    "Mythic.Progression.Runes.HudState.MoveCarries",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneHudStateMoveCarriesTest::RunTest(const FString &Parameters) {
    FMythicRuneHudFixture Fixture;
    const bool bReady = BuildRuneHudFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    if (!TestTrue(TEXT("the authored MaxSlots leaves room to move"), Runes->MaxSlots >= 2)) {
        return false;
    }
    Runes->GrantSlot();
    UMythicRuneDefinition *Rune = MakeHudTestRune(TEXT("/Game/Mythic/UI/Test/T_RuneA.T_RuneA"));
    Runes->ServerEquipRune(0, Rune);
    Runes->SetRuneHudState(0, EMythicRuneHudState::Cooldown, 30.0f, 2);
    if (!TestEqual(TEXT("socket one is on cooldown before the move"), Runes->GetRuneHudState(0), EMythicRuneHudState::Cooldown)) {
        return false;
    }
    UMythicRuneTestListener *Listener = ListenToHudState(Runes);

    Runes->ServerMoveRune(0, 1);
    TestTrue(TEXT("the rune moved"), Runes->GetRuneInSlot(1) == Rune);
    TestEqual(TEXT("the socket it left is Hidden"), Runes->GetRuneHudState(0), EMythicRuneHudState::Hidden);
    TestEqual(TEXT("the socket it reached is on cooldown"), Runes->GetRuneHudState(1), EMythicRuneHudState::Cooldown);
    TestTrue(TEXT("with the clock still running"), Runes->GetRuneHudRemainingSeconds(1) > 29.0f);
    TestEqual(TEXT("and its stacks"), static_cast<int32>(Runes->GetRuneHudStates()[0].Stacks), 2);
    TestEqual(TEXT("one row, not a copy"), Runes->GetRuneHudStates().Num(), 1);
    TestEqual(TEXT("the HUD heard the move once"), Listener->ChangedCount, 1);

    // The rune's own address follows it, so a passive that speaks by rune keeps reaching its badge.
    TestEqual(TEXT("the rune is found in its new socket"), Runes->FindSlotOfRune(Rune), 1);
    Runes->SetRuneHudStateForRune(Rune, EMythicRuneHudState::Ready);
    TestEqual(TEXT("addressing by rune lands in the new socket"), Runes->GetRuneHudState(1), EMythicRuneHudState::Ready);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneHudStateViewModelBadgesTest,
    "Mythic.Progression.Runes.HudState.ViewModelBadges",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneHudStateViewModelBadgesTest::RunTest(const FString &Parameters) {
    FMythicRuneHudFixture Fixture;
    const bool bReady = BuildRuneHudFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    if (!TestTrue(TEXT("the authored MaxSlots leaves room for two runes"), Runes->MaxSlots >= 2)) {
        return false;
    }
    Runes->GrantSlot();
    UMythicRuneDefinition *First = MakeHudTestRune(TEXT("/Game/Mythic/UI/Test/T_RuneA.T_RuneA"));
    UMythicRuneDefinition *Second = MakeHudTestRune(TEXT("/Game/Mythic/UI/Test/T_RuneB.T_RuneB"));
    Runes->ServerEquipRune(0, First);
    Runes->ServerEquipRune(1, Second);
    if (!TestTrue(TEXT("both runes are worn"), Runes->GetRuneInSlot(0) == First && Runes->GetRuneInSlot(1) == Second)) {
        return false;
    }

    UMythicPlayerStatusViewModel *ViewModel = NewObject<UMythicPlayerStatusViewModel>(Fixture.GameInstance);
    ViewModel->InitializeForASC(Fixture.ASC);

    int32 Broadcasts = 0;
    ViewModel->AddFieldValueChangedDelegate(
        UMythicPlayerStatusViewModel::FFieldNotificationClassDescriptor::RuneBadges,
        INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateLambda(
            [&Broadcasts](UObject *, UE::FieldNotification::FFieldId) { Broadcasts++; }));

    TestEqual(TEXT("two worn runes with nothing to show list no badge"), ViewModel->GetRuneBadges().Num(), 0);

    Runes->SetRuneHudState(0, EMythicRuneHudState::Ready);
    if (!TestEqual(TEXT("one Ready rune lists one badge"), ViewModel->GetRuneBadges().Num(), 1)) {
        return false;
    }
    {
        const FMythicRuneBadgeEntry &Badge = ViewModel->GetRuneBadges()[0];
        TestEqual(TEXT("the badge names its socket"), Badge.SlotIndex, 0);
        TestEqual(TEXT("the badge carries the rune's state"), Badge.State, EMythicRuneHudState::Ready);
        TestTrue(TEXT("the badge carries the definition's icon"), Badge.Icon.ToSoftObjectPath() == First->Icon.ToSoftObjectPath());
        TestEqual(TEXT("an untimed state has no end"), Badge.EndTimeSeconds, 0.0);
    }
    TestEqual(TEXT("the list broadcast once for the change"), Broadcasts, 1);

    Runes->SetRuneHudState(1, EMythicRuneHudState::Active, 5.0f);
    if (!TestEqual(TEXT("a second lit rune lists a second badge"), ViewModel->GetRuneBadges().Num(), 2)) {
        return false;
    }
    {
        const FMythicRuneBadgeEntry &Badge = ViewModel->GetRuneBadges()[1];
        TestEqual(TEXT("badges sit in socket order"), Badge.SlotIndex, 1);
        TestTrue(TEXT("the second badge carries its own icon"), Badge.Icon.ToSoftObjectPath() == Second->Icon.ToSoftObjectPath());
        TestTrue(TEXT("a timed state ends after it starts"), Badge.EndTimeSeconds > Badge.StartTimeSeconds);
        const double LocalNow = Fixture.World->GetTimeSeconds();
        TestEqual(TEXT("the end is written in local world seconds"), Badge.EndTimeSeconds - LocalNow, 5.0, 0.01);
    }
    TestEqual(TEXT("the list broadcast again"), Broadcasts, 2);

    // Hidden is omitted, not listed dark: the row pools four cells and collapses the ones with nothing to say.
    Runes->SetRuneHudState(0, EMythicRuneHudState::Hidden);
    if (!TestEqual(TEXT("a Hidden rune leaves the list"), ViewModel->GetRuneBadges().Num(), 1)) {
        return false;
    }
    TestEqual(TEXT("the remaining badge is the second socket"), ViewModel->GetRuneBadges()[0].SlotIndex, 1);

    // The sockets themselves are a change too: taking the rune off drops its badge without a state call.
    Runes->ServerUnequipRune(1);
    TestEqual(TEXT("unequipping drops the badge"), ViewModel->GetRuneBadges().Num(), 0);

    return true;
}
