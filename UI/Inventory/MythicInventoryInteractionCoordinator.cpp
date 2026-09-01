// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicInventoryInteractionCoordinator.h"

#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Player/MythicPlayerController.h"
#include "TimerManager.h"
#include "UI/HUD/MythicHudNotice.h"
#include "UI/MythicUIStyle.h"

void UMythicInventoryInteractionCoordinator::Initialize(
    FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);
    if (ULocalPlayer *LocalPlayer = GetLocalPlayer()) {
        AttachToController(
            Cast<AMythicPlayerController>(LocalPlayer->GetPlayerController(GetWorld())));
    }
}

void UMythicInventoryInteractionCoordinator::Deinitialize() {
    DetachFromController();
    PresentedReceiptIds.Reset();
    Super::Deinitialize();
}

void UMythicInventoryInteractionCoordinator::PlayerControllerChanged(
    APlayerController *NewPlayerController) {
    Super::PlayerControllerChanged(NewPlayerController);
    AttachToController(Cast<AMythicPlayerController>(NewPlayerController));
}

void UMythicInventoryInteractionCoordinator::AttachToController(
    AMythicPlayerController *Controller) {
    if (BoundController.Get() == Controller) {
        return;
    }

    DetachFromController();
    if (!Controller || !Controller->IsLocalController()) {
        return;
    }

    BoundController = Controller;
    Controller->OnInventoryActionSubmitted.AddUniqueDynamic(
        this, &ThisClass::HandleInventoryActionSubmitted);
    Controller->OnInventoryActionReceiptReceived.AddUniqueDynamic(
        this, &ThisClass::HandleInventoryActionReceipt);
}

void UMythicInventoryInteractionCoordinator::DetachFromController(
    AMythicPlayerController *Controller) {
    AMythicPlayerController *Bound = BoundController.Get();
    if (Controller && Bound != Controller) {
        return;
    }

    ClearSlowTimer();
    if (Bound) {
        Bound->OnInventoryActionSubmitted.RemoveDynamic(
            this, &ThisClass::HandleInventoryActionSubmitted);
        Bound->OnInventoryActionReceiptReceived.RemoveDynamic(
            this, &ThisClass::HandleInventoryActionReceipt);
    }
    BoundController.Reset();
    PresentedReceiptIds.Reset();

    if (bHasActiveSubmission) {
        bHasActiveSubmission = false;
        ActiveSubmission = FMythicInventoryActionSubmission();
        OnPendingChanged.Broadcast(false, ActiveSubmission);
    }
}

void UMythicInventoryInteractionCoordinator::SetCharacterInventoryPageActive(
    const bool bActive) {
    bCharacterInventoryPageActive = bActive;
    if (bActive && bHasActiveSubmission) {
        OnPendingChanged.Broadcast(true, ActiveSubmission);
        OnFeedback.Broadcast(
            NSLOCTEXT("MythicInventory", "RequestPending", "Updating inventory..."),
            false);
    }
}

FText UMythicInventoryInteractionCoordinator::DescribeResult(
    const EMythicInventoryActionResult Result) {
    switch (Result) {
    case EMythicInventoryActionResult::Succeeded:
        return NSLOCTEXT("MythicInventory", "RequestSucceeded", "Inventory updated.");
    case EMythicInventoryActionResult::StaleSource:
    case EMythicInventoryActionResult::StaleTarget:
        return NSLOCTEXT(
            "MythicInventory", "RequestStale",
            "The inventory changed before confirmation. Try again.");
    case EMythicInventoryActionResult::SourceProtected:
    case EMythicInventoryActionResult::TargetProtected:
    case EMythicInventoryActionResult::UnauthorizedInventory:
        return NSLOCTEXT(
            "MythicInventory", "RequestProtected",
            "That item or slot cannot be changed.");
    case EMythicInventoryActionResult::InventoryFull:
        return NSLOCTEXT(
            "MythicInventory", "RequestFull",
            "There is no compatible free slot.");
    case EMythicInventoryActionResult::IncompatibleTarget:
        return NSLOCTEXT(
            "MythicInventory", "RequestIncompatible",
            "That item cannot go in the selected slot.");
    case EMythicInventoryActionResult::InvalidQuantity:
        return NSLOCTEXT(
            "MythicInventory", "RequestQuantityChanged",
            "The stack quantity changed. Choose a new amount.");
    case EMythicInventoryActionResult::NotUsable:
        return NSLOCTEXT(
            "MythicInventory", "RequestNotUsable",
            "That item cannot be used right now.");
    case EMythicInventoryActionResult::InvalidGroup:
        return NSLOCTEXT(
            "MythicInventory", "RequestInvalidGroup",
            "That inventory category cannot be sorted.");
    case EMythicInventoryActionResult::SpawnFailed:
        return NSLOCTEXT(
            "MythicInventory", "RequestSpawnFailed",
            "The item could not be placed in the world.");
    case EMythicInventoryActionResult::CommitFailed:
        return NSLOCTEXT(
            "MythicInventory", "RequestCommitFailed",
            "The inventory could not be updated safely.");
    case EMythicInventoryActionResult::InvalidRequest:
    case EMythicInventoryActionResult::InvalidSlot:
    default:
        return NSLOCTEXT(
            "MythicInventory", "RequestRejected",
            "That action could not be completed.");
    }
}

void UMythicInventoryInteractionCoordinator::HandleInventoryActionSubmitted(
    const FMythicInventoryActionSubmission &Submission) {
    if (Submission.RequestId <= 0) {
        return;
    }

    ActiveSubmission = Submission;
    bHasActiveSubmission = true;
    OnPendingChanged.Broadcast(true, ActiveSubmission);
    if (bCharacterInventoryPageActive) {
        OnFeedback.Broadcast(
            NSLOCTEXT("MythicInventory", "RequestPending", "Updating inventory..."),
            false);
    }

    ClearSlowTimer();
    if (AMythicPlayerController *Controller = BoundController.Get()) {
        if (UWorld *World = Controller->GetWorld()) {
            World->GetTimerManager().SetTimer(
                SlowRequestTimer,
                this,
                &ThisClass::HandleSlowRequest,
                8.0f,
                false);
        }
    }
}

void UMythicInventoryInteractionCoordinator::HandleInventoryActionReceipt(
    const FMythicInventoryActionReceipt &Receipt) {
    if (Receipt.RequestId <= 0) {
        return;
    }

    if (bHasActiveSubmission
        && Receipt.RequestId == ActiveSubmission.RequestId) {
        ClearSlowTimer();
        bHasActiveSubmission = false;
        const FMythicInventoryActionSubmission CompletedSubmission =
            ActiveSubmission;
        ActiveSubmission = FMythicInventoryActionSubmission();
        OnPendingChanged.Broadcast(false, CompletedSubmission);
    }

    if (!MarkReceiptPresented(Receipt.RequestId)) {
        return;
    }

    const bool bIsError = !Receipt.WasSuccessful();
    PresentFeedback(DescribeResult(Receipt.Result), bIsError);
    OnCompleted.Broadcast(Receipt);
}

void UMythicInventoryInteractionCoordinator::HandleSlowRequest() {
    if (!bHasActiveSubmission) {
        return;
    }
    const FText Message = NSLOCTEXT(
        "MythicInventory", "RequestStillSyncing",
        "Still syncing inventory...");
    if (bCharacterInventoryPageActive) {
        OnFeedback.Broadcast(Message, false);
    }
    else if (AMythicPlayerController *Controller = BoundController.Get()) {
        FMythicHudNotice Notice;
        Notice.Kind = EMythicNoticeKind::Warning;
        Notice.Text = Message;
        Notice.Accent = FMythicUIStyle::Get().Caution;
        Notice.StackKey = TEXT("InventoryActionFeedback");
        Controller->RaiseHudNotice(Notice);
    }
}

void UMythicInventoryInteractionCoordinator::ClearSlowTimer() {
    if (AMythicPlayerController *Controller = BoundController.Get()) {
        if (UWorld *World = Controller->GetWorld()) {
            World->GetTimerManager().ClearTimer(SlowRequestTimer);
        }
    }
    SlowRequestTimer.Invalidate();
}

void UMythicInventoryInteractionCoordinator::PresentFeedback(
    const FText &Message,
    const bool bIsError) {
    if (bCharacterInventoryPageActive) {
        OnFeedback.Broadcast(Message, bIsError);
        return;
    }

    if (AMythicPlayerController *Controller = BoundController.Get()) {
        FMythicHudNotice Notice;
        Notice.Kind = bIsError
            ? EMythicNoticeKind::Warning : EMythicNoticeKind::Loot;
        Notice.Text = Message;
        Notice.Accent = bIsError
            ? FMythicUIStyle::Get().Negative
            : FMythicUIStyle::Get().InkSubtle;
        Notice.StackKey = TEXT("InventoryActionFeedback");
        Controller->RaiseHudNotice(Notice);
    }
}

bool UMythicInventoryInteractionCoordinator::MarkReceiptPresented(
    const int64 RequestId) {
    if (PresentedReceiptIds.Contains(RequestId)) {
        return false;
    }
    if (PresentedReceiptIds.Num() >= MaxPresentedReceiptIds) {
        PresentedReceiptIds.RemoveAt(0, 1, EAllowShrinking::No);
    }
    PresentedReceiptIds.Add(RequestId);
    return true;
}
