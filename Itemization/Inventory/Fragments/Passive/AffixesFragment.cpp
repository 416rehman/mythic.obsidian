#include "AffixesFragment.h"
#include "AbilitySystemComponent.h"
#include "Mythic/Itemization/Inventory/MythicItemInstance.h"
#include "AbilitySystemGlobals.h"
#include "GameModes/GameState/MythicGameState.h"

void UAffixesFragment::RollAffixes(int ItemLevel, int Qty) {
    int AffixesAdded = 0;
    // In case, there are not enough affixes to add, or the item level is too low, clamp to however many affixes there are.
    auto AffixPoolMap = this->AffixesBuildData.AffixPoolMap;
    int ClampedQty = FMath::Min(Qty, AffixPoolMap.Num());

    // If there are affixes to add, add them randomly
    if (AffixesAdded < ClampedQty && AffixPoolMap.Num() >= ClampedQty) {
        // Key array of the affix pool map
        TArray<FGameplayAttribute> AffixKeys;
        AffixPoolMap.GetKeys(AffixKeys);

        // Randomize the keys to get a random order
        AffixKeys.Sort([](const FGameplayAttribute &A, const FGameplayAttribute &B) {
            return FMath::RandBool();
        });

        // Loop over the keys and add the affixes to the group if not already added, if AffixesAdded >= MaxAffixes, break
        for (auto &AffixKey : AffixKeys) {
            if (AffixesAdded >= ClampedQty) {
                break;
            }

            // Check if this key is already added to the group
            if (IsAffixRolled(AffixKey, this->AffixesRuntimeReplicatedData.RolledAffixes)) {
                continue;
            }

            //random between min and max value weight based on the item level
            auto Attribute = AffixKey;
            auto RollDef = AffixPoolMap[AffixKey];

            this->AffixesRuntimeReplicatedData.RolledAffixes.Add(FRolledAffix(Attribute, ItemLevel, RollDef, false));

            AffixesAdded++;
        }
    }
}

bool UAffixesFragment::IsAffixRolled(const FGameplayAttribute &Affix, TArray<FRolledAffix> &RolledAffixes) {
    for (auto &RolledAffix : RolledAffixes) {
        if (RolledAffix.Attribute == Affix) {
            return true;
        }
    }

    return false;
}

void UAffixesFragment::RollCoreAffixes(int ItemLevel) {
    auto CoreAffixes = this->AffixesBuildData.CoreAffixes;

    if (CoreAffixes.Num() == 0) {
        UE_LOG(Mythic, Warning, TEXT("AffixesInstFragment::OnInstanced: No core stats to roll."));
        return;
    }
    // Roll core stats
    for (auto &CoreStat : CoreAffixes) {
        if (!CoreStat.Key.IsValid()) {
            UE_LOG(Mythic, Error, TEXT("AffixesInstFragment::OnInstanced: Invalid core stat attribute."));
            continue;
        }

        // If already rolled, skip
        if (IsAffixRolled(CoreStat.Key, this->AffixesRuntimeReplicatedData.RolledCoreAffixes)) {
            continue;
        }

        auto Attribute = CoreStat.Key;
        auto RollDef = CoreAffixes[CoreStat.Key];
        this->AffixesRuntimeReplicatedData.RolledCoreAffixes.Add(FRolledAffix(Attribute, ItemLevel, RollDef, true));
    }
}
#if WITH_EDITOR
bool UAffixesFragment::IsValidFragment(FText &OutErrorMessage) const {
    auto AffixPoolMap = this->AffixesBuildData.AffixPoolMap;
    auto CoreAffixes = this->AffixesBuildData.CoreAffixes;
    if (AffixPoolMap.Num() == 0 && CoreAffixes.Num() == 0) {
        OutErrorMessage = FText::FromString("No affixes to roll. Add some affixes to roll.");
        return false;
    }

    for (auto &Affix : AffixPoolMap) {
        if (!Affix.Key.IsValid()) {
            OutErrorMessage = FText::FromString("Invalid affix attribute in affix pool.");
            return false;
        }
        if (!Affix.Value.IsValid(OutErrorMessage)) {
            OutErrorMessage = FText::FromString("Invalid affix roll definition in affix pool.");
            return false;
        }
    }

    for (auto &CoreStat : CoreAffixes) {
        if (!CoreStat.Key.IsValid()) {
            OutErrorMessage = FText::FromString("Invalid affix attribute in core stats.");
            return false;
        }
        if (!CoreStat.Value.IsValid(OutErrorMessage)) {
            OutErrorMessage = FText::FromString("Invalid affix roll definition in core stats.");
            return false;
        }
    }

    return Super::IsValidFragment(OutErrorMessage);
}
#endif

void UAffixesFragment::OnInstanced(UMythicItemInstance *Instance) {
    Super::OnInstanced(Instance);

    // Quantity of affixes to roll is based on the item's rarity
    int RarityValue = Instance->GetItemDefinition()->Rarity;
    int AffixesToRoll = 1 + RarityValue;

    RollAffixes(Instance->GetItemLevel(), AffixesToRoll);
    RollCoreAffixes(Instance->GetItemLevel());
}

void UAffixesFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);

    auto Owner = ItemInstance->GetInventoryOwner();
    if (!Owner) {
        UE_LOG(Mythic, Error, TEXT("AffixesInstFragment::OnActiveItem: Invalid owner."));
        return;
    }
    auto ASC = &this->AffixesRuntimeReplicatedData.ASC;
    *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
    if (!ASC) {
        UE_LOG(Mythic, Error, TEXT("AffixesInstFragment::OnActiveItem: Invalid ASC."));
        return;
    }

    ApplyAffixes(*ASC, this->AffixesRuntimeReplicatedData.RolledAffixes);
    ApplyAffixes(*ASC, this->AffixesRuntimeReplicatedData.RolledCoreAffixes);
}

void UAffixesFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);
    auto ASC = this->AffixesRuntimeReplicatedData.ASC;
    if (!ASC) {
        UE_LOG(Mythic, Error, TEXT("AffixesInstFragment::DeactivateAffixes: Invalid ASC."));
        return;
    }

    // Remove affixes
    RemoveAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledAffixes);
    RemoveAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledCoreAffixes);

    // Clear the ASC
    ASC = nullptr;
}

bool UAffixesFragment::CanBeStackedWith(const UItemFragment *Other) const {
    if (!Super::CanBeStackedWith(Other)) {
        return false;
    }

    auto OtherFragment = Cast<UAffixesFragment>(Other);
    if (!OtherFragment) {
        return false;
    }
    // Check if the affixes are the same
    auto OtherAffixes = OtherFragment->AffixesRuntimeReplicatedData.RolledAffixes;
    auto Affixes = this->AffixesRuntimeReplicatedData.RolledAffixes;
    if (OtherAffixes.Num() != Affixes.Num()) {
        return false;
    }

    auto OtherCoreAffixes = OtherFragment->AffixesRuntimeReplicatedData.RolledCoreAffixes;
    auto OurCoreAffixes = this->AffixesRuntimeReplicatedData.RolledCoreAffixes;
    if (OtherCoreAffixes.Num() != OurCoreAffixes.Num()) {
        return false;
    }

    // Check if the affixes are the same
    for (int i = 0; i < Affixes.Num(); i++) {
        if (Affixes[i].Attribute != OtherAffixes[i].Attribute ||
            Affixes[i].Value != OtherAffixes[i].Value) {
            return false;
        }
    }

    // Check if the core affixes are the same
    for (int i = 0; i < OurCoreAffixes.Num(); i++) {
        if (OurCoreAffixes[i].Attribute != OtherCoreAffixes[i].Attribute ||
            OurCoreAffixes[i].Value != OtherCoreAffixes[i].Value) {
            return false;
        }
    }

    return true;
}

void UAffixesFragment::ApplyAffixes(UAbilitySystemComponent *ASC, TArray<FRolledAffix> &InRolledAffixes) {
    for (auto &Roll : InRolledAffixes) {
        if (!Roll.Attribute.IsValid()) {
            UE_LOG(Mythic, Error, TEXT("AffixesInstFragment::ApplyAffixes: Invalid affix attribute."));
            continue;
        }

        if (Roll.bIsApplied) {
            UE_LOG(Mythic, Warning, TEXT("AffixesInstFragment::ApplyAffixes: Affix %s already active."), *Roll.Attribute.GetName());
            continue;
        }

        // Check if the AttributeSet for this attribute exists on the ASC.
        ASC->ApplyModToAttribute(Roll.Attribute, Roll.Definition.Modifier, Roll.Value);
        Roll.bIsApplied = true;
    }
}

void UAffixesFragment::RemoveAffixes(UAbilitySystemComponent *ASC, TArray<FRolledAffix> &InRolledAffixes) {
    for (auto &Roll : InRolledAffixes) {
        if (!Roll.Attribute.IsValid()) {
            UE_LOG(Mythic, Error, TEXT("AffixesInstFragment::RemoveAffixes: Invalid affix attribute."));
            continue;
        }

        if (!Roll.bIsApplied) {
            UE_LOG(Mythic, Warning, TEXT("AffixesInstFragment::RemoveAffixes: Affix %s not active."), *Roll.Attribute.GetName());
            continue;
        }

        //Reverse the modifier
        auto ReversedValue = Roll.Value;
        auto modifier = Roll.Definition.Modifier;
        if (modifier == EGameplayModOp::Additive) {
            // Addition can be reversed by subtracting the value
            ReversedValue = -ReversedValue;
        }
        else if (modifier == EGameplayModOp::Multiplicitive || modifier == EGameplayModOp::Division) {
            // Multiplication and division can be reversed by taking the reciprocal
            ReversedValue = 1 / ReversedValue;
        }
        else {
            // Override is not supported
            UE_LOG(Mythic, Error, TEXT("AffixesInstFragment::RemoveAffixes: Invalid modifier."));
        }

        ASC->ApplyModToAttribute(Roll.Attribute, modifier, ReversedValue);
        Roll.bIsApplied = false;
    }
}
