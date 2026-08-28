// Copyright Stellar Games. All Rights Reserved.

#include "Stats/MythicStatCategoryDefinition.h"

#include "Internationalization/Text.h"
#include "System/MythicAssetManager.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace {
bool HasStableCategoryStringTableIdentity(const FText& Text) {
    FName TableId;
    FString Key;
    return !Text.IsEmpty() && FTextInspector::GetTableIdAndKey(Text, TableId, Key)
        && !TableId.IsNone() && !Key.IsEmpty();
}
}

FPrimaryAssetId UMythicStatCategoryDefinition::GetPrimaryAssetId() const {
    return FPrimaryAssetId(UMythicAssetManager::StatCategoryDefinitionType, CategoryTag.GetTagName());
}

bool UMythicStatCategoryDefinition::AppendValidationErrors(TArray<FText>& OutErrors) const {
    const int32 InitialErrorCount = OutErrors.Num();

    if (DeveloperName.IsNone()) {
        OutErrors.Add(NSLOCTEXT("MythicStatCategoryDefinition", "MissingDeveloperName", "DeveloperName is required."));
    }
    if (DesignerPurpose.TrimStartAndEnd().IsEmpty()) {
        OutErrors.Add(NSLOCTEXT("MythicStatCategoryDefinition", "MissingDesignerPurpose", "DesignerPurpose is required."));
    }
    if (Revision < 1) {
        OutErrors.Add(NSLOCTEXT("MythicStatCategoryDefinition", "InvalidRevision", "Revision must be at least 1."));
    }
    if (PresentationRevision < 1) {
        OutErrors.Add(NSLOCTEXT("MythicStatCategoryDefinition", "InvalidPresentationRevision", "PresentationRevision must be at least 1."));
    }
    if (!CategoryTag.IsValid() || !CategoryTag.ToString().StartsWith(TEXT("Stat.Category."))) {
        OutErrors.Add(NSLOCTEXT("MythicStatCategoryDefinition", "InvalidCategoryTag", "CategoryTag must be a valid child of Stat.Category."));
    }
    if (!HasStableCategoryStringTableIdentity(DisplayName)) {
        OutErrors.Add(NSLOCTEXT("MythicStatCategoryDefinition", "InvalidDisplayName", "DisplayName must be nonempty and use a stable String Table identity."));
    }
    return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult UMythicStatCategoryDefinition::IsDataValid(FDataValidationContext& Context) const {
    const EDataValidationResult ParentResult = Super::IsDataValid(Context);
    TArray<FText> Errors;
    AppendValidationErrors(Errors);
    for (const FText& Error : Errors) {
        Context.AddError(Error);
    }
    return ParentResult == EDataValidationResult::Invalid || !Errors.IsEmpty()
        ? EDataValidationResult::Invalid
        : EDataValidationResult::Valid;
}
#endif
