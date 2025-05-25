#include "ItemDefinition.h"

#include "Misc/DataValidation.h"

//////////////////
/// VALIDATION ///
//////////////////
#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
void UItemDefinition::PostLoad() {
    Super::PostLoad();

    // Remove any null fragments
    Fragments.RemoveAll([](const TObjectPtr<UItemFragment> &Fragment) {
        return Fragment == nullptr;
    });
}

void UItemDefinition::PreSave(FObjectPreSaveContext SaveContext) {
    Super::PreSave(SaveContext);

    // Remove any null fragments before saving
    Fragments.RemoveAll([](const TObjectPtr<UItemFragment> &Fragment) {
        return Fragment == nullptr;
    });
}

EDataValidationResult UItemDefinition::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);

    bool bHasNullFragments = false;
    for (int32 Index = 0; Index < Fragments.Num(); ++Index) {
        if (!Fragments[Index]) {
            Context.AddError(FText::Format(
                NSLOCTEXT("ItemDefinition", "NullFragmentError", "Null fragment at index {0}"),
                FText::AsNumber(Index)));
            bHasNullFragments = true;
        }
    }

    return bHasNullFragments ? EDataValidationResult::Invalid : Result;
}
#endif
