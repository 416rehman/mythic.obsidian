#include "AffixesFragment.h"
#include "AbilitySystemComponent.h"
#include "Settings/MythicCombatSettings.h"
#include "Mythic/Itemization/Inventory/MythicItemInstance.h"
#include "AbilitySystemGlobals.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Itemization/MythicLootSettings.h"
#include "Mythic/Mythic.h"
#include "GameFramework/Pawn.h"
#include "Player/MythicPlayerController.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"

void UAffixesFragment::RollAffixes(int ItemLevel, int Qty) {
    int AffixesAdded = 0;
    auto AffixPoolMap = this->AffixesBuildData.AffixPoolMap;
    int ClampedQty = FMath::Min(Qty, AffixPoolMap.Num());

    if (AffixesAdded < ClampedQty && AffixPoolMap.Num() >= ClampedQty) {
        TArray<FGameplayAttribute> AffixKeys;
        AffixPoolMap.GetKeys(AffixKeys);

        for (int32 i = AffixKeys.Num() - 1; i > 0; --i) {
            const int32 j = FMath::RandRange(0, i);
            AffixKeys.Swap(i, j);
        }

        for (auto &AffixKey : AffixKeys) {
            if (AffixesAdded >= ClampedQty) {
                break;
            }

            if (IsAffixRolled(AffixKey, this->AffixesRuntimeReplicatedData.RolledAffixes)) {
                continue;
            }

            auto Attribute = AffixKey;
            auto RollDef = AffixPoolMap[AffixKey];

            FRolledAffix NewAffix(Attribute, ItemLevel, RollDef, false);
            if ((RollDef.Modifier == EGameplayModOp::Multiplicitive || RollDef.Modifier == EGameplayModOp::Division)
                && FMath::IsNearlyZero(NewAffix.Value)) {
                NewAffix.Value = KINDA_SMALL_NUMBER;
            }
            this->AffixesRuntimeReplicatedData.RolledAffixes.Add(NewAffix);

            AffixesAdded++;
        }
    }
}

bool UAffixesFragment::ShouldEmitArmorEquipEvent(bool bIsCanonicalAffixesFragment, bool bItemHasWeaponFragment,
                                                 bool bAlreadyEmitted) {
    return bIsCanonicalAffixesFragment && !bItemHasWeaponFragment && !bAlreadyEmitted;
}

bool UAffixesFragment::IsAffixRolled(const FGameplayAttribute &Affix, TArray<FRolledAffix> &RolledAffixes) {
    for (auto &RolledAffix : RolledAffixes) {
        if (RolledAffix.Attribute == Affix) {
            return true;
        }
    }

    return false;
}

bool UAffixesFragment::ShouldApplyAffixes(bool bBroken) {
    return !bBroken;
}

void UAffixesFragment::OnDurabilityBrokenStateChanged(bool bBroken) {
    UAbilitySystemComponent *ASC = this->AffixesRuntimeReplicatedData.ASC;
    if (!ASC) {
        return;
    }

    if (ShouldApplyAffixes(bBroken)) {
        ApplyAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledAffixes);
        ApplyAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledCoreAffixes);
    }
    else {
        RemoveAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledCoreAffixes);
        RemoveAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledAffixes);
    }
}

void UAffixesFragment::RollCoreAffixes(int ItemLevel) {
    auto CoreAffixes = this->AffixesBuildData.CoreAffixes;

    if (CoreAffixes.Num() == 0) {
        UE_LOG(Myth, Warning, TEXT("AffixesInstFragment::OnInstanced: No core stats to roll."));
        return;
    }
    for (auto &CoreStat : CoreAffixes) {
        if (!CoreStat.Key.IsValid()) {
            UE_LOG(Myth, Error, TEXT("AffixesInstFragment::OnInstanced: Invalid core stat attribute."));
            continue;
        }

        if (IsAffixRolled(CoreStat.Key, this->AffixesRuntimeReplicatedData.RolledCoreAffixes)) {
            continue;
        }

        auto Attribute = CoreStat.Key;
        auto RollDef = CoreAffixes[CoreStat.Key];

        // Centrally scaled families derive their band from combat settings: the shared level curve consumes the
        // item level, so the private linear LevelScaling is zeroed to keep it from applying a second time. The
        // authored def still supplies presentation and the modifier op; attributes without a central row roll
        // exactly as authored.
        float CentralMin = 0.0f;
        float CentralMax = 0.0f;
        if (MythicCombat::ResolveCoreAffixBand(Attribute, RollDef.Min, RollDef.Max,
                                               static_cast<float>(ItemLevel), CentralMin, CentralMax)) {
            RollDef.Min = CentralMin;
            RollDef.Max = CentralMax;
            RollDef.LevelScaling = 0.0f;
        }
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

    int RarityValue = Instance->GetItemDefinition()->Rarity;
    const UMythicLootSettings *LootSettings = GetDefault<UMythicLootSettings>();
    int AffixesToRoll = (LootSettings && LootSettings->AffixCountByRarity.IsValidIndex(RarityValue))
                            ? LootSettings->AffixCountByRarity[RarityValue]
                            : (1 + RarityValue);

    const UMythicAffixPoolDataAsset *TieredPool = this->AffixesBuildData.TieredAffixPool;
    if (TieredPool && TieredPool->Defs.Num() > 0) {
        FGameplayTagContainer TypeProbe;
        Instance->GetTypeProbe(TypeProbe);
        RollAffixesTiered(Instance->GetItemLevel(), AffixesToRoll, TypeProbe, TieredPool);
    }
    else {
        RollAffixes(Instance->GetItemLevel(), AffixesToRoll);
    }
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
        UE_LOG(Myth, Error, TEXT("AffixesInstFragment::OnActiveItem: Invalid ASC."));
        return;
    }
    this->AffixesRuntimeReplicatedData.ASC = ASC;

    const UDurabilityFragment *Durability = ItemInstance->GetFragment<UDurabilityFragment>();
    const bool bBroken = Durability && Durability->IsBroken();
    if (ShouldApplyAffixes(bBroken)) {
        ApplyAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledAffixes);
        ApplyAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledCoreAffixes);
    }

    const bool bIsCanonical = ItemInstance->GetFragment<UAffixesFragment>() == this;
    const bool bHasWeapon = ItemInstance->GetFragment<UAttackFragment>() != nullptr;
    if (ShouldEmitArmorEquipEvent(bIsCanonical, bHasWeapon, this->AffixesRuntimeReplicatedData.bEquipEventEmitted)) {
        this->AffixesRuntimeReplicatedData.bEquipEventEmitted = true;
        if (AActor *Avatar = ASC->GetAvatarActor()) {
            if (const APawn *Pawn = Cast<APawn>(Avatar)) {
                if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(Pawn->GetController())) {
                    PC->NotifyItemEquipped(ItemInstance->GetItemDefinition());
                }
            }
        }
    }
}

void UAffixesFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);

    this->AffixesRuntimeReplicatedData.bEquipEventEmitted = false;

    auto ASC = this->AffixesRuntimeReplicatedData.ASC;
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("AffixesInstFragment::DeactivateAffixes: Invalid ASC."));
        return;
    }

    RemoveAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledCoreAffixes);
    RemoveAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledAffixes);

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
        return;
    }

    UAbilitySystemComponent *ASC = this->AffixesRuntimeReplicatedData.ASC;
    const bool bActive = (ASC != nullptr);

    if (bActive) {
        RemoveAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledCoreAffixes);
        RemoveAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledAffixes);
    }

    for (FRolledAffix &Affix : this->AffixesRuntimeReplicatedData.RolledAffixes) {
        if (Affix.bIsLocked || !Affix.Attribute.IsValid()) {
            continue;
        }
        Affix.Value = FMath::RandRange(Affix.Definition.GetScaledMin(ItemLevel), Affix.Definition.GetScaledMax(ItemLevel));
        if (Affix.Definition.bWholeNumber) {
            Affix.Value = FMath::RoundToFloat(Affix.Value);
        }
        if ((Affix.Definition.Modifier == EGameplayModOp::Multiplicitive || Affix.Definition.Modifier == EGameplayModOp::Division)
            && FMath::IsNearlyZero(Affix.Value)) {
            Affix.Value = KINDA_SMALL_NUMBER;
        }
    }

    if (bActive) {
        ApplyAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledAffixes);
        ApplyAffixes(ASC, this->AffixesRuntimeReplicatedData.RolledCoreAffixes);
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
        OutReversed = -Value;
        return true;
    }
    if (Modifier == EGameplayModOp::Multiplicitive || Modifier == EGameplayModOp::Division) {
        if (FMath::IsNearlyZero(Value)) {
            OutReversed = 0.0f;
            return false;
        }
        OutReversed = 1.0f / Value;
        return true;
    }
    OutReversed = 0.0f;
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
