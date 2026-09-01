// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicInventoryInteractionPolicy.h"

namespace {

void AddAction(
    TArray<FMythicInventoryContextAction> &Actions,
    const EMythicInventoryContextVerb Verb,
    const FText &Label,
    const bool bEnabled,
    const EMythicInventoryActionResult DisabledReason,
    const int32 MaximumQuantity = 0,
    const bool bRequiresHold = false) {
    if (Actions.Num() >= 5) {
        return;
    }
    FMythicInventoryContextAction &Action = Actions.AddDefaulted_GetRef();
    Action.Verb = Verb;
    Action.Label = Label;
    Action.bVisible = true;
    Action.bEnabled = bEnabled;
    Action.DisabledReason = bEnabled
        ? EMythicInventoryActionResult::Succeeded : DisabledReason;
    Action.MaximumQuantity = MaximumQuantity;
    Action.bRequiresHold = bRequiresHold;
}

} // namespace

TArray<FMythicInventoryContextAction>
FMythicInventoryInteractionPolicy::BuildContextActions(
    const FMythicInventoryInteractionPolicyInput &Input) {
    TArray<FMythicInventoryContextAction> Actions;
    Actions.Reserve(5);

    const bool bMutationsAllowed = !Input.bMutationPending;
    if (Input.bEquippable || Input.bUsable || Input.bIsEquipped) {
        const EMythicInventoryContextVerb PrimaryVerb = Input.bIsEquipped
            ? EMythicInventoryContextVerb::Unequip
            : Input.bEquippable
                ? EMythicInventoryContextVerb::Equip
                : EMythicInventoryContextVerb::Use;
        const FText PrimaryLabel = Input.bIsEquipped
            ? NSLOCTEXT("MythicInventory", "Unequip", "Unequip")
            : Input.bEquippable
                ? NSLOCTEXT("MythicInventory", "Equip", "Equip")
                : NSLOCTEXT("MythicInventory", "Use", "Use");
        AddAction(
            Actions,
            PrimaryVerb,
            PrimaryLabel,
            bMutationsAllowed && Input.bPrimaryEnabled,
            Input.bMutationPending
                ? EMythicInventoryActionResult::InvalidRequest
                : Input.PrimaryDisabledReason);
    }

    if (!Input.bIsEquipped && Input.Quantity > 1) {
        AddAction(
            Actions,
            EMythicInventoryContextVerb::Split,
            NSLOCTEXT("MythicInventory", "Split", "Split"),
            bMutationsAllowed,
            EMythicInventoryActionResult::InvalidRequest,
            Input.Quantity - 1);
    }

    if (Input.bMoveRelevant) {
        AddAction(
            Actions,
            EMythicInventoryContextVerb::Move,
            NSLOCTEXT("MythicInventory", "Move", "Move"),
            bMutationsAllowed && Input.bMoveEnabled,
            Input.bMutationPending
                ? EMythicInventoryActionResult::InvalidRequest
                : Input.MoveDisabledReason);
    }

    if (!Input.bIsEquipped && Input.bCanToggleManualJunk) {
        AddAction(
            Actions,
            EMythicInventoryContextVerb::ToggleManualJunk,
            Input.bManualJunk
                ? NSLOCTEXT("MythicInventory", "UnmarkJunk", "Unmark Junk")
                : NSLOCTEXT("MythicInventory", "MarkJunk", "Mark as Junk"),
            bMutationsAllowed,
            EMythicInventoryActionResult::InvalidRequest);
    }

    if (!Input.bIsEquipped && Input.bCanDrop) {
        const bool bRareOrHigher =
            static_cast<int32>(Input.Rarity) >= static_cast<int32>(EItemRarity::Rare);
        AddAction(
            Actions,
            EMythicInventoryContextVerb::Drop,
            NSLOCTEXT("MythicInventory", "Drop", "Drop"),
            bMutationsAllowed,
            EMythicInventoryActionResult::InvalidRequest,
            FMath::Max(1, Input.Quantity),
            bRareOrHigher && !Input.bManualJunk);
    }

    return Actions;
}
