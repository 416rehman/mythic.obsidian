// 
#include "ItemDefinition.h"
#include "Misc/DataValidation.h"

//////////////////
/// VALIDATION ///
//////////////////
#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#include "Windows/WindowsPlatformApplicationMisc.h"

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

    bool bHasValidType = false;
    // makes sure itemtype is a child of itemization.type
    if (ItemType.IsValid()) {
        FGameplayTag ParentTag = FGameplayTag::RequestGameplayTag(FName("Itemization.Type"));
        if (ItemType.MatchesTag(ParentTag)) {
            bHasValidType = true;
        }
    }

    bool result = bHasValidType && !bHasNullFragments;
    return result ? EDataValidationResult::Invalid : Result;
}
#endif
