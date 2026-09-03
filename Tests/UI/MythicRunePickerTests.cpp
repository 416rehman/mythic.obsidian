#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Misc/ScopeExit.h"
#include "Player/MythicPlayerState.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "Progression/Runes/MythicRuneDefinition.h"
#include "UI/Menu/MythicRunePickerCellWidget.h"
#include "UI/Menu/MythicRunePickerWidget.h"
#include "UI/Menu/MythicRuneSocketWidget.h"
#include "UObject/UnrealType.h"

namespace {

const TCHAR *RunePickerBlueprintPath = TEXT("/Game/Mythic/UI/Menu/WBP_RunePicker.WBP_RunePicker_C");

// The picker draws replicated state, so the honest fixture is the owner that replicates it: a player state with
// its rune component and ability system, spawned into a standalone world. No player controller exists here, which
// is exactly the case a spectator or a late-joining client puts the widget in.
struct FMythicRunePickerFixture {
    UGameInstance *GameInstance = nullptr;
    AMythicPlayerState *PlayerState = nullptr;
    UMythicRuneComponent *Runes = nullptr;
    UMythicAbilitySystemComponent *ASC = nullptr;
};

bool BuildPickerFixture(FAutomationTestBase &Test, FMythicRunePickerFixture &Out) {
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
    Out.Runes = Out.PlayerState->GetRuneComponent();
    Out.ASC = Out.PlayerState->GetMythicAbilitySystemComponent();
    if (!Test.TestNotNull(TEXT("the player state owns a rune component"), Out.Runes)
        || !Test.TestNotNull(TEXT("the player state owns an ability system"), Out.ASC)) {
        return false;
    }
    if (!Out.Runes->IsRegistered()) {
        Out.Runes->RegisterComponent();
    }
    if (!Out.ASC->IsRegistered()) {
        Out.ASC->RegisterComponent();
    }
    Out.ASC->InitAbilityActorInfo(Out.PlayerState, Out.PlayerState);
    return true;
}

// The authored picker when it loads, else the native class. Either way the pooled cells need a cell class and a
// socket class, which the native defaults leave unset; the test fills them the way the editor would.
UMythicRunePickerWidget *MakePicker(FAutomationTestBase &Test, UGameInstance *GameInstance, FString &OutClassName) {
    UClass *PickerClass = LoadClass<UMythicRunePickerWidget>(nullptr, RunePickerBlueprintPath);
    if (!PickerClass) {
        PickerClass = UMythicRunePickerWidget::StaticClass();
    }
    OutClassName = PickerClass->GetName();

    UMythicRunePickerWidget *Picker = NewObject<UMythicRunePickerWidget>(GameInstance, PickerClass);
    if (!Test.TestNotNull(TEXT("a picker can be constructed"), Picker)) {
        return nullptr;
    }

    struct FFallback {
        FName Property;
        UClass *Class;
    };
    const FFallback Fallbacks[] = {
        { TEXT("CellClass"), UMythicRunePickerCellWidget::StaticClass() },
        { TEXT("SocketClass"), UMythicRuneSocketWidget::StaticClass() },
    };
    for (const FFallback &Fallback : Fallbacks) {
        FClassProperty *Property = FindFProperty<FClassProperty>(PickerClass, Fallback.Property);
        if (Property && Property->GetPropertyValue_InContainer(Picker) == nullptr) {
            Property->SetPropertyValue_InContainer(Picker, Fallback.Class);
        }
    }

    Picker->Initialize();
    return Picker;
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRunePickerFocusTest,
    "Mythic.UI.RunePicker.Focus",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRunePickerFocusTest::RunTest(const FString &Parameters) {
    FMythicRunePickerFixture Fixture;
    const bool bReady = BuildPickerFixture(*this, Fixture);
    ON_SCOPE_EXIT {
        if (Fixture.GameInstance) {
            Fixture.GameInstance->Shutdown();
        }
    };
    if (!bReady) {
        return false;
    }

    FString PickerClassName;
    UMythicRunePickerWidget *Picker = MakePicker(*this, Fixture.GameInstance, PickerClassName);
    if (!Picker) {
        return false;
    }
    Picker->SetRuneSource(Fixture.Runes);

    const int32 CellCount = Picker->GetCellCount();
    AddInfo(FString::Printf(TEXT("picker: %s, cells: %d"), *PickerClassName, CellCount));
    if (!TestTrue(TEXT("the picker pooled a cell per library rune - zero cells means WBP_RunePicker has no CellGrid "
                       "or CellClass, or no rune assets were found"),
                  CellCount > 0)) {
        return false;
    }

    // Cells point at nothing until the picker opens; the first open sorts the library into them.
    Picker->OpenForSlot(0, nullptr);
    TestTrue(TEXT("opening puts the picker on its layer"), Picker->IsOnLayer());
    TestTrue(TEXT("opening activates the picker"), Picker->IsActivated());

    // One rune earned, the rest locked: the first-unlocked rule has exactly one right answer. It must also carry
    // an ability, so the equip below is refused only for a reason this test is about.
    const UMythicRuneDefinition *Earned = nullptr;
    for (int32 i = 0; i < CellCount && !Earned; ++i) {
        const UMythicRuneDefinition *Rune = Picker->GetCellRune(i);
        if (Rune && Rune->RequiredTag.IsValid() && Rune->HasPayload()) {
            Fixture.ASC->AddLooseGameplayTag(Rune->RequiredTag);
            Earned = Rune;
        }
    }
    if (!TestNotNull(TEXT("a rune with a deed and an ability exists to earn"), Earned)) {
        return false;
    }
    // Re-targeting sorts; a change of state alone never moves a cell.
    Picker->SelectSocket(0);

    int32 LockedCells = 0;
    int32 UnlockedCells = 0;
    for (int32 i = 0; i < CellCount; ++i) {
        UMythicRunePickerCellWidget *Cell = Picker->GetCellWidget(i);
        if (!TestNotNull(*FString::Printf(TEXT("cell %d exists"), i), Cell)) {
            continue;
        }
        const bool bUnlocked = Picker->IsCellUnlocked(i);
        (bUnlocked ? UnlockedCells : LockedCells)++;
        // A locked rune stays reachable by hover and pad focus; only the verb is withheld.
        TestTrue(*FString::Printf(TEXT("cell %d (%s) is enabled"), i, bUnlocked ? TEXT("earned") : TEXT("locked")),
                 Cell->GetIsEnabled());
    }
    AddInfo(FString::Printf(TEXT("earned: %d, locked: %d"), UnlockedCells, LockedCells));
    TestEqual(TEXT("exactly one rune reads as earned"), UnlockedCells, 1);
    TestTrue(TEXT("the earned rune sorts ahead of the locked ones"), Picker->GetCellRune(0) == Earned);

    UWidget *Target = Picker->GetDesiredFocusTarget();
    UMythicRunePickerCellWidget *EarnedCell = Picker->GetCellWidget(0);
    UWidget *EarnedHit = EarnedCell ? EarnedCell->GetFocusWidget() : nullptr;
    // A cell with no Hit would pass every focus assertion below by never being a candidate.
    if (!TestNotNull(TEXT("the earned cell exposes a focusable Hit"), EarnedHit)) {
        return false;
    }
    TestTrue(TEXT("with nothing worn, focus lands on the first earned rune"), Target == EarnedHit);
    for (int32 i = 1; i < CellCount; ++i) {
        UMythicRunePickerCellWidget *Cell = Picker->GetCellWidget(i);
        UWidget *Hit = Cell ? Cell->GetFocusWidget() : nullptr;
        TestFalse(*FString::Printf(TEXT("focus never opens on locked cell %d"), i), Hit && Target == Hit);
    }

    // Worn-here wins over first-earned, and the redraw that reports it must not move the cell under focus.
    Fixture.Runes->ServerEquipRune(0, const_cast<UMythicRuneDefinition *>(Earned));
    if (!TestTrue(TEXT("the earned rune is worn in socket one"), Fixture.Runes->GetRuneInSlot(0) == Earned)) {
        return false;
    }
    TestTrue(TEXT("the worn rune keeps its cell"), Picker->GetCellRune(0) == Earned);
    TestTrue(TEXT("the worn cell reads as worn here"), Picker->GetCellWorn(0) == EMythicRuneWorn::Here);
    TestTrue(TEXT("with a rune worn here, focus lands on its cell"), Picker->GetDesiredFocusTarget() == EarnedHit);

    Picker->Close();
    TestFalse(TEXT("closing takes the picker off its layer"), Picker->IsOnLayer());
    TestFalse(TEXT("closing deactivates the picker"), Picker->IsActivated());

    // A second close must be a no-op, never a second removal.
    Picker->Close();
    TestFalse(TEXT("a repeated close stays off the layer"), Picker->IsOnLayer());
    return true;
}
