// ItemDefinition.cpp
#include "ItemDefinition.h"
#include "Fragments/ItemFragment.h"
#include "Misc/DataValidation.h"

UItemDefinition::UItemDefinition() {
    ItemType = ITEMIZATION_TYPE_MISC;
    Rarity = Common;
    StackSizeMax = 1;
}

#if WITH_EDITOR
#include "UObject/ObjectSaveContext.h"
#include "Fragments/Actionable/AttackFragment.h"
#include "Fragments/Actionable/ConsumableActionFragment.h"
#include "Fragments/Passive/AffixesFragment.h"

template <typename T>
void UItemDefinition::EnsureFragment() {
    for (const auto &Frag : Fragments) {
        if (Frag && Frag->IsA<T>()) {
            return;
        }
    }
    T *NewFragment = NewObject<T>(this, T::StaticClass(), NAME_None, RF_Transactional);
    Fragments.Add(NewFragment);
}

void UItemDefinition::Weapon() {
    ItemType = ITEMIZATION_TYPE_EQUIPMENT_WEAPON;
    EnsureFragment<UAttackFragment>();
    Modify();
}

void UItemDefinition::Tool() {
    ItemType = ITEMIZATION_TYPE_EQUIPMENT_TOOL;
    EnsureFragment<UAttackFragment>();
    Modify();
}

void UItemDefinition::Gear() {
    ItemType = ITEMIZATION_TYPE_EQUIPMENT_GEAR;
    EnsureFragment<UAffixesFragment>();
    Modify();
}

void UItemDefinition::Accessory() {
    ItemType = ITEMIZATION_TYPE_EQUIPMENT_ACCESSORY;
    EnsureFragment<UAffixesFragment>();
    Modify();
}

void UItemDefinition::Artifact() {
    ItemType = ITEMIZATION_TYPE_EQUIPMENT_ARTIFACT;
    EnsureFragment<UAffixesFragment>();
    Modify();
}

void UItemDefinition::Consumable() {
    ItemType = ITEMIZATION_TYPE_CONSUMABLE;
    EnsureFragment<UConsumableActionFragment>();
    Modify();
}

void UItemDefinition::Learning() {
    ItemType = ITEMIZATION_TYPE_LEARNING;
    EnsureFragment<UConsumableActionFragment>();
    Modify();
}

void UItemDefinition::Farming() {
    ItemType = ITEMIZATION_TYPE_FARMING;
    Modify();
}

void UItemDefinition::Mining() {
    ItemType = ITEMIZATION_TYPE_MINING;
    Modify();
}

void UItemDefinition::Placable() {
    ItemType = ITEMIZATION_TYPE_PLACABLE;
    Modify();
}

void UItemDefinition::Exploration() {
    ItemType = ITEMIZATION_TYPE_EXPLORATION;
    Modify();
}

void UItemDefinition::Misc() {
    ItemType = ITEMIZATION_TYPE_MISC;
    Modify();
}

void UItemDefinition::PostLoad() {
    Super::PostLoad();
    Fragments.RemoveAll([](const TObjectPtr<UItemFragment> &Fragment) {
        return Fragment == nullptr;
    });
}

void UItemDefinition::PreSave(FObjectPreSaveContext SaveContext) {
    Super::PreSave(SaveContext);
    Fragments.RemoveAll([](const TObjectPtr<UItemFragment> &Fragment) {
        return Fragment == nullptr;
    });
}

EDataValidationResult UItemDefinition::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);

    for (int32 Index = 0; Index < Fragments.Num(); ++Index) {
        if (!Fragments[Index]) {
            Context.AddError(FText::Format(
                NSLOCTEXT("ItemDefinition", "NullFragmentError", "Null fragment at index {0}"),
                FText::AsNumber(Index)));
            Result = EDataValidationResult::Invalid;
        }
    }

    if (ItemType.IsValid()) {
        FGameplayTag ParentTag = FGameplayTag::RequestGameplayTag(FName("Itemization.Type"));
        if (!ItemType.MatchesTag(ParentTag)) {
            Context.AddError(FText::Format(
                NSLOCTEXT("ItemDefinition", "InvalidItemTypeError", "ItemType '{0}' is not a child of 'Itemization.Type'"),
                FText::FromString(ItemType.ToString())));
            Result = EDataValidationResult::Invalid;
        }
    }
    else {
        Context.AddError(NSLOCTEXT("ItemDefinition", "MissingItemTypeError", "ItemType is not set"));
        Result = EDataValidationResult::Invalid;
    }

    return Result;
}
#endif
