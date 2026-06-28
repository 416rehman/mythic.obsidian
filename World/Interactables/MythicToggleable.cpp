// Mythic — toggleable world interactable implementation

#include "MythicToggleable.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
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
    DOREPLIFETIME(AMythicToggleable, bLocked);
}

void AMythicToggleable::BeginPlay() {
    Super::BeginPlay();
    // Apply the authored initial state on the server before any save restore overrides it. (Save restore runs its
    // own Serialize pass on bIsOn; this only seeds a freshly-placed actor.)
    if (HasAuthority() && bStartsOn && !bIsOn) {
        bIsOn = true;
    }
    // Match the visuals to the (possibly restored) state on spawn, every machine.
    OnToggleVisualChanged(bIsOn);
    OnLockStateChanged(bLocked);
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

bool AMythicToggleable::DoesKeyOpenLock(const FGameplayTagContainer &KeyTypeProbe, const FGameplayTag &RequiredKeyTag) {
    return RequiredKeyTag.IsValid() && KeyTypeProbe.HasTag(RequiredKeyTag);
}

FMythicUnlockOutcome AMythicToggleable::PlanKeyedUnlock(bool bLocked, bool bHasMatchingKey, bool bConsumeKey) {
    FMythicUnlockOutcome Outcome;
    if (bLocked && bHasMatchingKey) {
        Outcome.bUnlock = true;
        Outcome.bConsumeKey = bConsumeKey;
    }
    return Outcome;
}

UMythicItemInstance *AMythicToggleable::FindMatchingKey(AActor *Interactor) const {
    if (!RequiredKeyTag.IsValid()) {
        return nullptr;
    }
    // Resolve the acting controller (the interaction may pass a controller or a pawn).
    AController *Controller = Cast<AController>(Interactor);
    if (!Controller) {
        if (const APawn *Pawn = Cast<APawn>(Interactor)) {
            Controller = Pawn->GetController();
        }
    }
    const IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(Controller);
    if (!Provider) {
        return nullptr; // a non-player / inventory-less interactor can't carry keys
    }

    FGameplayTagContainer Probe;
    for (UMythicInventoryComponent *Inventory : Provider->GetAllInventoryComponents()) {
        if (!Inventory) {
            continue;
        }
        for (const FMythicInventorySlotEntry &Slot : Inventory->GetAllSlots()) {
            if (UMythicItemInstance *Item = Slot.SlottedItemInstance) {
                Probe.Reset();
                Item->GetTypeProbe(Probe);
                if (DoesKeyOpenLock(Probe, RequiredKeyTag)) {
                    return Item;
                }
            }
        }
    }
    return nullptr;
}

bool AMythicToggleable::ServerTryUnlockWithKey(AActor *Interactor) {
    if (!HasAuthority()) {
        return false;
    }
    if (!bLocked) {
        return true; // already unlocked
    }
    UMythicItemInstance *Key = FindMatchingKey(Interactor);
    const FMythicUnlockOutcome Plan = PlanKeyedUnlock(bLocked, Key != nullptr, bConsumeKeyOnUnlock);
    if (!Plan.bUnlock) {
        return false; // no matching key (or no key tag) — stays locked
    }

    bLocked = false;
    FlushNetDormancy();       // wake the dormant actor so the bLocked change replicates now
    OnLockStateChanged(false); // server/listen-host visual; remote clients get it via OnRep_Locked
    if (Plan.bConsumeKey && Key) {
        Key->ConsumeItem(1); // single-use key spent on the unlock
    }
    return true;
}

void AMythicToggleable::ServerToggle(AActor *Interactor) {
    if (!HasAuthority()) {
        return;
    }
    // A locked object first tries the interactor's key: a matching key unlocks it (and this same interaction then
    // opens it). Without a key it stays locked and the toggle below no-ops. (Pre-keyed behaviour for an empty
    // RequiredKeyTag is unchanged — FindMatchingKey returns nullptr, so bLocked is untouched.)
    if (bLocked) {
        ServerTryUnlockWithKey(Interactor);
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

void AMythicToggleable::OnRep_Locked() {
    OnLockStateChanged(bLocked);
}

void AMythicToggleable::DeserializeCustomData(const TArray<uint8> & /*InCustomData*/) {
    // The raw SaveGame Serialize just restored bIsOn + bLocked directly into memory — it did NOT wake this dormant
    // actor (so the restored replicated values wouldn't reach already-relevant clients) and did NOT re-fire the cosmetic
    // BP events (BeginPlay fired them with the pre-restore authored values). Reconcile both, for bLocked AND bIsOn.
    if (HasAuthority()) {
        FlushNetDormancy(); // push the restored bLocked + bIsOn to connected clients
    }
    OnToggleVisualChanged(bIsOn);
    OnLockStateChanged(bLocked);
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
