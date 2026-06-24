// Mythic — client-local placement mode
// The inventory "Place" action enters this mode; a ghost then tracks the player's aim each frame (tinted via the
// shared placement rules), Confirm asks the server to deploy and STAYS in mode to place the next from the stack, and
// the mode exits on cancel or when the stack runs out. Purely client/cosmetic — the authoritative deploy round-trips
// through AMythicPlayerController::ServerDeployPlaceable; nothing here is replicated (never replicate preview/UI).

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Itemization/Inventory/Fragments/Passive/PlaceableFragment.h" // FPlaceablePreview + the pure placement rules
#include "MythicPlacementModeComponent.generated.h"

class UMythicInventoryComponent;
class AMythicPlayerController;

/** The per-step action of an active placement session — the pure brain of the mode (model B + stay-in-mode). */
UENUM(BlueprintType)
enum class EMythicPlacementAction : uint8 {
    UpdateGhost, // keep aiming — refresh the ghost only
    Deploy,      // a valid confirm — ask the server to deploy, then STAY in mode for the next from the stack
    Exit         // cancel, or the source item is gone (stack exhausted / moved) — leave placement mode
};

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicPlacementModeComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicPlacementModeComponent();

    // Begin placing the placeable item in Inventory's SlotIndex. Local client only; no-op if the slot holds no
    // placeable. Spawns the optional ghost + starts the per-frame aim preview. Wire the inventory "Place" button here.
    UFUNCTION(BlueprintCallable, Category = "Placement")
    bool EnterPlacementMode(UMythicInventoryComponent *Inventory, int32 SlotIndex);

    // Confirm the current aim: if it's a legal spot, ask the server to deploy and STAY in mode (auto-exits when the
    // stack is exhausted).
    UFUNCTION(BlueprintCallable, Category = "Placement")
    void ConfirmPlacement();

    // Rotate the ghost (and the deploy) about its up axis.
    UFUNCTION(BlueprintCallable, Category = "Placement")
    void RotatePlacement(float DeltaYawDegrees);

    // Leave placement mode without placing.
    UFUNCTION(BlueprintCallable, Category = "Placement")
    void CancelPlacement();

    UFUNCTION(BlueprintPure, Category = "Placement")
    bool IsPlacing() const { return bPlacing; }

    // Whether the current aim is a legal spot — for the HUD reticle / confirm affordance.
    UFUNCTION(BlueprintPure, Category = "Placement")
    bool CanConfirmHere() const { return CurrentPreview.bCanConfirm; }

    // Latest preview (tint + reason + can-confirm). The designer ghost/HUD reads this to drive the visual; C++ owns
    // the position + validity + deploy, the BP owns the cosmetic tint.
    UFUNCTION(BlueprintPure, Category = "Placement")
    const FPlaceablePreview &GetCurrentPreview() const { return CurrentPreview; }

    // Pure session brain: cancel OR source-item-gone → Exit; a valid confirm → Deploy (stay in mode); else refresh
    // the ghost. Static + no engine state so the mode's control flow is unit-testable without a live world.
    static EMythicPlacementAction DecidePlacementAction(bool bCancelRequested, bool bSourceItemPresent, bool bConfirmRequested, bool bPlacementValid);

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

protected:
    // Optional designer ghost actor (translucent mesh; reads GetCurrentPreview() to tint itself). None → no visual,
    // but the mechanism (aim/validate/confirm/deploy) still works and exposes the preview for a HUD.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Placement")
    TSubclassOf<AActor> GhostActorClass;

private:
    void Step(bool bConfirmRequested, bool bCancelRequested);
    void UpdatePreview(); // trace + evaluate + move the ghost + cache CurrentPreview
    bool IsSourcePlaceablePresent() const;
    const UPlaceableFragment *ResolveActivePlaceable() const;
    bool ResolveAimRay(FVector &OutOrigin, FVector &OutDir) const;
    void ExitPlacementMode();

    UPROPERTY(Transient)
    AMythicPlayerController *OwnerPC = nullptr;

    UPROPERTY(Transient)
    UMythicInventoryComponent *ActiveInventory = nullptr;

    UPROPERTY(Transient)
    AActor *GhostActor = nullptr;

    int32 ActiveSlot = INDEX_NONE;
    float CurrentYaw = 0.0f;
    bool bPlacing = false;
    FVector CurrentCandidatePoint = FVector::ZeroVector;
    FPlaceablePreview CurrentPreview;
};
