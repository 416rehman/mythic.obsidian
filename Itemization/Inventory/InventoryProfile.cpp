#include "Itemization/Inventory/InventoryProfile.h"

#include "Misc/DataValidation.h"

#define LOCTEXT_NAMESPACE "InventoryProfile"

const FName UInventoryProfile::EquipmentGroupTagRoot(TEXT("Inventory.Group.Equipment"));

#if WITH_EDITOR
EDataValidationResult UInventoryProfile::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);

    const FString EquipmentPrefix = EquipmentGroupTagRoot.ToString() + TEXT(".");
    for (const TPair<FGameplayTag, FInventorySlotGroup> &Pair : SlotGroups) {
        if (!Pair.Key.IsValid()) {
            Context.AddError(LOCTEXT("InvalidGroupTag", "Every slot group must be keyed by a registered Inventory.Group tag."));
            Result = EDataValidationResult::Invalid;
            continue;
        }
        // Carried is the struct default, so a schema change that drops an authored domain silently demotes gear to
        // plain storage: the group keeps its slots but IsGearSlot() turns false and the loadout panel renders nothing.
        if (Pair.Key.ToString().StartsWith(EquipmentPrefix)
            && Pair.Value.SlotDomain == EMythicInventorySlotDomain::Carried) {
            Context.AddError(FText::Format(
                LOCTEXT("EquipmentGroupIsCarried",
                        "Slot group '{0}' is under {1} but declares the Carried domain, so its slots are not gear. Author the Equipment domain for armour, accessories, the weapon and the tool slots."),
                FText::FromName(Pair.Key.GetTagName()), FText::FromName(EquipmentGroupTagRoot)));
            Result = EDataValidationResult::Invalid;
        }
        if (Pair.Value.Slots.IsEmpty()) {
            Context.AddError(FText::Format(
                LOCTEXT("EmptySlotGroup", "Slot group '{0}' instantiates no slots."),
                FText::FromName(Pair.Key.GetTagName())));
            Result = EDataValidationResult::Invalid;
        }
        for (const FInventoryProfileEntry &Entry : Pair.Value.Slots) {
            if (!Entry.SlotDefinition || Entry.Count < 1) {
                Context.AddError(FText::Format(
                    LOCTEXT("InvalidSlotEntry", "Slot group '{0}' has an entry with no Slot Definition or a Count below one."),
                    FText::FromName(Pair.Key.GetTagName())));
                Result = EDataValidationResult::Invalid;
            }
        }
    }

    // Super returns NotValidated when it has no opinion; this asset always inspects every group, so say so.
    return Result == EDataValidationResult::NotValidated ? EDataValidationResult::Valid : Result;
}
#endif

#undef LOCTEXT_NAMESPACE
