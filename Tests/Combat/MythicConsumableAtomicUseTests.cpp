#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/Combat/MythicConsumableUseTestTypes.h"
#include "Tests/Progression/MythicAbilityTestFixture.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Tests/GAS/MythicGA_RuneTestTypes.h"
#include "Itemization/Inventory/Fragments/Actionable/ConsumableEffectFragment.h"
#include "Itemization/Inventory/Fragments/Actionable/ConsumableActionFragment.h"
#include "Player/MythicPlayerController.h"
#include "Itemization/Inventory/MythicItemFactorySubsystem.h"
#include "Input/MythicQuickUseComponent.h"
#include "Misc/ScopeExit.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/StrongObjectPtr.h"

namespace {
void MakeConsumableTestSlots(UMythicInventoryComponent* Inventory,bool bPouch) {
    auto* Profile=NewObject<UInventoryProfile>(Inventory);
    FInventorySlotGroup Group;
    Group.SlotDomain=bPouch ? EMythicInventorySlotDomain::Equipment : EMythicInventorySlotDomain::Carried;
    Group.bAppliesEquipmentEffects=false;
    FInventoryProfileEntry Entry; Entry.SlotDefinition=NewObject<UInventorySlotDefinition>(Profile); Entry.Count=4;
    Group.Slots.Add(Entry);
    Profile->SlotGroups.Add(FGameplayTag::RequestGameplayTag(bPouch ? TEXT("Inventory.Group.Equipment.QuickPouch") : TEXT("Inventory.Group.Inv.Consumables")),Group);
    Inventory->InventoryProfile=Profile; Inventory->InitializeSlots();
}
FMythicInventorySourceLocator LocateUse(UMythicInventoryComponent* Inventory,int32 Slot,EMythicItemUsePurpose Purpose) {
    FMythicInventorySourceLocator Source; Source.Inventory=Inventory; Source.SlotIndex=Slot; Source.UsePurpose=Purpose;
    if (const auto* Item=Inventory->GetItem(Slot)) { Source.ExpectedItemGuid=Item->GetItemInstanceGuid(); Source.ExpectedQuantity=Item->GetStacks(); }
    return Source;
}
bool SaveUseItemHasError(UMythicItemInstance* Item) {
    TArray<uint8> Bytes; FMemoryWriter Writer(Bytes,true); FObjectAndNameAsStringProxyArchive Archive(Writer,false);
    Archive.ArIsSaveGame=true; Item->Serialize(Archive); return Archive.IsError();
}
UItemDefinition* MakeEffectUseDefinition(UObject* Outer,bool bUtility=true,int32 EffectCount=1) {
    auto* Definition=NewObject<UItemDefinition>(Outer);
    Definition->ItemType=ITEMIZATION_TYPE_CONSUMABLE_FOOD; Definition->StackSizeMax=5;
    auto* Effect=NewObject<UConsumableEffectFragment>(Definition); Effect->InputTag=FGameplayTag();
    Effect->ConsumableEffectConfig.bHealing=true; Effect->ConsumableEffectConfig.bUtility=bUtility;
    for (int32 Index=0;Index<EffectCount;++Index) {
        auto& Entry=Effect->ConsumableEffectConfig.Effects.AddDefaulted_GetRef();
        Entry.GameplayEffectClass=UMythicRuneTestHealMetaEffect::StaticClass(); Entry.EffectLevel=1;
        Entry.SetByCallerMagnitudes.Add(GAS_SETBYCALLER_GENERIC,10.f);
    }
    Definition->Fragments.Add(Effect);
    Definition->Fragments.Add(NewObject<UMythicConsumableUseAuditFragment>(Definition));
    return Definition;
}
UMythicItemInstance* PutUseItem(UMythicInventoryComponent* Inventory,UItemDefinition* Definition,int32 Slot,int32 Quantity) {
    // Use the same transactional factory as loot/rewards. No test-only initialization or action-tag injection.
    auto* Factory=Inventory->GetWorld()->GetGameInstance()->GetSubsystem<UMythicItemFactorySubsystem>();
    if (!Factory) return nullptr;
    FMythicCreateItemRequest Request;
    Request.ItemDefinition=Definition; Request.Quantity=Quantity; Request.ItemLevel=1;
    Request.OwningActor=Inventory->GetOwner();
    const auto Created=Factory->CreateItemReady(Request);
    if (!Created.IsSuccess()) return nullptr;
    auto* Item=Created.Item.Get();
    if (Inventory->SetItemInSlot(Slot,Item)) return Item;
    Item->Destroy(); return nullptr;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicConsumableAtomicUseTest,"Mythic.Controls.ConsumableAtomicUse",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicConsumableAtomicUseTest::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    UWorld* World=Game->GetWorld();
    auto* Player=World->SpawnActor<AMythicPlayerState>();
    if (!TestNotNull(TEXT("real authority player state"),Player)) return false;
    InitializeAbilityTestPlayer(Player);
    if (!World->AreActorsInitialized()) World->InitializeActorsForPlay(FURL());
    auto* Owner=World->SpawnActor<AMythicPlayerController>();
    if (!TestNotNull(TEXT("real inventory/controller owner"),Owner)) return false;
    Owner->SetPlayer(NewObject<ULocalPlayer>(GEngine)); Owner->PlayerState=Player; Player->SetOwner(Owner);
    auto* Avatar=World->SpawnActor<AMythicCharacter_Player>();
    if (!TestNotNull(TEXT("real possessed avatar"),Avatar)) return false;
    Owner->Possess(Avatar);
    auto* ASC=Player->GetMythicAbilitySystemComponent(); auto* Life=UMythicLifeComponent::FindHealthComponent(Avatar);
    if (!TestTrue(TEXT("production possession initialized exact avatar life"),Life && Life->IsInitialized() && ASC->GetAvatarActor()==Avatar)) return false;
    auto* Inventory=Owner->GetAllInventoryComponents()[0]; MakeConsumableTestSlots(Inventory,true);
    auto* OtherOwner=World->SpawnActor<AMythicPlayerController>();
    if (!TestNotNull(TEXT("independent receiving owner"),OtherOwner)) return false;
    auto* OtherInventory=OtherOwner->GetAllInventoryComponents()[0]; MakeConsumableTestSlots(OtherInventory,false);
    auto* Definition=MakeEffectUseDefinition(Game);
    auto* Item=PutUseItem(Inventory,Definition,0,2);
    auto* OtherItem=PutUseItem(OtherInventory,Definition,0,1);
    if (!TestTrue(TEXT("actual source and destination stacks placed"),Item && OtherItem)) return false;
    TestFalse(TEXT("factory-created typed consumable needs no legacy action tag"),Item->HasTag(ITEMIZATION_ACTIONTYPE_ININVENTORY));
    TestTrue(TEXT("authored typed payload qualifies in passive pouch"),Inventory->CanUseItemInSlot(0));
    TestTrue(TEXT("pouch remains a loadout slot"),Inventory->GetAllSlots()[0].IsGearSlot());
    TestFalse(TEXT("pouch grants no worn effects"),Inventory->GetAllSlots()[0].AppliesEquipmentEffects());
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetMaxHealthAttribute(),100);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),50);
    const auto OriginalGuid=Item->GetItemInstanceGuid();
    const auto QueryCount=ASC->GameplayEffectApplicationQueries.Num();
    ASC->GameplayEffectApplicationQueries.Add(FGameplayEffectApplicationQuery::CreateLambda([](const FActiveGameplayEffectsContainer&,const FGameplayEffectSpec&){return false;}));
    int32 Consumed=-1;
    auto Result=Inventory->TryUseItemInSlot(LocateUse(Inventory,0,EMythicItemUsePurpose::Healing),Consumed);
    ASC->GameplayEffectApplicationQueries.SetNum(QueryCount);
    TestEqual(TEXT("all-rejected GE returns no accepted transaction"),Result,EMythicInventoryActionResult::CommitFailed);
    TestEqual(TEXT("all-rejected GE consumes zero"),Consumed,0);
    TestTrue(TEXT("all-rejected exact pointer remains slotted"),Inventory->GetItem(0)==Item);
    TestEqual(TEXT("all-rejected exact quantity remains"),Item->GetStacks(),2);
    TestEqual(TEXT("all-rejected physical identity remains"),Item->GetItemInstanceGuid(),OriginalGuid);
    TestEqual(TEXT("all-rejected health remains"),Life->GetHealth(),50.f);

    int32 HealCallbacks=0;
    const auto Delegate=ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_HEAL_RECEIVED).AddLambda([&](const FGameplayEventData*) {
        ++HealCallbacks;
        TestTrue(TEXT("real heal callback sees frozen owner"),UMythicInventoryComponent::IsOwnerMutationFrozen(Owner));
        TestTrue(TEXT("real heal callback sees exact item reservation"),UMythicInventoryComponent::IsItemMutationFrozen(Item));
        TestTrue(TEXT("before settlement source pointer and positive quantity are coherent"),Inventory->GetItem(0)==Item && Item->GetStacks()>0);
        const int32 Before=Item->GetStacks();
        Item->SetStackSize(5); TestEqual(TEXT("direct source quantity callback cannot mutate"),Item->GetStacks(),Before);
        Item->Destroy(); TestTrue(TEXT("source destruction callback is frozen"),IsValid(Item));
        TestFalse(TEXT("source slot detachment callback is frozen"),Inventory->SetItemInSlot(0,nullptr));
        Item->SetInventory(OtherInventory,0);
        TestTrue(TEXT("source back-pointer cannot transfer during callback"),Item->GetInventoryComponent()==Inventory);
        TestEqual(TEXT("foreign-owner stack merge cannot duplicate units"),OtherInventory->AddToSlot(Item,0,true),0);
        TestEqual(TEXT("foreign receive cannot transfer frozen source"),OtherInventory->ReceiveItem(Item,0,true),0);
        TestEqual(TEXT("foreign recipient remains exact"),OtherItem->GetStacks(),1);
        int32 NestedConsumed=-1;
        TestEqual(TEXT("nested use rejects"),Inventory->TryUseItemInSlot(LocateUse(Inventory,0,EMythicItemUsePurpose::Utility),NestedConsumed),EMythicInventoryActionResult::CommitFailed);
        TestEqual(TEXT("nested use reports zero units"),NestedConsumed,0);
        TestTrue(TEXT("save cannot observe an unresolved use"),SaveUseItemHasError(Item));
    });
    ON_SCOPE_EXIT { ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_HEAL_RECEIVED).Remove(Delegate); };
    Result=Inventory->TryUseItemInSlot(LocateUse(Inventory,0,EMythicItemUsePurpose::Healing),Consumed);
    TestEqual(TEXT("accepted canonical instant GE succeeds"),Result,EMythicInventoryActionResult::Succeeded);
    TestEqual(TEXT("accepted instant GE consumes exactly one"),Consumed,1);
    TestEqual(TEXT("one unit remains in source"),Item->GetStacks(),1);
    TestEqual(TEXT("canonical Healing meta attribute really healed"),Life->GetHealth(),60.f);
    TestEqual(TEXT("one real heal callback ran"),HealCallbacks,1);
    TestFalse(TEXT("owner reservation releases on unwind"),UMythicInventoryComponent::IsOwnerUseInProgress(Owner));
    TestFalse(TEXT("source serializes normally after settlement"),SaveUseItemHasError(Item));
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),100);
    Result=Inventory->TryUseItemInSlot(LocateUse(Inventory,0,EMythicItemUsePurpose::Healing),Consumed);
    TestEqual(TEXT("full-health Healing rejects"),Result,EMythicInventoryActionResult::NotUsable);
    TestEqual(TEXT("full-health Healing consumes zero"),Consumed,0);
    TestEqual(TEXT("full-health rejection preserves final unit"),Item->GetStacks(),1);

    auto* Audit=const_cast<UMythicConsumableUseAuditFragment*>(Item->GetFragment<UMythicConsumableUseAuditFragment>());
    if (!TestNotNull(TEXT("real fragment lifecycle probe"),Audit)) return false;
    int32 RetireCallbacks=0;
    Audit->InventoryChanged=[&](UMythicInventoryComponent* NewInventory,int32 NewSlot) {
        if (NewInventory) return;
        ++RetireCallbacks;
        TestNull(TEXT("retirement callback sees empty source slot"),Inventory->GetItem(0));
        TestNull(TEXT("retirement callback sees no item back-pointer"),Item->GetInventoryComponent());
        TestEqual(TEXT("retirement callback sees no slot index"),NewSlot,INDEX_NONE);
        TestEqual(TEXT("quantity reaches zero only after complete detach"),Item->GetStacks(),0);
        TestTrue(TEXT("exact reservation survives detached source"),UMythicInventoryComponent::IsItemMutationFrozen(Item));
        TestTrue(TEXT("owner remains frozen through retirement"),UMythicInventoryComponent::IsOwnerMutationFrozen(Owner));
        Item->Destroy(); TestTrue(TEXT("detached source cannot be destroyed reentrantly"),IsValid(Item));
        TestEqual(TEXT("detached-source merge stays forbidden"),OtherInventory->AddToSlot(Item,0,true),0);
        TestEqual(TEXT("detached-source AddToAnySlot stays forbidden"),OtherInventory->AddToAnySlot(Item,true),0);
        TestTrue(TEXT("detached source cannot serialize mid-retirement"),SaveUseItemHasError(Item));
    };
    TArray<TWeakObjectPtr<UMythicReplicatedObject>> ReplicatedObjects;
    ReplicatedObjects.Add(Item); for (const auto& Fragment:Item->GetFragmentsView()) ReplicatedObjects.Add(Fragment.Get());
    TArray<UMythicReplicatedObject*> RegistrationObjects;
    RegistrationObjects.Add(Item); for (const auto& Fragment:Item->GetFragmentsView()) RegistrationObjects.Add(Fragment.Get());
    Result=Inventory->TryUseItemInSlot(LocateUse(Inventory,0,EMythicItemUsePurpose::Utility),Consumed);
    TestEqual(TEXT("full-health Utility remains accepted"),Result,EMythicInventoryActionResult::Succeeded);
    TestEqual(TEXT("full-health Utility consumes its final unit"),Consumed,1);
    TestNull(TEXT("final slot is empty"),Inventory->GetItem(0));
    TestEqual(TEXT("exact final retirement lifecycle runs once"),RetireCallbacks,1);
    for (const auto& Object:ReplicatedObjects) TestFalse(TEXT("consumed item/fragment is retired"),Object.IsValid());
    for (const auto* Object:RegistrationObjects) {
        TestFalse(TEXT("consumed source no longer occupies actor replication list"),Owner->IsReplicatedSubObjectRegistered(Object));
        TestFalse(TEXT("consumed source no longer occupies inventory replication list"),Owner->IsActorComponentReplicatedSubObjectRegistered(Inventory,Object));
    }
    TestFalse(TEXT("final settlement releases owner"),UMythicInventoryComponent::IsOwnerUseInProgress(Owner));
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_HEAL_RECEIVED).Remove(Delegate);

    auto* HealingOnly=PutUseItem(Inventory,MakeEffectUseDefinition(Game,false),1,1);
    if (!TestNotNull(TEXT("healing-only source placed"),HealingOnly)) return false;
    Result=Inventory->TryUseItemInSlot(LocateUse(Inventory,1,EMythicItemUsePurpose::Inventory),Consumed);
    TestEqual(TEXT("healing-only cannot bypass full-health gate through Inventory purpose"),Result,EMythicInventoryActionResult::NotUsable);
    TestEqual(TEXT("healing-only rejected stack remains"),HealingOnly->GetStacks(),1);

    auto* Multi=PutUseItem(Inventory,MakeEffectUseDefinition(Game,true,2),2,1);
    if (!TestNotNull(TEXT("two-effect source placed"),Multi)) return false;
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),50);
    int32 MultiCallbacks=0;
    const auto LossDelegate=ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_HEAL_RECEIVED).AddLambda([&](const FGameplayEventData*) {
        ++MultiCallbacks; ASC->InitAbilityActorInfo(Player,Player);
    });
    Result=Inventory->TryUseItemInSlot(LocateUse(Inventory,2,EMythicItemUsePurpose::Healing),Consumed);
    ASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_HEAL_RECEIVED).Remove(LossDelegate);
    ASC->InitAbilityActorInfo(Player,Avatar);
    TestEqual(TEXT("earlier accepted effect still settles after avatar loss"),Result,EMythicInventoryActionResult::Succeeded);
    TestEqual(TEXT("earlier accepted effect charges one unit"),Consumed,1);
    TestEqual(TEXT("second effect does not execute against changed avatar"),MultiCallbacks,1);
    TestEqual(TEXT("only first ten-point heal applied"),Life->GetHealth(),60.f);
    TestNull(TEXT("invalidated-recipient final source still settled"),Inventory->GetItem(2));
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicShippedConsumableFactoryEligibility,
    "Mythic.Controls.ShippedConsumableFactoryEligibility",
    EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicShippedConsumableFactoryEligibility::RunTest(const FString&) {
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone();
    ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    UWorld* World=Game->GetWorld();
    auto* Player=World->SpawnActor<AMythicPlayerState>();
    if (!TestNotNull(TEXT("real authority player state"),Player)) return false;
    InitializeAbilityTestPlayer(Player);
    if (!World->AreActorsInitialized()) World->InitializeActorsForPlay(FURL());
    auto* Owner=World->SpawnActor<AMythicPlayerController>();
    if (!TestNotNull(TEXT("real inventory owner"),Owner)) return false;
    Owner->SetPlayer(NewObject<ULocalPlayer>(GEngine)); Owner->PlayerState=Player; Player->SetOwner(Owner);
    auto* Avatar=World->SpawnActor<AMythicCharacter_Player>();
    if (!TestNotNull(TEXT("real possessed avatar"),Avatar)) return false;
    Owner->Possess(Avatar);
    auto* ASC=Player->GetMythicAbilitySystemComponent();
    auto* Life=UMythicLifeComponent::FindHealthComponent(Avatar);
    if (!TestTrue(TEXT("actual actor-info/life initialized"),Life && Life->IsInitialized() && ASC->GetAvatarActor()==Avatar)) return false;
    const auto Inventories=Owner->GetAllInventoryComponents();
    if (!TestTrue(TEXT("canonical controller inventory exists"),Inventories.Num()>0 && Inventories[0])) return false;
    auto* Inventory=Inventories[0]; MakeConsumableTestSlots(Inventory,false);
    auto* Quick=Owner->FindComponentByClass<UMythicQuickUseComponent>();
    if (!TestNotNull(TEXT("production quick-use component exists"),Quick)) return false;

    auto* AppleDefinition=LoadObject<UItemDefinition>(nullptr,TEXT("/Game/Mythic/Itemization/ItemDefinitions/Consumables/Apple.Apple"));
    auto* PotionDefinition=LoadObject<UItemDefinition>(nullptr,TEXT("/Game/Mythic/Itemization/ItemDefinitions/Consumables/HealingPotion/HealingPotion.HealingPotion"));
    if (!TestTrue(TEXT("published consumable definitions load"),AppleDefinition && PotionDefinition)) return false;
    auto* Apple=PutUseItem(Inventory,AppleDefinition,0,1);
    auto* Potion=PutUseItem(Inventory,PotionDefinition,1,2);
    if (!TestTrue(TEXT("actual factory creates both shipped stacks"),Apple && Potion)) return false;
    for (auto* Item:{Apple,Potion}) {
        TestFalse(TEXT("production factory did not inject an InInventory tag"),Item->HasTag(ITEMIZATION_ACTIONTYPE_ININVENTORY));
        TestEqual(TEXT("factory keeps requested legal quantity"),Item->GetStacks(),Item==Apple ? 1:2);
        TestTrue(TEXT("factory registers exact instance under controller"),Owner->IsReplicatedSubObjectRegistered(Item));
    }
    auto* AppleEffect=const_cast<UConsumableEffectFragment*>(Apple->GetFragment<UConsumableEffectFragment>());
    const auto* PotionEffect=Potion->GetFragment<UConsumableEffectFragment>();
    if (!TestTrue(TEXT("typed runtime effects exist"),AppleEffect && PotionEffect)) return false;
    TestTrue(TEXT("Apple is explicitly Healing and Utility"),AppleEffect->ConsumableEffectConfig.bHealing && AppleEffect->ConsumableEffectConfig.bUtility);
    TestTrue(TEXT("Healing Draught is explicitly Healing only"),PotionEffect->ConsumableEffectConfig.bHealing && !PotionEffect->ConsumableEffectConfig.bUtility);
    TestEqual(TEXT("Apple retains three canonical effects"),AppleEffect->ConsumableEffectConfig.Effects.Num(),3);
    TestEqual(TEXT("Draught retains one canonical effect"),PotionEffect->ConsumableEffectConfig.Effects.Num(),1);
    TestTrue(TEXT("tagless Apple has inventory use admission"),Inventory->CanUseItemInSlot(0));
    TestTrue(TEXT("tagless Draught has inventory use admission"),Inventory->CanUseItemInSlot(1));
    const auto EntryCount=[&](bool bHealing,UItemDefinition* Definition) {
        int32 Count=0;
        for (const auto& Entry:Quick->GetEntries(bHealing)) if (Entry.Key.Definition==FSoftObjectPath(Definition)) {
            ++Count; TestEqual(TEXT("quick-use count comes from exact physical stack"),Entry.Quantity,Definition==AppleDefinition ? 1:2);
        }
        return Count;
    };
    Quick->RefreshInventories();
    TestEqual(TEXT("Apple appears once in Healing"),EntryCount(true,AppleDefinition),1);
    TestEqual(TEXT("Apple appears once in Utility"),EntryCount(false,AppleDefinition),1);
    TestEqual(TEXT("Draught appears once in Healing"),EntryCount(true,PotionDefinition),1);
    TestEqual(TEXT("Draught cannot enter Utility"),EntryCount(false,PotionDefinition),0);

    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetMaxHealthAttribute(),100);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),50);
    const FConsumableEffectConfig OriginalConfig=AppleEffect->ConsumableEffectConfig;
    const auto RejectApple=[&](const TCHAR* Reason) {
        TestFalse(Reason,Inventory->CanUseItemInSlot(0));
        int32 Consumed=-1;
        TestEqual(TEXT("server repeats structural rejection"),Inventory->TryUseItemInSlot(LocateUse(Inventory,0,EMythicItemUsePurpose::Healing),Consumed),EMythicInventoryActionResult::NotUsable);
        TestEqual(TEXT("rejected payload consumes nothing"),Consumed,0);
        TestTrue(TEXT("rejected payload keeps exact slot pointer"),Inventory->GetItem(0)==Apple);
        TestEqual(TEXT("rejected payload keeps quantity"),Apple->GetStacks(),1);
        TestEqual(TEXT("rejected payload applies no heal"),Life->GetHealth(),50.f);
        Quick->RefreshInventories();
        TestEqual(TEXT("invalid item is absent from the Healing catalogue"),EntryCount(true,AppleDefinition),0);
    };
    AppleEffect->ConsumableEffectConfig.bHealing=false;
    RejectApple(TEXT("runtime classification cannot contradict immutable definition"));
    AppleEffect->ConsumableEffectConfig=OriginalConfig;
    AppleEffect->ConsumableEffectConfig.QuickUseVariant=TEXT("UnauthoredVariant");
    RejectApple(TEXT("runtime variant cannot contradict immutable definition"));
    AppleEffect->ConsumableEffectConfig=OriginalConfig;
    AppleEffect->ConsumableEffectConfig.Effects[0].EffectLevel=0.f;
    RejectApple(TEXT("zero effect level fails structural admission"));
    AppleEffect->ConsumableEffectConfig=OriginalConfig;
    AppleEffect->ConsumableEffectConfig.Effects.Reset();
    RejectApple(TEXT("empty effect payload fails structural admission"));
    AppleEffect->ConsumableEffectConfig=OriginalConfig;
    AppleEffect->SetOwnerItemInstance(Potion);
    RejectApple(TEXT("foreign fragment back-pointer cannot authorize use"));
    AppleEffect->SetOwnerItemInstance(Apple);
    TestTrue(TEXT("restoring exact runtime state restores structural eligibility"),Inventory->CanUseItemInSlot(0));

    // Invalid authored shapes use private transient definitions; the shipped assets above remain read-only.
    auto* BothDefinition=MakeEffectUseDefinition(Game);
    auto* Action=NewObject<UConsumableActionFragment>(BothDefinition); Action->InputTag=FGameplayTag();
    BothDefinition->Fragments.Add(Action);
    auto* Both=PutUseItem(Inventory,BothDefinition,2,1);
    if (!TestNotNull(TEXT("factory materializes ambiguous historical shape for quarantine test"),Both)) return false;
    TestFalse(TEXT("effect plus generic action is ambiguous and unusable"),Inventory->CanUseItemInSlot(2));
    auto* DuplicateDefinition=MakeEffectUseDefinition(Game);
    auto* Extra=DuplicateObject<UConsumableEffectFragment>(UItemDefinition::GetFragment<UConsumableEffectFragment>(DuplicateDefinition),DuplicateDefinition);
    DuplicateDefinition->Fragments.Add(Extra);
    auto* Duplicate=PutUseItem(Inventory,DuplicateDefinition,3,1);
    if (!TestNotNull(TEXT("factory materializes duplicate effect shape for quarantine test"),Duplicate)) return false;
    TestFalse(TEXT("two consumable effect fragments are unusable"),Inventory->CanUseItemInSlot(3));

    int32 Consumed=-1;
    TestEqual(TEXT("tagless shipped Apple really executes canonical use"),Inventory->TryUseItemInSlot(LocateUse(Inventory,0,EMythicItemUsePurpose::Healing),Consumed),EMythicInventoryActionResult::Succeeded);
    TestEqual(TEXT("Apple accepts exactly one unit"),Consumed,1);
    TestNull(TEXT("final Apple unit clears its exact slot"),Inventory->GetItem(0));
    TestTrue(TEXT("Apple's canonical instant heal reaches real life"),Life->GetHealth()>50.f);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),100);
    TestEqual(TEXT("Draught cannot bypass full-health gate through Inventory purpose"),Inventory->TryUseItemInSlot(LocateUse(Inventory,1,EMythicItemUsePurpose::Inventory),Consumed),EMythicInventoryActionResult::NotUsable);
    TestEqual(TEXT("full-health Draught consumes zero"),Consumed,0);
    TestEqual(TEXT("full-health Draught remains two units"),Potion->GetStacks(),2);
    ASC->SetNumericAttributeBase(UMythicAttributeSet_Life::GetHealthAttribute(),50);
    TestEqual(TEXT("tagless shipped Draught really executes canonical use"),Inventory->TryUseItemInSlot(LocateUse(Inventory,1,EMythicItemUsePurpose::Healing),Consumed),EMythicInventoryActionResult::Succeeded);
    TestEqual(TEXT("Draught accepts exactly one unit"),Consumed,1);
    TestEqual(TEXT("Draught leaves exactly one unit"),Potion->GetStacks(),1);
    TestTrue(TEXT("Draught's instant heal reaches real life"),Life->GetHealth()>50.f);
    return true;
}
#endif
