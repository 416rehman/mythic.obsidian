// Mythic — toggleable world interactable implementation

#include "MythicToggleable.h"

#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/MythicPlayerController.h" // routes client interaction → server via ServerInteractPrimary

AMythicToggleable::AMythicToggleable() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    // A door/lever changes state rarely — stay dormant until a toggle wakes it (relevancy/dormancy at scale).
    NetDormancy = DORM_DormantAll;
    SetNetCullDistanceSquared(FMath::Square(6000.f));

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);
}

void AMythicToggleable::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMythicToggleable, bIsOn);
}

void AMythicToggleable::BeginPlay() {
    Super::BeginPlay();
    // Apply the authored initial state on the server before any save restore overrides it. (Save restore runs its
    // own Serialize pass on bIsOn; this only seeds a freshly-placed actor.)
    if (HasAuthority() && bStartsOn && !bIsOn) {
        bIsOn = true;
    }
    // Match the visual to the (possibly restored) state on spawn, every machine.
    OnToggleVisualChanged(bIsOn);
}

FMythicToggleOutcome AMythicToggleable::ResolveToggle(const bool bCurrentlyOn, const bool bLocked, const bool bOneShot, const bool bHasActivated) {
    FMythicToggleOutcome Outcome;
    Outcome.bNewIsOn = bCurrentlyOn; // default: unchanged

    // A locked object ignores interaction entirely.
    if (bLocked) {
        return Outcome;
    }
    // A one-shot that has already fired stays put.
    if (bOneShot && bHasActivated) {
        return Outcome;
    }
    // A one-shot that hasn't fired only ever goes ON; a normal toggle flips.
    const bool bTarget = bOneShot ? true : !bCurrentlyOn;
    Outcome.bNewIsOn = bTarget;
    Outcome.bChanged = (bTarget != bCurrentlyOn);
    return Outcome;
}

void AMythicToggleable::ServerToggle(AActor *Interactor) {
    if (!HasAuthority()) {
        return;
    }
    const FMythicToggleOutcome Outcome = ResolveToggle(bIsOn, bLocked, bOneShot, bHasActivated);
    if (bOneShot && Outcome.bChanged) {
        bHasActivated = true;
    }
    if (!Outcome.bChanged) {
        return; // locked / one-shot-spent / already in the target state — nothing to do
    }
    ApplyState(Outcome.bNewIsOn);

    // Mirror the new state to linked targets (a lever → its gates). One hop: set their state directly, no re-toggle,
    // so a link cycle can't cascade.
    for (const TObjectPtr<AMythicToggleable> &Linked : LinkedToggleables) {
        if (Linked && Linked != this) {
            Linked->ApplyState(Outcome.bNewIsOn);
        }
    }
}

void AMythicToggleable::ApplyState(const bool bNewIsOn) {
    if (!HasAuthority() || bIsOn == bNewIsOn) {
        return;
    }
    bIsOn = bNewIsOn;
    FlushNetDormancy();       // wake the dormant actor so the bIsOn change replicates now
    OnToggleVisualChanged(bIsOn); // server/listen-host visual; remote clients get it via OnRep
}

void AMythicToggleable::OnRep_IsOn() {
    OnToggleVisualChanged(bIsOn);
}

void AMythicToggleable::OnPrimaryInteract_Implementation(AActor *Interactor) {
    // The prompt widget calls this on the CLIENT (Interactor is the player controller). Route to the server via the
    // generic interaction RPC; when it re-invokes us server-side, do the authoritative toggle.
    if (HasAuthority()) {
        ServerToggle(Interactor);
        return;
    }
    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(Interactor)) {
        if (PC->IsLocalController()) {
            PC->ServerInteractPrimary(this);
        }
    }
}

void AMythicToggleable::OnSecondaryInteract_Implementation(AActor *Interactor) {
    // No default secondary action.
}

USceneComponent *AMythicToggleable::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicToggleable::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    // A locked toggleable still shows its prompt (so the player learns it's locked); the server toggle no-ops.
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    return true;
}

void AMythicToggleable::OnFocused_Implementation(AActor *Interactor) {
    // Visual feedback handled in Blueprint.
}

void AMythicToggleable::OnUnfocused_Implementation(AActor *Interactor) {
    // Visual feedback handled in Blueprint.
}
