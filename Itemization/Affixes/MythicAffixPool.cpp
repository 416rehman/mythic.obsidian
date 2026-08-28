#include "Itemization/Affixes/MythicAffixPool.h"

#include "Misc/DataValidation.h"
#include "System/MythicAssetManager.h"

#define LOCTEXT_NAMESPACE "MythicAffixPool"

FPrimaryAssetId UMythicAffixPool::GetPrimaryAssetId() const {
    return PoolTag.IsValid() ? FPrimaryAssetId(UMythicAssetManager::AffixPoolType, PoolTag.GetTagName())
                             : FPrimaryAssetId();
}

#if WITH_EDITOR
EDataValidationResult UMythicAffixPool::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto Error = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };
    if (!PoolTag.IsValid() || !PoolTag.ToString().StartsWith(TEXT("Itemization.AffixPool."))) {
        Error(LOCTEXT("InvalidPoolTag", "PoolTag must be a valid Itemization.AffixPool.* tag."));
    }
    if (DeveloperName.IsNone() || DesignerPurpose.TrimStartAndEnd().IsEmpty()
        || Revision < 1 || Entries.IsEmpty()) {
        Error(LOCTEXT("InvalidPoolMetadata", "Pool metadata, revision, and at least one row are required."));
    }
    TSet<FGuid> RowGuids;
    TSet<FMythicAffixDefinitionHandle> Handles;
    for (const FMythicAffixPoolEntry &Row : Entries) {
        if (!Row.PoolRowGuid.IsValid() || RowGuids.Contains(Row.PoolRowGuid)) {
            Error(LOCTEXT("DuplicateRowGuid", "PoolRowGuid values must be nonzero and unique."));
        }
        RowGuids.Add(Row.PoolRowGuid);
        if (Row.AffixDefinition.Asset.IsNull() || !Row.AffixDefinition.IsValid()
            || Handles.Contains(Row.AffixDefinition)) {
            Error(LOCTEXT("DuplicateDefinition", "Each pool row must directly reference a distinct valid Affix Definition asset."));
        }
        Handles.Add(Row.AffixDefinition);
        if (Row.DeveloperName.IsNone() || Row.RowRevision < 1 || !Row.RollGroup.IsValid()
            || !FMath::IsFinite(Row.SelectionWeight) || Row.SelectionWeight <= 0.0f) {
            Error(LOCTEXT("InvalidRow", "Every pool row needs developer identity, revision, Roll Group, and positive weight."));
        }
    }
    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
