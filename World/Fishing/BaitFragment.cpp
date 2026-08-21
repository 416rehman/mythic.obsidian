#include "Itemization/Inventory/Fragments/Passive/BaitFragment.h"

#include "Itemization/Inventory/MythicItemInstance.h"

void UBaitFragment::OnInstanced(UMythicItemInstance *Instance) {
    Super::OnInstanced(Instance);
    if (Instance) {
        for (const FGameplayTag &Tag : BaitTags) {
            if (Tag.IsValid()) {
                Instance->AddTag(Tag);
            }
        }
    }
}

bool UBaitFragment::CanBeStackedWith(const UItemFragment *Other) const {
    const UBaitFragment *OtherFrag = Cast<UBaitFragment>(Other);
    if (!OtherFrag) {
        return false;
    }
    return Super::CanBeStackedWith(Other) && BaitTags == OtherFrag->BaitTags;
}
