
#include "Misc/AutomationTest.h"

#include "Abilities/GameplayAbility.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Misc/ScopeExit.h"
#include "Player/MythicPlayerState.h"
#include "Progression/MythicAchievementComponent.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "Progression/Runes/MythicRuneDefinition.h"
#include "Subsystem/SaveSystem/Character/CharacterData.h"
#include "UObject/UnrealType.h"

namespace {

// Every rune verb is authority-gated and grants on the OWNER's ASC, so none of them can be reached without a live
// owner. The player state is that owner in the game and already carries both the ability system and the sockets,
// so spawning one is the honest fixture rather than a stand-in.
struct FMythicRuneFixture {
    UGameInstance *GameInstance = nullptr;
    AMythicPlayerState *PlayerState = nullptr;
    UMythicRuneComponent *Runes = nullptr;
    UMythicAbilitySystemComponent *ASC = nullptr;
};

bool BuildRuneFixture(FAutomationTestBase &Test, FMythicRuneFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    UWorld *World = Out.GameInstance->GetWorld();
    if (!Test.TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    Out.PlayerState = World->SpawnActor<AMythicPlayerState>();
    if (!Test.TestNotNull(TEXT("the player state spawned"), Out.PlayerState)) {
        return false;
    }
    if (!Test.TestTrue(TEXT("the owner holds authority"), Out.PlayerState->HasAuthority())) {
        return false;
    }
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

UMythicRuneDefinition *MakeRune(const FGameplayTag RequiredTag) {
    UMythicRuneDefinition *Rune = NewObject<UMythicRuneDefinition>();
    Rune->Ability = UGameplayAbility::StaticClass();
    Rune->RequiredTag = RequiredTag;
    return Rune;
}

int32 OpenEverySocket(UMythicRuneComponent *Runes) {
    for (int32 Attempt = 0; Attempt < Runes->MaxSlots + 4; Attempt++) {
        Runes->GrantSlot();
    }
    return Runes->GetUnlockedSlots();
}

// Every public reader of the socket count clamps to MaxSlots, so a count that ran past the ceiling is invisible
// through the component's own API. Reflection is the only way to assert the stored number itself.
int32 RawUnlockedSlots(const UMythicRuneComponent *Runes) {
    const FIntProperty *Field = CastField<FIntProperty>(
        UMythicRuneComponent::StaticClass()->FindPropertyByName(FName("UnlockedSlots")));
    return Field ? Field->GetPropertyValue_InContainer(Runes) : INDEX_NONE;
}

// A rune's passive is granted with the rune as the spec's source object, so the ASC's own list says whether it is
// still worn. The component keeps its handles private; this is the seam it leaves open.
int32 CountGrantedFrom(const UAbilitySystemComponent *ASC, const UMythicRuneDefinition *Rune) {
    int32 Count = 0;
    for (const FGameplayAbilitySpec &Spec : ASC->GetActivatableAbilities()) {
        if (!Spec.PendingRemove && Spec.SourceObject.Get() == Rune) {
            Count++;
        }
    }
    return Count;
}

FGameplayTag RegisteredTag(const TCHAR *Name) {
    return FGameplayTag::RequestGameplayTag(FName(Name), false);
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneSlotGatingTest,
    "Mythic.Progression.Runes.SlotGating",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneSlotGatingTest::RunTest(const FString &Parameters) {
    FMythicRuneFixture Fixture;
    const bool bReady = BuildRuneFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    if (!TestTrue(TEXT("the authored MaxSlots leaves room to grow"), Runes->MaxSlots >= 2)) {
        return false;
    }

    // A fresh character wears one rune and no more. Sockets two upward are a deed the player has not done yet.
    TestEqual(TEXT("a fresh character starts with one socket"), Runes->GetUnlockedSlots(), 1);
    TestTrue(TEXT("socket one is free"), Runes->IsSlotUnlocked(0));
    for (int32 Slot = 1; Slot < Runes->MaxSlots; Slot++) {
        TestFalse(FString::Printf(TEXT("socket %d is shut until it is earned"), Slot), Runes->IsSlotUnlocked(Slot));
    }
    TestFalse(TEXT("a negative socket is never open"), Runes->IsSlotUnlocked(-1));

    Runes->GrantSlot();
    TestEqual(TEXT("one unlock opens exactly one more socket"), Runes->GetUnlockedSlots(), 2);
    TestTrue(TEXT("the newly granted socket is open"), Runes->IsSlotUnlocked(1));
    if (Runes->MaxSlots > 2) {
        TestFalse(TEXT("the socket after it is still shut"), Runes->IsSlotUnlocked(2));
    }

    // Rules fire more than once in a long save. The count must stop at the authored ceiling, not run away.
    const int32 Ceiling = Runes->MaxSlots;
    OpenEverySocket(Runes);
    TestEqual(TEXT("repeated grants stop at MaxSlots"), RawUnlockedSlots(Runes), Ceiling);
    TestTrue(TEXT("the last socket is open"), Runes->IsSlotUnlocked(Ceiling - 1));
    TestFalse(TEXT("nothing opens past MaxSlots"), Runes->IsSlotUnlocked(Ceiling));

    // A count that overshot stays hidden behind the clamp until the ceiling moves, then pays out sockets nobody
    // earned. Raising MaxSlots is what makes the stored number answer for itself.
    Runes->MaxSlots = Ceiling + 2;
    TestEqual(TEXT("a raised ceiling opens no socket the player did not earn"), Runes->GetUnlockedSlots(), Ceiling);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneLockedSlotRefusedTest,
    "Mythic.Progression.Runes.LockedSlotRefused",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneLockedSlotRefusedTest::RunTest(const FString &Parameters) {
    FMythicRuneFixture Fixture;
    const bool bReady = BuildRuneFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    if (!TestTrue(TEXT("the authored MaxSlots leaves a shut socket in range"), Runes->MaxSlots >= 2)) {
        return false;
    }
    if (!TestEqual(TEXT("a fresh character starts with one socket"), Runes->GetUnlockedSlots(), 1)) {
        return false;
    }

    UMythicRuneDefinition *First = MakeRune(FGameplayTag());
    UMythicRuneDefinition *Second = MakeRune(FGameplayTag());

    // Socket two is a real entry in the strip, just an unearned one. The range check cannot be what refuses it.
    Runes->ServerEquipRune(1, Second);
    if (!TestTrue(TEXT("socket two is a real index, so only the lock can refuse it"),
                  Runes->GetEquippedRunes().IsValidIndex(1))) {
        return false;
    }
    TestNull(TEXT("a shut socket takes no rune"), Runes->GetRuneInSlot(1));
    TestEqual(TEXT("and grants nothing"), CountGrantedFrom(Fixture.ASC, Second), 0);

    // The denominator: the open socket takes a rune, and socket two takes one the moment it is earned.
    Runes->ServerEquipRune(0, First);
    TestTrue(TEXT("the open socket takes its rune"), Runes->GetRuneInSlot(0) == First);

    Runes->GrantSlot();
    Runes->ServerEquipRune(1, Second);
    TestTrue(TEXT("the earned socket takes the rune it just refused"), Runes->GetRuneInSlot(1) == Second);
    TestEqual(TEXT("and grants its passive"), CountGrantedFrom(Fixture.ASC, Second), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneUnlockGatingTest,
    "Mythic.Progression.Runes.UnlockGating",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneUnlockGatingTest::RunTest(const FString &Parameters) {
    const FGameplayTag EarnedDeed = RegisteredTag(TEXT("Achievement.Delver"));
    const FGameplayTag UnearnedDeed = RegisteredTag(TEXT("Achievement.Wanderer"));
    if (!EarnedDeed.IsValid() || !UnearnedDeed.IsValid()) {
        AddWarning(TEXT("Achievement.Delver / Achievement.Wanderer are not registered in this build; unlock gating untested"));
        return true;
    }

    FMythicRuneFixture Fixture;
    const bool bReady = BuildRuneFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;

    // #45 says the deed may be recorded anywhere, so the ASC's own tags count as proof.
    Fixture.ASC->AddLooseGameplayTag(EarnedDeed);

    UMythicRuneDefinition *Deeded = MakeRune(EarnedDeed);
    UMythicRuneDefinition *Locked = MakeRune(UnearnedDeed);
    UMythicRuneDefinition *Free = MakeRune(FGameplayTag());

    TestTrue(TEXT("a rune whose deed is done is available"), Runes->IsRuneUnlocked(Deeded));
    TestFalse(TEXT("a rune whose deed is undone is not"), Runes->IsRuneUnlocked(Locked));
    TestTrue(TEXT("a rune with no required tag needs no deed"), Runes->IsRuneUnlocked(Free));
    TestFalse(TEXT("a null rune is never unlocked"), Runes->IsRuneUnlocked(nullptr));

    // The predicate is advice; the verb is the gate. Both have to hold.
    Runes->ServerEquipRune(0, Locked);
    TestNull(TEXT("an unearned rune never reaches a socket"), Runes->GetRuneInSlot(0));

    Runes->ServerEquipRune(0, Deeded);
    TestTrue(TEXT("an earned rune equips"), Runes->GetRuneInSlot(0) == Deeded);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneAchievementDeedTest,
    "Mythic.Progression.Runes.AchievementDeed",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneAchievementDeedTest::RunTest(const FString &Parameters) {
    const FGameplayTag EarnedDeed = RegisteredTag(TEXT("Achievement.Smith"));
    const FGameplayTag UnearnedDeed = RegisteredTag(TEXT("Achievement.Forager"));
    if (!EarnedDeed.IsValid() || !UnearnedDeed.IsValid()) {
        AddWarning(TEXT("Achievement.Smith / Achievement.Forager are not registered in this build; achievement gating untested"));
        return true;
    }

    FMythicRuneFixture Fixture;
    const bool bReady = BuildRuneFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;

    UMythicAchievementComponent *Achievements = Fixture.PlayerState->GetAchievementComponent();
    if (!TestNotNull(TEXT("the player state owns an achievement ledger"), Achievements)) {
        return false;
    }
    FGameplayTagContainer Deeds;
    Deeds.AddTag(EarnedDeed);
    Achievements->RestoreUnlockedAchievements(Deeds);
    if (!TestTrue(TEXT("the ledger holds the deed"), Achievements->IsAchievementUnlocked(EarnedDeed))) {
        return false;
    }

    // The achievement ledger is the only place this deed is written. If the rune gate cannot read that ledger it
    // has nowhere else to find the tag, so the whole Achievement.* content set stays locked forever.
    TestFalse(TEXT("the ability system does not hold the deed"), Fixture.ASC->HasMatchingGameplayTag(EarnedDeed));

    UMythicRuneDefinition *Deeded = MakeRune(EarnedDeed);
    UMythicRuneDefinition *Locked = MakeRune(UnearnedDeed);

    TestTrue(TEXT("a rune earned by an achievement is available"), Runes->IsRuneUnlocked(Deeded));
    TestFalse(TEXT("a rune whose achievement is undone is not"), Runes->IsRuneUnlocked(Locked));

    Runes->ServerEquipRune(0, Deeded);
    TestTrue(TEXT("and the verb lets it into a socket"), Runes->GetRuneInSlot(0) == Deeded);
    TestEqual(TEXT("granting its passive"), CountGrantedFrom(Fixture.ASC, Deeded), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRunePayloadRequiredTest,
    "Mythic.Progression.Runes.PayloadRequired",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRunePayloadRequiredTest::RunTest(const FString &Parameters) {
    FMythicRuneFixture Fixture;
    const bool bReady = BuildRuneFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;

    UMythicRuneDefinition *Inert = NewObject<UMythicRuneDefinition>();
    TestFalse(TEXT("a rune with no ability has no payload"), Inert->HasPayload());

    Runes->ServerEquipRune(0, Inert);
    TestNull(TEXT("an inert rune never occupies a socket"), Runes->GetRuneInSlot(0));

    Runes->ServerEquipRune(0, nullptr);
    TestNull(TEXT("nor does nothing at all"), Runes->GetRuneInSlot(0));

    // The denominator: the same socket, same call, a rune that does carry an ability.
    UMythicRuneDefinition *Carrying = MakeRune(FGameplayTag());
    TestTrue(TEXT("a rune with an ability has a payload"), Carrying->HasPayload());
    Runes->ServerEquipRune(0, Carrying);
    TestTrue(TEXT("and it equips"), Runes->GetRuneInSlot(0) == Carrying);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneOneSocketEachTest,
    "Mythic.Progression.Runes.OneSocketEach",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneOneSocketEachTest::RunTest(const FString &Parameters) {
    FMythicRuneFixture Fixture;
    const bool bReady = BuildRuneFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    if (!TestTrue(TEXT("the character can open at least two sockets"), OpenEverySocket(Runes) >= 2)) {
        return false;
    }

    UMythicRuneDefinition *Worn = MakeRune(FGameplayTag());
    Runes->ServerEquipRune(0, Worn);
    if (!TestTrue(TEXT("the rune took the first socket"), Runes->GetRuneInSlot(0) == Worn)) {
        return false;
    }

    // Doubling one rune across two sockets would double its effect for free. It must be refused, and the
    // original wear must survive the refusal.
    Runes->ServerEquipRune(1, Worn);
    TestNull(TEXT("the same rune cannot be worn twice"), Runes->GetRuneInSlot(1));
    TestTrue(TEXT("the first wear survives the refusal"), Runes->GetRuneInSlot(0) == Worn);
    TestEqual(TEXT("and it is still granted exactly once"), CountGrantedFrom(Fixture.ASC, Worn), 1);

    // The denominator: a different rune still fits that socket.
    UMythicRuneDefinition *Other = MakeRune(FGameplayTag());
    Runes->ServerEquipRune(1, Other);
    TestTrue(TEXT("a different rune fits the second socket"), Runes->GetRuneInSlot(1) == Other);
    TestTrue(TEXT("and the first socket is still its own"), Runes->GetRuneInSlot(0) == Worn);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneSlotRangeTest,
    "Mythic.Progression.Runes.SlotRange",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneSlotRangeTest::RunTest(const FString &Parameters) {
    FMythicRuneFixture Fixture;
    const bool bReady = BuildRuneFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    OpenEverySocket(Runes);

    UMythicRuneDefinition *Worn = MakeRune(FGameplayTag());
    Runes->ServerEquipRune(0, Worn);
    const int32 Sockets = Runes->GetEquippedRunes().Num();
    if (!TestEqual(TEXT("the socket strip is MaxSlots long"), Sockets, Runes->MaxSlots)) {
        return false;
    }

    // A client can name any index it likes. None of these may grow the strip or reach the array.
    Runes->ServerEquipRune(-1, Worn);
    Runes->ServerEquipRune(Sockets, Worn);
    Runes->ServerEquipRune(TNumericLimits<int32>::Max(), Worn);
    Runes->ServerUnequipRune(-1);
    Runes->ServerUnequipRune(Sockets);
    Runes->ServerUnequipRune(TNumericLimits<int32>::Max());

    TestEqual(TEXT("an out-of-range index never grows the strip"), Runes->GetEquippedRunes().Num(), Sockets);
    TestNull(TEXT("nothing landed off the end"), Runes->GetRuneInSlot(Sockets));
    TestNull(TEXT("nor before the start"), Runes->GetRuneInSlot(-1));
    TestTrue(TEXT("and the worn rune is untouched"), Runes->GetRuneInSlot(0) == Worn);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneUnequipTest,
    "Mythic.Progression.Runes.Unequip",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneUnequipTest::RunTest(const FString &Parameters) {
    FMythicRuneFixture Fixture;
    const bool bReady = BuildRuneFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    const int32 Sockets = OpenEverySocket(Runes);
    if (!TestTrue(TEXT("the character can open at least two sockets"), Sockets >= 2)) {
        return false;
    }

    TArray<UMythicRuneDefinition *> Worn;
    for (int32 Slot = 0; Slot < Sockets; Slot++) {
        Worn.Add(MakeRune(FGameplayTag()));
        Runes->ServerEquipRune(Slot, Worn[Slot]);
    }
    for (int32 Slot = 0; Slot < Sockets; Slot++) {
        if (!TestTrue(FString::Printf(TEXT("socket %d wears its rune"), Slot), Runes->GetRuneInSlot(Slot) == Worn[Slot])) {
            return false;
        }
    }

    const int32 Emptied = Sockets - 1;
    Runes->ServerUnequipRune(Emptied);
    TestNull(TEXT("the unequipped socket is empty"), Runes->GetRuneInSlot(Emptied));
    for (int32 Slot = 0; Slot < Emptied; Slot++) {
        TestTrue(FString::Printf(TEXT("socket %d is untouched"), Slot), Runes->GetRuneInSlot(Slot) == Worn[Slot]);
    }

    Runes->ServerEquipRune(Emptied, Worn[Emptied]);
    TestTrue(TEXT("the emptied socket takes the rune back"), Runes->GetRuneInSlot(Emptied) == Worn[Emptied]);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneAbilityLifetimeTest,
    "Mythic.Progression.Runes.AbilityLifetime",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneAbilityLifetimeTest::RunTest(const FString &Parameters) {
    FMythicRuneFixture Fixture;
    const bool bReady = BuildRuneFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    if (!TestTrue(TEXT("the character can open at least one socket"), OpenEverySocket(Runes) >= 1)) {
        return false;
    }

    UMythicRuneDefinition *First = MakeRune(FGameplayTag());
    UMythicRuneDefinition *Second = MakeRune(FGameplayTag());

    Runes->ServerEquipRune(0, First);
    if (!TestEqual(TEXT("wearing a rune grants its passive once"), CountGrantedFrom(Fixture.ASC, First), 1)) {
        return false;
    }

    // Swapping a socket is where a leak pays: the old passive would keep firing for a rune the player took off.
    Runes->ServerEquipRune(0, Second);
    TestTrue(TEXT("the socket now wears the second rune"), Runes->GetRuneInSlot(0) == Second);
    TestEqual(TEXT("the replaced rune's passive is handed back"), CountGrantedFrom(Fixture.ASC, First), 0);
    TestEqual(TEXT("and the new one is granted once"), CountGrantedFrom(Fixture.ASC, Second), 1);

    // Taking a rune off must clear the passive, not merely blank the socket.
    Runes->ServerUnequipRune(0);
    TestNull(TEXT("the socket is empty"), Runes->GetRuneInSlot(0));
    TestEqual(TEXT("and its passive is gone"), CountGrantedFrom(Fixture.ASC, Second), 0);

    // The denominator: wearing it again grants it back, exactly once.
    Runes->ServerEquipRune(0, Second);
    TestEqual(TEXT("re-wearing grants it once more"), CountGrantedFrom(Fixture.ASC, Second), 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneSaveFieldsTest,
    "Mythic.Progression.Runes.SaveFields",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneSaveFieldsTest::RunTest(const FString &Parameters) {
    FMythicRuneFixture Fixture;
    const bool bReady = BuildRuneFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    if (!TestTrue(TEXT("the character can open at least two sockets"), OpenEverySocket(Runes) >= 2)) {
        return false;
    }

    UMythicRuneDefinition *Worn = MakeRune(FGameplayTag());
    Runes->ServerEquipRune(1, Worn);
    if (!TestTrue(TEXT("the rune is worn"), Runes->GetRuneInSlot(1) == Worn)) {
        return false;
    }

    // The component's SaveGame flag is inert here: this project persists an explicit field list, so sockets and
    // runes only survive a reload if the character save writes them itself.
    FSerializedCharacterData Data;
    if (!TestTrue(TEXT("the character serialises"), FSerializedCharacterData::Serialize(Fixture.PlayerState, Data))) {
        return false;
    }

    TestEqual(TEXT("the save carries the open socket count"), Data.UnlockedRuneSlots, Runes->GetUnlockedSlots());
    if (!TestEqual(TEXT("the save carries one entry per socket"), Data.EquippedRunes.Num(),
                   Runes->GetEquippedRunes().Num())) {
        return false;
    }
    TestEqual(TEXT("the worn rune is written at its own slot"), Data.EquippedRunes[1].ToString(),
              FSoftObjectPath(Worn).ToString());
    TestTrue(TEXT("an empty socket is written as an empty path"), Data.EquippedRunes[0].IsNull());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneRestoreTest,
    "Mythic.Progression.Runes.Restore",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneRestoreTest::RunTest(const FString &Parameters) {
    const FGameplayTag EarnedDeed = RegisteredTag(TEXT("Achievement.Smith"));
    const FGameplayTag UnearnedDeed = RegisteredTag(TEXT("Achievement.Forager"));
    if (!EarnedDeed.IsValid() || !UnearnedDeed.IsValid()) {
        AddWarning(TEXT("Achievement.Smith / Achievement.Forager are not registered in this build; rune restore untested"));
        return true;
    }

    FMythicRuneFixture Fixture;
    const bool bReady = BuildRuneFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    const int32 Ceiling = Runes->MaxSlots;
    if (!TestTrue(TEXT("the authored MaxSlots leaves room for an empty socket and a dropped one"), Ceiling >= 3)) {
        return false;
    }

    UMythicAchievementComponent *Achievements = Fixture.PlayerState->GetAchievementComponent();
    if (!TestNotNull(TEXT("the player state owns an achievement ledger"), Achievements)) {
        return false;
    }
    FGameplayTagContainer Deeds;
    Deeds.AddTag(EarnedDeed);
    Achievements->RestoreUnlockedAchievements(Deeds);

    UMythicRuneDefinition *Deeded = MakeRune(EarnedDeed);
    UMythicRuneDefinition *Lost = MakeRune(UnearnedDeed);

    // The save as it comes off disk: a worn rune, an empty socket, a rune whose deed this character no longer has,
    // and a socket count from a character who was later given a smaller ceiling.
    TArray<TSoftObjectPtr<UMythicRuneDefinition>> Saved;
    Saved.Add(Deeded);
    Saved.AddDefaulted();
    Saved.Add(Lost);
    if (!TestFalse(TEXT("a saved rune carries a real path"), Saved[0].ToSoftObjectPath().IsNull())) {
        return false;
    }

    Runes->RestoreRunes(Saved, Ceiling + 5);

    TestEqual(TEXT("a saved count above the ceiling is clamped to it"), RawUnlockedSlots(Runes), Ceiling);
    TestEqual(TEXT("the strip is sized to the ceiling"), Runes->GetEquippedRunes().Num(), Ceiling);
    TestTrue(TEXT("the worn rune comes back to its socket"), Runes->GetRuneInSlot(0) == Deeded);
    TestEqual(TEXT("and its passive is granted again"), CountGrantedFrom(Fixture.ASC, Deeded), 1);
    TestNull(TEXT("an empty socket stays empty"), Runes->GetRuneInSlot(1));
    TestNull(TEXT("a rune whose deed is gone is dropped"), Runes->GetRuneInSlot(2));
    TestEqual(TEXT("and nothing is granted for it"), CountGrantedFrom(Fixture.ASC, Lost), 0);

    // Loading twice in one session must not leave two copies of the same passive on the ASC.
    TArray<TSoftObjectPtr<UMythicRuneDefinition>> Reloaded;
    Reloaded.Add(Deeded);
    Runes->RestoreRunes(Reloaded, 2);

    TestEqual(TEXT("a saved count below the ceiling is kept, not reset to one"), RawUnlockedSlots(Runes), 2);
    TestTrue(TEXT("the rune is worn once more"), Runes->GetRuneInSlot(0) == Deeded);
    TestEqual(TEXT("its passive is still granted exactly once"), CountGrantedFrom(Fixture.ASC, Deeded), 1);

    // The restored count is what the player grows from; a later unlock adds to it.
    Runes->GrantSlot();
    TestEqual(TEXT("a later unlock builds on the restored count"), Runes->GetUnlockedSlots(), 3);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneSaveMigrationTest,
    "Mythic.Progression.Runes.SaveMigration",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneSaveMigrationTest::RunTest(const FString &Parameters) {
    /**
     * A save written before rune sockets were persisted carries the GrantPerkSlot rules but no count, and
     * RestoreUnlockState re-latches those rules so they never fire again. Without this repair a character who
     * had earned every socket reloads with one, permanently.
     */
    auto Rule = [](const TCHAR *Name) { return FGameplayTag::RequestGameplayTag(FName(Name), false); };

    const FGameplayTag Slot2 = Rule(TEXT("Unlock.Rule.RuneSlot2"));
    const FGameplayTag Slot3 = Rule(TEXT("Unlock.Rule.RuneSlot3"));
    const FGameplayTag Slot4 = Rule(TEXT("Unlock.Rule.RuneSlot4"));
    const FGameplayTag Unrelated = Rule(TEXT("Unlock.Rule.TitleSlayer"));
    if (!Slot2.IsValid() || !Slot3.IsValid() || !Slot4.IsValid()) {
        AddInfo(TEXT("rune slot unlock rule tags are not registered; skipping"));
        return true;
    }

    using Migration = FMythicCharacterSaveMigration;

    TestEqual(TEXT("a save that earned nothing still owns the free socket"),
              Migration::RuneSlotsFromAppliedRules({}), 1);
    TestEqual(TEXT("one earned rule reopens one socket"),
              Migration::RuneSlotsFromAppliedRules({Slot2}), 2);
    TestEqual(TEXT("every earned rule is recovered"),
              Migration::RuneSlotsFromAppliedRules({Slot2, Slot3, Slot4}), 4);

    // The scan must key on the rune rules, not simply count what was applied.
    if (Unrelated.IsValid()) {
        TestEqual(TEXT("an unrelated unlock does not open a socket"),
                  Migration::RuneSlotsFromAppliedRules({Unrelated}), 1);
        TestEqual(TEXT("and does not inflate a genuine count"),
                  Migration::RuneSlotsFromAppliedRules({Unrelated, Slot2, Unrelated}), 2);
    }

    TestEqual(TEXT("an invalid entry is ignored rather than counted"),
              Migration::RuneSlotsFromAppliedRules({FGameplayTag(), Slot2}), 2);
    return true;
}
