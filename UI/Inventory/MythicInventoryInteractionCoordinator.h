// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/MythicInventoryActionTypes.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "MythicInventoryInteractionCoordinator.generated.h"

class AMythicPlayerController;

/** Local UI edge fired when the controller's one-request inventory gate changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FMythicInventoryInteractionPendingChanged,
    bool, bPending,
    const FMythicInventoryActionSubmission &, Submission);

/** Local, already-localized inventory feedback for the active character page. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FMythicInventoryInteractionFeedback,
    const FText &, Message,
    bool, bIsError);

/** Local completion edge retaining the disclosure-safe authoritative receipt. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FMythicInventoryInteractionCompleted,
    const FMythicInventoryActionReceipt &, Receipt);

/**
 * Local-player presentation coordinator for authoritative inventory requests.
 *
 * The player controller remains transport authority and owns the one-request gate, reliable RPCs, replay cache, and
 * receipts. This subsystem only mirrors transient presentation state across character-page activation, schedules a
 * slow-request notice, and routes off-page completion into the existing HUD feed.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicInventoryInteractionCoordinator final
    : public ULocalPlayerSubsystem {
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual void PlayerControllerChanged(APlayerController *NewPlayerController) override;

    /** Idempotently observes submissions and receipts from this LocalPlayer's Mythic controller. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Inventory|Interaction")
    void AttachToController(AMythicPlayerController *Controller);

    /** Detaches only when Controller is the currently observed controller; null detaches unconditionally. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Inventory|Interaction")
    void DetachFromController(AMythicPlayerController *Controller = nullptr);

    /** Records whether the character inventory page can present feedback inline instead of through the HUD feed. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Inventory|Interaction")
    void SetCharacterInventoryPageActive(bool bActive);

    /** True while the controller is awaiting the authoritative receipt for its one allowed mutation. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Inventory|Interaction")
    bool IsMutationPending() const { return bHasActiveSubmission; }

    /** Returns the mirrored active submission, or an empty projection when no mutation is pending. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Inventory|Interaction")
    FMythicInventoryActionSubmission GetActiveSubmission() const {
        return bHasActiveSubmission
            ? ActiveSubmission : FMythicInventoryActionSubmission();
    }

    /** Returns the localized, disclosure-safe player message for one authoritative result. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Inventory|Interaction")
    static FText DescribeResult(EMythicInventoryActionResult Result);

    /** Broadcast when the mirrored one-request gate becomes pending or resolves. */
    UPROPERTY(BlueprintAssignable, Category = "Mythic|Inventory|Interaction")
    FMythicInventoryInteractionPendingChanged OnPendingChanged;

    /** Broadcast only for feedback that the active character page should present inline. */
    UPROPERTY(BlueprintAssignable, Category = "Mythic|Inventory|Interaction")
    FMythicInventoryInteractionFeedback OnFeedback;

    /** Broadcast once per request ID after a committed, rejected, or replayed authoritative receipt arrives. */
    UPROPERTY(BlueprintAssignable, Category = "Mythic|Inventory|Interaction")
    FMythicInventoryInteractionCompleted OnCompleted;

private:
    UFUNCTION()
    void HandleInventoryActionSubmitted(
        const FMythicInventoryActionSubmission &Submission);

    UFUNCTION()
    void HandleInventoryActionReceipt(
        const FMythicInventoryActionReceipt &Receipt);

    void HandleSlowRequest();
    void ClearSlowTimer();
    void PresentFeedback(const FText &Message, bool bIsError);
    bool MarkReceiptPresented(int64 RequestId);

    TWeakObjectPtr<AMythicPlayerController> BoundController;

    UPROPERTY(Transient)
    FMythicInventoryActionSubmission ActiveSubmission;

    UPROPERTY(Transient)
    bool bHasActiveSubmission = false;

    UPROPERTY(Transient)
    bool bCharacterInventoryPageActive = false;

    /** Oldest-to-newest request IDs retained only to suppress duplicate presentation, never to cache receipts. */
    TArray<int64> PresentedReceiptIds;

    FTimerHandle SlowRequestTimer;

    static constexpr int32 MaxPresentedReceiptIds = 64;
};
