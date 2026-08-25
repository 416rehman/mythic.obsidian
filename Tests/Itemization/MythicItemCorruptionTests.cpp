#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Loot/MythicWorldItem.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicItemCorruptionTest,
                                 "Mythic.Itemization.Corruption",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * Corruption is the risk half of crafting, so it has to refuse the actual craft verbs.
 *
 * Testing CanApplyCraftOp alone proves nothing: the verbs are what a player reaches, and deleting their calls to
 * the gate left the suite green. Both verbs early-out without an authoritative owner, so the fragment is given a
 * real one rather than tested in isolation.
 */
bool FMythicItemCorruptionTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }

    // A world item is what actually owns an item's fragments, and SetOwner requires an owner that replicates
    // subobjects through the registered list - which a player state does not.
    AActor *Owner = World->SpawnActor<AMythicWorldItem>();
    if (!TestNotNull(TEXT("an owner spawned"), Owner) || !TestTrue(TEXT("the owner holds authority"), Owner->HasAuthority())) {
        return false;
    }
    if (!TestTrue(TEXT("the owner replicates subobjects through the registered list"),
                  Owner->IsUsingRegisteredSubObjectList())) {
        return false;
    }

    UAffixesFragment *Fragment = NewObject<UAffixesFragment>(Owner);
    Fragment->SetOwner(Owner);
    if (!TestNotNull(TEXT("the fragment has an authoritative owner"), Fragment->GetOwningActor())) {
        return false;
    }

    FRolledAffix Affix;
    Affix.bIsLocked = false;
    Fragment->AffixesRuntimeReplicatedData.RolledAffixes.Add(Affix);

    FText Reason;

    // The verb works before corruption, or a refusal afterwards proves nothing.
    TestTrue(TEXT("a clean item accepts a craft op"), Fragment->CanApplyCraftOp(Reason));
    Fragment->SetAffixLocked(0, true);
    if (!TestTrue(TEXT("locking an affix on a clean item takes effect"),
                  Fragment->AffixesRuntimeReplicatedData.RolledAffixes[0].bIsLocked)) {
        return false;
    }

    // The writer the flag never had.
    Fragment->ServerCorruptItem();
    TestTrue(TEXT("corrupting the item sets the flag"), Fragment->IsCorrupted());
    TestFalse(TEXT("and the gate now refuses"), Fragment->CanApplyCraftOp(Reason));
    TestFalse(TEXT("with a reason a player can read"), Reason.IsEmpty());

    // The point of the whole feature: the verb itself is refused, not merely the gate function.
    Fragment->SetAffixLocked(0, false);
    TestTrue(TEXT("unlocking is refused on a corrupted item, so the lock still stands"),
             Fragment->AffixesRuntimeReplicatedData.RolledAffixes[0].bIsLocked);

    // Corruption is one-way; nothing may lift it.
    Fragment->ServerCorruptItem();
    TestTrue(TEXT("corruption stays set"), Fragment->IsCorrupted());

    return true;
}

#endif
