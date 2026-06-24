// Mythic — client-local placement mode implementation

#include "Itemization/Placeable/MythicPlacementModeComponent.h"

#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Player/MythicPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UMythicPlacementModeComponent::UMythicPlacementModeComponent() {
    // Default no-tick; enable the per-frame preview ONLY while a placement session is active (toggle, don't poll).
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(false); // preview/UI is client-local; the deploy round-trips via the server RPC
}

EMythicPlacementAction UMythicPlacementModeComponent::DecidePlacementAction(const bool bCancelRequested, const bool bSourceItemPresent,
                                                                            const bool bConfirmRequested, const bool bPlacementValid) {
    // Cancel always exits.
    if (bCancelRequested) {
        return EMythicPlacementAction::Exit;
    }
    // The source item being gone (last of the stack just deployed, or moved away) exits.
    if (!bSourceItemPresent) {
        return EMythicPlacementAction::Exit;
    }
    // A confirm on a legal spot deploys (the caller stays in mode for the next from the stack).
    if (bConfirmRequested && bPlacementValid) {
        return EMythicPlacementAction::Deploy;
    }
    // Otherwise just keep the ghost current.
    return EMythicPlacementAction::UpdateGhost;
}

const UPlaceableFragment *UMythicPlacementModeComponent::ResolveActivePlaceable() const {
    if (!ActiveInventory) {
        return nullptr;
    }
    UMythicItemInstance *Item = ActiveInventory->GetItem(ActiveSlot);
    return Item ? Item->GetFragment<UPlaceableFragment>() : nullptr;
}

bool UMythicPlacementModeComponent::IsSourcePlaceablePresent() const {
    return ResolveActivePlaceable() != nullptr;
}

bool UMythicPlacementModeComponent::ResolveAimRay(FVector &OutOrigin, FVector &OutDir) const {
    if (!OwnerPC) {
        return false;
    }
    const APawn *P = OwnerPC->GetPawn();
    if (!P) {
        return false;
    }
    // No camera is wired on the player pawn, so aim from the pawn's eyes along its facing (matches the DeployPlaceable
    // cheat + what the server re-traces). A camera, once added, is the single place to upgrade this.
    FRotator Rot;
    P->GetActorEyesViewPoint(OutOrigin, Rot);
    OutDir = Rot.Vector();
    return true;
}

bool UMythicPlacementModeComponent::EnterPlacementMode(UMythicInventoryComponent *Inventory, int32 SlotIndex) {
    OwnerPC = Cast<AMythicPlayerController>(GetOwner());
    if (!OwnerPC || !OwnerPC->IsLocalController()) {
        return false; // placement mode is a client-local interaction
    }
    if (!Inventory || SlotIndex < 0) {
        return false;
    }

    ActiveInventory = Inventory;
    ActiveSlot = SlotIndex;
    if (!IsSourcePlaceablePresent()) { // the slot holds no placeable — nothing to place
        ActiveInventory = nullptr;
        ActiveSlot = INDEX_NONE;
        return false;
    }

    CurrentYaw = 0.0f;
    bPlacing = true;

    // Spawn the optional designer ghost (client-local, collision off so it never blocks its own placement trace).
    if (UWorld *World = GetWorld(); World && GhostActorClass && !GhostActor) {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Params.ObjectFlags |= RF_Transient;
        GhostActor = World->SpawnActor<AActor>(GhostActorClass, FTransform::Identity, Params);
        if (GhostActor) {
            GhostActor->SetActorEnableCollision(false);
        }
    }

    SetComponentTickEnabled(true);
    UpdatePreview();
    return true;
}

void UMythicPlacementModeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!bPlacing) {
        return;
    }
    UpdatePreview();
    Step(/*bConfirmRequested*/ false, /*bCancelRequested*/ false); // auto-exits if the source item is gone
}

void UMythicPlacementModeComponent::UpdatePreview() {
    const UPlaceableFragment *Placeable = ResolveActivePlaceable();
    UWorld *World = GetWorld();
    FVector Origin, Dir;
    if (!Placeable || !World || !ResolveAimRay(Origin, Dir)) {
        CurrentPreview = UPlaceableFragment::DescribePlacement(EPlaceablePlacementResult::NoSurface);
        return;
    }

    const FVector TraceEnd = Origin + Dir * Placeable->MaxPlacementReach;

    FCollisionQueryParams Params(FName(TEXT("MythicPlacementPreview")), /*bTraceComplex*/ false);
    if (OwnerPC) {
        Params.AddIgnoredActor(OwnerPC->GetPawn());
    }
    if (GhostActor) {
        Params.AddIgnoredActor(GhostActor); // the ghost must not block its own placement
    }

    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, Origin, TraceEnd, ECC_Visibility, Params);
    CurrentCandidatePoint = bHit ? Hit.ImpactPoint : TraceEnd;

    // Pawn-clearance only (matches the server's deploy check); geometry clearance is a logged refinement.
    const bool bBlocked = World->OverlapAnyTestByChannel(CurrentCandidatePoint, FQuat::Identity, ECC_Pawn,
                                                         FCollisionShape::MakeSphere(Placeable->RequiredClearanceRadius), Params);

    const FVector InstigatorLoc = (OwnerPC && OwnerPC->GetPawn()) ? OwnerPC->GetPawn()->GetActorLocation() : Origin;
    const FPlaceablePlacementQuery Query = UPlaceableFragment::BuildPlacementQuery(bHit, Hit.ImpactPoint, Hit.ImpactNormal, TraceEnd, InstigatorLoc, bBlocked);
    CurrentPreview = UPlaceableFragment::DescribePlacement(Placeable->EvaluatePlacement(Query));

    if (GhostActor) {
        GhostActor->SetActorLocationAndRotation(CurrentCandidatePoint, FRotator(0.0f, CurrentYaw, 0.0f));
    }
}

void UMythicPlacementModeComponent::ConfirmPlacement() {
    UpdatePreview(); // refresh so the validity check + the aim sent to the server are the same frame
    Step(/*bConfirmRequested*/ true, /*bCancelRequested*/ false);
}

void UMythicPlacementModeComponent::CancelPlacement() {
    Step(/*bConfirmRequested*/ false, /*bCancelRequested*/ true);
}

void UMythicPlacementModeComponent::RotatePlacement(float DeltaYawDegrees) {
    CurrentYaw = FRotator::ClampAxis(CurrentYaw + DeltaYawDegrees);
    if (GhostActor) {
        GhostActor->SetActorRotation(FRotator(0.0f, CurrentYaw, 0.0f));
    }
}

void UMythicPlacementModeComponent::Step(const bool bConfirmRequested, const bool bCancelRequested) {
    if (!bPlacing) {
        return;
    }
    const EMythicPlacementAction Action =
        DecidePlacementAction(bCancelRequested, IsSourcePlaceablePresent(), bConfirmRequested, CurrentPreview.bCanConfirm);

    switch (Action) {
    case EMythicPlacementAction::Exit:
        ExitPlacementMode();
        break;
    case EMythicPlacementAction::Deploy: {
        FVector Origin, Dir;
        if (OwnerPC && ResolveAimRay(Origin, Dir)) {
            // Server re-validates + re-traces authoritatively; we send our aim ray. STAY in mode afterwards — the
            // next tick re-checks the stack (and exits when it empties).
            OwnerPC->ServerDeployPlaceable(ActiveInventory, ActiveSlot, Origin, Dir);
        }
        break;
    }
    case EMythicPlacementAction::UpdateGhost:
    default:
        break; // UpdatePreview already refreshed the ghost
    }
}

void UMythicPlacementModeComponent::ExitPlacementMode() {
    bPlacing = false;
    SetComponentTickEnabled(false);
    if (GhostActor) {
        GhostActor->Destroy();
        GhostActor = nullptr;
    }
    ActiveInventory = nullptr;
    ActiveSlot = INDEX_NONE;
    CurrentYaw = 0.0f;
    CurrentPreview = FPlaceablePreview();
}
