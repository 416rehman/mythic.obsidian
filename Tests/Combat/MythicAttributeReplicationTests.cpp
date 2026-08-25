
#include "AttributeSet.h"
#include "Misc/AutomationTest.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UObjectIterator.h"

#include "GAS/AttributeSets/MythicAttributeSet.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAttributeReplicationIndexTest,
    "Mythic.Combat.AttributeReplicationIndices",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

/**
 * Reading an attribute set's lifetime props must give one entry per replicated attribute, on a class nothing has
 * replicated yet.
 *
 * RegisterReplicatedLifetimeProperty matches an already-registered entry by FProperty::RepIndex alone, and
 * RepIndex starts at 0 for every property until UClass::SetUpRuntimeReplicationData assigns it - which the engine
 * does lazily, only off the replication path. A caller that reads lifetime props without going near that path
 * therefore sees one class-wide index of 0: the set folds into a single entry when its conditions agree, and
 * asserts inside FLifetimeProperty::operator== when they do not. The Gameplay Debugger is such a caller (#148).
 *
 * This test deliberately does not force the setup itself. That is the whole point - it fails if the guard in
 * UMythicAttributeSet::GetLifetimeReplicatedProps is ever removed.
 */
bool FMythicAttributeReplicationIndexTest::RunTest(const FString &Parameters) {
    TArray<UClass *> SetClasses;
    for (TObjectIterator<UClass> It; It; ++It) {
        UClass *Class = *It;
        if (Class->IsChildOf(UMythicAttributeSet::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract)) {
            SetClasses.Add(Class);
        }
    }
    SetClasses.Sort([](const UClass &A, const UClass &B) { return A.GetName() < B.GetName(); });

    if (!TestTrue(TEXT("the sweep found attribute sets to check"), SetClasses.Num() > 3)) {
        return false;
    }

    int32 LazyOnEntry = 0;
    int32 Checked = 0;
    for (UClass *Class : SetClasses) {
        LazyOnEntry += Class->HasAnyClassFlags(CLASS_ReplicationDataIsSetUp) ? 0 : 1;

        TArray<FLifetimeProperty> LifetimeProps;
        Class->GetDefaultObject()->GetLifetimeReplicatedProps(LifetimeProps);

        TArray<uint16> Indices;
        for (TFieldIterator<FProperty> Prop(Class, EFieldIteratorFlags::ExcludeSuper); Prop; ++Prop) {
            if (Prop->HasAnyPropertyFlags(CPF_Net)) {
                Indices.Add(Prop->RepIndex);
            }
        }
        if (Indices.Num() == 0) {
            continue;
        }
        ++Checked;

        const TSet<uint16> Unique(Indices);
        TestEqual(*FString::Printf(TEXT("%s: every replicated attribute has its own RepIndex"), *Class->GetName()),
                  Unique.Num(), Indices.Num());
        TestEqual(*FString::Printf(TEXT("%s: every replicated attribute registers its own lifetime entry"), *Class->GetName()),
                  LifetimeProps.Num(), Indices.Num());
    }

    // A sweep that checked nothing would report the same clean bill of health as one that checked everything.
    TestTrue(TEXT("the sweep found sets with replicated attributes"), Checked > 3);
    AddInfo(FString::Printf(TEXT("%d attribute set classes, %d with replicated attributes; %d had no replication data set up on entry"),
                            SetClasses.Num(), Checked, LazyOnEntry));

    return true;
}
