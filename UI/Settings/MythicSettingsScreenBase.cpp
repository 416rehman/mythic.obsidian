
#include "UI/Settings/MythicSettingsScreenBase.h"

#include "Groups/CommonButtonGroupBase.h"
#include "InputAction.h"

#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "CommonTextBlock.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/MythicUIStyle.h"
#include "UI/Settings/MythicSettingAccess.h"
#include "UI/Settings/MythicSettingRowBase.h"
#include "UI/Settings/MythicUserSettings.h"

namespace {
const FGameplayTag TagApply = FGameplayTag::RequestGameplayTag(FName("UI.Action.ApplySettings"), false);
const FGameplayTag TagRestore = FGameplayTag::RequestGameplayTag(FName("UI.Action.RestoreDefaults"), false);
}

void UMythicSettingsScreenBase::HandleApplyAction() {
    ApplyAndSave();
}

void UMythicSettingsScreenBase::HandleRestoreDefaultsAction() {
    RestoreDefaults();
}

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

    // Everything from here is a preview until Apply. Opening the screen is the baseline.
    UMythicSettingAccess::BeginStaging();

    // The UI context has to be live before the bar looks, or Enhanced Input reports no keys for these
    // actions and the bar filters both prompts out.
    AddUIInputContext();

    if (UInputAction *Apply = ApplyInputAction.LoadSynchronous()) {
        FInputActionExecutedDelegate OnApply;
        OnApply.BindDynamic(this, &UMythicSettingsScreenBase::HandleApplyAction);
        RegisterInputActionBinding(Apply, IE_Pressed, OnApply, /*ShowInActionBar*/ true, ApplyBinding);
    }
    if (UInputAction *Restore = RestoreDefaultsInputAction.LoadSynchronous()) {
        FInputActionExecutedDelegate OnRestore;
        OnRestore.BindDynamic(this, &UMythicSettingsScreenBase::HandleRestoreDefaultsAction);
        RegisterInputActionBinding(Restore, IE_Pressed, OnRestore, /*ShowInActionBar*/ true, RestoreBinding);
    }

    UE_LOG(Myth, Log, TEXT("Settings activated: apply action %s (handle %s), restore action %s (handle %s)"),
           ApplyInputAction.IsNull() ? TEXT("UNSET") : TEXT("ok"),
           ApplyBinding.Handle.IsValid() ? TEXT("ok") : TEXT("INVALID"),
           RestoreDefaultsInputAction.IsNull() ? TEXT("UNSET") : TEXT("ok"),
           RestoreBinding.Handle.IsValid() ? TEXT("ok") : TEXT("INVALID"));
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
            if (!RailGroup) {
                RailGroup = NewObject<UCommonButtonGroupBase>(this);
                RailGroup->SetSelectionRequired(true);
            }
            UCommonButtonBase *Button = WidgetTree->ConstructWidget<UCommonButtonBase>(TabButtonClass);
            if (UWidget *Found = Button->GetWidgetFromName(TabLabelWidgetName)) {
                if (UTextBlock *Label = Cast<UTextBlock>(Found)) {
                    Label->SetText(Cats[CatIndex].Label);
                }
            }
            Button->SetIsSelectable(true);
            Button->OnClicked().AddUObject(this, &UMythicSettingsScreenBase::HandleTabClicked, CatIndex);
            Rail->AddChild(Button);
            RailGroup->AddWidget(Button);
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
            UPanelSlot *Added = Container->AddChild(Row);
            if (UVerticalBoxSlot *VSlot = Cast<UVerticalBoxSlot>(Added)) {
                // Wide above a heading, tight below it. Every row butted against the next before this, so
                // five sections read as one long list and the headings looked like rows that had lost
                // their control.
                const bool bHeading = IsGroupHeading(Def);
                const bool bFirst = Container->GetChildrenCount() <= 1;
                const UMythicUIStyleSettings &S = FMythicUIStyle::Get();
                VSlot->SetPadding(FMargin(0.0f, (bHeading && !bFirst) ? S.SectionGap : 0.0f,
                                          0.0f, bHeading ? S.SectionHeadingGap : 0.0f));
            }
        }
    }

    // No hand-placed buttons: Apply and Restore Defaults are CommonUI actions on the bound action bar, so
    // they carry the glyph for whatever the player is holding and work identically on a pad.
    ApplyCategoryVisibility();
}

void UMythicSettingsScreenBase::LabelButton(UCommonButtonBase *Button, const FText &Label) const {
    if (!Button) {
        return;
    }
    // Same lookup the menu shell uses for tab labels: the button class names its own text widget, so a
    // different button only needs a config change.
    if (UWidget *Found = Button->GetWidgetFromName(TabLabelWidgetName)) {
        if (UTextBlock *Text = Cast<UTextBlock>(Found)) {
            Text->SetText(Label);
        }
    }
}

void UMythicSettingsScreenBase::PushChrome() {
    bPendingApply = UMythicSettingAccess::HasStagedChanges();

    const TArray<FMythicSettingCategory> Cats = GetCategories();

    if (Text_Title) {
        Text_Title->SetText(Cats.IsValidIndex(ActiveCategory) ? Cats[ActiveCategory].Label : FText::GetEmpty());
    }
    if (Text_Breadcrumb) {
        // The shell header already says SETTINGS. A second copy of it is noise.
        Text_Breadcrumb->SetVisibility(ESlateVisibility::Collapsed);
    }

    const bool bHasFocus = !FocusedRow.Label.IsEmpty();
    if (Text_DetailTitle) {
        Text_DetailTitle->SetText(bHasFocus ? FocusedRow.Label : FText::GetEmpty());
    }
    if (Text_DetailBody) {
        Text_DetailBody->SetText(bHasFocus ? FocusedRow.Description : EmptyDetailHint);
    }
    if (Text_DetailNow) {
        // A label introducing nothing. With no row focused the panel read "CURRENTLY" over blank space.
        Text_DetailNow->SetVisibility(bHasFocus ? ESlateVisibility::HitTestInvisible
                                                : ESlateVisibility::Collapsed);
    }
    if (Text_DetailValue) {
        Text_DetailValue->SetVisibility(bHasFocus ? ESlateVisibility::HitTestInvisible
                                                  : ESlateVisibility::Collapsed);
        if (bHasFocus) {
            Text_DetailValue->SetText(UMythicSettingAccess::GetDisplayText(FocusedRow));
        }
    }
    if (Text_Status) {
        // Apply is meaningless unless the screen says something is waiting for it. Counting them also
        // tells the player how much they are about to discard if they back out.
        const bool bPending = UMythicSettingAccess::HasStagedChanges();
        Text_Status->SetText(bPending
                                 ? NSLOCTEXT("MythicSettings", "PendingApply",
                                             "Unapplied changes — press Apply to confirm")
                                 : FText::GetEmpty());
        Text_Status->SetVisibility(bPending ? ESlateVisibility::HitTestInvisible
                                            : ESlateVisibility::Collapsed);
    }
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

    // The group deselects whatever was selected before; asking each button to drop its own state does not
    // work when the button is neither toggleable nor grouped.
    if (RailGroup && TabButtons.IsValidIndex(ActiveCategory)) {
        RailGroup->SelectButtonAtIndex(ActiveCategory, false);
    }

    /**
     * Move the detail panel to this tab's first real setting.
     *
     * FocusedRow survived a tab change, so the panel went on describing a setting from the tab you had
     * just left - Controls open, the panel explaining Always Show HUD. Headings are skipped for the same
     * reason focus skips them: a heading has nothing to say about a value.
     */
    FocusedRow = FMythicSettingDefinition();
    for (UMythicSettingRowBase *Row : ActiveRows) {
        if (Row && !IsGroupHeading(Row->GetDefinition())) {
            FocusedRow = Row->GetDefinition();
            break;
        }
    }

    PushChrome();
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

    // A heading is only worth its line when there is more than one group to tell apart. With a single
    // group it restates the category the rail already has selected - a section inside a section of one,
    // which is what makes the page read as over-structured.
    int32 LiveGroups = 0;
    for (const FText &GroupName : Order) {
        for (const FMythicSettingDefinition &Def : Loaded->Settings) {
            if (Def.Category == Category.Id && Def.Group.EqualTo(GroupName)) {
                ++LiveGroups;
                break;
            }
        }
    }
    const bool bShowHeadings = LiveGroups > 1;

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

        if (bShowHeadings) {
            FMythicSettingDefinition Heading;
            Heading.Group = GroupName;
            Heading.Label = GroupName;
            Heading.Category = Category.Id;
            Rows.Add(Heading);
        }
        Rows.Append(InGroup);
    }

    return Rows;
}

void UMythicSettingsScreenBase::SetFocusedRow(const FMythicSettingDefinition &Row) {
    FocusedRow = Row;
    PushChrome();
    OnFocusedRowChanged();
}

void UMythicSettingsScreenBase::MarkPendingApply() {
    if (bPendingApply) {
        return;
    }
    bPendingApply = true;
    PushChrome();
    OnPendingApplyChanged();
}

void UMythicSettingsScreenBase::NativeOnDeactivated() {
    // Leaving without applying puts everything back. A settings screen that keeps changes you walked away
    // from is how players end up with a renderer they never chose and cannot retrace.
    // Nothing was applied, so discarding is just forgetting - no restore pass to get wrong.
    RemoveUIInputContext();
    UMythicSettingAccess::RevertStaged();
    bPendingApply = false;
    Super::NativeOnDeactivated();
}

void UMythicSettingsScreenBase::ApplyAndSave() {
    UMythicSettingAccess::CommitStaged();
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
