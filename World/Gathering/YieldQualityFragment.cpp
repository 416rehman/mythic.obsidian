#include "Itemization/Inventory/Fragments/Passive/YieldQualityFragment.h"

#include "Itemization/Inventory/MythicItemInstance.h"

FGameplayTag UYieldQualityFragment::GetQualityTag() const {
    return FGameplayTag::RequestGameplayTag(FMythicYieldQuality::QualityTagName(QualityTier), false);
}

EMythicYieldQuality UYieldQualityFragment::GetTierOfInstance(UMythicItemInstance *Instance) {
    if (Instance) {
        if (const UYieldQualityFragment *Frag = Instance->GetFragment<UYieldQualityFragment>()) {
            return Frag->QualityTier;
        }
    }
    return EMythicYieldQuality::Common;
}

void UYieldQualityFragment::OnInstanced(UMythicItemInstance *Instance) {
    Super::OnInstanced(Instance);
    if (Instance) {
        const FGameplayTag QualityTag = GetQualityTag();
        if (QualityTag.IsValid()) {
            Instance->AddTag(QualityTag);
        }
    }
}

bool UYieldQualityFragment::CanBeStackedWith(const UItemFragment *Other) const {
    const UYieldQualityFragment *OtherFrag = Cast<UYieldQualityFragment>(Other);
    if (!OtherFrag) {
        return false;
    }
    return Super::CanBeStackedWith(Other) && QualityTier == OtherFrag->QualityTier;
}
