// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
// ItemDefinition.h, reached through the fragment headers, names USkeletalMesh without declaring it.
#include "Engine/SkeletalMesh.h"
#include "GameplayTagContainer.h"
#include "Misc/ScopeExit.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "Itemization/Inventory/Fragments/Passive/MythicGemFragment.h"
#include "Itemization/Inventory/Fragments/Passive/SocketsFragment.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Sockets/MythicSocketTypes.h"
#include "Itemization/Sockets/MythicTags_Sockets.h"
#include "Player/MythicPlayerController.h"
#include "UI/Inventory/MythicGemPickerWidget.h"

namespace {

// Every socket verb is authority-gated on the fragment's owning actor, so none of them can be reached without a
// live owner. The player controller is what the loot manager hands an item instance in the game, and it is the
// actor the row reads bags off, so spawning one is the honest fixture rather than a stand-in.
struct FMythicSocketFixture {
    UGameInstance *GameInstance = nullptr;
    AMythicPlayerController *PC = nullptr;
};

bool BuildSocketFixture(FAutomationTestBase &Test, FMythicSocketFixture &Out) {
    if (!Test.TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }
    Out.GameInstance = NewObject<UGameInstance>(GEngine);
    Out.GameInstance->InitializeStandalone();
    UWorld *World = Out.GameInstance->GetWorld();
    if (!Test.TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }
    Out.PC = World->SpawnActor<AMythicPlayerController>();
    if (!Test.TestNotNull(TEXT("the owning controller spawned"), Out.PC)) {
        return false;
    }
    return Test.TestTrue(TEXT("the owner holds authority, so every refusal below is a real refusal"),
                         Out.PC->HasAuthority());
}

FRolledAffix MakeGemAffix() {
    FRollDefinition Roll;
    Roll.Min = 1.0f;
    Roll.Max = 5.0f;
    return FRolledAffix(UMythicAttributeSet_Offense::GetPowerAttribute(), 1, Roll, false);
}

USocketsFragment *MakeSockets(AActor *Owner, int32 Count, const FGameplayTag &Colour) {
    USocketsFragment *Frag = NewObject<USocketsFragment>(Owner);
    Frag->SetOwner(Owner);
    Frag->RolledSocketColor = Colour;
    for (int32 i = 0; i < Count; ++i) {
        Frag->ServerAddSocket();
    }
    return Frag;
}

// AddFragment hard-checks the owner's authority, so an instance whose SetOwner did not take must never reach it.
UMythicItemInstance *MakeItem(AActor *Owner, const FGameplayTag &GemType, int32 GrantedAffixes) {
    UMythicItemInstance *Item = NewObject<UMythicItemInstance>();
    Item->SetOwner(Owner);
    const bool bWantsFragment = GemType.IsValid() || GrantedAffixes > 0;
    if (!Item->GetOwningActor() || !bWantsFragment) {
        return Item;
    }
    UMythicGemFragment *Template = NewObject<UMythicGemFragment>();
    Template->GemType = GemType;
    for (int32 i = 0; i < GrantedAffixes; ++i) {
        Template->GrantedAffixes.Add(MakeGemAffix());
    }
    Item->AddFragment(Template);
    return Item;
}

void SeedSlots(UMythicInventoryComponent *Inv, const TArray<UMythicItemInstance *> &Items) {
    TArray<FMythicInventorySlotEntry> &Slots = Inv->GetAllSlotsMutable();
    Slots.Reset();
    for (UMythicItemInstance *Item : Items) {
        FMythicInventorySlotEntry Entry;
        Entry.SlottedItemInstance = Item;
        Slots.Add(Entry);
    }
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocketGemCompatibilityTest,
    "Mythic.Itemization.Sockets.GemCompatibility",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocketGemCompatibilityTest::RunTest(const FString &Parameters) {
    const FGameplayTag AnyGem = ITEMIZATION_GEM.GetTag();
    const FGameplayTag Ruby = ITEMIZATION_GEM_RUBY.GetTag();
    const FGameplayTag Sapphire = ITEMIZATION_GEM_SAPPHIRE.GetTag();
    const FGameplayTag Universal;

    if (!TestTrue(TEXT("the native gem tags are registered"),
                  AnyGem.IsValid() && Ruby.IsValid() && Sapphire.IsValid())) {
        return false;
    }

    TestTrue(TEXT("a socket with no colour takes a ruby"), FMythicSocketMath::IsGemCompatible(Ruby, Universal));
    TestTrue(TEXT("and a sapphire, so an uncoloured socket restricts nothing"),
             FMythicSocketMath::IsGemCompatible(Sapphire, Universal));
    // The one caller gates on UMythicGemFragment::IsGem() before asking, so the universal branch answering an
    // unset gem type is the caller's guarantee to keep, not this function's to re-check.
    TestTrue(TEXT("an uncoloured socket is decided by the socket alone, never by the gem"),
             FMythicSocketMath::IsGemCompatible(FGameplayTag(), Universal));

    TestTrue(TEXT("a ruby socket takes a ruby"), FMythicSocketMath::IsGemCompatible(Ruby, Ruby));
    TestFalse(TEXT("a ruby socket refuses a sapphire"), FMythicSocketMath::IsGemCompatible(Sapphire, Ruby));
    TestFalse(TEXT("a coloured socket refuses a gem carrying no type at all"),
              FMythicSocketMath::IsGemCompatible(FGameplayTag(), Ruby));

    TestTrue(TEXT("a socket coloured on the parent takes any gem beneath it"),
             FMythicSocketMath::IsGemCompatible(Ruby, AnyGem));
    TestFalse(TEXT("but matching runs one way only - a ruby socket refuses the bare parent type"),
              FMythicSocketMath::IsGemCompatible(AnyGem, Ruby));

    const FGameplayTag Flawless = FGameplayTag::RequestGameplayTag(FName("Itemization.Gem.Ruby.Flawless"), false);
    if (Flawless.IsValid()) {
        TestTrue(TEXT("a ruby socket takes a deeper variant of its own colour"),
                 FMythicSocketMath::IsGemCompatible(Flawless, Ruby));
        TestFalse(TEXT("and a variant socket still refuses the broader colour"),
                  FMythicSocketMath::IsGemCompatible(Ruby, Flawless));
    }
    else {
        AddInfo(TEXT("Itemization.Gem.Ruby.Flawless is not registered; the deeper-variant case is unverified."));
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocketFragmentStateTest,
    "Mythic.Itemization.Sockets.FragmentSocketState",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocketFragmentStateTest::RunTest(const FString &Parameters) {
    FMythicSocketFixture Fixture;
    const bool bReady = BuildSocketFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    const FGameplayTag Ruby = ITEMIZATION_GEM_RUBY.GetTag();
    const FGameplayTag Sapphire = ITEMIZATION_GEM_SAPPHIRE.GetTag();
    if (!TestTrue(TEXT("the native gem tags are registered"), Ruby.IsValid() && Sapphire.IsValid())) {
        return false;
    }

    USocketsFragment *Frag = MakeSockets(Fixture.PC, 3, FGameplayTag());
    if (!TestEqual(TEXT("three sockets were opened on the item"), Frag->Sockets.Num(), 3)) {
        return false;
    }
    TestEqual(TEXT("the count the row reads agrees with the array"), Frag->GetSocketCount(), 3);
    TestEqual(TEXT("and none of them is set yet"), Frag->GetFilledSocketCount(), 0);

    Frag->ServerSocketGem(1, Ruby, {MakeGemAffix()});
    TestEqual(TEXT("socketing changes no socket count"), Frag->GetSocketCount(), 3);
    TestEqual(TEXT("exactly one socket is now set"), Frag->GetFilledSocketCount(), 1);
    if (TestTrue(TEXT("the array is still three long"), Frag->Sockets.IsValidIndex(2))) {
        TestTrue(TEXT("the socket that took the gem is filled"), Frag->Sockets[1].bFilled);
        TestEqual(TEXT("and remembers which gem type it took"),
                  Frag->Sockets[1].SocketedGemType.ToString(), Ruby.ToString());
        TestEqual(TEXT("with the gem's granted affixes copied onto it"), Frag->Sockets[1].SocketedAffixes.Num(), 1);
        TestFalse(TEXT("its neighbours are untouched"), Frag->Sockets[0].bFilled);
        TestFalse(TEXT("on both sides"), Frag->Sockets[2].bFilled);
    }

    // A second gem into a set socket would silently replace the first and orphan the affixes it already applied.
    Frag->ServerSocketGem(1, Sapphire, {MakeGemAffix()});
    TestEqual(TEXT("a socket that already holds a gem takes no second one"), Frag->GetFilledSocketCount(), 1);
    if (Frag->Sockets.IsValidIndex(1)) {
        TestEqual(TEXT("and keeps the gem it had"), Frag->Sockets[1].SocketedGemType.ToString(), Ruby.ToString());
    }

    // The gem type comes back so the caller can mint the gem into the bag; losing it silently destroys the gem.
    const FGameplayTag Removed = Frag->ServerUnsocketGem(1);
    TestEqual(TEXT("unsocketing hands back the gem type that was in the slot"), Removed.ToString(), Ruby.ToString());
    TestEqual(TEXT("the socket count never moves"), Frag->GetSocketCount(), 3);
    TestEqual(TEXT("and the slot reads empty again"), Frag->GetFilledSocketCount(), 0);
    if (TestTrue(TEXT("the array is still three long"), Frag->Sockets.IsValidIndex(1))) {
        TestFalse(TEXT("the slot is cleared"), Frag->Sockets[1].bFilled);
        TestFalse(TEXT("its gem type is cleared with it"), Frag->Sockets[1].SocketedGemType.IsValid());
        TestEqual(TEXT("and so are the affixes it was granting"), Frag->Sockets[1].SocketedAffixes.Num(), 0);
    }

    TestFalse(TEXT("unsocketing an empty socket hands back nothing"), Frag->ServerUnsocketGem(1).IsValid());
    TestEqual(TEXT("and takes nothing away"), Frag->GetSocketCount(), 3);

    // The denominator for every refusal above: the emptied socket takes a gem again.
    Frag->ServerSocketGem(1, Sapphire, {MakeGemAffix()});
    TestEqual(TEXT("the emptied socket takes the gem it just refused"), Frag->GetFilledSocketCount(), 1);
    if (Frag->Sockets.IsValidIndex(1)) {
        TestEqual(TEXT("and it is the new gem"), Frag->Sockets[1].SocketedGemType.ToString(), Sapphire.ToString());
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocketOutOfRangeTest,
    "Mythic.Itemization.Sockets.OutOfRangeRefused",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocketOutOfRangeTest::RunTest(const FString &Parameters) {
    FMythicSocketFixture Fixture;
    const bool bReady = BuildSocketFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    const FGameplayTag Ruby = ITEMIZATION_GEM_RUBY.GetTag();
    if (!TestTrue(TEXT("the native gem tags are registered"), Ruby.IsValid())) {
        return false;
    }

    USocketsFragment *Frag = MakeSockets(Fixture.PC, 2, FGameplayTag());
    if (!TestEqual(TEXT("two sockets were opened on the item"), Frag->Sockets.Num(), 2)) {
        return false;
    }

    // A well the row never drew still names an index. Growing the array to meet it would hand the player a socket
    // the item never rolled.
    Frag->ServerSocketGem(5, Ruby, {MakeGemAffix()});
    TestEqual(TEXT("socketing past the end never grows the array"), Frag->Sockets.Num(), 2);
    TestEqual(TEXT("and sets nothing"), Frag->GetFilledSocketCount(), 0);

    Frag->ServerSocketGem(-1, Ruby, {MakeGemAffix()});
    TestEqual(TEXT("a negative index never grows the array either"), Frag->Sockets.Num(), 2);
    TestEqual(TEXT("and sets nothing"), Frag->GetFilledSocketCount(), 0);

    TestFalse(TEXT("unsocketing past the end hands back nothing"), Frag->ServerUnsocketGem(5).IsValid());
    TestEqual(TEXT("and leaves the array alone"), Frag->Sockets.Num(), 2);

    // The denominator: an index that IS in range fills, so the two refusals above were the range check and not an
    // inert fixture that could never socket anything.
    Frag->ServerSocketGem(0, Ruby, {MakeGemAffix()});
    TestEqual(TEXT("an index inside the array fills its socket"), Frag->GetFilledSocketCount(), 1);
    if (TestTrue(TEXT("the array is still two long"), Frag->Sockets.IsValidIndex(0))) {
        TestTrue(TEXT("and it is the socket that was named"), Frag->Sockets[0].bFilled);
    }

    // The same ceiling on the other verb: sockets are rolled, and nothing may add past the table's hard cap.
    USocketsFragment *Growable = MakeSockets(Fixture.PC, 0, FGameplayTag());
    const int32 HardCap = FMythicSocketMath::DefaultSocketCountTable().HardCap;
    if (TestTrue(TEXT("the default table authors a cap worth testing"), HardCap > 0)) {
        for (int32 i = 0; i < HardCap; ++i) {
            Growable->ServerAddSocket();
        }
        TestEqual(TEXT("sockets can be added up to the table's hard cap"), Growable->GetSocketCount(), HardCap);
        TestFalse(TEXT("the add past the cap is refused"), Growable->ServerAddSocket());
        TestEqual(TEXT("and does not grow the array"), Growable->GetSocketCount(), HardCap);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocketGemIdentityTest,
    "Mythic.Itemization.Sockets.GemIdentity",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocketGemIdentityTest::RunTest(const FString &Parameters) {
    FMythicSocketFixture Fixture;
    const bool bReady = BuildSocketFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    const FGameplayTag Ruby = ITEMIZATION_GEM_RUBY.GetTag();
    if (!TestTrue(TEXT("the native gem tags are registered"), Ruby.IsValid())) {
        return false;
    }

    // The picker's notion of "is this a gem" has to be the server's, or the list offers a row ServerSocketGem
    // will refuse - a click that does nothing, which is the failure the whole row was shaped to avoid.
    TestFalse(TEXT("nothing is not a gem"), UMythicGemPickerWidget::GetGemType(nullptr).IsValid());

    UMythicItemInstance *Plain = MakeItem(Fixture.PC, FGameplayTag(), 0);
    TestFalse(TEXT("an item carrying no gem fragment is not a gem"),
              UMythicGemPickerWidget::GetGemType(Plain).IsValid());

    UMythicItemInstance *Typeless = MakeItem(Fixture.PC, FGameplayTag(), 1);
    TestFalse(TEXT("a gem fragment with affixes but no type is not a gem"),
              UMythicGemPickerWidget::GetGemType(Typeless).IsValid());

    UMythicItemInstance *Barren = MakeItem(Fixture.PC, Ruby, 0);
    TestFalse(TEXT("a gem fragment that grants nothing is not a gem the picker may offer"),
              UMythicGemPickerWidget::GetGemType(Barren).IsValid());

    UMythicItemInstance *Gem = MakeItem(Fixture.PC, Ruby, 1);
    TestEqual(TEXT("a typed gem that grants an affix answers with its type"),
              UMythicGemPickerWidget::GetGemType(Gem).ToString(), Ruby.ToString());

    // Both halves of the identity must reach the socket filter, or a barren gem is offered for a ruby socket.
    TestFalse(TEXT("so a barren gem fits no coloured socket"),
              FMythicSocketMath::IsGemCompatible(UMythicGemPickerWidget::GetGemType(Barren), Ruby));
    TestTrue(TEXT("while the usable one does"),
             FMythicSocketMath::IsGemCompatible(UMythicGemPickerWidget::GetGemType(Gem), Ruby));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocketCarriedGemsTest,
    "Mythic.Itemization.Sockets.CarriedGems",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocketCarriedGemsTest::RunTest(const FString &Parameters) {
    FMythicSocketFixture Fixture;
    const bool bReady = BuildSocketFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    const FGameplayTag Ruby = ITEMIZATION_GEM_RUBY.GetTag();
    const FGameplayTag Sapphire = ITEMIZATION_GEM_SAPPHIRE.GetTag();
    if (!TestTrue(TEXT("the native gem tags are registered"), Ruby.IsValid() && Sapphire.IsValid())) {
        return false;
    }

    UMythicInventoryComponent *Inv = Fixture.PC->GetInventoryComponent();
    if (!TestNotNull(TEXT("the controller carries an inventory"), Inv)) {
        return false;
    }

    UMythicItemInstance *RubyGem = MakeItem(Fixture.PC, Ruby, 1);
    UMythicItemInstance *SapphireGem = MakeItem(Fixture.PC, Sapphire, 1);
    UMythicItemInstance *Plain = MakeItem(Fixture.PC, FGameplayTag(), 0);
    UMythicItemInstance *Barren = MakeItem(Fixture.PC, Ruby, 0);
    SeedSlots(Inv, {RubyGem, Plain, Barren, nullptr, SapphireGem});

    TArray<UMythicItemInstance *> Carried;
    Carried.Add(Plain);
    UMythicGemPickerWidget::CollectGems(Fixture.PC, Carried);

    // Five slots in, two out: the scan runs AND the filter bites. A list of five or of none would not say that.
    if (TestEqual(TEXT("only the usable gems come back out of five slots"), Carried.Num(), 2)) {
        TestEqual(TEXT("in bag order, first slot first"), Carried[0], RubyGem);
        TestEqual(TEXT("then the later one"), Carried[1], SapphireGem);
    }
    TestFalse(TEXT("the caller's stale entry is cleared, never appended to"), Carried.Contains(Plain));
    TestFalse(TEXT("and a gem granting nothing is left out of the list"), Carried.Contains(Barren));

    // A picker opened before the controller exists must show its empty state, not last open's rows.
    UMythicGemPickerWidget::CollectGems(nullptr, Carried);
    TestEqual(TEXT("no controller means no gems, and the previous list is cleared"), Carried.Num(), 0);

    // Bags holding no gem at all is the case the picker's empty state exists for; it must read as zero, not as
    // "everything you carry".
    SeedSlots(Inv, {Plain, Barren});
    UMythicGemPickerWidget::CollectGems(Fixture.PC, Carried);
    TestEqual(TEXT("bags with no usable gem collect none"), Carried.Num(), 0);

    return true;
}

#endif
