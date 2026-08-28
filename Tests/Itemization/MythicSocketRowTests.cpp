// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
// ItemDefinition.h, reached through the fragment headers, names USkeletalMesh without declaring it.
#include "Engine/SkeletalMesh.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GameplayTagContainer.h"
#include "Internationalization/StringTableRegistry.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Misc/ScopeExit.h"

#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Itemization/Inventory/Fragments/Passive/MythicGemFragment.h"
#include "Itemization/Inventory/Fragments/Passive/SocketsFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Sockets/MythicSocketTypes.h"
#include "Itemization/Sockets/MythicTags_Sockets.h"
#include "Player/MythicPlayerController.h"
#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"
#include "UI/Inventory/MythicGemPickerWidget.h"

namespace {

const FName SocketStringTableId(TEXT("MythicSocketRowTests"));

// Every socket verb is authority-gated on the fragment's owning actor, so none of them can be reached without a
// live owner. The player controller is what the loot manager hands an item instance in the game, and it is the
// actor the row reads bags off, so spawning one is the honest fixture rather than a stand-in.
struct FMythicSocketFixture {
    ~FMythicSocketFixture() {
        FStringTableRegistry::Get().UnregisterStringTable(SocketStringTableId);
    }

    UGameInstance *GameInstance = nullptr;
    AMythicPlayerController *PC = nullptr;
    UMythicAffixDefinition *AffixDefinition = nullptr;
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

    FStringTableRegistry::Get().UnregisterStringTable(SocketStringTableId);
    FStringTableRegistry::Get().Internal_NewLocTable(
        SocketStringTableId, TEXT("MythicSocketRowTests"));
    auto AddText = [](const TCHAR *Key, const TCHAR *Source) {
        FStringTableRegistry::Get().Internal_SetLocTableEntry(SocketStringTableId, Key, Source);
        return FText::FromStringTable(SocketStringTableId, Key);
    };

    UMythicStatCategoryDefinition *Category =
        NewObject<UMythicStatCategoryDefinition>(GetTransientPackage());
    Category->DeveloperName = TEXT("Offense");
    Category->DesignerPurpose = TEXT("Socket integration fixture.");
    Category->CategoryTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Stat.Category.Offense")), true);
    Category->DisplayName = AddText(TEXT("Offense"), TEXT("Offense"));

    UMythicStatDefinition *Stat =
        NewObject<UMythicStatDefinition>(GetTransientPackage());
    Stat->DeveloperName = TEXT("Power");
    Stat->DesignerPurpose = TEXT("Socket integration fixture.");
    Stat->StatTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Stat.Attribute.Power")), true);
    Stat->Attribute = UMythicAttributeSet_Offense::GetPowerAttribute();
    Stat->DisplayName = AddText(TEXT("PowerStat"), TEXT("Power"));
    Stat->Category.SetAsset(Category);
    Stat->bCanBeAffixTarget = true;

    Out.AffixDefinition = NewObject<UMythicAffixDefinition>(GetTransientPackage());
    Out.AffixDefinition->DeveloperName = TEXT("Power");
    Out.AffixDefinition->DesignerPurpose = TEXT("Socket integration fixture.");
    Out.AffixDefinition->AffixTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Itemization.Affix.Power")), true);
    Out.AffixDefinition->DisplayNameTemplate = AddText(
        TEXT("PowerAffix"), TEXT("Power"));
    Out.AffixDefinition->TargetStat.SetAsset(Stat);
    Out.AffixDefinition->ModifierOp = EGameplayModOp::AddBase;
    Out.AffixDefinition->StackingRule = EMythicAffixStackingRule::StackAll;
    FMythicAffixTierProgressionDefinition &Progression =
        Out.AffixDefinition->TierProgressions.AddDefaulted_GetRef();
    Progression.DeveloperName = TEXT("Fallback");
    Progression.TuningContext = TEXT("Core");
    FMythicAffixTierDefinition &Tier = Progression.Tiers.AddDefaulted_GetRef();
    Tier.DeveloperName = TEXT("Rank1");
    Tier.Magnitude.Min = 1.0f;
    Tier.Magnitude.Max = 5.0f;

    UMythicItemizationDataRegistrySubsystem *Registry =
        Out.GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>();
    if (!Test.TestNotNull(TEXT("the itemization registry exists"), Registry)) {
        return false;
    }
    TArray<UObject *> Assets{Category, Stat, Out.AffixDefinition};
    TArray<FText> Errors;
    const bool bPublished = Registry->PublishCoreSemanticAssetsForTests(Assets, Errors);
    for (const FText &Error : Errors) {
        Test.AddError(Error.ToString());
    }
    return Test.TestTrue(TEXT("the owner holds authority, so every refusal below is a real refusal"),
                         Out.PC->HasAuthority())
        && Test.TestTrue(TEXT("the socket fixture publishes a complete typed semantic closure"),
                         bPublished);
}

FRolledAffix MakeGemAffix(UMythicAffixDefinition *Definition,
                          const FGuid SourceGemItemGuid = FGuid::NewGuid()) {
    FRolledAffix Affix;
    Affix.RollGuid = FGuid::NewGuid();
    Affix.AffixDefinition.SetAsset(Definition);
    Affix.TierRank = 1;
    Affix.Magnitude = 3.0f;
    Affix.Provenance.RollGroup = AFFIX_ROLL_GROUP_PREFIX;
    Affix.Provenance.SourceKind = AFFIX_SOURCE_GEM;
    Affix.Provenance.SourceItemGuid = SourceGemItemGuid;
    Affix.Provenance.GeneratedItemLevel = 1;
    Affix.Provenance.AlgorithmVersion = 1;
    Affix.bIsLocked = true;
    return Affix;
}

bool SocketTestGem(USocketsFragment *Fragment, UMythicAffixDefinition *Definition,
                   const int32 SocketIndex, const FGameplayTag &GemType) {
    const FGuid SourceGemItemGuid = FGuid::NewGuid();
    const TArray<FRolledAffix> Grants{MakeGemAffix(Definition, SourceGemItemGuid)};
    return Fragment && Fragment->ServerSocketGem(
        SocketIndex, GemType, SourceGemItemGuid, Grants);
}

USocketsFragment *MakeSockets(AActor *Owner, int32 Count, const FGameplayTag &Colour) {
    // Production item instances are outered to their owning actor/component, which gives nested fragments a
    // world and therefore the GameInstance-scoped typed registry. A transient-package outer makes
    // BuildSocketCandidates fail closed at ResolveRegistry even when the fixture published a valid closure.
    UMythicItemInstance *Host = NewObject<UMythicItemInstance>(Owner);
    Host->SetOwner(Owner);
    UItemDefinition *Definition = NewObject<UItemDefinition>(Owner);
    Definition->StackSizeMax = 1;
    USocketsFragment *Template = NewObject<USocketsFragment>(Definition);
    Template->RolledSocketColor = Colour;
    Definition->Fragments.Add(Template);
    Host->InitializeFixtureForTests(Definition, 1, 1);
    USocketsFragment *Frag = const_cast<USocketsFragment *>(Host->GetFragment<USocketsFragment>());
    if (!Frag) {
        return nullptr;
    }
    for (int32 i = 0; i < Count; ++i) {
        if (!Frag->ServerAddSocket()) return nullptr;
    }
    return Frag;
}

// AddFragment hard-checks the owner's authority, so an instance whose SetOwner did not take must never reach it.
UMythicItemInstance *MakeItem(AActor *Owner, UMythicAffixDefinition *Definition,
                              const FGameplayTag &GemType, int32 GrantCount) {
    UMythicItemInstance *Item = NewObject<UMythicItemInstance>(Owner);
    Item->SetOwner(Owner);
    Item->EnsureNewItemInstanceGuid();
    const bool bWantsFragment = GemType.IsValid() || GrantCount > 0;
    if (!Item->GetOwningActor() || !bWantsFragment) {
        return Item;
    }
    UMythicGemFragment *Template = NewObject<UMythicGemFragment>();
    Template->GemType = GemType;
    TArray<FRolledAffix> Snapshots;
    for (int32 i = 0; i < GrantCount; ++i) {
        Snapshots.Add(MakeGemAffix(Definition, Item->GetItemInstanceGuid()));
    }
    Template->GrantedAffixSnapshots.ReplaceAll(MoveTemp(Snapshots));
    Item->AddFragmentFixtureForTests(Template);
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
    if (!TestEqual(TEXT("three sockets were opened on the item"), Frag->GetSocketCount(), 3)) {
        return false;
    }
    TestEqual(TEXT("the count the row reads agrees with the array"), Frag->GetSocketCount(), 3);
    TestEqual(TEXT("and none of them is set yet"), Frag->GetFilledSocketCount(), 0);

    TestTrue(TEXT("the canonical socket route accepts a source-provenanced gem"),
             SocketTestGem(Frag, Fixture.AffixDefinition, 1, Ruby));
    TestEqual(TEXT("socketing changes no socket count"), Frag->GetSocketCount(), 3);
    TestEqual(TEXT("exactly one socket is now set"), Frag->GetFilledSocketCount(), 1);
    if (TestNotNull(TEXT("the array is still three long"), Frag->GetSocketState(2))) {
        TestTrue(TEXT("the socket that took the gem is filled"), Frag->IsSocketFilled(1));
        TestEqual(TEXT("and remembers which gem type it took"),
                  Frag->GetSocketedGemType(1).ToString(), Ruby.ToString());
        const FMythicReplicatedSocketItem *Filled = Frag->GetSocketState(1);
        TestEqual(TEXT("with the gem's granted affixes copied onto it"),
                  Filled ? Filled->SocketedAffixSnapshots.Num() : 0, 1);
        TestFalse(TEXT("its neighbours are untouched"), Frag->IsSocketFilled(0));
        TestFalse(TEXT("on both sides"), Frag->IsSocketFilled(2));
    }

    // A second gem into a set socket would silently replace the first and orphan the affixes it already applied.
    TestFalse(TEXT("the canonical route refuses a second gem"),
              SocketTestGem(Frag, Fixture.AffixDefinition, 1, Sapphire));
    TestEqual(TEXT("a socket that already holds a gem takes no second one"), Frag->GetFilledSocketCount(), 1);
    if (Frag->GetSocketState(1)) {
        TestEqual(TEXT("and keeps the gem it had"), Frag->GetSocketedGemType(1).ToString(), Ruby.ToString());
    }

    // The gem type comes back so the caller can mint the gem into the bag; losing it silently destroys the gem.
    const FGameplayTag Removed = Frag->ServerUnsocketGem(1);
    TestEqual(TEXT("unsocketing hands back the gem type that was in the slot"), Removed.ToString(), Ruby.ToString());
    TestEqual(TEXT("the socket count never moves"), Frag->GetSocketCount(), 3);
    TestEqual(TEXT("and the slot reads empty again"), Frag->GetFilledSocketCount(), 0);
    if (TestNotNull(TEXT("the array is still three long"), Frag->GetSocketState(1))) {
        TestFalse(TEXT("the slot is cleared"), Frag->IsSocketFilled(1));
        TestFalse(TEXT("its gem type is cleared with it"), Frag->GetSocketedGemType(1).IsValid());
        TestEqual(TEXT("and so are the affixes it was granting"),
                  Frag->GetSocketState(1)->SocketedAffixSnapshots.Num(), 0);
    }

    TestFalse(TEXT("unsocketing an empty socket hands back nothing"), Frag->ServerUnsocketGem(1).IsValid());
    TestEqual(TEXT("and takes nothing away"), Frag->GetSocketCount(), 3);

    // The denominator for every refusal above: the emptied socket takes a gem again.
    TestTrue(TEXT("the emptied socket accepts a new source-provenanced gem"),
             SocketTestGem(Frag, Fixture.AffixDefinition, 1, Sapphire));
    TestEqual(TEXT("the emptied socket takes the gem it just refused"), Frag->GetFilledSocketCount(), 1);
    if (Frag->GetSocketState(1)) {
        TestEqual(TEXT("and it is the new gem"), Frag->GetSocketedGemType(1).ToString(), Sapphire.ToString());
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
    if (!TestEqual(TEXT("two sockets were opened on the item"), Frag->GetSocketCount(), 2)) {
        return false;
    }

    // A well the row never drew still names an index. Growing the array to meet it would hand the player a socket
    // the item never rolled.
    TestFalse(TEXT("socketing past the end is refused"),
              SocketTestGem(Frag, Fixture.AffixDefinition, 5, Ruby));
    TestEqual(TEXT("socketing past the end never grows the array"), Frag->GetSocketCount(), 2);
    TestEqual(TEXT("and sets nothing"), Frag->GetFilledSocketCount(), 0);

    TestFalse(TEXT("a negative socket index is refused"),
              SocketTestGem(Frag, Fixture.AffixDefinition, -1, Ruby));
    TestEqual(TEXT("a negative index never grows the array either"), Frag->GetSocketCount(), 2);
    TestEqual(TEXT("and sets nothing"), Frag->GetFilledSocketCount(), 0);

    TestFalse(TEXT("unsocketing past the end hands back nothing"), Frag->ServerUnsocketGem(5).IsValid());
    TestEqual(TEXT("and leaves the array alone"), Frag->GetSocketCount(), 2);

    // The denominator: an index that IS in range fills, so the two refusals above were the range check and not an
    // inert fixture that could never socket anything.
    TestTrue(TEXT("an in-range canonical insertion succeeds"),
             SocketTestGem(Frag, Fixture.AffixDefinition, 0, Ruby));
    TestEqual(TEXT("an index inside the array fills its socket"), Frag->GetFilledSocketCount(), 1);
    if (TestNotNull(TEXT("the array is still two long"), Frag->GetSocketState(0))) {
        TestTrue(TEXT("and it is the socket that was named"), Frag->IsSocketFilled(0));
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

    UMythicItemInstance *Plain = MakeItem(
        Fixture.PC, Fixture.AffixDefinition, FGameplayTag(), 0);
    TestFalse(TEXT("an item carrying no gem fragment is not a gem"),
              UMythicGemPickerWidget::GetGemType(Plain).IsValid());

    UMythicItemInstance *Typeless = MakeItem(
        Fixture.PC, Fixture.AffixDefinition, FGameplayTag(), 1);
    TestFalse(TEXT("a gem fragment with affixes but no type is not a gem"),
              UMythicGemPickerWidget::GetGemType(Typeless).IsValid());

    UMythicItemInstance *Barren = MakeItem(
        Fixture.PC, Fixture.AffixDefinition, Ruby, 0);
    TestFalse(TEXT("a gem fragment that grants nothing is not a gem the picker may offer"),
              UMythicGemPickerWidget::GetGemType(Barren).IsValid());

    UMythicItemInstance *Gem = MakeItem(
        Fixture.PC, Fixture.AffixDefinition, Ruby, 1);
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

    UMythicItemInstance *RubyGem = MakeItem(
        Fixture.PC, Fixture.AffixDefinition, Ruby, 1);
    UMythicItemInstance *SapphireGem = MakeItem(
        Fixture.PC, Fixture.AffixDefinition, Sapphire, 1);
    UMythicItemInstance *Plain = MakeItem(
        Fixture.PC, Fixture.AffixDefinition, FGameplayTag(), 0);
    UMythicItemInstance *Barren = MakeItem(
        Fixture.PC, Fixture.AffixDefinition, Ruby, 0);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCanonicalSocketProvenanceTest,
    "Mythic.Itemization.Sockets.CanonicalProvenanceAndStableRollGuid",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCanonicalSocketProvenanceTest::RunTest(const FString &Parameters) {
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

    USocketsFragment *Fragment = MakeSockets(Fixture.PC, 1, FGameplayTag());
    if (!TestNotNull(TEXT("canonical socket fixture exists"), Fragment)) {
        return false;
    }
    const FMythicReplicatedSocketItem *EmptySocket = Fragment->GetSocketState(0);
    if (!TestNotNull(TEXT("authority generated a stable socket identity"), EmptySocket)) {
        return false;
    }
    const FGuid SocketGuid = EmptySocket->SocketGuid;
    const FGuid SourceGemGuid(100, 200, 300, 400);
    const FGameplayTag Ruby = ITEMIZATION_GEM_RUBY.GetTag();
    const TArray<FRolledAffix> Grants{
        MakeGemAffix(Fixture.AffixDefinition, SourceGemGuid)};
    const FGuid ExpectedRollGuid = USocketsFragment::DeriveSocketAffixRollGuid(
        Fragment->GetOwningItemInstance()->GetItemInstanceGuid(), SocketGuid,
        SourceGemGuid, Grants[0].RollGuid);

    TestTrue(TEXT("canonical insertion commits"),
             Fragment->ServerSocketGem(0, Ruby, SourceGemGuid, Grants));
    const FMythicReplicatedSocketItem *Filled = Fragment->GetSocketState(0);
    if (!TestNotNull(TEXT("filled socket state remains addressable"), Filled)
        || !TestEqual(TEXT("one immutable grant was copied"), Filled->SocketedAffixSnapshots.Num(), 1)) {
        return false;
    }
    const FRolledAffix &Copied = Filled->SocketedAffixSnapshots[0];
    TestEqual(TEXT("host rekey uses the stable canonical tuple"), Copied.RollGuid, ExpectedRollGuid);
    TestEqual(TEXT("source gem identity remains snapshot provenance"),
              Copied.Provenance.SourceItemGuid, SourceGemGuid);
    TestEqual(TEXT("origin socket identity remains snapshot provenance"),
              Copied.Provenance.OriginSocketGuid, SocketGuid);
    TestEqual(TEXT("socket source kind is explicit"), Copied.Provenance.SourceKind, AFFIX_SOURCE_SOCKET.GetTag());
    TestEqual(TEXT("copying never rescales the gem magnitude"),
              Copied.Magnitude, Grants[0].Magnitude);

    TestEqual(TEXT("unsocket returns the gem type"), Fragment->ServerUnsocketGem(0), Ruby);
    TestTrue(TEXT("reinserting the same grant into the same socket succeeds"),
             Fragment->ServerSocketGem(0, Ruby, SourceGemGuid, Grants));
    TestEqual(TEXT("reinsertion deterministically recreates the same host RollGuid"),
              Fragment->GetSocketState(0)->SocketedAffixSnapshots[0].RollGuid, ExpectedRollGuid);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSocketSaveFramingTest,
    "Mythic.Itemization.Sockets.SaveRoundTripResetsDeltaBookkeeping",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSocketSaveFramingTest::RunTest(const FString &Parameters) {
    UMythicAffixDefinition *Definition = NewObject<UMythicAffixDefinition>(
        GetTransientPackage());
    Definition->AffixTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Itemization.Affix.Power")), true);
    FMythicReplicatedSocketArray Source;
    FMythicReplicatedSocketItem &Socket = Source.Items.AddDefaulted_GetRef();
    Socket.SocketGuid = FGuid(11, 12, 13, 14);
    Socket.SocketedGemType = ITEMIZATION_GEM_RUBY.GetTag();
    Socket.SourceGemItemGuid = FGuid(21, 22, 23, 24);
    Socket.bFilled = true;
    Socket.SocketedAffixSnapshots.Add(MakeGemAffix(
        Definition, Socket.SourceGemItemGuid));
    Socket.SocketedAffixSnapshots[0].Provenance.SourceKind = AFFIX_SOURCE_SOCKET;
    Socket.SocketedAffixSnapshots[0].Provenance.SourceItemGuid = Socket.SourceGemItemGuid;
    Socket.SocketedAffixSnapshots[0].Provenance.OriginSocketGuid = Socket.SocketGuid;

    FBufferArchive Buffer;
    FObjectAndNameAsStringProxyArchive SaveArchive(Buffer, false);
    SaveArchive.ArIsSaveGame = true;
    Source.Serialize(SaveArchive);
    if (!TestFalse(TEXT("bounded socket state saves without error"), SaveArchive.IsError())) {
        return false;
    }

    FMythicReplicatedSocketArray Restored;
    FMemoryReader Reader(Buffer, true);
    FObjectAndNameAsStringProxyArchive LoadArchive(Reader, true);
    LoadArchive.ArIsSaveGame = true;
    Restored.Serialize(LoadArchive);
    if (!TestFalse(TEXT("bounded socket state loads without error"), LoadArchive.IsError())
        || !TestEqual(TEXT("one socket roundtrips"), Restored.Items.Num(), 1)) {
        return false;
    }
    const FMythicReplicatedSocketItem &Loaded = Restored.Items[0];
    TestEqual(TEXT("socket identity roundtrips"), Loaded.SocketGuid, Socket.SocketGuid);
    TestEqual(TEXT("source gem identity roundtrips"), Loaded.SourceGemItemGuid, Socket.SourceGemItemGuid);
    if (!TestEqual(TEXT("ordinary snapshot array roundtrips"),
                   Loaded.SocketedAffixSnapshots.Num(), 1)) {
        return false;
    }
    TestEqual(TEXT("socket save retains the direct Affix Definition reference"),
              Loaded.SocketedAffixSnapshots[0].AffixDefinition.GetAsset(), Definition);
    TestEqual(TEXT("socket save retains the singular rolled magnitude"),
              Loaded.SocketedAffixSnapshots[0].Magnitude,
              Socket.SocketedAffixSnapshots[0].Magnitude);
    TestEqual(TEXT("snapshot socket origin roundtrips"),
              Loaded.SocketedAffixSnapshots[0].Provenance.OriginSocketGuid, Socket.SocketGuid);
    TestEqual(TEXT("replication id is rebuilt, never persisted"), Loaded.ReplicationID, INDEX_NONE);
    TestEqual(TEXT("replication key is rebuilt, never persisted"), Loaded.ReplicationKey, INDEX_NONE);
    return true;
}

#endif
