#include "Itemization/Affixes/MythicAffixRollPolicy.h"

#include "Misc/DataValidation.h"
#include "System/MythicAssetManager.h"

#define LOCTEXT_NAMESPACE "MythicAffixRollPolicy"

const FMythicAffixRarityBudget *UMythicAffixRollPolicy::FindBudget(EItemRarity InRarity) const {
    return BudgetsByRarity.FindByPredicate([InRarity](const FMythicAffixRarityBudget &Budget) {
        return Budget.Rarity == InRarity;
    });
}

FPrimaryAssetId UMythicAffixRollPolicy::GetPrimaryAssetId() const {
    return PolicyTag.IsValid() ? FPrimaryAssetId(UMythicAssetManager::AffixRollPolicyType, PolicyTag.GetTagName())
                               : FPrimaryAssetId();
}

#if WITH_EDITOR
EDataValidationResult UMythicAffixRollPolicy::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto Error = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };
    if (!PolicyTag.IsValid() || !PolicyTag.ToString().StartsWith(TEXT("Itemization.AffixRollPolicy."))) {
        Error(LOCTEXT("InvalidPolicyTag", "PolicyTag must be a valid Itemization.AffixRollPolicy.* tag."));
    }
    if (DeveloperName.IsNone() || DesignerPurpose.TrimStartAndEnd().IsEmpty()
        || Revision < 1 || AlgorithmVersion < 1 || BudgetsByRarity.IsEmpty()) {
        Error(LOCTEXT("InvalidMetadata",
                      "Policy metadata, revision, algorithm version, and rarity budgets are required."));
    }
    TSet<uint8> Rarities;
    for (const FMythicAffixRarityBudget &Budget : BudgetsByRarity) {
        const uint8 Rarity = static_cast<uint8>(Budget.Rarity.GetValue());
        if (Rarities.Contains(Rarity) || Budget.RandomRollCount < 0 || !FMath::IsFinite(Budget.MagnitudeBudget)
            || Budget.MagnitudeBudget < 0.0f) {
            Error(LOCTEXT("InvalidBudget", "Rarity rows must be unique with nonnegative finite budgets/counts."));
        }
        Rarities.Add(Rarity);
        TSet<FGameplayTag> RollGroups;
        int32 Capacity = 0;
        for (const FMythicAffixRollGroupBudget &RollGroupBudget : Budget.RollGroupBudgets) {
            if (!RollGroupBudget.RollGroup.IsValid() || RollGroups.Contains(RollGroupBudget.RollGroup)
                || RollGroupBudget.MaxRolls < 0) {
                Error(LOCTEXT("InvalidRollGroupBudget", "Roll Group limits require unique valid groups and nonnegative caps."));
            }
            RollGroups.Add(RollGroupBudget.RollGroup);
            if (RollGroupBudget.MaxRolls > MAX_int32 - Capacity) {
                Error(LOCTEXT("RollGroupCapacityOverflow", "Roll Group capacity exceeds the supported integer range."));
                Capacity = MAX_int32;
            }
            else {
                Capacity += FMath::Max(0, RollGroupBudget.MaxRolls);
            }
        }
        if (Capacity < Budget.RandomRollCount) {
            Error(LOCTEXT("InsufficientCapacity", "Total Roll Group capacity cannot satisfy Random Roll Count."));
        }
    }
    for (uint8 Rarity = static_cast<uint8>(EItemRarity::Common);
         Rarity <= static_cast<uint8>(EItemRarity::Mythic); ++Rarity) {
        if (!Rarities.Contains(Rarity)) {
            Error(LOCTEXT("MissingRarityBudget",
                          "A roll policy must explicitly define a budget for every supported item rarity."));
            break;
        }
    }
    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
