#include "AttributeSet.h"
#include "Engine/DataTable.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectIterator.h"

#include "UI/ViewModels/MythicStatDisplay.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace {
    /** Every attribute the game declares, gathered the way the stat sheet gathers them. */
    void CollectMythicAttributes(TArray<FGameplayAttribute> &Out) {
        for (TObjectIterator<UClass> It; It; ++It) {
            UClass *Class = *It;
            if (!Class->IsChildOf(UAttributeSet::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract)) {
                continue;
            }
            // Engine and plugin sets are not ours to roster.
            if (!Class->GetName().StartsWith(TEXT("MythicAttributeSet"))) {
                continue;
            }
            for (TFieldIterator<FProperty> Prop(Class, EFieldIteratorFlags::ExcludeSuper); Prop; ++Prop) {
                if (FStructProperty *Struct = CastField<FStructProperty>(*Prop)) {
                    if (Struct->Struct == FGameplayAttributeData::StaticStruct()) {
                        Out.Add(FGameplayAttribute(*Prop));
                    }
                }
            }
        }
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicStatRosterTest,
                                 "Mythic.UI.StatRoster",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * Every attribute must be rostered in DT_StatDisplay.
 *
 * An unrostered attribute still renders - MythicStatDisplay::GetRule infers a format from the property name
 * suffix and files the row under Utility at SortOrder 9000. So the failure is invisible in code review and
 * invisible in a smoke test: the stat simply appears at the bottom of the sheet, in the wrong section, and
 * only if the player has gear that grants it. Fifteen attributes sat like that.
 */
bool FMythicStatRosterTest::RunTest(const FString &Parameters) {
    const UMythicStatDisplaySettings *Settings = GetDefault<UMythicStatDisplaySettings>();
    if (!TestNotNull(TEXT("stat display settings resolve"), Settings)) {
        return false;
    }

    const UDataTable *Table = Settings->DisplayTable.LoadSynchronous();
    if (!TestNotNull(TEXT("DisplayTable is authored and loads"), Table)) {
        return false;
    }

    TSet<FString> Rostered;
    for (const FName &RowName : Table->GetRowNames()) {
        Rostered.Add(RowName.ToString());
    }

    // A capacity half is drawn by its partner's row as "current / max", so it is rostered by reference rather
    // than by name. Reading the column keeps that exemption in the data instead of an allowlist here.
    TSet<FString> CapacityHalves;
    Table->ForeachRow<FMythicStatDisplayRow>(TEXT("StatRoster"),
                                             [&CapacityHalves](const FName &, const FMythicStatDisplayRow &Row) {
                                                 if (!Row.MaxAttribute.IsEmpty()) {
                                                     CapacityHalves.Add(Row.MaxAttribute);
                                                 }
                                             });

    TArray<FGameplayAttribute> Attributes;
    CollectMythicAttributes(Attributes);

    // A roster check that found no attributes would pass silently, which is the same shape as the bug it guards.
    if (!TestTrue(TEXT("the sweep actually found attributes to check"), Attributes.Num() > 50)) {
        return false;
    }

    TArray<FString> Unrostered;
    for (const FGameplayAttribute &Attribute : Attributes) {
        const FString Name = Attribute.GetName();
        if (!Rostered.Contains(Name) && !CapacityHalves.Contains(Name)) {
            Unrostered.Add(Name);
        }
    }

    Unrostered.Sort();
    AddInfo(FString::Printf(TEXT("%d attributes against %d rostered rows and %d capacity halves: %d unrostered"),
                            Attributes.Num(), Rostered.Num(), CapacityHalves.Num(), Unrostered.Num()));

    // The exemption must be earned, not assumed - a table with no capacity pairs would exempt nothing and the
    // check above would silently widen into "every attribute", which is not the rule being tested.
    TestTrue(TEXT("capacity halves were actually found in the table"), CapacityHalves.Num() > 0);

    if (Unrostered.Num() > 0) {
        AddError(FString::Printf(TEXT("%d attributes have no DT_StatDisplay row and would file under Utility/9000: %s"),
                                 Unrostered.Num(), *FString::Join(Unrostered, TEXT(", "))));
    }

    // Prove the roster can fail: an attribute that does not exist must not be found in it.
    TestFalse(TEXT("a made-up attribute is not rostered"), Rostered.Contains(TEXT("NotARealMythicAttribute")));

    return Unrostered.Num() == 0;
}

#endif
