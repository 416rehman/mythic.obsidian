
#include "Itemization/Placeable/MythicPlacementModeComponent.h"

#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Player/MythicPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

UMythicPlacementModeComponent::UMythicPlacementModeComponent() {
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(false);
}

EMythicPlacementAction UMythicPlacementModeComponent::DecidePlacementAction(const bool bCancelRequested, const bool bSourceItemPresent,
                                                                            const bool bConfirmRequested, const bool bPlacementValid) {
    if (bCancelRequested) {
        return EMythicPlacementAction::Exit;
    }
    if (!bSourceItemPresent) {
        return EMythicPlacementAction::Exit;
    }
    if (bConfirmRequested && bPlacementValid) {
        return EMythicPlacementAction::Deploy;
    }
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
    FRotator Rot;
    P->GetActorEyesViewPoint(OutOrigin, Rot);
    OutDir = Rot.Vector();
    return true;
}

bool UMythicPlacementModeComponent::EnterPlacementMode(UMythicInventoryComponent *Inventory, int32 SlotIndex) {
    OwnerPC = Cast<AMythicPlayerController>(GetOwner());
    if (!OwnerPC || !OwnerPC->IsLocalController()) {
        return false;
    }
    if (!Inventory || SlotIndex < 0) {
        return false;
    }

    ActiveInventory = Inventory;
    ActiveSlot = SlotIndex;
    if (!IsSourcePlaceablePresent()) {
        ActiveInventory = nullptr;
        ActiveSlot = INDEX_NONE;
        return false;
    }

    CurrentYaw = 0.0f;
    bPlacing = true;

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
    Step( false, false);
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

    FCollisionQueryParams Params(FName(TEXT("MythicPlacementPreview")), false);
    if (OwnerPC) {
        Params.AddIgnoredActor(OwnerPC->GetPawn());
    }
    if (GhostActor) {
        Params.AddIgnoredActor(GhostActor);
    }

    FHitResult Hit;
    const bool bHit = World->LineTraceSingleByChannel(Hit, Origin, TraceEnd, ECC_Visibility, Params);
    CurrentCandidatePoint = bHit ? Hit.ImpactPoint : TraceEnd;

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
    UpdatePreview();
    Step( true, false);
}

void UMythicPlacementModeComponent::CancelPlacement() {
    Step( false, true);
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
            OwnerPC->ServerDeployPlaceable(ActiveInventory, ActiveSlot, Origin, Dir);
        }
        break;
    }
    case EMythicPlacementAction::UpdateGhost:
    default:
        break;
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
