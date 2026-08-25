#include "AffixesFragment.h"
#include "AbilitySystemComponent.h"
#include "Settings/MythicCombatSettings.h"
#include "Mythic/Itemization/Inventory/MythicItemInstance.h"
#include "AbilitySystemGlobals.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Itemization/Affixes/MythicAffixCatalogue.h"
#include "Settings/MythicDeveloperSettings.h"
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

            // Core rolls first and owns its attributes: rolling one again here would apply the stat twice, print
            // it twice on the tooltip, and let a Refine reroll only half of it.
            if (IsAffixRolled(AffixKey, this->AffixesRuntimeReplicatedData.RolledAffixes)
                || IsAffixRolled(AffixKey, this->AffixesRuntimeReplicatedData.RolledCoreAffixes)) {
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

    for (auto &CoreStat : CoreAffixes) {
        if (!CoreStat.Key.IsValid()) {
            UE_LOG(Myth, Error, TEXT("AffixesInstFragment::OnInstanced: Invalid core stat attribute."));
            continue;
        }

        // Both arrays: this roller is public and ungated, so it can run after the random half. An attribute on
        // either list must not land twice, or the stat applies twice and a Refine reroll touches only one copy.
        if (IsAffixRolled(CoreStat.Key, this->AffixesRuntimeReplicatedData.RolledCoreAffixes)
            || IsAffixRolled(CoreStat.Key, this->AffixesRuntimeReplicatedData.RolledAffixes)) {
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
        FRolledAffix NewAffix(Attribute, ItemLevel, RollDef, true);
        // A zero multiply wipes the attribute to 0, and ComputeReversedModValue refuses to reverse it, so unequipping
        // never gives it back. A whole-number roll on a sub-1 band rounds to exactly zero about half the time.
        if ((RollDef.Modifier == EGameplayModOp::Multiplicitive || RollDef.Modifier == EGameplayModOp::Division)
            && FMath::IsNearlyZero(NewAffix.Value)) {
            NewAffix.Value = KINDA_SMALL_NUMBER;
        }
        this->AffixesRuntimeReplicatedData.RolledCoreAffixes.Add(NewAffix);
    }
}

namespace {
int32 LowestTierIndex(TConstArrayView<FMythicAffixTier> Tiers) {
    int32 Lowest = INDEX_NONE;
    for (int32 i = 0; i < Tiers.Num(); ++i) {
        if (Lowest == INDEX_NONE || Tiers[i].MinItemLevel < Tiers[Lowest].MinItemLevel) {
            Lowest = i;
        }
    }
    return Lowest;
}
}

void UAffixesFragment::RollCoreAffixesTiered(int ItemLevel, const FGameplayTagContainer &TypeProbe,
                                             TConstArrayView<FMythicTieredAffixDef> Defs) {
    for (const FMythicTieredAffixDef &Def : Defs) {
        if (!Def.Attribute.IsValid()) {
            UE_LOG(Myth, Error, TEXT("AffixesInstFragment::RollCoreAffixesTiered: Invalid core stat attribute."));
            continue;
        }
        // Both arrays: this roller is public and ungated, so it can run after the random half. An attribute on
        // either list must not land twice, or the stat applies twice and a Refine reroll touches only one copy.
        if (IsAffixRolled(Def.Attribute, this->AffixesRuntimeReplicatedData.RolledCoreAffixes)
            || IsAffixRolled(Def.Attribute, this->AffixesRuntimeReplicatedData.RolledAffixes)) {
            continue;
        }

        int32 TierIdx = FMythicAffixTierMath::SelectTierIndex(ItemLevel, Def.Tiers, FMath::FRand());
        if (!Def.Tiers.IsValidIndex(TierIdx)) {
            // A core affix is guaranteed, so an item level below every rung still rolls the lowest one rather
            // than dropping the stat.
            TierIdx = LowestTierIndex(Def.Tiers);
        }
        if (!Def.Tiers.IsValidIndex(TierIdx)) {
            UE_LOG(Myth, Error, TEXT("AffixesInstFragment::RollCoreAffixesTiered: core affix %s has an empty tier ladder."),
                   *Def.Attribute.GetName());
            continue;
        }
        const FMythicAffixTier &Tier = Def.Tiers[TierIdx];

        FRollDefinition RollDef;
        RollDef.Min = Tier.Min;
        RollDef.Max = Tier.Max;
        RollDef.Modifier = Def.ModOp;
        RollDef.LevelScaling = Tier.LevelScaling;
        RollDef.bWholeNumber = Def.bWholeNumber;

        // Centrally scaled families derive their band from combat settings: the shared level curve consumes the
        // item level, so the private linear LevelScaling is zeroed to keep it from applying a second time. The
        // authored tier still supplies presentation and the modifier op; attributes without a central row roll
        // exactly as authored.
        FMythicAffixTier RollTier = Tier;
        float CentralMin = 0.0f;
        float CentralMax = 0.0f;
        if (MythicCombat::ResolveCoreAffixBand(Def.Attribute, Tier.Min, Tier.Max,
                                               static_cast<float>(ItemLevel), CentralMin, CentralMax)) {
            RollDef.Min = CentralMin;
            RollDef.Max = CentralMax;
            RollDef.LevelScaling = 0.0f;
            RollTier.Min = CentralMin;
            RollTier.Max = CentralMax;
            RollTier.LevelScaling = 0.0f;
        }

        FRolledAffix NewAffix(Def.Attribute, ItemLevel, RollDef, true);
        // The tier roll overwrites the ctor's value, so the whole-number snap must re-apply here.
        NewAffix.Value = FMythicAffixTierMath::RollValueInTier(RollTier, ItemLevel, FMath::FRand());
        if (RollDef.bWholeNumber) {
            NewAffix.Value = FMath::RoundToFloat(NewAffix.Value);
        }
        if ((RollDef.Modifier == EGameplayModOp::Multiplicitive || RollDef.Modifier == EGameplayModOp::Division)
            && FMath::IsNearlyZero(NewAffix.Value)) {
            NewAffix.Value = KINDA_SMALL_NUMBER;
        }
        NewAffix.TierIndex = TierIdx;
        NewAffix.TierLabel = Tier.TierLabel;

        this->AffixesRuntimeReplicatedData.RolledCoreAffixes.Add(NewAffix);
    }
}

#if WITH_EDITOR
bool UAffixesFragment::IsValidFragment(FText &OutErrorMessage) const {
    const TMap<FGameplayAttribute, FRollDefinition> &AffixPoolMap = this->AffixesBuildData.AffixPoolMap;
    const TMap<FGameplayAttribute, FRollDefinition> &CoreAffixes = this->AffixesBuildData.CoreAffixes;

    // Authoring nothing is a valid shape ONLY because the shared catalogue fills both halves at OnInstanced.
    // With no catalogue configured nothing fills them and the item ships with zero core stats and zero affixes.
    const bool bAuthorsNothing = AffixPoolMap.Num() == 0 && CoreAffixes.Num() == 0
        && this->AffixesBuildData.AffixCatalogueOverride == nullptr;
    if (bAuthorsNothing) {
        const UMythicLootSettings *LootSettings = GetDefault<UMythicLootSettings>();
        if (!LootSettings || LootSettings->AffixCatalogue.IsNull()) {
            OutErrorMessage = FText::FromString(
                "This fragment authors no core stats and no affix pool, and no affix catalogue is set in Mythic Loot "
                "Settings to fill them, so the item would roll nothing.");
            return false;
        }
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

    // A catalogue's entries and rules are checked by UMythicAffixCatalogue::IsDataValid on the catalogue asset,
    // so the fragment validates only what it authors itself.
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

    const int ItemLevel = Instance->GetItemLevel();
    FGameplayTagContainer TypeProbe;
    Instance->GetTypeProbe(TypeProbe);

    const UMythicAffixCatalogue *Override = this->AffixesBuildData.AffixCatalogueOverride;
    const bool bHasOwnFlatPool = this->AffixesBuildData.AffixPoolMap.Num() > 0;
    const bool bHasOwnCore = this->AffixesBuildData.CoreAffixes.Num() > 0;

    // ONE authoring surface, resolved once. The item's own catalogue when it carries one, the shared catalogue
    // otherwise, and the SAME asset answers both halves - an override covering only the random half would leave a
    // bespoke item still taking the shared type baseline it was written to replace.
    const UMythicAffixCatalogue *Catalogue = Override
                                                 ? Override
                                                 : (LootSettings ? LootSettings->GetAffixCatalogue() : nullptr);
    const FGameplayTag ItemType = Instance->GetItemDefinition()->ItemType;

    // Core rolls FIRST so the random half can de-dupe against it: an attribute must never land on one item twice.
    //
    // The two core sources ADD rather than compete. An item's own map is its signature - the family damage bonus on
    // a sword, the slot-appropriate Armor band on a helm - while the type rule is the baseline every item of that
    // type is guaranteed, like damage per hit and attack speed on anything that swings. The item's map rolls first,
    // so where both name an attribute the authored band is the one that survives.
    if (bHasOwnCore) {
        RollCoreAffixes(ItemLevel);
    }
    TArray<FMythicTieredAffixDef> CoreDefs;
    if (Catalogue) {
        Catalogue->BuildCoreDefs(ItemType, TypeProbe, CoreDefs);
    }
    RollCoreAffixesTiered(ItemLevel, TypeProbe, CoreDefs);
    if (this->AffixesRuntimeReplicatedData.RolledCoreAffixes.Num() == 0) {
        UE_LOG(Myth, Warning, TEXT("AffixesInstFragment::OnInstanced: No core stats to roll for item type %s."),
               *ItemType.ToString());
    }

    // The legacy flat map beats the SHARED catalogue, or configuring one would silently kill every flat pool a
    // designer wrote by hand. An override is the more specific statement, so it beats the flat map in turn.
    if (bHasOwnFlatPool && !Override) {
        RollAffixes(ItemLevel, AffixesToRoll);
    }
    else {
        TArray<FMythicTieredAffixDef> RandomDefs;
        if (Catalogue) {
            Catalogue->BuildRandomDefs(ItemType, TypeProbe, RandomDefs);
        }
        RollAffixesTiered(ItemLevel, AffixesToRoll, TypeProbe, RandomDefs);
    }

    // Count what LANDED, not what was offered. A pool can be non-empty and still roll nothing when every ladder is
    // gated above the item level or the core half already took the attributes.
    const int32 RolledRandom = this->AffixesRuntimeReplicatedData.RolledAffixes.Num();
    if (RolledRandom < AffixesToRoll) {
        UE_LOG(Myth, Warning,
               TEXT("AffixesInstFragment::OnInstanced: %s rolled %d of %d random affixes at item level %d."),
               *ItemType.ToString(), RolledRandom, AffixesToRoll, ItemLevel);
    }
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

bool UAffixesFragment::CanApplyCraftOp(FText &OutReason) const {
    if (this->AffixesRuntimeReplicatedData.bCorrupted) {
        OutReason = NSLOCTEXT("Mythic", "CraftRefusedCorrupted", "This item is corrupted and can no longer be crafted.");
        return false;
    }
    OutReason = FText::GetEmpty();
    return true;
}

void UAffixesFragment::ServerCorruptItem() {
    const AActor *Owner = GetOwningActor();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (Settings && !Settings->bItemCorruptionEnabled) {
        UE_LOG(Myth, Warning, TEXT("Affixes: corruption refused - item corruption is switched off in settings."));
        return;
    }
    // Deliberately one-way. Corruption is the price of the gamble, so nothing here un-sets it.
    this->AffixesRuntimeReplicatedData.bCorrupted = true;
}

void UAffixesFragment::RerollUnlockedAffixes(int32 ItemLevel) {
    const AActor *Owner = GetOwningActor();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    FText Refusal;
    if (!CanApplyCraftOp(Refusal)) {
        UE_LOG(Myth, Warning, TEXT("Affixes: reroll refused - %s"), *Refusal.ToString());
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
    FText Refusal;
    if (!CanApplyCraftOp(Refusal)) {
        UE_LOG(Myth, Warning, TEXT("Affixes: lock refused - %s"), *Refusal.ToString());
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
