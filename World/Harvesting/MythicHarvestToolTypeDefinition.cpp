#include "World/Harvesting/MythicHarvestToolTypeDefinition.h"

#include "System/MythicAssetManager.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#define LOCTEXT_NAMESPACE "MythicHarvestToolTypeDefinition"

FPrimaryAssetId UMythicHarvestToolTypeDefinition::GetPrimaryAssetId() const {
    return FPrimaryAssetId(UMythicAssetManager::HarvestToolTypeDefinitionType, GetFName());
}

bool UMythicHarvestToolTypeDefinition::AppendValidationErrors(TArray<FText> &OutErrors) const {
    const int32 InitialErrorCount = OutErrors.Num();
    if (DisplayName.IsEmpty()) {
        OutErrors.Add(LOCTEXT("MissingDisplayName", "Harvest tool type Display Name is required."));
    }
    return OutErrors.Num() == InitialErrorCount;
}

#if WITH_EDITOR
EDataValidationResult UMythicHarvestToolTypeDefinition::IsDataValid(FDataValidationContext &Context) const {
    const EDataValidationResult ParentResult = Super::IsDataValid(Context);
    TArray<FText> Errors;
    AppendValidationErrors(Errors);
    for (const FText &Error : Errors) {
        Context.AddError(Error);
    }
    return ParentResult == EDataValidationResult::Invalid || !Errors.IsEmpty() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

#undef LOCTEXT_NAMESPACE
