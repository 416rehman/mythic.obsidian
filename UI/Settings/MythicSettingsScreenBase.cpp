
#include "UI/Settings/MythicSettingsScreenBase.h"

#include "UI/Settings/MythicSettingAccess.h"
#include "UI/Settings/MythicUserSettings.h"

UMythicSettingsCatalog *UMythicSettingsScreenBase::GetCatalog() const {
    return Catalog.IsNull() ? nullptr : Catalog.LoadSynchronous();
}

TArray<FMythicSettingCategory> UMythicSettingsScreenBase::GetCategories() const {
    const UMythicSettingsCatalog *Loaded = GetCatalog();
    return Loaded ? Loaded->Categories : TArray<FMythicSettingCategory>();
}

void UMythicSettingsScreenBase::SetActiveCategoryIndex(int32 Index) {
    const TArray<FMythicSettingCategory> Cats = GetCategories();
    if (!Cats.IsValidIndex(Index) || Index == ActiveCategory) {
        return;
    }
    ActiveCategory = Index;
    OnCategoryChanged();
}

bool UMythicSettingsScreenBase::IsGroupHeading(const FMythicSettingDefinition &Row) {
    // A heading carries its group name and nothing to read or write, which is what marks it apart from a
    // real setting without needing a second list for the Blueprint to keep in step.
    return Row.SourceName.IsNone() && !Row.Group.IsEmpty();
}

TArray<FMythicSettingDefinition> UMythicSettingsScreenBase::GetRowsForActiveCategory() const {
    TArray<FMythicSettingDefinition> Rows;

    const UMythicSettingsCatalog *Loaded = GetCatalog();
    const TArray<FMythicSettingCategory> Cats = GetCategories();
    if (!Loaded || !Cats.IsValidIndex(ActiveCategory)) {
        return Rows;
    }
    const FMythicSettingCategory &Category = Cats[ActiveCategory];

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
    OnCategoryChanged();
}
