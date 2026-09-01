// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MythicInventoryActionTypes.generated.h"

class UMythicInventoryComponent;

/** Closed set of authoritative player-inventory operations exposed through the production request seam. */
UENUM(BlueprintType)
enum class EMythicInventoryAction : uint8 {
    Move,
    Split,
    DropQuantity,
    Use,
    SetJunk,
    Sort,
};

/** Stable, disclosure-safe outcome returned for an authoritative inventory request. */
UENUM(BlueprintType)
enum class EMythicInventoryActionResult : uint8 {
    Succeeded,
    InvalidRequest,
    UnauthorizedInventory,
    InvalidSlot,
    StaleSource,
    StaleTarget,
    SourceProtected,
    TargetProtected,
    IncompatibleTarget,
    InvalidQuantity,
    InventoryFull,
    NotUsable,
    InvalidGroup,
    SpawnFailed,
    CommitFailed,
};

/** Whether a receipt describes first-time execution, rejection, or replay of a cached request. */
UENUM(BlueprintType)
enum class EMythicInventoryReceiptDisposition : uint8 {
    Committed,
    Rejected,
    Replayed,
};

/**
 * Identity-checked address of the item a player saw when submitting an action.
 * Slot index alone is never authority: the server also requires the same physical item GUID and quantity.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicInventorySourceLocator {
    GENERATED_BODY()

    /** Player-owned inventory expected to contain the source item. */
    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Inventory|Action")
    TObjectPtr<UMythicInventoryComponent> Inventory = nullptr;

    /** Absolute slot index observed by the requesting client. */
    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Inventory|Action")
    int32 SlotIndex = INDEX_NONE;

    /** Stable physical item identity observed in Slot Index. */
    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Inventory|Action")
    FGuid ExpectedItemGuid;

    /** Exact stack quantity observed when the request was submitted. */
    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Inventory|Action")
    int32 ExpectedQuantity = 0;

    bool IsStructurallyValid() const {
        return Inventory != nullptr && SlotIndex >= 0 && ExpectedItemGuid.IsValid()
            && ExpectedQuantity > 0;
    }
};

/**
 * Identity-checked destination for a player move. Empty and occupied expectations are explicit so a delayed request
 * cannot replace an item that arrived after the client rendered the slot.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicInventoryTargetLocator {
    GENERATED_BODY()

    /** Player-owned inventory expected to contain the destination slot. */
    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Inventory|Action")
    TObjectPtr<UMythicInventoryComponent> Inventory = nullptr;

    /** Absolute destination slot index observed by the requesting client. */
    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Inventory|Action")
    int32 SlotIndex = INDEX_NONE;

    /** True when the destination was empty at submission time. */
    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Inventory|Action")
    bool bExpectEmpty = true;

    /** Stable occupant identity expected when bExpectEmpty is false; invalid for an expected-empty slot. */
    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Inventory|Action")
    FGuid ExpectedOccupantGuid;

    /** Exact occupant quantity expected when bExpectEmpty is false; zero for an expected-empty slot. */
    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Inventory|Action")
    int32 ExpectedOccupantQuantity = 0;

    bool IsStructurallyValid() const {
        if (!Inventory || SlotIndex < 0) {
            return false;
        }
        return bExpectEmpty
            ? !ExpectedOccupantGuid.IsValid() && ExpectedOccupantQuantity == 0
            : ExpectedOccupantGuid.IsValid() && ExpectedOccupantQuantity > 0;
    }
};

/** Correlated authoritative completion delivered to the owning client for every accepted request ID. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicInventoryActionReceipt {
    GENERATED_BODY()

    /** Monotonic client request identifier used only for correlation and duplicate suppression. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    int64 RequestId = 0;

    /** Operation whose request produced this receipt. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    EMythicInventoryAction Action = EMythicInventoryAction::Move;

    /** First execution, rejection, or replay classification for this delivery. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    EMythicInventoryReceiptDisposition Disposition = EMythicInventoryReceiptDisposition::Rejected;

    /** Disclosure-safe semantic outcome of the authoritative operation. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    EMythicInventoryActionResult Result = EMythicInventoryActionResult::InvalidRequest;

    /** Physical source identity the server validated, when one was available. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    FGuid ItemGuid;

    /** Source slot involved in the request, or INDEX_NONE for inventory-wide actions. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    int32 SourceSlotIndex = INDEX_NONE;

    /** Destination or newly-created slot, or INDEX_NONE when the action has no destination. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    int32 TargetSlotIndex = INDEX_NONE;

    /** Quantity moved, split, dropped, or consumed when the committed operation reports one. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    int32 QuantityProcessed = 0;

    bool WasSuccessful() const {
        return Result == EMythicInventoryActionResult::Succeeded;
    }
};

/**
 * Client-local snapshot published immediately before one inventory request enters the reliable RPC seam.
 *
 * This is presentation metadata only. The server continues to validate the full source and target locators carried by
 * the request itself; UI code must never treat this projection as authority or mutate inventory from it.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicInventoryActionSubmission {
    GENERATED_BODY()

    /** Positive controller-local correlation identifier shared with the eventual authoritative receipt. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    int64 RequestId = 0;

    /** Inventory operation submitted through the canonical player-controller seam. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    EMythicInventoryAction Action = EMythicInventoryAction::Move;

    /** Player-owned inventory that supplied the source item, or the sorted inventory for a group-wide request. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    TObjectPtr<UMythicInventoryComponent> Inventory = nullptr;

    /** Stable physical item identity captured from the source locator; invalid for a group-wide sort. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    FGuid ItemGuid;

    /** Absolute source slot, or INDEX_NONE when the request applies to a carried group. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    int32 SourceSlotIndex = INDEX_NONE;

    /** Absolute destination slot for a move, otherwise INDEX_NONE. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    int32 TargetSlotIndex = INDEX_NONE;

    /** Requested split/drop quantity, or the observed source quantity for actions that affect the whole item. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    int32 RequestedQuantity = 0;

    /** Carried group affected by Sort; invalid for item-local actions. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    FGameplayTag GroupTag;

    /** Desired manual-junk state for SetJunk; ignored by every other action. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Inventory|Action")
    bool bDesiredManualJunk = false;
};
