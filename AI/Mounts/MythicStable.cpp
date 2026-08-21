
#include "AI/Mounts/MythicStable.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"

#include "GAS/Mounts/MythicMountRosterComponent.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Mythic.h"

AMythicStable::AMythicStable() {
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);
}

AMythicPlayerController *AMythicStable::ResolveMythicPC(AActor *Interactor) {
    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(Interactor)) {
        return PC;
    }
    if (const APawn *InteractorPawn = Cast<APawn>(Interactor)) {
        return Cast<AMythicPlayerController>(InteractorPawn->GetController());
    }
    return nullptr;
}

UMythicMountRosterComponent *AMythicStable::ResolveRoster(const AMythicPlayerController *PC) {
    const AMythicPlayerState *PS = PC ? PC->GetPlayerState<AMythicPlayerState>() : nullptr;
    return PS ? PS->GetMountRosterComponent() : nullptr;
}

bool AMythicStable::IsActorInRange(const AActor *Actor) const {
    if (InteractRangeSq <= 0.0f) {
        return true;
    }
    if (!Actor) {
        return false;
    }
    return FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation()) <= InteractRangeSq;
}

void AMythicStable::OnPrimaryInteract_Implementation(AActor *Interactor) {
    AMythicPlayerController *PC = ResolveMythicPC(Interactor);
    if (!PC) {
        return;
    }

    if (GetNetMode() == NM_Client) {
        if (PC->IsLocalController()) {
            PC->ServerInteractPrimary(this);
        }
        return;
    }

    if (!IsActorInRange(PC->GetPawn())) {
        return;
    }
    UMythicMountRosterComponent *Roster = ResolveRoster(PC);
    if (!Roster) {
        return;
    }
    const bool bStash = Roster->IsMountSummoned();
    if (bStash) {
        Roster->ServerStashMount();
    }
    else {
        Roster->ServerSummonMount();
    }
    OnStableUsed(PC, bStash);
}

USceneComponent *AMythicStable::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicStable::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    return true;
}

void AMythicStable::OnFocused_Implementation(AActor *Interactor) {
}

void AMythicStable::OnUnfocused_Implementation(AActor *Interactor) {
}
