#include "ItemDefinition.h"
#include "Fragments/ItemFragment.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Misc/DataValidation.h"

UItemDefinition::UItemDefinition() {
    ItemType = ITEMIZATION_TYPE_MISC;
    Rarity = Common;
    StackSizeMax = 1;
}

FLinearColor UItemDefinition::GetRarityColor(EItemRarity InRarity) {
    switch (InRarity) {
    case Common:
        return FLinearColor::FromSRGBColor(FColor::FromHex("#808080"));
    case Rare:
        return FLinearColor::FromSRGBColor(FColor::FromHex("#15c965"));
    case Epic:
        return FLinearColor::FromSRGBColor(FColor::FromHex("#732BD2FF"));
    case Legendary:
        return FLinearColor::FromSRGBColor(FColor::FromHex("#BE6009FF"));
    case Mythic:
        return FLinearColor::FromSRGBColor(FColor::FromHex("#FF3F36FF"));
    default:
        return FLinearColor::Black;
    }
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
    Modify();
    ItemType = ITEMIZATION_TYPE_EQUIPMENT_WEAPON;
    EnsureFragment<UAttackFragment>();
    EnsureFragment<UAffixesFragment>();
    PostEditChange();
    MarkPackageDirty();
}

void UItemDefinition::Tool() {
    Modify();
    ItemType = ITEMIZATION_TYPE_EQUIPMENT_TOOL;
    EnsureFragment<UAttackFragment>();
    PostEditChange();
    MarkPackageDirty();
}

void UItemDefinition::Gear() {
    Modify();
    ItemType = ITEMIZATION_TYPE_EQUIPMENT_GEAR;
    EnsureFragment<UAffixesFragment>();
    PostEditChange();
    MarkPackageDirty();
}

void UItemDefinition::Accessory() {
    Modify();
    ItemType = ITEMIZATION_TYPE_EQUIPMENT_ACCESSORY;
    EnsureFragment<UAffixesFragment>();
    PostEditChange();
    MarkPackageDirty();
}

void UItemDefinition::Artifact() {
    Modify();
    ItemType = ITEMIZATION_TYPE_EQUIPMENT_ARTIFACT;
    EnsureFragment<UAffixesFragment>();
    PostEditChange();
    MarkPackageDirty();
}

void UItemDefinition::Consumable() {
    Modify();
    ItemType = ITEMIZATION_TYPE_CONSUMABLE;
    EnsureFragment<UConsumableActionFragment>();
    PostEditChange();
    MarkPackageDirty();
}

void UItemDefinition::Learning() {
    Modify();
    ItemType = ITEMIZATION_TYPE_LEARNING;
    EnsureFragment<UConsumableActionFragment>();
    PostEditChange();
    MarkPackageDirty();
}

void UItemDefinition::Farming() {
    Modify();
    ItemType = ITEMIZATION_TYPE_FARMING;
    PostEditChange();
    MarkPackageDirty();
}

void UItemDefinition::Mining() {
    Modify();
    ItemType = ITEMIZATION_TYPE_MINING;
    PostEditChange();
    MarkPackageDirty();
}

void UItemDefinition::Placable() {
    Modify();
    ItemType = ITEMIZATION_TYPE_PLACABLE;
    PostEditChange();
    MarkPackageDirty();
}

void UItemDefinition::Exploration() {
    Modify();
    ItemType = ITEMIZATION_TYPE_EXPLORATION;
    PostEditChange();
    MarkPackageDirty();
}

void UItemDefinition::Misc() {
    Modify();
    ItemType = ITEMIZATION_TYPE_MISC;
    PostEditChange();
    MarkPackageDirty();
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

    int32 AttackFragmentCount = 0;
    int32 AffixesFragmentCount = 0;

    for (int32 Index = 0; Index < Fragments.Num(); ++Index) {
        if (!Fragments[Index]) {
            Context.AddError(FText::Format(
                NSLOCTEXT("ItemDefinition", "NullFragmentError", "Null fragment at index {0}"),
                FText::AsNumber(Index)));
            Result = EDataValidationResult::Invalid;
            continue;
        }

        FText FragmentError;
        if (!Fragments[Index]->IsValidFragment(FragmentError)) {
            Context.AddError(FText::Format(
                NSLOCTEXT(
                    "ItemDefinition", "InvalidFragmentError",
                    "Fragment {0} ({1}) at '{2}' is invalid: {3}"),
                FText::AsNumber(Index),
                FText::FromString(Fragments[Index]->GetClass()->GetName()),
                FText::FromString(Fragments[Index]->GetPathName()),
                FragmentError.IsEmpty()
                    ? NSLOCTEXT("ItemDefinition", "MissingFragmentDiagnostic", "No diagnostic was provided")
                    : FragmentError));
            Result = EDataValidationResult::Invalid;
        }
        AttackFragmentCount += Cast<UAttackFragment>(Fragments[Index]) ? 1 : 0;
        AffixesFragmentCount += Cast<UAffixesFragment>(Fragments[Index]) ? 1 : 0;
    }

    if (AttackFragmentCount > 1 || AffixesFragmentCount > 1) {
        Context.AddError(NSLOCTEXT(
            "ItemDefinition", "DuplicateAuthorityFragmentError",
            "An Item Definition may contain at most one Attack Fragment and one Affixes Fragment."));
        Result = EDataValidationResult::Invalid;
    }
    if (ItemType.MatchesTag(ITEMIZATION_TYPE_EQUIPMENT_WEAPON)
        && (AttackFragmentCount != 1 || AffixesFragmentCount != 1)) {
        Context.AddError(NSLOCTEXT(
            "ItemDefinition", "InvalidWeaponFragmentShapeError",
            "Every weapon requires exactly one action-only Attack Fragment and one typed Affixes Fragment."));
        Result = EDataValidationResult::Invalid;
    }

    if (ItemType.IsValid()) {
        const FGameplayTag ParentTag = ITEMIZATION_TYPE;
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
