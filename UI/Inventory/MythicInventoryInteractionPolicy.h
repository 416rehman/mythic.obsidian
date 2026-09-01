// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryActionTypes.h"
#include "MythicInventoryInteractionPolicy.generated.h"

/** Contextual verbs available from one selected physical inventory item. */
UENUM(BlueprintType)
enum class EMythicInventoryContextVerb : uint8 {
    Equip,
    Unequip,
    Use,
    Split,
    Move,
    ToggleManualJunk,
    Drop
};

/** One shallow, presentation-only inventory action row; authority always revalidates the eventual request. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicInventoryContextAction {
    GENERATED_BODY()

    /** Semantic command represented by this row. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Interaction")
    EMythicInventoryContextVerb Verb = EMythicInventoryContextVerb::Move;

    /** Localized, player-facing action label. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Interaction")
    FText Label;

    /** False only for actions that are irrelevant to the selected item and therefore omitted. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Interaction")
    bool bVisible = false;

    /** True when the current replicated snapshot permits submission and no local mutation is pending. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Interaction")
    bool bEnabled = false;

    /** Semantic reason shown when a relevant action is temporarily blocked. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Interaction")
    EMythicInventoryActionResult DisabledReason =
        EMythicInventoryActionResult::Succeeded;

    /** True when the command must use the existing remap-aware held-confirm presentation. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Interaction")
    bool bRequiresHold = false;

    /** Maximum selectable quantity; zero means this command has no quantity step. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Interaction")
    int32 MaximumQuantity = 0;
};

/** C++ snapshot consumed by the pure advisory action policy. */
struct MYTHIC_API FMythicInventoryInteractionPolicyInput {
    bool bIsEquipped = false;
    bool bEquippable = false;
    bool bUsable = false;
    bool bPrimaryEnabled = false;
    bool bMoveRelevant = true;
    bool bMoveEnabled = false;
    bool bManualJunk = false;
    bool bCanToggleManualJunk = false;
    bool bCanDrop = false;
    bool bMutationPending = false;
    int32 Quantity = 0;
    EItemRarity Rarity = EItemRarity::Common;
    EMythicInventoryActionResult PrimaryDisabledReason =
        EMythicInventoryActionResult::InvalidRequest;
    EMythicInventoryActionResult MoveDisabledReason =
        EMythicInventoryActionResult::IncompatibleTarget;
};

/** Pure deterministic projection used by mouse, keyboard, and controller Context presentation. */
struct MYTHIC_API FMythicInventoryInteractionPolicy {
    /** Builds at most five rows in fixed Primary, Split, Move, Manual Junk, Drop order. */
    static TArray<FMythicInventoryContextAction> BuildContextActions(
        const FMythicInventoryInteractionPolicyInput &Input);
};
