
#include "UI/Settings/MythicSettingDefinition.h"

#include "UI/Settings/MythicSettingAccess.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "MythicSettings"

TArray<FMythicSettingDefinition> UMythicSettingsCatalog::GetSettingsInCategory(FGameplayTag CategoryId) const {
    TArray<FMythicSettingDefinition> Out;
    for (const FMythicSettingDefinition &Def : Settings) {
        if (Def.Category == CategoryId) {
            Out.Add(Def);
        }
    }
    return Out;
}

#if WITH_EDITOR
EDataValidationResult UMythicSettingsCatalog::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);

    TSet<FName> SeenSources;
    TSet<FGameplayTag> DeclaredCategories;
    for (const FMythicSettingCategory &Category : Categories) {
        if (!Category.Id.IsValid()) {
            Context.AddError(LOCTEXT("CategoryNoId", "A category has no Id, so nothing can belong to it."));
            Result = EDataValidationResult::Invalid;
        }
        DeclaredCategories.Add(Category.Id);
    }

    for (const FMythicSettingDefinition &Def : Settings) {
        const FText Named = Def.Label.IsEmpty() ? FText::FromString(Def.SourceName.ToString()) : Def.Label;

        // A setting nobody can reach is the failure this catalog exists to prevent: the ambient occlusion
        // setting had working code and no row for hours, and nothing could detect it.
        if (!DeclaredCategories.Contains(Def.Category)) {
            Context.AddError(FText::Format(
                LOCTEXT("NoCategory", "'{0}' names category '{1}', which no tab declares, so it can never be shown."),
                Named, FText::FromString(Def.Category.ToString())));
            Result = EDataValidationResult::Invalid;
        }

        // Staging is keyed on SourceName, so two settings sharing one would share a single pending
        // change - the second row would silently overwrite the first every time either was touched.
        if (SeenSources.Contains(Def.SourceName)) {
            Context.AddError(FText::Format(
                LOCTEXT("DupSource", "'{0}' writes '{1}', which another setting already writes."),
                Named, FText::FromString(Def.SourceName.ToString())));
            Result = EDataValidationResult::Invalid;
        }
        SeenSources.Add(Def.SourceName);

        // A setting whose source does not resolve reads back its default forever and silently ignores every
        // change the player makes. A mistyped cvar looks exactly like a working row.
        FString Why;
        if (!UMythicSettingAccess::ResolvesSource(Def, Why)) {
            Context.AddError(FText::Format(LOCTEXT("BadSource", "'{0}' cannot be read or written: {1}."),
                                           Named, FText::FromString(Why)));
            Result = EDataValidationResult::Invalid;
        }

        if (Def.Description.IsEmpty()) {
            Context.AddWarning(FText::Format(
                LOCTEXT("NoDesc", "'{0}' has no description, so the detail panel is blank when it is focused."), Named));
        }

        if (Def.Control == EMythicSettingControl::Select && Def.Options.Num() < 2) {
            Context.AddError(FText::Format(
                LOCTEXT("SelectNoOptions", "'{0}' is a Select with fewer than two options."), Named));
            Result = EDataValidationResult::Invalid;
        }

        if (Def.Control == EMythicSettingControl::Slider && Def.MaxValue <= Def.MinValue) {
            Context.AddError(FText::Format(
                LOCTEXT("SliderRange", "'{0}' is a Slider whose Max is not above its Min."), Named));
            Result = EDataValidationResult::Invalid;
        }
    }

    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
