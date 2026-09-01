
#include "ItemTooltipVM.h"

#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Inventory/Fragments/Passive/TalentFragment.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "Itemization/Affixes/MythicPermanentStatLedger.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Stats/MythicStatDefinition.h"
#include "Settings/MythicCombatSettings.h"
#include "UI/ViewModels/MythicStatDisplay.h"
#include "Animation/AnimMontage.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"

namespace {

bool ComposeItemLocalAttackAttribute(
    const FGameplayAttribute &Attribute,
    const UMythicStatDefinition &StatDefinition,
    const UMythicItemizationDataRegistrySubsystem &Registry,
    const TConstArrayView<FAffixDisplayData> DisplayAffixes,
    float &OutValue) {
    if (!Attribute.IsValid() || StatDefinition.Attribute != Attribute
        || !FMath::IsFinite(StatDefinition.NeutralValue)) {
        return false;
    }

    TArray<FMythicPermanentStatContribution> Contributions;
    for (const FAffixDisplayData &DisplayData : DisplayAffixes) {
        const UMythicStatDefinition *EntryStat = Registry.FindStat(DisplayData.StatTag);
        if (!EntryStat || EntryStat->Attribute != Attribute) {
            continue;
        }
        if (DisplayData.ViewData.Values.Num() != 1
            || DisplayData.ViewData.Values[0].StatTag != DisplayData.StatTag
            || !DisplayData.ViewData.RollGuid.IsValid()) {
            return false;
        }

        const FMythicAffixValueViewData &Value = DisplayData.ViewData.Values[0];
        FMythicPermanentStatContribution &Contribution = Contributions.AddDefaulted_GetRef();
        Contribution.SourceGuid = DisplayData.ViewData.RollGuid;
        Contribution.Attribute = Attribute;
        Contribution.ModifierOp = Value.ModifierOp;
        Contribution.Magnitude = Value.RawValue;
        Contribution.Layer = EMythicPermanentStatContributionLayer::Equipment;
    }

    return !Contributions.IsEmpty()
        && FMythicPermanentStatLedger::Compose(
            StatDefinition.NeutralValue, Contributions, OutValue);
}

// The projected DPS reads the same authored bounds the combat path clamps the montage to, so a displayed
// attacks-per-second can never drift from the swing the player gets.
void ResolveAttackSpeedPlayRateBounds(float &OutMinRate, float &OutMaxRate) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    OutMinRate = Settings->MinAttackSpeedPlayRate;
    OutMaxRate = Settings->MaxAttackSpeedPlayRate;
}

FMythicStatNumberPresentation MakeAttacksPerSecondPresentation() {
    FMythicStatNumberPresentation Presentation;
    Presentation.Format = EMythicStatFormat::Flat;
    Presentation.DecimalPlaces = 2;
    return Presentation;
}

} // namespace

void UItemTooltipVM::SetName(FText InName) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Name, InName)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Name);
    }
}

FText UItemTooltipVM::GetName() const { return Name; }

void UItemTooltipVM::SetDescription(FText InDescription) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Description, InDescription)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Description);
    }
}

FText UItemTooltipVM::GetDescription() const { return Description; }

void UItemTooltipVM::SetRarity(EItemRarity InRarity) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Rarity, InRarity)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Rarity);
    }
}

EItemRarity UItemTooltipVM::GetRarity() const { return Rarity; }

void UItemTooltipVM::SetItemLevel(int32 InItemLevel) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ItemLevel, InItemLevel)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ItemLevel);
    }
}

int32 UItemTooltipVM::GetItemLevel() const { return ItemLevel; }

void UItemTooltipVM::SetIcon(UTexture2D *InIcon) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Icon, InIcon)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Icon);
    }
}

UTexture2D *UItemTooltipVM::GetIcon() const { return Icon; }

void UItemTooltipVM::SetItemType(FGameplayTag InItemType) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ItemType, InItemType)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ItemType);
    }
}

FGameplayTag UItemTooltipVM::GetItemType() const { return ItemType; }

void UItemTooltipVM::SetAffixes(TArray<FAffixDisplayData> InAffixes) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Affixes, InAffixes)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Affixes);
    }
}

TArray<FAffixDisplayData> UItemTooltipVM::GetAffixes() const { return Affixes; }

void UItemTooltipVM::SetTalents(TArray<FTalentDisplayData> InTalents) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Talents, InTalents)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Talents);
    }
}

TArray<FTalentDisplayData> UItemTooltipVM::GetTalents() const { return Talents; }

void UItemTooltipVM::SetCurrentDurability(float InCurrentDurability) {
    if (UE_MVVM_SET_PROPERTY_VALUE(CurrentDurability, InCurrentDurability)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CurrentDurability);
    }
}

float UItemTooltipVM::GetCurrentDurability() const { return CurrentDurability; }

void UItemTooltipVM::SetMaxDurability(float InMaxDurability) {
    if (UE_MVVM_SET_PROPERTY_VALUE(MaxDurability, InMaxDurability)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MaxDurability);
    }
}

float UItemTooltipVM::GetMaxDurability() const { return MaxDurability; }

void UItemTooltipVM::SetDurabilityPercent(float InDurabilityPercent) {
    if (UE_MVVM_SET_PROPERTY_VALUE(DurabilityPercent, InDurabilityPercent)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DurabilityPercent);
    }
}

float UItemTooltipVM::GetDurabilityPercent() const { return DurabilityPercent; }

void UItemTooltipVM::SetWeaponAttack(FMythicWeaponAttackViewData InWeaponAttack) {
    if (UE_MVVM_SET_PROPERTY_VALUE(WeaponAttack, InWeaponAttack)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(WeaponAttack);
    }
}

FMythicWeaponAttackViewData UItemTooltipVM::GetWeaponAttack() const { return WeaponAttack; }

void UItemTooltipVM::SetWeight(float InWeight) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Weight, InWeight)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Weight);
    }
}

float UItemTooltipVM::GetWeight() const { return Weight; }

void UItemTooltipVM::SetValue(int32 InValue) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Value, InValue)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Value);
    }
}

int32 UItemTooltipVM::GetValue() const { return Value; }

void UItemTooltipVM::SetStackSize(int32 InStackSize) {
    if (UE_MVVM_SET_PROPERTY_VALUE(StackSize, InStackSize)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StackSize);
    }
}

int32 UItemTooltipVM::GetStackSize() const { return StackSize; }

void UItemTooltipVM::SetStackMax(int32 InStackMax) {
    if (UE_MVVM_SET_PROPERTY_VALUE(StackMax, InStackMax)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StackMax);
    }
}

int32 UItemTooltipVM::GetStackMax() const { return StackMax; }

void UItemTooltipVM::SetRequiredEquipTag(FGameplayTag InRequiredEquipTag) {
    if (UE_MVVM_SET_PROPERTY_VALUE(RequiredEquipTag, InRequiredEquipTag)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(RequiredEquipTag);
    }
}

FGameplayTag UItemTooltipVM::GetRequiredEquipTag() const { return RequiredEquipTag; }

bool UItemTooltipVM::BuildAffixDisplayData(UMythicItemInstance *Item,
                                           const UMythicItemizationDataRegistrySubsystem *Registry,
                                           TArray<FAffixDisplayData> &OutAffixes) {
    OutAffixes.Reset();
    if (!Item || !Registry || !Registry->IsCoreSemanticReady()) {
        return false;
    }

    const UAffixesFragment *AffixFragment = Item->GetFragment<UAffixesFragment>();
    if (!AffixFragment) {
        return true;
    }

    // This is the same canonical context source used by live affix generation and rerolling: definition ItemType
    // plus the instance's current semantic ItemTags. Never reconstruct progression context from asset names.
    FGameplayTagContainer ItemContextTags;
    Item->GetTypeProbe(ItemContextTags);
    const int32 ItemLevel = FMath::Max(1, Item->GetItemLevel());
    const bool bUseWeaponAttackPresentation = ShouldUseWeaponAttackPresentation(Item);

    TArray<FAffixDisplayData> Candidate;
    Candidate.Reserve(AffixFragment->GetAffixSnapshots().Items.Num());
    for (const FMythicReplicatedAffixItem &SnapshotItem : AffixFragment->GetAffixSnapshots().Items) {
        const FRolledAffix &Rolled = SnapshotItem.Affix;
        FAffixDisplayData Entry;
        if (!UMythicAffixViewDataLibrary::BuildViewDataWithItemContext(
                Rolled, ItemContextTags, ItemLevel, Registry, Entry.ViewData)
            || Entry.ViewData.Values.Num() != 1) {
            return false; // Retain the immutable source snapshots and retry after presentation readiness changes.
        }

        const FMythicAffixValueViewData *Primary = &Entry.ViewData.Values[0];
        if (Primary->StatTag != Entry.ViewData.PrimaryStatTag) {
            return false;
        }
        const UMythicStatDefinition *StatDefinition = Registry->FindStat(Primary->StatTag);
        if (!StatDefinition) {
            return false;
        }
        Entry.StatTag = Primary->StatTag;
        Entry.AttributeName = Entry.ViewData.DisplayName;
        Entry.Value = Primary->RawValue;
        Entry.bLowerIsBetter = Primary->ComparisonDirection == EMythicStatComparisonDirection::LowerIsBetter;
        Entry.RichText = Entry.ViewData.RichText;
        const EMythicStatFormat Format = Primary->NumberPresentation.Format;
        Entry.bIsPercentage = Format == EMythicStatFormat::Percent
            || Format == EMythicStatFormat::Multiplier;
        Entry.bOwnedByWeaponAttackPresentation = bUseWeaponAttackPresentation
            && Entry.ViewData.SourceKind == AFFIX_SOURCE_IMPLICIT
            && IsWeaponAttackAttribute(StatDefinition->Attribute);

        Candidate.Add(MoveTemp(Entry));
    }

    OutAffixes = MoveTemp(Candidate);
    return true;
}

bool UItemTooltipVM::ShouldUseWeaponAttackPresentation(UMythicItemInstance *Item) {
    if (!Item || !Item->GetFragment<UAttackFragment>()) {
        return false;
    }

    const UItemDefinition *Definition = Item->GetItemDefinition();
    return Definition && Definition->ItemType.MatchesTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON);
}

bool UItemTooltipVM::IsWeaponAttackAttribute(const FGameplayAttribute &Attribute) {
    return Attribute == UMythicAttributeSet_Offense::GetDamagePerHitAttribute()
        || Attribute == UMythicAttributeSet_Offense::GetAttackSpeedAttribute();
}

bool UItemTooltipVM::CalculateWeaponAttackMetrics(
    const float InDamagePerHit,
    const float InAttackSpeedBonus,
    const float MontageDurationSeconds,
    const float MinAttackSpeedPlayRate,
    const float MaxAttackSpeedPlayRate,
    FMythicWeaponAttackViewData &OutAttackData) {
    OutAttackData = FMythicWeaponAttackViewData();
    if (!FMath::IsFinite(InDamagePerHit) || InDamagePerHit < 0.0f
        || !FMath::IsFinite(InAttackSpeedBonus)
        || !FMath::IsFinite(MontageDurationSeconds)
        || MontageDurationSeconds <= KINDA_SMALL_NUMBER
        || !FMath::IsFinite(MinAttackSpeedPlayRate)
        || !FMath::IsFinite(MaxAttackSpeedPlayRate)) {
        return false;
    }

    const float PlayRate = UMythicGameplayAbility::ComputeAttackSpeedPlayRate(
        InAttackSpeedBonus, MinAttackSpeedPlayRate, MaxAttackSpeedPlayRate);
    if (!FMath::IsFinite(PlayRate) || PlayRate <= 0.0f) {
        return false;
    }

    FMythicWeaponAttackViewData Candidate;
    Candidate.DamageNumberPresentation.Format = EMythicStatFormat::Flat;
    Candidate.DamageNumberPresentation.DecimalPlaces = 0;
    Candidate.AttacksPerSecondNumberPresentation = MakeAttacksPerSecondPresentation();
    if (!MythicCombat::ResolveWeaponDamageRange(
            InDamagePerHit,
            Candidate.MinimumDamagePerHit,
            Candidate.MaximumDamagePerHit,
            Candidate.AverageDamagePerHit)) {
        return false;
    }
    Candidate.AttackSpeedBonus = InAttackSpeedBonus;
    Candidate.BaseAttacksPerSecond = 1.0f / MontageDurationSeconds;
    Candidate.AttacksPerSecond = Candidate.BaseAttacksPerSecond * PlayRate;
    Candidate.AttackTimeSeconds = MontageDurationSeconds / PlayRate;
    Candidate.DamagePerSecond = Candidate.AverageDamagePerHit * Candidate.AttacksPerSecond;
    if (!FMath::IsFinite(Candidate.BaseAttacksPerSecond)
        || !FMath::IsFinite(Candidate.AttacksPerSecond)
        || !FMath::IsFinite(Candidate.AttackTimeSeconds)
        || !FMath::IsFinite(Candidate.DamagePerSecond)) {
        return false;
    }

    OutAttackData = MoveTemp(Candidate);
    return true;
}

bool UItemTooltipVM::BuildWeaponAttackDisplayData(
    UMythicItemInstance *Item,
    const UMythicItemizationDataRegistrySubsystem *Registry,
    const TConstArrayView<FAffixDisplayData> DisplayAffixes,
    FMythicWeaponAttackViewData &OutAttackData) {
    OutAttackData = FMythicWeaponAttackViewData();
    if (!ShouldUseWeaponAttackPresentation(Item)
        || !Registry || !Registry->IsCoreSemanticReady()) {
        return false;
    }

    const UAttackFragment *AttackFragment = Item->GetFragment<UAttackFragment>();
    const UAnimMontage *AttackMontage = AttackFragment
        ? AttackFragment->AttackConfig.AttackMontage
        : nullptr;
    if (!AttackMontage) {
        return false;
    }

    const FGameplayAttribute DamageAttribute =
        UMythicAttributeSet_Offense::GetDamagePerHitAttribute();
    const FGameplayAttribute AttackSpeedAttribute =
        UMythicAttributeSet_Offense::GetAttackSpeedAttribute();
    const UMythicStatDefinition *DamageStat = Registry->FindStat(DamageAttribute);
    const UMythicStatDefinition *AttackSpeedStat = Registry->FindStat(AttackSpeedAttribute);
    if (!DamageStat || !AttackSpeedStat) {
        return false;
    }

    float ComposedDamagePerHit = 0.0f;
    float AttackSpeedBonus = 0.0f;
    if (!ComposeItemLocalAttackAttribute(DamageAttribute, *DamageStat, *Registry,
                                         DisplayAffixes, ComposedDamagePerHit)
        || !ComposeItemLocalAttackAttribute(AttackSpeedAttribute, *AttackSpeedStat, *Registry,
                                            DisplayAffixes, AttackSpeedBonus)) {
        return false;
    }

    float MinPlayRate = 0.0f;
    float MaxPlayRate = 0.0f;
    ResolveAttackSpeedPlayRateBounds(MinPlayRate, MaxPlayRate);

    FMythicWeaponAttackViewData Candidate;
    const float BaseAttackCycleDuration =
        AttackFragment->GetRuntimeNominalAttackCycleDuration();
    if (!CalculateWeaponAttackMetrics(ComposedDamagePerHit, AttackSpeedBonus,
                                      BaseAttackCycleDuration, MinPlayRate, MaxPlayRate,
                                      Candidate)) {
        return false;
    }

    Candidate.DamageNumberPresentation = DamageStat->NumberPresentation;
    Candidate.AttacksPerSecondNumberPresentation = MakeAttacksPerSecondPresentation();

    Candidate.DamagePerHitText = FText::Format(
        NSLOCTEXT("MythicItemAttack", "DamagePerHitRange", "{0} - {1}"),
        MythicStatDisplay::FormatValue(Candidate.MinimumDamagePerHit,
                                       DamageStat->NumberPresentation),
        MythicStatDisplay::FormatValue(Candidate.MaximumDamagePerHit,
                                       DamageStat->NumberPresentation));
    Candidate.AttacksPerSecondText = MythicStatDisplay::FormatValue(
        Candidate.AttacksPerSecond, Candidate.AttacksPerSecondNumberPresentation);
    Candidate.DamagePerSecondText = MythicStatDisplay::FormatValue(
        Candidate.DamagePerSecond, DamageStat->NumberPresentation);
    if (Candidate.DamagePerHitText.IsEmpty() || Candidate.AttacksPerSecondText.IsEmpty()
        || Candidate.DamagePerSecondText.IsEmpty()) {
        return false;
    }

    Candidate.bIsValid = true;
    OutAttackData = MoveTemp(Candidate);
    return true;
}

UItemTooltipVM *UItemTooltipVM::CreateFromItemInstance(UObject *Outer, UMythicItemInstance *Item) {
    if (!Outer || !Item) {
        return nullptr;
    }

    UItemDefinition *Def = Item->GetItemDefinition();
    if (!Def) {
        return nullptr;
    }

    UItemTooltipVM *VM = NewObject<UItemTooltipVM>(Outer);

    VM->SetName(Def->Name);
    VM->SetDescription(Def->Description);
    VM->SetRarity(Def->Rarity);
    VM->SetItemLevel(Item->GetItemLevel());
    VM->SetIcon(Def->Icon2d.IsNull() ? nullptr : Def->Icon2d.LoadSynchronous());
    VM->SetItemType(Def->ItemType);
    VM->SetWeight(Def->Weight);
    VM->SetValue(Def->Value);
    VM->SetStackSize(Item->GetStacks());
    VM->SetStackMax(Def->StackSizeMax);
    VM->SetRequiredEquipTag(Def->RequiredEquipTag);

    const UWorld *World = Item->GetWorld();
    const UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    const UMythicItemizationDataRegistrySubsystem *Registry = GameInstance
        ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>()
        : nullptr;
    TArray<FAffixDisplayData> AffixData;
    if (BuildAffixDisplayData(Item, Registry, AffixData)) {
        VM->SetAffixes(AffixData);

        FMythicWeaponAttackViewData AttackData;
        if (ShouldUseWeaponAttackPresentation(Item)
            && BuildWeaponAttackDisplayData(Item, Registry, AffixData, AttackData)) {
            VM->SetWeaponAttack(MoveTemp(AttackData));
        }
    }

    const UTalentFragment *TalentFrag = Item->GetFragment<UTalentFragment>();
    if (TalentFrag) {
        TArray<FTalentDisplayData> TalentData;
        for (const FTalentSpec &Spec : TalentFrag->TalentRuntimeReplicatedData.RolledTalents) {
            FTalentDisplayData TEntry;
            UTalentDefinition *TalentDef = Spec.TalentDef.IsNull() ? nullptr : Spec.TalentDef.LoadSynchronous();
            if (TalentDef) {
                TEntry.Name = TalentDef->Name;
                TEntry.Icon = TalentDef->Icon.IsNull() ? nullptr : TalentDef->Icon.LoadSynchronous();
                TEntry.Description = Spec.RichText;
            }
            else {
                TEntry.Name = FText::FromString(TEXT("Unknown Talent"));
                TEntry.Icon = nullptr;
                TEntry.Description = Spec.RichText;
            }
            TalentData.Add(TEntry);
        }
        VM->SetTalents(TalentData);
    }

    const UDurabilityFragment *DurFrag = Item->GetFragment<UDurabilityFragment>();
    if (DurFrag) {
        float CurDur = static_cast<float>(DurFrag->GetCurrentDurability());
        float MaxDur = static_cast<float>(DurFrag->GetMaxDurability());
        VM->SetCurrentDurability(CurDur);
        VM->SetMaxDurability(MaxDur);
        VM->SetDurabilityPercent(MaxDur > 0.0f ? CurDur / MaxDur : 0.0f);
    }

    return VM;
}
