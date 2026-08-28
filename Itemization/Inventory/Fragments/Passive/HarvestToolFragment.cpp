#include "Itemization/Inventory/Fragments/Passive/HarvestToolFragment.h"

#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "World/Harvesting/MythicHarvestTypes.h"

const UHarvestToolFragment *UHarvestToolFragment::FindOnItem(UMythicItemInstance *ItemInstance) {
    return ItemInstance ? ItemInstance->GetFragment<UHarvestToolFragment>() : nullptr;
}

const UHarvestToolFragment *UHarvestToolFragment::FindOnDefinition(const UItemDefinition *ItemDefinition) {
    if (!ItemDefinition) {
        return nullptr;
    }
    for (const UItemFragment *Fragment : ItemDefinition->Fragments) {
        if (const UHarvestToolFragment *HarvestFragment = Cast<UHarvestToolFragment>(Fragment)) {
            return HarvestFragment;
        }
    }
    return nullptr;
}

#if WITH_EDITOR
bool UHarvestToolFragment::IsValidFragment(FText &OutErrorMessage) const {
    if (!Super::IsValidFragment(OutErrorMessage)) {
        return false;
    }
    if (!ToolType) {
        OutErrorMessage = NSLOCTEXT("HarvestToolFragment", "MissingToolType", "Harvest Tool Fragment requires one direct Harvest Tool Type Definition.");
        return false;
    }
    if (ToolTier < 0) {
        OutErrorMessage = NSLOCTEXT("HarvestToolFragment", "InvalidTier", "Harvest Tool Fragment tier must be non-negative.");
        return false;
    }

    FMythicHarvestWork QuantizedBaseWork;
    if (!FMythicHarvestWork::TryFromWorkUnits(BaseWork, QuantizedBaseWork) || QuantizedBaseWork.IsZero()) {
        OutErrorMessage = NSLOCTEXT("HarvestToolFragment", "InvalidBaseWork",
                                    "Harvest Tool Fragment Base Work must be finite, positive, and representable at the fixed work quantum.");
        return false;
    }
    if (DurabilityWearPerAcceptedHit < 0) {
        OutErrorMessage = NSLOCTEXT("HarvestToolFragment", "InvalidWear", "Harvest Tool Fragment durability wear must be non-negative.");
        return false;
    }
    if (MaxNodesPerCycle < 1) {
        OutErrorMessage = NSLOCTEXT("HarvestToolFragment", "InvalidNodeBudget", "Harvest Tool Fragment Max Nodes Per Cycle must be at least one.");
        return false;
    }

    const UItemDefinition *Definition = Cast<UItemDefinition>(GetOuter());
    if (!Definition) {
        return true;
    }

    int32 HarvestFragmentCount = 0;
    int32 AttackFragmentCount = 0;
    int32 DurabilityFragmentCount = 0;
    const UDurabilityFragment *DurabilityFragment = nullptr;
    for (const UItemFragment *Fragment : Definition->Fragments) {
        HarvestFragmentCount += Cast<UHarvestToolFragment>(Fragment) ? 1 : 0;
        AttackFragmentCount += Cast<UAttackFragment>(Fragment) ? 1 : 0;
        if (const UDurabilityFragment *Candidate = Cast<UDurabilityFragment>(Fragment)) {
            ++DurabilityFragmentCount;
            DurabilityFragment = Candidate;
        }
    }
    if (HarvestFragmentCount != 1 || DurabilityFragmentCount != 1) {
        OutErrorMessage = NSLOCTEXT(
            "HarvestToolFragment", "InvalidComposition",
            "A harvest tool Item Definition requires exactly one Harvest Tool Fragment and one Durability Fragment on the same item.");
        return false;
    }
    // A tool is passive gear that authorizes harvesting by occupying its slot; it is never swung, so an Attack
    // Fragment on it would bind a second attacker beside the weapon.
    if (AttackFragmentCount != 0) {
        OutErrorMessage = NSLOCTEXT(
            "HarvestToolFragment", "ToolCarriesAttackFragment",
            "A harvest tool Item Definition must not carry an Attack Fragment: tools are never wielded, and only the equipped weapon attacks.");
        return false;
    }
    if (!DurabilityFragment || DurabilityFragment->DurabilityConfig.MaxDurability <= 0) {
        OutErrorMessage = NSLOCTEXT(
            "HarvestToolFragment", "DisabledDurability",
            "A harvest tool Item Definition requires a positive definition-authored maximum durability.");
        return false;
    }

    return true;
}
#endif

bool UHarvestToolFragment::CanBeStackedWith(const UItemFragment *Other) const {
    const UHarvestToolFragment *OtherHarvest = Cast<UHarvestToolFragment>(Other);
    return OtherHarvest && ToolType == OtherHarvest->ToolType && ToolTier == OtherHarvest->ToolTier && BaseWork == OtherHarvest->BaseWork &&
        DurabilityWearPerAcceptedHit == OtherHarvest->DurabilityWearPerAcceptedHit && MaxNodesPerCycle == OtherHarvest->MaxNodesPerCycle;
}
