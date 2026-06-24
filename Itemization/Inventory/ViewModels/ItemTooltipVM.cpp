// 

#include "ItemTooltipVM.h"

#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Inventory/Fragments/Passive/TalentFragment.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"

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

void UItemTooltipVM::SetDamageRange(FText InDamageRange) {
    if (UE_MVVM_SET_PROPERTY_VALUE(DamageRange, InDamageRange)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DamageRange);
    }
}

FText UItemTooltipVM::GetDamageRange() const { return DamageRange; }

void UItemTooltipVM::SetAttackSpeed(float InAttackSpeed) {
    if (UE_MVVM_SET_PROPERTY_VALUE(AttackSpeed, InAttackSpeed)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AttackSpeed);
    }
}

float UItemTooltipVM::GetAttackSpeed() const { return AttackSpeed; }

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

// helper to collect affix display data from rolled affix arrays
static void CollectAffixDisplayData(const TArray<FRolledAffix> &RolledAffixes, TArray<FAffixDisplayData> &OutAffixes) {
    for (const FRolledAffix &Rolled : RolledAffixes) {
        FAffixDisplayData Entry;
        Entry.AttributeName = Rolled.Attribute.IsValid()
            ? FText::FromString(Rolled.Attribute.GetName())
            : FText::FromString(TEXT("Unknown"));
        Entry.Value = Rolled.Value;
        Entry.bIsPercentage = Rolled.Definition.bIsPercentage;
        Entry.bLowerIsBetter = Rolled.Definition.bLowerIsBetter;
        OutAffixes.Add(Entry);
    }
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

    // identity
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

    // affixes from AffixesFragment
    const UAffixesFragment *AffixFrag = Item->GetFragment<UAffixesFragment>();
    if (AffixFrag) {
        TArray<FAffixDisplayData> AffixData;
        CollectAffixDisplayData(AffixFrag->AffixesRuntimeReplicatedData.RolledCoreAffixes, AffixData);
        CollectAffixDisplayData(AffixFrag->AffixesRuntimeReplicatedData.RolledAffixes, AffixData);
        VM->SetAffixes(AffixData);
    }

    // talents from TalentFragment
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

    // durability from DurabilityFragment
    const UDurabilityFragment *DurFrag = Item->GetFragment<UDurabilityFragment>();
    if (DurFrag) {
        float CurDur = static_cast<float>(DurFrag->GetCurrentDurability());
        float MaxDur = static_cast<float>(DurFrag->GetMaxDurability());
        VM->SetCurrentDurability(CurDur);
        VM->SetMaxDurability(MaxDur);
        VM->SetDurabilityPercent(MaxDur > 0.0f ? CurDur / MaxDur : 0.0f);
    }

    // attack from AttackFragment
    const UAttackFragment *AtkFrag = Item->GetFragment<UAttackFragment>();
    if (AtkFrag) {
        const FRolledAttributeSpec &DmgSpec = AtkFrag->AttackRuntimeReplicatedData.RolledDamageSpec;
        float MinDmg = DmgSpec.Definition.GetScaledMin(Item->GetItemLevel());
        float MaxDmg = DmgSpec.Definition.GetScaledMax(Item->GetItemLevel());
        VM->SetDamageRange(FText::FromString(FString::Printf(TEXT("%d-%d"),
            FMath::RoundToInt32(MinDmg), FMath::RoundToInt32(MaxDmg))));

        // attack speed from montage play length if available
        UAnimMontage *Montage = AtkFrag->AttackConfig.AttackMontage;
        if (Montage) {
            float Duration = Montage->GetPlayLength();
            VM->SetAttackSpeed(Duration > 0.0f ? 1.0f / Duration : 0.0f);
        }
    }

    return VM;
}
