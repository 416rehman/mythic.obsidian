#include "AffixesFragment.h"
#include "AbilitySystemComponent.h"
#include "Mythic/Itemization/Inventory/MythicItemInstance.h"
#include "AbilitySystemGlobals.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Itemization/MythicLootSettings.h" // data-driven per-rarity affix count
#include "Mythic/Mythic.h"

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

        // Randomize the keys to get a UNIFORM random order. (Was an AffixKeys.Sort with a RandBool comparator, which
        // violates strict-weak-ordering — UE's introsort under a random predicate yields a strongly NON-uniform
        // permutation, biasing which affixes land on every Rare+ item. Fisher-Yates gives a fair shuffle.)
        for (int32 i = AffixKeys.Num() - 1; i > 0; --i) {
            const int32 j = FMath::RandRange(0, i);
            AffixKeys.Swap(i, j);
        }

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

            FRolledAffix NewAffix(Attribute, ItemLevel, RollDef, false);
            // Mirror the reroll's invertibility guard (RerollUnlockedAffixes): a multiplicative/division affix that
            // rolls (near) zero would make RemoveAffixes' 1/Value reciprocal inf, permanently poisoning the attribute.
            // Keep the INITIAL magnitude invertible too (the reroll already does this; the initial roll did not).
            if ((RollDef.Modifier == EGameplayModOp::Multiplicitive || RollDef.Modifier == EGameplayModOp::Division)
                && FMath::IsNearlyZero(NewAffix.Value)) {
                NewAffix.Value = KINDA_SMALL_NUMBER;
            }
            this->AffixesRuntimeReplicatedData.RolledAffixes.Add(NewAffix);

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
        UE_LOG(Myth, Warning, TEXT("AffixesInstFragment::OnInstanced: No core stats to roll."));
        return;
    }
    // Roll core stats
    for (auto &CoreStat : CoreAffixes) {
        if (!CoreStat.Key.IsValid()) {
            UE_LOG(Myth, Error, TEXT("AffixesInstFragment::OnInstanced: Invalid core stat attribute."));
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

    // Quantity of random affixes to roll is based on the item's rarity — now DATA-DRIVEN (Project Settings -> Game ->
    // Mythic Loot Settings, AffixCountByRarity). GetDefault is a world-independent CDO read, safe here (OnInstanced can
    // run during save-load with no world). Bounds-guarded fallback to the prior hardcoded "1 + rarity" so an out-of-range
    // rarity or a short/empty array can never crash or mis-roll.
    int RarityValue = Instance->GetItemDefinition()->Rarity;
    const UMythicLootSettings *LootSettings = GetDefault<UMythicLootSettings>();
    int AffixesToRoll = (LootSettings && LootSettings->AffixCountByRarity.IsValidIndex(RarityValue))
                            ? LootSettings->AffixCountByRarity[RarityValue]
                            : (1 + RarityValue);

    RollAffixes(Instance->GetItemLevel(), AffixesToRoll);
    RollCoreAffixes(Instance->GetItemLevel());
}

void UAffixesFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);

    auto Owner = ItemInstance->GetInventoryOwner();
    if (!Owner) {
        UE_LOG(Myth, Error, TEXT("AffixesInstFragment::OnActiveItem: Invalid owner."));
        return;
    }
    UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
    if (!ASC) {
        // Guard the actual ASC, not the never-null address of the member (the old `if (!&member)` was dead code).
        UE_LOG(Myth, Error, TEXT("AffixesInstFragment::OnActiveItem: Invalid ASC."));
        return;
    }
    this->AffixesRuntimeReplicatedData.ASC = ASC; // member is the single source of truth for "active on an ASC"

    ApplyAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledAffixes);
    ApplyAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledCoreAffixes);
}

void UAffixesFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);
    auto ASC = this->AffixesRuntimeReplicatedData.ASC;
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("AffixesInstFragment::DeactivateAffixes: Invalid ASC."));
        return;
    }

    // Remove affixes in REVERSE of the apply order (OnItemActivated applies RolledAffixes then RolledCoreAffixes).
    // ApplyModToAttribute mutates the BASE value, so a non-commutative (mult/div) op must be undone in reverse order —
    // otherwise a random affix and a core affix on the SAME attribute with mixed ops would not restore the original
    // value on unequip (e.g. core +50 then random ×1.2 reversed in forward order leaves the attribute corrupted).
    RemoveAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledCoreAffixes);
    RemoveAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledAffixes);

    // Clear the MEMBER (not a local copy) so the item reads as inactive — otherwise RerollUnlockedAffixes would
    // see a stale ASC and re-apply re-rolled magnitudes onto the previous wearer's attributes.
    this->AffixesRuntimeReplicatedData.ASC = nullptr;
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

    // Compare affixes ORDER-INDEPENDENTLY: roll order is randomized (Fisher-Yates in RollAffixes), so a positional
    // compare would wrongly reject two otherwise-identical items whose affixes rolled in a different order, breaking
    // stacking. Multiset match — every affix in ours must pair with a distinct same-(Attribute,Value) affix in theirs
    // (counts are already equal-checked above).
    auto AffixMultisetMatch = [](const TArray<FRolledAffix> &Ours, const TArray<FRolledAffix> &Theirs) -> bool {
        TArray<bool> Used;
        Used.Init(false, Theirs.Num());
        for (const FRolledAffix &A : Ours) {
            bool bMatched = false;
            for (int32 j = 0; j < Theirs.Num(); ++j) {
                if (!Used[j] && Theirs[j].Attribute == A.Attribute && Theirs[j].Value == A.Value) {
                    Used[j] = true;
                    bMatched = true;
                    break;
                }
            }
            if (!bMatched) {
                return false;
            }
        }
        return true;
    };

    if (!AffixMultisetMatch(Affixes, OtherAffixes)) {
        return false;
    }
    if (!AffixMultisetMatch(OurCoreAffixes, OtherCoreAffixes)) {
        return false;
    }

    return true;
}

void UAffixesFragment::RerollUnlockedAffixes(int32 ItemLevel) {
    const AActor *Owner = GetOwningActor();
    if (!Owner || !Owner->HasAuthority()) {
        return; // server-authoritative (mirrors UDurabilityFragment::ServerApplyWear)
    }

    UAbilitySystemComponent *ASC = this->AffixesRuntimeReplicatedData.ASC;
    const bool bActive = (ASC != nullptr);

    // If the item is live on an ASC, reverse the current modifiers using the CURRENT values BEFORE re-rolling —
    // otherwise RemoveAffixes would reverse with the new (wrong) magnitudes and corrupt the attribute.
    if (bActive) {
        RemoveAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledAffixes);
    }

    // Re-roll only the unlocked random affixes. Core affixes live in RolledCoreAffixes (and roll locked), so they
    // are never touched here. The roll mirrors the FRolledAffix construction (RandRange over level-scaled bounds).
    for (FRolledAffix &Affix : this->AffixesRuntimeReplicatedData.RolledAffixes) {
        if (Affix.bIsLocked || !Affix.Attribute.IsValid()) {
            continue;
        }
        Affix.Value = FMath::RandRange(Affix.Definition.GetScaledMin(ItemLevel), Affix.Definition.GetScaledMax(ItemLevel));
        // A multiplicative/division affix that rolls (near) zero would make RemoveAffixes' 1/Value reciprocal inf,
        // permanently poisoning the attribute on the next reroll. Keep the magnitude invertible.
        if ((Affix.Definition.Modifier == EGameplayModOp::Multiplicitive || Affix.Definition.Modifier == EGameplayModOp::Division)
            && FMath::IsNearlyZero(Affix.Value)) {
            Affix.Value = KINDA_SMALL_NUMBER;
        }
    }

    // Re-apply with the new values (locked affixes round-trip to the same value; only unlocked ones changed).
    if (bActive) {
        ApplyAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledAffixes);
    }
}

void UAffixesFragment::SetAffixLocked(int32 AffixIndex, bool bLocked) {
    const AActor *Owner = GetOwningActor();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (this->AffixesRuntimeReplicatedData.RolledAffixes.IsValidIndex(AffixIndex)) {
        this->AffixesRuntimeReplicatedData.RolledAffixes[AffixIndex].bIsLocked = bLocked;
    }
}

bool UAffixesFragment::ComputeReversedModValue(TEnumAsByte<EGameplayModOp::Type> Modifier, float Value, float &OutReversed) {
    if (Modifier == EGameplayModOp::Additive) {
        OutReversed = -Value; // subtract back out
        return true;
    }
    if (Modifier == EGameplayModOp::Multiplicitive || Modifier == EGameplayModOp::Division) {
        if (FMath::IsNearlyZero(Value)) {
            OutReversed = 0.0f; // 1/0 = inf would poison the attribute — caller skips
            return false;
        }
        OutReversed = 1.0f / Value; // reciprocal reverses both multiply-by and divide-by
        return true;
    }
    OutReversed = 0.0f; // Override (or any other op) is not reversible by a single mod
    return false;
}

void UAffixesFragment::ApplyAffixes(UAbilitySystemComponent *ASC, TArray<FRolledAffix> &InRolledAffixes) {
    for (auto &Roll : InRolledAffixes) {
        if (!Roll.Attribute.IsValid()) {
            UE_LOG(Myth, Error, TEXT("AffixesInstFragment::ApplyAffixes: Invalid affix attribute."));
            continue;
        }

        if (Roll.bIsApplied) {
            UE_LOG(Myth, Warning, TEXT("AffixesInstFragment::ApplyAffixes: Affix %s already active."), *Roll.Attribute.GetName());
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
            UE_LOG(Myth, Error, TEXT("AffixesInstFragment::RemoveAffixes: Invalid affix attribute."));
            continue;
        }

        if (!Roll.bIsApplied) {
            UE_LOG(Myth, Warning, TEXT("AffixesInstFragment::RemoveAffixes: Affix %s not active."), *Roll.Attribute.GetName());
            continue;
        }

        // Reverse the modifier (Additive → negate; Mult/Div → reciprocal; zero/Override → skip). Shared, tested rule.
        const TEnumAsByte<EGameplayModOp::Type> modifier = Roll.Definition.Modifier;
        float ReversedValue = 0.0f;
        if (!ComputeReversedModValue(modifier, Roll.Value, ReversedValue)) {
            UE_LOG(Myth, Error,
                   TEXT("AffixesInstFragment::RemoveAffixes: non-invertible modifier (zero mult/div, or Override) for affix %s; skipping."),
                   *Roll.Attribute.GetName());
            Roll.bIsApplied = false;
            continue;
        }

        ASC->ApplyModToAttribute(Roll.Attribute, modifier, ReversedValue);
        Roll.bIsApplied = false;
    }
}
