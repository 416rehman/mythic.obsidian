#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Internationalization/StringTableRegistry.h"
#include "NativeGameplayTags.h"
#include "ScalableFloat.h"
#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"
#include "Stats/MythicStatRegistry.h"
#include "UI/ViewModels/MythicStatSheetViewModel.h"

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StatRegistryTestCategory, "Stat.Category.RegistryTest");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StatRegistryTestCategoryB, "Stat.Category.RegistryTestB");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StatRegistryTestCurrent, "Stat.Attribute.RegistryTestCurrent");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StatRegistryTestCapacity, "Stat.Attribute.RegistryTestCapacity");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StatRegistryTestDuplicate, "Stat.Attribute.RegistryTestDuplicate");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_StatRegistryTestArmor, "Stat.Attribute.RegistryTestArmor");

namespace {
const FName TestStringTableId(TEXT("MythicStatRegistryTests"));

class FScopedStatTestStringTable {
public:
    FScopedStatTestStringTable() {
        FStringTableRegistry::Get().UnregisterStringTable(TestStringTableId);
        FStringTableRegistry::Get().Internal_NewLocTable(TestStringTableId, TEXT("MythicStatRegistryTests"));
    }

    ~FScopedStatTestStringTable() {
        FStringTableRegistry::Get().UnregisterStringTable(TestStringTableId);
    }

    FText Add(const TCHAR* Key, const TCHAR* Source) const {
        FStringTableRegistry::Get().Internal_SetLocTableEntry(TestStringTableId, Key, Source);
        return FText::FromStringTable(TestStringTableId, Key);
    }
};

UMythicStatCategoryDefinition* MakeCategory(const FScopedStatTestStringTable& Texts,
                                            const TCHAR* ObjectName = TEXT("DA_TestStatCategory"),
                                            FGameplayTag CategoryTag = TAG_StatRegistryTestCategory,
                                            int32 SheetOrder = 10) {
    UMythicStatCategoryDefinition* Category =
        NewObject<UMythicStatCategoryDefinition>(GetTransientPackage(), FName(ObjectName));
    Category->DeveloperName = FName(ObjectName);
    Category->DesignerPurpose = TEXT("Focused registry automation fixture.");
    Category->CategoryTag = CategoryTag;
    Category->DisplayName = Texts.Add(ObjectName, TEXT("Registry Test"));
    Category->SheetOrder = SheetOrder;
    return Category;
}

UMythicStatDefinition* MakeStat(const FScopedStatTestStringTable& Texts, const TCHAR* ObjectName,
                                FGameplayTag StatTag, FGameplayAttribute Attribute, int32 SheetOrder,
                                UMythicStatCategoryDefinition* Category) {
    UMythicStatDefinition* Stat =
        NewObject<UMythicStatDefinition>(GetTransientPackage(), FName(ObjectName));
    Stat->DeveloperName = FName(ObjectName);
    Stat->DesignerPurpose = TEXT("Focused registry automation fixture.");
    Stat->StatTag = StatTag;
    Stat->Attribute = Attribute;
    Stat->DisplayName = Texts.Add(ObjectName, ObjectName);
    Stat->Category.SetAsset(Category);
    Stat->SheetOrder = SheetOrder;
    Stat->NumberPresentation.Format = EMythicStatFormat::Integer;
    Stat->NumberPresentation.DecimalPlaces = 0;
    return Stat;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatRegistryResolutionTest,
    "Mythic.Stats.Registry.ResolutionAndPairs",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatRegistryResolutionTest::RunTest(const FString& Parameters) {
    FScopedStatTestStringTable Texts;
    UMythicStatCategoryDefinition* Category = MakeCategory(Texts);
    UMythicStatDefinition* Current = MakeStat(
        Texts, TEXT("Current"), TAG_StatRegistryTestCurrent,
        UMythicAttributeSet_Life::GetHealthAttribute(), 10, Category);
    UMythicStatDefinition* Capacity = MakeStat(
        Texts, TEXT("Capacity"), TAG_StatRegistryTestCapacity,
        UMythicAttributeSet_Life::GetMaxHealthAttribute(), 20, Category);

    Current->PairRole = EMythicStatPairRole::Current;
    Current->PairedStat.SetAsset(Capacity);
    Capacity->PairRole = EMythicStatPairRole::Capacity;
    Capacity->PairedStat.SetAsset(Current);
    Capacity->SheetVisibility = EMythicStatSheetVisibility::Hidden;

    TArray<UMythicStatCategoryDefinition*> Categories{Category};
    TArray<UMythicStatDefinition*> Stats{Capacity, Current};
    TArray<FText> Errors;
    FMythicStatRegistry Registry;
    const bool bBuilt = Registry.Build(Categories, Stats, Errors);
    for (const FText& Error : Errors) {
        AddError(Error.ToString());
    }
    if (!TestTrue(TEXT("a complete reciprocal semantic closure builds"), bBuilt)) {
        return false;
    }

    TestTrue(TEXT("category resolves by logical tag"), Registry.FindCategory(Category->CategoryTag) == Category);
    TestTrue(TEXT("stat resolves by primary ID"), Registry.FindStat(Current->GetPrimaryAssetId()) == Current);
    TestTrue(TEXT("stat resolves by logical tag"), Registry.FindStat(Current->StatTag) == Current);
    TestTrue(TEXT("stat resolves by GAS attribute"), Registry.FindStat(Current->Attribute) == Current);

    TArray<const UMythicStatDefinition*> Ordered;
    Registry.GetAllStatDefinitions(Ordered);
    TestTrue(TEXT("sheet order, not input/load order, is canonical"), Ordered[0] == Current);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatRegistryCategoryIndexTest,
    "Mythic.Stats.Registry.CategoryIndex",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatRegistryCategoryIndexTest::RunTest(const FString& Parameters) {
    FScopedStatTestStringTable Texts;
    UMythicStatCategoryDefinition* First = MakeCategory(Texts);
    UMythicStatCategoryDefinition* Second =
        MakeCategory(Texts, TEXT("DA_TestStatCategoryB"), TAG_StatRegistryTestCategoryB, 20);

    UMythicStatDefinition* Health = MakeStat(
        Texts, TEXT("Health"), TAG_StatRegistryTestCurrent,
        UMythicAttributeSet_Life::GetHealthAttribute(), 20, First);
    UMythicStatDefinition* MaxHealth = MakeStat(
        Texts, TEXT("MaxHealth"), TAG_StatRegistryTestCapacity,
        UMythicAttributeSet_Life::GetMaxHealthAttribute(), 10, First);
    UMythicStatDefinition* Armor = MakeStat(
        Texts, TEXT("Armor"), TAG_StatRegistryTestArmor,
        UMythicAttributeSet_Defense::GetArmorAttribute(), 10, Second);

    TArray<UMythicStatCategoryDefinition*> Categories{First, Second};
    TArray<UMythicStatDefinition*> Stats{Health, Armor, MaxHealth};
    TArray<FText> Errors;
    FMythicStatRegistry Registry;
    if (!TestTrue(TEXT("the two-category closure builds"), Registry.Build(Categories, Stats, Errors))) {
        for (const FText& Error : Errors) {
            AddError(Error.ToString());
        }
        return false;
    }

    const TConstArrayView<const UMythicStatDefinition*> FirstStats =
        Registry.GetStatsInCategory(First->GetPrimaryAssetId());
    const TConstArrayView<const UMythicStatDefinition*> SecondStats =
        Registry.GetStatsInCategory(Second->GetPrimaryAssetId());

    TestEqual(TEXT("the index is category-scoped, not the whole roster"), FirstStats.Num(), 2);
    TestEqual(TEXT("the second category keeps only its own stat"), SecondStats.Num(), 1);
    if (FirstStats.Num() == 2) {
        TestTrue(TEXT("the index preserves sheet order within a category"),
                 FirstStats[0] == MaxHealth && FirstStats[1] == Health);
    }
    if (SecondStats.Num() == 1) {
        TestTrue(TEXT("armor belongs to the second category"), SecondStats[0] == Armor);
    }
    TestEqual(TEXT("an unknown category yields an empty view, never the full roster"),
              Registry.GetStatsInCategory(FPrimaryAssetId()).Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatRegistryRejectsDuplicateAttributeTest,
    "Mythic.Stats.Registry.RejectsDuplicateAttribute",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatRegistryRejectsDuplicateAttributeTest::RunTest(const FString& Parameters) {
    FScopedStatTestStringTable Texts;
    UMythicStatCategoryDefinition* Category = MakeCategory(Texts);
    UMythicStatDefinition* First = MakeStat(
        Texts, TEXT("First"), TAG_StatRegistryTestCurrent,
        UMythicAttributeSet_Life::GetHealthAttribute(), 10, Category);
    UMythicStatDefinition* Duplicate = MakeStat(
        Texts, TEXT("Duplicate"), TAG_StatRegistryTestDuplicate,
        UMythicAttributeSet_Life::GetHealthAttribute(), 20, Category);

    TArray<UMythicStatCategoryDefinition*> Categories{Category};
    TArray<UMythicStatDefinition*> Stats{First, Duplicate};
    TArray<FText> Errors;
    FMythicStatRegistry Registry;
    TestFalse(TEXT("duplicate GAS attribute mappings fail transactionally"), Registry.Build(Categories, Stats, Errors));
    TestFalse(TEXT("a failed build never publishes partial lookup state"), Registry.IsBuilt());
    TestTrue(TEXT("the failure is diagnostic"), Errors.ContainsByPredicate([](const FText& Error) {
        return Error.ToString().Contains(TEXT("GAS attribute"));
    }));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatSheetReadsGASAggregatesTest,
    "Mythic.Stats.SheetReadsGASBaseAndFinal",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatSheetReadsGASAggregatesTest::RunTest(const FString& Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance* GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    UWorld* World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        GameInstance->Shutdown();
        return false;
    }

    AActor* Owner = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent* ASC = NewObject<UMythicAbilitySystemComponent>(Owner);
    ASC->RegisterComponent();
    ASC->InitAbilityActorInfo(Owner, Owner);
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Life>(Owner));
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Defense>(Owner));

    const FGameplayAttribute HealthAttribute = UMythicAttributeSet_Life::GetHealthAttribute();
    const FGameplayAttribute MaxHealthAttribute = UMythicAttributeSet_Life::GetMaxHealthAttribute();
    const FGameplayAttribute ArmorAttribute = UMythicAttributeSet_Defense::GetArmorAttribute();
    ASC->SetNumericAttributeBase(HealthAttribute, 75.0f);
    ASC->SetNumericAttributeBase(MaxHealthAttribute, 100.0f);
    ASC->SetNumericAttributeBase(ArmorAttribute, 10.0f);

    UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("GE_StatSheetAggregateTest"));
    Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
    FGameplayModifierInfo Modifier;
    Modifier.Attribute = ArmorAttribute;
    Modifier.ModifierOp = EGameplayModOp::Additive;
    Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(5.0f));
    Effect->Modifiers.Add(Modifier);
    FGameplayModifierInfo CapacityModifier;
    CapacityModifier.Attribute = MaxHealthAttribute;
    CapacityModifier.ModifierOp = EGameplayModOp::AddBase;
    CapacityModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(25.0f));
    Effect->Modifiers.Add(CapacityModifier);
    const FActiveGameplayEffectHandle EffectHandle =
        ASC->ApplyGameplayEffectToSelf(Effect, 1.0f, ASC->MakeEffectContext());
    TestTrue(TEXT("the focused fixture has a live aggregated modifier"), EffectHandle.IsValid());

    FScopedStatTestStringTable Texts;
    UMythicStatCategoryDefinition* Category = MakeCategory(Texts);
    UMythicStatDefinition* Health = MakeStat(
        Texts, TEXT("SheetHealth"), TAG_StatRegistryTestCurrent, HealthAttribute, 10, Category);
    UMythicStatDefinition* MaxHealth = MakeStat(
        Texts, TEXT("SheetMaxHealth"), TAG_StatRegistryTestCapacity, MaxHealthAttribute, 20, Category);
    UMythicStatDefinition* Armor = MakeStat(
        Texts, TEXT("SheetArmor"), TAG_StatRegistryTestArmor, ArmorAttribute, 30, Category);
    Health->PairRole = EMythicStatPairRole::Current;
    Health->PairedStat.SetAsset(MaxHealth);
    MaxHealth->PairRole = EMythicStatPairRole::Capacity;
    MaxHealth->PairedStat.SetAsset(Health);
    MaxHealth->SheetVisibility = EMythicStatSheetVisibility::Hidden;

    TArray<UMythicStatCategoryDefinition*> Categories{Category};
    TArray<UMythicStatDefinition*> Stats{Health, MaxHealth, Armor};
    TArray<FText> Errors;
    FMythicStatRegistry Registry;
    const bool bBuilt = Registry.Build(Categories, Stats, Errors);
    for (const FText& Error : Errors) {
        AddError(Error.ToString());
    }

    UMythicStatSheetViewModel* ViewModel = NewObject<UMythicStatSheetViewModel>(GetTransientPackage());
    if (TestTrue(TEXT("the focused semantic closure builds"), bBuilt)) {
        ViewModel->InitializeForASCWithRegistry(ASC, Registry);
        TestEqual(TEXT("one authored category is rendered"), ViewModel->Sections.Num(), 1);
        if (ViewModel->Sections.Num() == 1) {
            const TArray<FMythicStatLine>& Lines = ViewModel->Sections[0].Lines;
            TestEqual(TEXT("the hidden capacity is folded into its current row"), Lines.Num(), 2);

            const FMythicStatLine* HealthLine = Lines.FindByPredicate([](const FMythicStatLine& Line) {
                return Line.StatTag == TAG_StatRegistryTestCurrent;
            });
            const FMythicStatLine* ArmorLine = Lines.FindByPredicate([](const FMythicStatLine& Line) {
                return Line.StatTag == TAG_StatRegistryTestArmor;
            });
            TestTrue(TEXT("the current/capacity row resolves by canonical stat identity"), HealthLine != nullptr);
            TestTrue(TEXT("the aggregated stat row resolves by canonical stat identity"), ArmorLine != nullptr);
            if (HealthLine) {
                TestEqual(TEXT("the pair reads both authoritative GAS values"),
                          HealthLine->Value.ToString(), FString(TEXT("75 / 125")));
                TestEqual(TEXT("the folded capacity retains its canonical identity"),
                          HealthLine->PairedStatTag, TAG_StatRegistryTestCapacity.GetTag());
                TestEqual(TEXT("the folded capacity base comes directly from GAS"),
                          HealthLine->PairedBaseValue, 100.0f);
                TestEqual(TEXT("the folded capacity final includes the live aggregate"),
                          HealthLine->PairedCurrentValue, 125.0f);
                TestEqual(TEXT("the folded capacity exposes its own bonus"),
                          HealthLine->PairedBonusValue, 25.0f);
                TestTrue(TEXT("a capacity-only change marks the visible pair row modified"),
                         HealthLine->bPairedStatHasBonus && HealthLine->bHasBonus);
                TestEqual(TEXT("the compatibility bonus text names the capacity contribution"),
                          HealthLine->BonusText.ToString(), FString(TEXT("Max +25")));
            }
            if (ArmorLine) {
                TestEqual(TEXT("base value comes directly from the ASC"), ArmorLine->BaseValue, 10.0f);
                TestEqual(TEXT("final value includes the live GAS aggregate"), ArmorLine->CurrentValue, 15.0f);
                TestEqual(TEXT("the displayed bonus is final minus base"), ArmorLine->BonusValue, 5.0f);
                TestEqual(TEXT("the live aggregate is formatted from the StatDefinition"),
                          ArmorLine->BonusText.ToString(), FString(TEXT("+5")));
            }
            TestEqual(TEXT("modified rows include capacity and primary aggregates without double counting pairs"),
                      ViewModel->ModifiedStatCount, 2);
        }

        Health->SheetVisibility = EMythicStatSheetVisibility::Hidden;
        ViewModel->InitializeForASCWithRegistry(ASC, Registry);
        TestEqual(TEXT("an explicitly hidden current row cannot be resurrected by a modified capacity"),
                  ViewModel->Sections.Num(), 1);
        if (ViewModel->Sections.Num() == 1) {
            const TArray<FMythicStatLine>& HiddenCurrentLines = ViewModel->Sections[0].Lines;
            TestEqual(TEXT("only the unrelated visible stat remains"), HiddenCurrentLines.Num(), 1);
            if (HiddenCurrentLines.Num() == 1) {
                TestEqual(TEXT("the remaining row is the visible armor stat"),
                          HiddenCurrentLines[0].StatTag, TAG_StatRegistryTestArmor.GetTag());
            }
        }
    }

    ViewModel->Shutdown();
    GameInstance->Shutdown();
    return true;
}

#endif
