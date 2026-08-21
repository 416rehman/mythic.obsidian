
#include "UI/Settings/MythicSettingsScreenBase.h"

#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/Settings/MythicSettingAccess.h"
#include "UI/Settings/MythicSettingRowBase.h"
#include "UI/Settings/MythicUserSettings.h"

UMythicSettingsCatalog *UMythicSettingsScreenBase::GetCatalog() const {
    return Catalog.IsNull() ? nullptr : Catalog.LoadSynchronous();
}

TArray<FMythicSettingCategory> UMythicSettingsScreenBase::GetCategories() const {
    const UMythicSettingsCatalog *Loaded = GetCatalog();
    return Loaded ? Loaded->Categories : TArray<FMythicSettingCategory>();
}

void UMythicSettingsScreenBase::NativeOnInitialized() {
    Super::NativeOnInitialized();
    BuildScreen();
}

void UMythicSettingsScreenBase::NativeOnActivated() {
    // Built before Super, because activation drives the first focus pass and a screen whose rows do not
    // exist yet focuses nothing and never gets asked again.
    BuildScreen();
    Super::NativeOnActivated();
}

void UMythicSettingsScreenBase::BuildScreen() {
    if (bScreenBuilt || !RowList) {
        return;
    }
    const UMythicSettingsCatalog *Loaded = GetCatalog();
    if (!Loaded) {
        return;
    }
    bScreenBuilt = true;

    const TArray<FMythicSettingCategory> Cats = GetCategories();
    for (int32 CatIndex = 0; CatIndex < Cats.Num(); ++CatIndex) {
        if (Rail && TabButtonClass) {
            UCommonButtonBase *Button = WidgetTree->ConstructWidget<UCommonButtonBase>(TabButtonClass);
            if (UWidget *Found = Button->GetWidgetFromName(TabLabelWidgetName)) {
                if (UTextBlock *Label = Cast<UTextBlock>(Found)) {
                    Label->SetText(Cats[CatIndex].Label);
                }
            }
            Button->SetIsSelectable(true);
            Button->OnClicked().AddUObject(this, &UMythicSettingsScreenBase::HandleTabClicked, CatIndex);
            Rail->AddChild(Button);
            TabButtons.Add(Button);
        }

        UVerticalBox *Container = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
        RowList->AddChild(Container);
        CategoryContainers.Add(Container);

        for (const FMythicSettingDefinition &Def : GetRowsForCategory(CatIndex)) {
            const TSubclassOf<UMythicSettingRowBase> RowClass =
                IsGroupHeading(Def) ? GroupHeadingClass : GetRowClassFor(Def.Control);
            if (!RowClass) {
                continue;
            }
            UMythicSettingRowBase *Row = WidgetTree->ConstructWidget<UMythicSettingRowBase>(RowClass);
            Row->SetDefinition(Def, this);
            Container->AddChild(Row);
        }
    }

    ApplyCategoryVisibility();
}

void UMythicSettingsScreenBase::ApplyCategoryVisibility() {
    ActiveRows.Reset();
    for (int32 Index = 0; Index < CategoryContainers.Num(); ++Index) {
        UVerticalBox *Container = CategoryContainers[Index];
        if (!Container) {
            continue;
        }
        const bool bActive = Index == ActiveCategory;
        // Collapsed, not Hidden: Hidden still walks every child during the Slate pre-pass to compute
        // geometry nothing will draw.
        Container->SetVisibility(bActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
        if (!bActive) {
            continue;
        }
        for (UWidget *Child : Container->GetAllChildren()) {
            if (UMythicSettingRowBase *Row = Cast<UMythicSettingRowBase>(Child)) {
                ActiveRows.Add(Row);
            }
        }
    }

    for (int32 Index = 0; Index < TabButtons.Num(); ++Index) {
        if (TabButtons[Index]) {
            TabButtons[Index]->SetIsSelected(Index == ActiveCategory, false);
        }
    }
}

void UMythicSettingsScreenBase::SetActiveCategoryIndex(int32 Index) {
    const TArray<FMythicSettingCategory> Cats = GetCategories();
    if (!Cats.IsValidIndex(Index) || Index == ActiveCategory) {
        return;
    }
    ActiveCategory = Index;
    ApplyCategoryVisibility();
    OnCategoryChanged();
}

void UMythicSettingsScreenBase::CycleCategory(int32 Delta) {
    const int32 Count = GetCategories().Num();
    if (Count <= 0) {
        return;
    }
    SetActiveCategoryIndex(((ActiveCategory + Delta) % Count + Count) % Count);
}

void UMythicSettingsScreenBase::HandleTabClicked(int32 CategoryIndex) {
    SetActiveCategoryIndex(CategoryIndex);
}

UWidget *UMythicSettingsScreenBase::NativeGetDesiredFocusTarget() const {
    // First real setting, never a heading: a heading takes focus but answers no input, so the pad feels dead.
    for (UMythicSettingRowBase *Row : ActiveRows) {
        if (Row && !IsGroupHeading(Row->GetDefinition())) {
            return Row;
        }
    }
    return TabButtons.Num() > 0 ? TabButtons[0].Get() : Super::NativeGetDesiredFocusTarget();
}

TSubclassOf<UMythicSettingRowBase> UMythicSettingsScreenBase::GetRowClassFor(EMythicSettingControl Control) const {
    const TSubclassOf<UMythicSettingRowBase> *Found = RowClasses.Find(Control);
    return Found ? *Found : nullptr;
}

bool UMythicSettingsScreenBase::IsGroupHeading(const FMythicSettingDefinition &Row) {
    // A heading carries its group name and nothing to read or write, which is what marks it apart from a
    // real setting without needing a second list for the Blueprint to keep in step.
    return Row.SourceName.IsNone() && !Row.Group.IsEmpty();
}

TArray<FMythicSettingDefinition> UMythicSettingsScreenBase::GetRowsForActiveCategory() const {
    return GetRowsForCategory(ActiveCategory);
}

TArray<FMythicSettingDefinition> UMythicSettingsScreenBase::GetRowsForCategory(int32 CategoryIndex) const {
    TArray<FMythicSettingDefinition> Rows;

    const UMythicSettingsCatalog *Loaded = GetCatalog();
    const TArray<FMythicSettingCategory> Cats = GetCategories();
    if (!Loaded || !Cats.IsValidIndex(CategoryIndex)) {
        return Rows;
    }
    const FMythicSettingCategory &Category = Cats[CategoryIndex];

    // Groups come out in the order the category authored, and anything in a group the category forgot to
    // list still appears - at the end, rather than vanishing.
    TArray<FText> Order = Category.GroupOrder;
    for (const FMythicSettingDefinition &Def : Loaded->Settings) {
        if (Def.Category != Category.Id) {
            continue;
        }
        const bool bKnown = Order.ContainsByPredicate(
            [&Def](const FText &G) { return G.EqualTo(Def.Group); });
        if (!bKnown) {
            Order.Add(Def.Group);
        }
    }

    for (const FText &GroupName : Order) {
        TArray<FMythicSettingDefinition> InGroup;
        for (const FMythicSettingDefinition &Def : Loaded->Settings) {
            if (Def.Category == Category.Id && Def.Group.EqualTo(GroupName)) {
                InGroup.Add(Def);
            }
        }
        if (InGroup.Num() == 0) {
            continue;
        }

        FMythicSettingDefinition Heading;
        Heading.Group = GroupName;
        Heading.Label = GroupName;
        Heading.Category = Category.Id;
        Rows.Add(Heading);
        Rows.Append(InGroup);
    }

    return Rows;
}

void UMythicSettingsScreenBase::SetFocusedRow(const FMythicSettingDefinition &Row) {
    FocusedRow = Row;
    OnFocusedRowChanged();
}

void UMythicSettingsScreenBase::MarkPendingApply() {
    if (bPendingApply) {
        return;
    }
    bPendingApply = true;
    OnPendingApplyChanged();
}

void UMythicSettingsScreenBase::ApplyAndSave() {
    if (UMythicUserSettings *Settings = UMythicUserSettings::Get()) {
        Settings->ApplySettings(false);
        Settings->SaveSettings();
    }
    bPendingApply = false;
    OnPendingApplyChanged();
    OnCategoryChanged();
}

void UMythicSettingsScreenBase::RestoreDefaults() {
    UMythicSettingAccess::RestoreDefaults(GetCatalog());
    bPendingApply = false;
    OnPendingApplyChanged();

    for (UMythicSettingRowBase *Row : ActiveRows) {
        if (Row) {
            Row->Redraw();
        }
    }
    OnCategoryChanged();
}
