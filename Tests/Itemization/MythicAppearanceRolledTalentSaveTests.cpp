#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Tests/Combat/MythicTestEffects.h"
#include "Itemization/Inventory/Fragments/Passive/TalentFragment.h"
#include "Itemization/Inventory/MythicItemFactorySubsystem.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Player/MythicPlayerController.h"
#include "Subsystem/SaveSystem/Character/SavedInventory.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicSavedRolledTalentTextTest,
    "Mythic.Appearance.SavedInventoryRolledTalentText",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMythicSavedRolledTalentTextTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* Owner=Game->GetWorld()->SpawnActor<AMythicPlayerController>();
    if (!TestNotNull(TEXT("real authority inventory owner"),Owner)) return false;
    auto Inventories=Owner->GetAllInventoryComponents();
    if (!TestTrue(TEXT("production inventory exists"),Inventories.Num()>0)) return false;
    auto* Inventory=Inventories[0];

    // A small real profile; both slots are carried, so this serialization test
    // neither grants the passive nor requires a gameplay/ASC fixture.
    auto* Profile=NewObject<UInventoryProfile>(Inventory);
    auto* SlotDefinition=NewObject<UInventorySlotDefinition>(Profile);
    FInventoryProfileEntry Entry; Entry.SlotDefinition=SlotDefinition; Entry.Count=2;
    FInventorySlotGroup Group; Group.SlotDomain=EMythicInventorySlotDomain::Carried;
    Group.bAppliesEquipmentEffects=false; Group.Slots.Add(Entry);
    Profile->SlotGroups.Add(FGameplayTag::RequestGameplayTag(TEXT("Inventory.Group.Inv.Equipment")),Group);
    Inventory->InventoryProfile=Profile; Inventory->InitializeSlots();

    // Actual production rolling, with one eligible entry and a fixed roll. The
    // already-loaded pool makes LoadAsync's normal Get() path synchronous.
    auto* Definition=NewObject<UItemDefinition>(Game);
    Definition->ItemType=FGameplayTag::RequestGameplayTag(TEXT("Itemization.Type.Equipment.Gear.Head"));
    Definition->Rarity=Rare; Definition->StackSizeMax=1;
    auto* Pool=NewObject<UTalentPool>(Definition);
    auto* Talent=NewObject<UTalentDefinition>(Pool);
    Talent->AbilityDef.Ability=UMythicTestPassiveAbility::StaticClass();
    const FGameplayTag Parameter=FGameplayTag::RequestGameplayTag(TEXT("SetByCaller.Generic"));
    FRollDefinition Roll; Roll.Min=17; Roll.Max=17;
    Talent->AbilityDef.ParameterRolls.Add(Parameter,Roll);
    Talent->AbilityDef.RichText=FText::FromString(
        FString::Printf(TEXT("Gain <#%s> defense."),*Parameter.ToString()));
    Pool->TalentDefs.Add(Talent);
    auto* Template=NewObject<UTalentFragment>(Definition);
    Template->TalentBuildData.TalentPool=Pool; Definition->Fragments.Add(Template);
    auto* Factory=Game->GetSubsystem<UMythicItemFactorySubsystem>();
    if (!TestNotNull(TEXT("production item factory"),Factory)) return false;
    FMythicCreateItemRequest Request;
    Request.ItemDefinition=Definition; Request.OwningActor=Owner;
    Request.Quantity=1; Request.ItemLevel=1; Request.ServerSeed=771;
    const auto Created=Factory->CreateItemReady(Request);
    if (!TestTrue(TEXT("canonical factory created the rolled gear"),Created.IsSuccess())) return false;
    auto* Item=Created.Item.Get();
    const auto* Fragment=Item->GetFragment<UTalentFragment>();
    if (!TestTrue(TEXT("real talent rolling produced one rich-text spec"),Fragment &&
        Fragment->TalentRuntimeReplicatedData.RolledTalents.Num()==1)) return false;
    const FTalentSpec Expected=Fragment->TalentRuntimeReplicatedData.RolledTalents[0];
    if (!TestFalse(TEXT("rolled text must be nonempty"),Expected.RichText.IsEmpty())) return false;
    const FGuid Guid=Item->GetItemInstanceGuid();
    if (!TestTrue(TEXT("place the factory item through canonical inventory"),Inventory->SetItemInSlot(0,Item))) return false;
    FMythicInventoryRestoreContext Context;
    Context.SaveGameGuid=FGuid::NewGuid(); Context.StableContainerId=TEXT("appearance/rolled-talent");
    FSerializedInventoryData Saved;
    if (!TestTrue(TEXT("canonical SavedInventory writer"),FSerializedInventoryData::Serialize(Inventory,Saved,Context))) return false;
    if (!TestTrue(TEXT("move the same item after saving"),Inventory->TryTransferToSlot(Item,1))) return false;

    for (int32 Pass=0;Pass<2;++Pass) {
        if (!TestTrue(TEXT("canonical SavedInventory restore consumes every tagged payload"),
            FSerializedInventoryData::Deserialize(Inventory,Saved,Context))) return false;
        const auto* Restored=Inventory->GetItem(0);
        if (!TestTrue(TEXT("exact saved physical identity restored once"),Restored &&
            Restored->GetItemInstanceGuid()==Guid && Restored->GetStacks()==1 && Inventory->GetItem(1)==nullptr)) return false;
        const auto* RestoredTalent=Restored->GetFragment<UTalentFragment>();
        if (!TestTrue(TEXT("nested rolled talent restored"),RestoredTalent &&
            RestoredTalent->TalentRuntimeReplicatedData.RolledTalents.Num()==1)) return false;
        const FTalentSpec& Actual=RestoredTalent->TalentRuntimeReplicatedData.RolledTalents[0];
        TestTrue(TEXT("talent definition and lock preserved"),Actual.TalentDef==Expected.TalentDef && Actual.bIsLocked==Expected.bIsLocked);
        TestTrue(TEXT("rolled parameter preserved"),Actual.RolledAttributes==Expected.RolledAttributes);
        TestEqual(TEXT("nonempty nested rich text preserved"),Actual.RichText.ToString(),Expected.RichText.ToString());
        if (Pass==0 && !TestTrue(TEXT("canonical save after restore"),FSerializedInventoryData::Serialize(Inventory,Saved,Context))) return false;
    }
    return true;
}


#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "Itemization/Affixes/MythicAffixApplicationComponent.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Inventory/Fragments/Passive/SocketsFragment.h"
#include "Internationalization/StringTableRegistry.h"
#include "Player/MythicPlayerState.h"
#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicSavedEquippedSocketReadyTest,
    "Mythic.Appearance.SavedEquippedSocketClosure",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)
bool FMythicSavedEquippedSocketReadyTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* PC=Game->GetWorld()->SpawnActor<AMythicPlayerController>();
    auto* PS=Game->GetWorld()->SpawnActor<AMythicPlayerState>();
    if (!TestTrue(TEXT("real authority PC and PS"),PC && PS)) return false;
    PC->PlayerState=PS; PS->SetOwner(PC);
    auto* ASC=PS->GetMythicAbilitySystemComponent();
    if (!TestNotNull(TEXT("production player ASC"),ASC)) return false;
    if (!ASC->IsRegistered()) ASC->RegisterComponent();
    if (!ASC->HasBeenInitialized()) ASC->InitializeComponent();
    ASC->InitAbilityActorInfo(PS,PS);
    auto* Application=PS->FindComponentByClass<UMythicAffixApplicationComponent>();
    if (!TestNotNull(TEXT("production equipment affix application"),Application)) return false;
    if (!Application->UActorComponent::IsRegistered()) Application->RegisterComponent();

    // Real resident semantic registry, not a forced readiness flag. The test
    // item has no affix modifiers, but its equipped socket closure must pass
    // through this production application/registry before slot publication.
    const FName TableId(TEXT("MythicSavedEquippedSocketClosure"));
    FStringTableRegistry::Get().Internal_NewLocTable(TableId,TEXT("MythicSavedEquippedSocketClosure"));
    ON_SCOPE_EXIT { FStringTableRegistry::Get().UnregisterStringTable(TableId); };
    auto Text=[&](const TCHAR* Key,const TCHAR* Value) {
        FStringTableRegistry::Get().Internal_SetLocTableEntry(TableId,Key,Value);
        return FText::FromStringTable(TableId,Key);
    };
    auto* Category=NewObject<UMythicStatCategoryDefinition>(Game);
    Category->DeveloperName=TEXT("Defense"); Category->DesignerPurpose=TEXT("Saved socket fixture.");
    Category->CategoryTag=FGameplayTag::RequestGameplayTag(TEXT("Stat.Category.Defense"));
    Category->DisplayName=Text(TEXT("Defense"),TEXT("Defense"));
    auto* Stat=NewObject<UMythicStatDefinition>(Game);
    Stat->DeveloperName=TEXT("Armor"); Stat->DesignerPurpose=TEXT("Saved socket fixture.");
    Stat->StatTag=FGameplayTag::RequestGameplayTag(TEXT("Stat.Attribute.Armor"));
    Stat->Attribute=UMythicAttributeSet_Defense::GetArmorAttribute();
    Stat->DisplayName=Text(TEXT("Armor"),TEXT("Armor")); Stat->Category.SetAsset(Category);
    Stat->bCanBeAffixTarget=true;
    auto* Affix=NewObject<UMythicAffixDefinition>(Game);
    Affix->DeveloperName=TEXT("Armor"); Affix->DesignerPurpose=TEXT("Saved socket fixture.");
    Affix->AffixTag=FGameplayTag::RequestGameplayTag(TEXT("Itemization.Affix.Armor"));
    Affix->DisplayNameTemplate=Text(TEXT("ArmorAffix"),TEXT("Armor"));
    Affix->TargetStat.SetAsset(Stat); Affix->ModifierOp=EGameplayModOp::AddBase;
    Affix->StackingRule=EMythicAffixStackingRule::StackAll;
    auto& Progression=Affix->TierProgressions.AddDefaulted_GetRef();
    Progression.DeveloperName=TEXT("Fallback"); Progression.TuningContext=TEXT("Core");
    auto& Tier=Progression.Tiers.AddDefaulted_GetRef();
    Tier.DeveloperName=TEXT("Rank1"); Tier.Magnitude.Min=1; Tier.Magnitude.Max=5;
    auto* Registry=Game->GetSubsystem<UMythicItemizationDataRegistrySubsystem>();
    if (!TestNotNull(TEXT("real itemization registry"),Registry)) return false;
    TArray<UObject*> Assets{Category,Stat,Affix}; TArray<FText> Errors;
    const bool bPublished=Registry->PublishCoreSemanticAssetsForTests(Assets,Errors);
    for (const auto& Error:Errors) AddError(Error.ToString());
    if (!TestTrue(TEXT("canonical semantic core resident"),bPublished && Registry->IsCoreSemanticReady())) return false;

    auto Inventories=PC->GetAllInventoryComponents();
    if (!TestTrue(TEXT("production owner inventory"),Inventories.Num()>0)) return false;
    auto* Inventory=Inventories[0];
    // Unequip while the real registry, ASC and application are still alive.
    // World teardown may destroy the player state before its controller.
    ON_SCOPE_EXIT {
        for (int32 Index=0;Index<Inventory->GetAllSlots().Num();++Index)
            if (Inventory->GetItem(Index))
                TestTrue(TEXT("canonical fixture cleanup before subsystem shutdown"),Inventory->SetItemInSlot(Index,nullptr));
    };
    auto* Profile=NewObject<UInventoryProfile>(Inventory);
    auto* WornDefinition=NewObject<UInventorySlotDefinition>(Profile);
    WornDefinition->SlotType=EInventorySlotType::Chest;
    auto* BagDefinition=NewObject<UInventorySlotDefinition>(Profile);
    FInventoryProfileEntry WornEntry; WornEntry.SlotDefinition=WornDefinition;
    FInventoryProfileEntry BagEntry; BagEntry.SlotDefinition=BagDefinition;
    FInventorySlotGroup Worn; Worn.SlotDomain=EMythicInventorySlotDomain::Equipment;
    Worn.bAppliesEquipmentEffects=true; Worn.DisplayOrder=0; Worn.Slots.Add(WornEntry);
    FInventorySlotGroup Bag; Bag.SlotDomain=EMythicInventorySlotDomain::Carried;
    Bag.bAppliesEquipmentEffects=false; Bag.DisplayOrder=1; Bag.Slots.Add(BagEntry);
    Profile->SlotGroups.Add(FGameplayTag::RequestGameplayTag(TEXT("Inventory.Group.Equipment.Armor")),Worn);
    Profile->SlotGroups.Add(FGameplayTag::RequestGameplayTag(TEXT("Inventory.Group.Inv.Equipment")),Bag);
    Inventory->InventoryProfile=Profile; Inventory->InitializeSlots();
    int32 WornIndex=INDEX_NONE, BagIndex=INDEX_NONE;
    for (int32 Index=0;Index<Inventory->GetAllSlots().Num();++Index) {
        const auto& Slot=Inventory->GetAllSlots()[Index];
        if (Slot.SlotDefinition==WornDefinition) WornIndex=Index;
        if (Slot.SlotDefinition==BagDefinition) BagIndex=Index;
    }
    if (!TestTrue(TEXT("one effect-active slot and one carried slot"),WornIndex!=INDEX_NONE && BagIndex!=INDEX_NONE
        && Inventory->GetAllSlots()[WornIndex].AppliesEquipmentEffects()
        && !Inventory->GetAllSlots()[BagIndex].AppliesEquipmentEffects())) return false;

    auto* Definition=NewObject<UItemDefinition>(Game);
    Definition->ItemType=FGameplayTag::RequestGameplayTag(TEXT("Itemization.Type.Equipment.Gear.Chest"));
    Definition->Rarity=Rare; Definition->StackSizeMax=1;
    auto* SocketTemplate=NewObject<USocketsFragment>(Definition);
    // Generate zero sockets deterministically, then use the canonical authority
    // add operation to obtain one real empty socket; do not fabricate its GUID.
    SocketTemplate->CountTableOverride.HardCap=1;
    auto& Rule=SocketTemplate->CountTableOverride.Rules.AddDefaulted_GetRef();
    Rule.ItemTypeParent=Definition->ItemType; Rule.MaxByRarity.Init(0,8);
    Definition->Fragments.Add(SocketTemplate);
    auto* Factory=Game->GetSubsystem<UMythicItemFactorySubsystem>();
    if (!TestNotNull(TEXT("production item factory"),Factory)) return false;
    FMythicCreateItemRequest Request;
    Request.ItemDefinition=Definition; Request.OwningActor=PC;
    Request.Quantity=1; Request.ItemLevel=1; Request.ServerSeed=883;
    const auto Created=Factory->CreateItemReady(Request);
    if (!TestTrue(TEXT("canonical socket-bearing gear factory"),Created.IsSuccess())) return false;
    auto* Item=Created.Item.Get();
    auto* Sockets=const_cast<USocketsFragment*>(Item->GetFragment<USocketsFragment>());
    if (!TestTrue(TEXT("deterministic initial zero sockets"),Sockets && Sockets->GetSocketCount()==0)) return false;
    if (!TestTrue(TEXT("authority adds the saved empty socket"),Sockets->ServerAddSocket())) return false;
    const FGuid Guid=Item->GetItemInstanceGuid();
    const FGuid SocketGuid=Sockets->GetSocketState(0)->SocketGuid;
    if (!TestTrue(TEXT("canonical equip reaches affix reconciliation"),Inventory->SetItemInSlot(WornIndex,Item))) return false;
    FMythicInventoryRestoreContext Context;
    Context.SaveGameGuid=FGuid::NewGuid(); Context.StableContainerId=TEXT("appearance/equipped-socket");
    FSerializedInventoryData Saved;
    if (!TestTrue(TEXT("save real equipped socket state"),FSerializedInventoryData::Serialize(Inventory,Saved,Context))) return false;

    for (int32 Pass=0;Pass<2;++Pass) {
        auto* Current=Inventory->GetItem(WornIndex);
        if (!TestTrue(TEXT("unequip through the canonical transaction"),Current && Inventory->TryTransferToSlot(Current,BagIndex))) return false;
        // Deserialize allocates a new fragment with transient readiness false.
        // The first equipped install must request the resident closure, then
        // recheck readiness. Before the production fix this first call fails.
        if (!TestTrue(TEXT("first restore accepts synchronously completed socket closure"),
            FSerializedInventoryData::Deserialize(Inventory,Saved,Context))) return false;
        const auto* Restored=Inventory->GetItem(WornIndex);
        if (!TestTrue(TEXT("same physical item restored once into active equipment"),Restored
            && Restored->GetItemInstanceGuid()==Guid && Restored->GetStacks()==1
            && Inventory->GetItem(BagIndex)==nullptr)) return false;
        const auto* RestoredSockets=Restored->GetFragment<USocketsFragment>();
        if (!TestTrue(TEXT("exact nonempty empty-socket state restored and ready"),RestoredSockets
            && RestoredSockets->IsRuntimeDataReady() && RestoredSockets->GetSocketCount()==1
            && RestoredSockets->GetFilledSocketCount()==0
            && RestoredSockets->GetSocketState(0)->SocketGuid==SocketGuid)) return false;
        if (Pass==0 && !TestTrue(TEXT("save restored equipped socket again"),
            FSerializedInventoryData::Serialize(Inventory,Saved,Context))) return false;
    }
    return true;
}
#endif
