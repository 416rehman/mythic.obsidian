#include "Misc/AutomationTest.h"

#include "Abilities/GameplayAbility.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Misc/ScopeExit.h"
#include "NativeGameplayTags.h"
#include "Player/MythicPlayerState.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "Progression/Runes/MythicRuneDefinition.h"
#include "Subsystem/SaveSystem/Character/CharacterData.h"
#include "Tests/GAS/MythicGA_RuneTestTypes.h"
#include "World/Entity/MythicEntityId.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RuneRollTestDistance, "Rune.Param.Automation.Roll.Distance");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RuneRollTestWindow, "Rune.Param.Automation.Roll.Window");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RuneRollTestCooldown, "Rune.Param.Automation.Roll.Cooldown");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RuneRollTestUnrolled, "Rune.Param.Automation.Roll.Unrolled");

namespace {

// Rolls land on the server inside the equip verb and grant on the owner's ASC, so the owner is the player state
// that carries both.
struct FMythicRuneRollFixture {
    UGameInstance *GameInstance = nullptr;
    AMythicPlayerState *PlayerState = nullptr;
    UMythicRuneComponent *Runes = nullptr;
    UMythicAbilitySystemComponent *ASC = nullptr;
};

bool BuildRuneRollFixture(FAutomationTestBase &Test, FMythicRuneRollFixture &Out) {
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
    // The character save refuses a player with no canonical identity, which the game assigns on login.
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

FRollDefinition RuneRollRange(float Min, float Max, bool bWholeNumber) {
    FRollDefinition Roll;
    Roll.Min = Min;
    Roll.Max = Max;
    Roll.bWholeNumber = bWholeNumber;
    return Roll;
}

// A whole-number distance, a sub-second window and a long cooldown: the three shapes the shipping set rolls.
UMythicRuneDefinition *MakeRollTestRune() {
    UMythicRuneDefinition *Rune = NewObject<UMythicRuneDefinition>();
    Rune->Ability = UMythicRuneTestAbility::StaticClass();
    Rune->Parameters.Add(TAG_RuneRollTestDistance, RuneRollRange(8.0f, 12.0f, true));
    Rune->Parameters.Add(TAG_RuneRollTestWindow, RuneRollRange(0.25f, 0.45f, false));
    Rune->Parameters.Add(TAG_RuneRollTestCooldown, RuneRollRange(240.0f, 360.0f, false));
    return Rune;
}

UMythicRuneDefinition *MakeUnrolledTestRune() {
    UMythicRuneDefinition *Rune = NewObject<UMythicRuneDefinition>();
    Rune->Ability = UMythicRuneTestAbility::StaticClass();
    return Rune;
}

int32 OpenEveryRollTestSocket(UMythicRuneComponent *Runes) {
    for (int32 Attempt = 0; Attempt < Runes->MaxSlots + 4; Attempt++) {
        Runes->GrantSlot();
    }
    return Runes->GetUnlockedSlots();
}

int32 RollTestCountGrantedFrom(const UAbilitySystemComponent *ASC, const UMythicRuneDefinition *Rune) {
    int32 Count = 0;
    for (const FGameplayAbilitySpec &Spec : ASC->GetActivatableAbilities()) {
        if (!Spec.PendingRemove && Spec.SourceObject.Get() == Rune) {
            Count++;
        }
    }
    return Count;
}

// Every parameter the rune rolls, read back through the public getter. Empty when any is missing, so a caller
// comparing two snapshots cannot pass on a half-rolled set.
TMap<FGameplayTag, float> SnapshotRolls(const UMythicRuneComponent *Runes, const UMythicRuneDefinition *Rune) {
    TMap<FGameplayTag, float> Snapshot;
    for (const TPair<FGameplayTag, FRollDefinition> &Param : Rune->Parameters) {
        float Value = 0.0f;
        if (!Runes->GetRolledRuneValue(Rune, Param.Key, Value)) {
            return {};
        }
        Snapshot.Add(Param.Key, Value);
    }
    return Snapshot;
}

bool SameRolls(FAutomationTestBase &Test, const TCHAR *What, const TMap<FGameplayTag, float> &Expected,
               const TMap<FGameplayTag, float> &Actual) {
    bool bSame = Test.TestEqual(FString::Printf(TEXT("%s: every parameter is present"), What), Actual.Num(), Expected.Num());
    for (const TPair<FGameplayTag, float> &Pair : Expected) {
        const float *Value = Actual.Find(Pair.Key);
        bSame &= Test.TestTrue(FString::Printf(TEXT("%s: %s is still %g"), What, *Pair.Key.ToString(), Pair.Value),
                               Value && *Value == Pair.Value);
    }
    return bSame;
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneRollFirstSocketTest,
    "Mythic.Progression.Runes.Rolls.FirstSocket",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneRollFirstSocketTest::RunTest(const FString &Parameters) {
    FMythicRuneRollFixture Fixture;
    const bool bReady = BuildRuneRollFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    UMythicRuneDefinition *Rune = MakeRollTestRune();

    // Only a rune ability can be socketed; the definition is where that rule lives.
    UMythicRuneDefinition *NotARune = NewObject<UMythicRuneDefinition>();
    NotARune->Ability = UGameplayAbility::StaticClass();
    TestFalse(TEXT("a plain gameplay ability is no rune payload"), NotARune->HasPayload());
    TestTrue(TEXT("a rune ability is"), Rune->HasPayload());

    // Before any socket: no numbers, and the definition answers with midpoints.
    float Value = 0.0f;
    TestFalse(TEXT("a rune never socketed has no roll"), Runes->GetRolledRuneValue(Rune, TAG_RuneRollTestDistance, Value));
    TestEqual(TEXT("and no roll set"), Runes->GetRuneRolls().Num(), 0);
    TestEqual(TEXT("a whole-number midpoint rounds"), Rune->GetParameterMidpoint(TAG_RuneRollTestDistance, -1.0f), 10.0f);
    TestTrue(TEXT("a fractional midpoint is the middle of the range"),
             FMath::IsNearlyEqual(Rune->GetParameterMidpoint(TAG_RuneRollTestWindow, -1.0f), 0.35f, 0.001f));
    TestEqual(TEXT("a parameter the rune does not roll falls back"), Rune->GetParameterMidpoint(TAG_RuneRollTestUnrolled, -1.0f), -1.0f);

    Runes->ServerEquipRune(0, Rune);
    if (!TestTrue(TEXT("the rune took the first socket"), Runes->GetRuneInSlot(0) == Rune)) {
        return false;
    }
    TestEqual(TEXT("its passive is granted once"), RollTestCountGrantedFrom(Fixture.ASC, Rune), 1);

    // The first socket rolls every parameter, inside its own range, whole where the range says so.
    if (!TestEqual(TEXT("the first socket writes one roll set"), Runes->GetRuneRolls().Num(), 1)) {
        return false;
    }
    TestTrue(TEXT("the set names the rune"), Runes->GetRuneRolls()[0].Rune.ToSoftObjectPath() == FSoftObjectPath(Rune));
    TestEqual(TEXT("the set carries one value per parameter"), Runes->GetRuneRolls()[0].Values.Num(), Rune->Parameters.Num());
    int32 Rolled = 0;
    for (const TPair<FGameplayTag, FRollDefinition> &Param : Rune->Parameters) {
        if (!TestTrue(FString::Printf(TEXT("%s was rolled"), *Param.Key.ToString()),
                      Runes->GetRolledRuneValue(Rune, Param.Key, Value))) {
            continue;
        }
        ++Rolled;
        TestTrue(FString::Printf(TEXT("%s = %g sits inside [%g-%g]"), *Param.Key.ToString(), Value, Param.Value.Min, Param.Value.Max),
                 Value >= Param.Value.Min && Value <= Param.Value.Max);
        if (Param.Value.bWholeNumber) {
            TestTrue(FString::Printf(TEXT("%s = %g is a whole number"), *Param.Key.ToString(), Value),
                     FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value)));
        }
    }
    TestEqual(TEXT("every parameter rolled"), Rolled, Rune->Parameters.Num());

    // What the getter refuses: a parameter the rune does not roll, a rune nobody socketed, and no rune at all.
    TestFalse(TEXT("a parameter the rune does not roll has no value"), Runes->GetRolledRuneValue(Rune, TAG_RuneRollTestUnrolled, Value));
    TestEqual(TEXT("and the out value is zeroed"), Value, 0.0f);
    UMythicRuneDefinition *Stranger = MakeRollTestRune();
    TestFalse(TEXT("an unknown rune has no value"), Runes->GetRolledRuneValue(Stranger, TAG_RuneRollTestDistance, Value));
    TestFalse(TEXT("nor does no rune"), Runes->GetRolledRuneValue(nullptr, TAG_RuneRollTestDistance, Value));

    // A rune with nothing to roll equips and adds no set; a second rolling rune gets its own set beside the first.
    if (TestTrue(TEXT("the character can open a second socket"), OpenEveryRollTestSocket(Runes) >= 2)) {
        const TMap<FGameplayTag, float> First = SnapshotRolls(Runes, Rune);
        UMythicRuneDefinition *Unrolled = MakeUnrolledTestRune();
        Runes->ServerEquipRune(1, Unrolled);
        TestTrue(TEXT("a rune with no parameters still equips"), Runes->GetRuneInSlot(1) == Unrolled);
        TestEqual(TEXT("and adds no roll set"), Runes->GetRuneRolls().Num(), 1);

        Runes->ServerEquipRune(1, Stranger);
        TestTrue(TEXT("a second rolling rune equips"), Runes->GetRuneInSlot(1) == Stranger);
        TestEqual(TEXT("and rolls its own set"), Runes->GetRuneRolls().Num(), 2);
        TestTrue(TEXT("with its own numbers"), Runes->GetRolledRuneValue(Stranger, TAG_RuneRollTestWindow, Value));
        SameRolls(*this, TEXT("the first rune's set is untouched by the second roll"), First, SnapshotRolls(Runes, Rune));
    }

    AddInfo(FString::Printf(TEXT("parameters rolled: %d of %d, roll sets: %d"), Rolled, Rune->Parameters.Num(), Runes->GetRuneRolls().Num()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneRollKeptAcrossSocketsTest,
    "Mythic.Progression.Runes.Rolls.KeptAcrossSockets",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneRollKeptAcrossSocketsTest::RunTest(const FString &Parameters) {
    FMythicRuneRollFixture Fixture;
    const bool bReady = BuildRuneRollFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;
    if (!TestTrue(TEXT("the character can open at least two sockets"), OpenEveryRollTestSocket(Runes) >= 2)) {
        return false;
    }

    UMythicRuneDefinition *Rune = MakeRollTestRune();
    Runes->ServerEquipRune(0, Rune);
    if (!TestTrue(TEXT("the rune took the first socket"), Runes->GetRuneInSlot(0) == Rune)) {
        return false;
    }
    // Two of the three ranges are continuous, so a fresh roll could not collide with the snapshot by chance.
    const TMap<FGameplayTag, float> Original = SnapshotRolls(Runes, Rune);
    if (!TestEqual(TEXT("the first socket rolled every parameter"), Original.Num(), Rune->Parameters.Num())) {
        return false;
    }

    // Off: the numbers outlive the socket.
    Runes->ServerUnequipRune(0);
    TestNull(TEXT("the socket is empty"), Runes->GetRuneInSlot(0));
    SameRolls(*this, TEXT("an unequipped rune keeps its roll"), Original, SnapshotRolls(Runes, Rune));

    // Back on, in the same socket: no reroll.
    Runes->ServerEquipRune(0, Rune);
    TestTrue(TEXT("the rune is worn again"), Runes->GetRuneInSlot(0) == Rune);
    SameRolls(*this, TEXT("re-socketing keeps the roll"), Original, SnapshotRolls(Runes, Rune));
    TestEqual(TEXT("and writes no second set"), Runes->GetRuneRolls().Num(), 1);

    // Moved: still the same numbers.
    Runes->ServerMoveRune(0, 1);
    TestTrue(TEXT("the rune moved to the second socket"), Runes->GetRuneInSlot(1) == Rune);
    SameRolls(*this, TEXT("a moved rune keeps its roll"), Original, SnapshotRolls(Runes, Rune));

    // Replaced by another rune, then worn once more in a different socket: still the same numbers.
    UMythicRuneDefinition *Other = MakeRollTestRune();
    Runes->ServerEquipRune(1, Other);
    TestTrue(TEXT("another rune replaced it"), Runes->GetRuneInSlot(1) == Other);
    Runes->ServerEquipRune(0, Rune);
    TestTrue(TEXT("the first rune is worn again elsewhere"), Runes->GetRuneInSlot(0) == Rune);
    SameRolls(*this, TEXT("a rune worn again after a swap keeps its roll"), Original, SnapshotRolls(Runes, Rune));
    TestEqual(TEXT("one set per rune ever socketed"), Runes->GetRuneRolls().Num(), 2);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneRollSaveRoundTripTest,
    "Mythic.Progression.Runes.Rolls.SaveRoundTrip",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneRollSaveRoundTripTest::RunTest(const FString &Parameters) {
    FMythicRuneRollFixture Fixture;
    const bool bReady = BuildRuneRollFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }
    UMythicRuneComponent *Runes = Fixture.Runes;

    UMythicRuneDefinition *Rune = MakeRollTestRune();
    Runes->ServerEquipRune(0, Rune);
    if (!TestTrue(TEXT("the rune took the first socket"), Runes->GetRuneInSlot(0) == Rune)) {
        return false;
    }
    const TMap<FGameplayTag, float> Original = SnapshotRolls(Runes, Rune);
    if (!TestEqual(TEXT("the first socket rolled every parameter"), Original.Num(), Rune->Parameters.Num())) {
        return false;
    }

    // The component's SaveGame flags are inert here: the character save writes an explicit field list.
    FSerializedCharacterData Data;
    if (!TestTrue(TEXT("the character serialises"), FSerializedCharacterData::Serialize(Fixture.PlayerState, Data))) {
        return false;
    }
    if (!TestEqual(TEXT("the save carries the roll set"), Data.RuneRolls.Num(), 1)) {
        return false;
    }
    TestTrue(TEXT("the saved set names the rune"), Data.RuneRolls[0].Rune.ToSoftObjectPath() == FSoftObjectPath(Rune));
    TestEqual(TEXT("the saved set carries every value"), Data.RuneRolls[0].Values.Num(), Rune->Parameters.Num());

    // The component verbs on their own: rolls restored, then the sockets. A restore never rerolls.
    {
        TArray<TSoftObjectPtr<UMythicRuneDefinition>> Saved;
        for (const FSoftObjectPath &Path : Data.EquippedRunes) {
            Saved.Emplace(Path);
        }
        Runes->RestoreRuneRolls({});
        float Value = 0.0f;
        if (!TestFalse(TEXT("the in-session rolls are wiped before the restore"), Runes->GetRolledRuneValue(Rune, TAG_RuneRollTestWindow, Value))) {
            return false;
        }
        Runes->RestoreRuneRolls(Data.RuneRolls);
        SameRolls(*this, TEXT("restored rolls read back"), Original, SnapshotRolls(Runes, Rune));
        Runes->RestoreRunes(Saved, Data.UnlockedRuneSlots);
        TestTrue(TEXT("the rune comes back to its socket"), Runes->GetRuneInSlot(0) == Rune);
        TestEqual(TEXT("its passive is granted exactly once"), RollTestCountGrantedFrom(Fixture.ASC, Rune), 1);
        SameRolls(*this, TEXT("re-granting a restored rune keeps its roll"), Original, SnapshotRolls(Runes, Rune));
        TestEqual(TEXT("and writes no second set"), Runes->GetRuneRolls().Num(), 1);
    }

    // The whole character save, the way a reload runs it: the numbers come back exactly.
    {
        Runes->RestoreRuneRolls({});
        Runes->ServerUnequipRune(0);
        if (!TestTrue(TEXT("the character save deserialises"), FSerializedCharacterData::Deserialize(Fixture.PlayerState, Data))) {
            return false;
        }
        TestTrue(TEXT("the reload wears the rune"), Runes->GetRuneInSlot(0) == Rune);
        TestEqual(TEXT("its passive is granted exactly once after the reload"), RollTestCountGrantedFrom(Fixture.ASC, Rune), 1);
        SameRolls(*this, TEXT("a reload keeps the roll"), Original, SnapshotRolls(Runes, Rune));
        TestEqual(TEXT("one set survives the reload"), Runes->GetRuneRolls().Num(), 1);
    }

    // A save older than rune rolls: the worn rune is rolled on restore as if socketed for the first time.
    {
        FSerializedCharacterData Older = Data;
        Older.RuneRolls.Reset();
        Runes->RestoreRuneRolls({});
        Runes->ServerUnequipRune(0);
        if (!TestTrue(TEXT("a save with no rolls deserialises"), FSerializedCharacterData::Deserialize(Fixture.PlayerState, Older))) {
            return false;
        }
        TestTrue(TEXT("the rune is worn"), Runes->GetRuneInSlot(0) == Rune);
        const TMap<FGameplayTag, float> Fresh = SnapshotRolls(Runes, Rune);
        TestEqual(TEXT("a worn rune with no saved roll is rolled on restore"), Fresh.Num(), Rune->Parameters.Num());
        for (const TPair<FGameplayTag, float> &Pair : Fresh) {
            const FRollDefinition &Range = Rune->Parameters[Pair.Key];
            TestTrue(FString::Printf(TEXT("%s = %g sits inside [%g-%g]"), *Pair.Key.ToString(), Pair.Value, Range.Min, Range.Max),
                     Pair.Value >= Range.Min && Pair.Value <= Range.Max);
        }
        TestEqual(TEXT("exactly one set was rolled"), Runes->GetRuneRolls().Num(), 1);
    }

    return true;
}
