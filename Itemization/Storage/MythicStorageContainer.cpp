//

#include "MythicStorageContainer.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Player/MythicPlayerController.h"
#include "Subsystem/SaveSystem/Character/SavedInventory.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

AMythicStorageContainer::AMythicStorageContainer() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    // Item-instance subobjects replicate through the registered-subobject list (matches PlayerController /
    // GameState). Belt-and-braces alongside the component's own opt-in.
    bReplicateUsingRegisteredSubObjectList = true;
    SetNetCullDistanceSquared(FMath::Square(4000.f));

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);

    ContainerInventory = CreateDefaultSubobject<UMythicInventoryComponent>(TEXT("ContainerInventory"));
    ContainerInventory->SetIsReplicated(true);
}

void AMythicStorageContainer::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    Openers.Empty();
    Super::EndPlay(EndPlayReason);
}

AController *AMythicStorageContainer::ResolveController(AActor *Interactor) {
    if (AController *C = Cast<AController>(Interactor)) {
        return C;
    }
    if (const APawn *P = Cast<APawn>(Interactor)) {
        return P->GetController();
    }
    return nullptr;
}

TArray<UMythicInventoryComponent *> AMythicStorageContainer::GetAllInventoryComponents() const {
    return {ContainerInventory};
}

UAbilitySystemComponent *AMythicStorageContainer::GetSchematicsASC() const {
    return nullptr;
}

void AMythicStorageContainer::OnPrimaryInteract_Implementation(AActor *Interactor) {
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(ResolveController(Interactor));
    if (!PC) {
        return;
    }

    if (HasAuthority()) {
        // Server (incl. the listen-server host): register the player as an opener, range-gated exactly as before.
        // Reached directly on the host, or re-entered by the generic ServerInteractPrimary route for a remote client.
        if (IsActorInRange(PC->GetPawn())) {
            Server_AddOpener(PC);
        }
    }
    else {
        // Remote client: route through the single generic interaction primitive instead of a bespoke open RPC.
        PC->ServerInteractPrimary(this);
    }

    // The local controller (host OR remote client; never a dedicated server / remote proxy) pushes the dual-pane
    // widget. Optimistic open, exactly as before — actual access to the container inventory is independently
    // re-gated server-side on every item move in CanPlayerAccessInventory.
    if (PC->IsLocalController()) {
        OnContainerOpened(PC);
    }
}

void AMythicStorageContainer::OnSecondaryInteract_Implementation(AActor *Interactor) {
    // No default secondary action; Blueprints may override.
}

USceneComponent *AMythicStorageContainer::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicStorageContainer::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    return true;
}

void AMythicStorageContainer::OnFocused_Implementation(AActor *Interactor) {
    // Visual feedback handled in Blueprint.
}

void AMythicStorageContainer::OnUnfocused_Implementation(AActor *Interactor) {
    // Visual feedback handled in Blueprint.
}

bool AMythicStorageContainer::IsActorInRange(const AActor *Actor) const {
    if (ServerUseRangeSq <= 0.0f) {
        return true;
    }
    if (!Actor) {
        return false;
    }
    return FVector::DistSquared(Actor->GetActorLocation(), GetActorLocation()) <= ServerUseRangeSq;
}

void AMythicStorageContainer::Server_AddOpener(AMythicPlayerController *PC) {
    if (PC && HasAuthority()) {
        Openers.Add(PC);
    }
}

bool AMythicStorageContainer::Server_IsOpener(const AMythicPlayerController *PC) const {
    return PC && Openers.Contains(const_cast<AMythicPlayerController *>(PC));
}

void AMythicStorageContainer::Server_RemoveOpener(AMythicPlayerController *PC) {
    if (PC) {
        Openers.Remove(PC);
    }
}

void AMythicStorageContainer::SerializeCustomData(TArray<uint8> &OutCustomData) {
    if (!ContainerInventory) {
        return;
    }

    // Convert the inventory to the engine-agnostic FSerializedInventoryData (items -> bytes), then write that
    // struct to OutCustomData. Reuses the same byte-archive template as AMythicWorldItem.
    FSerializedInventoryData Data;
    FSerializedInventoryData::Serialize(ContainerInventory, Data);

    FMemoryWriter MemWriter(OutCustomData);
    FObjectAndNameAsStringProxyArchive Ar(MemWriter, false);
    FSerializedInventoryData::StaticStruct()->SerializeItem(Ar, &Data, nullptr);
}

void AMythicStorageContainer::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    if (InCustomData.Num() == 0 || !ContainerInventory) {
        return;
    }

    FMemoryReader MemReader(InCustomData);
    FObjectAndNameAsStringProxyArchive Ar(MemReader, false);

    FSerializedInventoryData Data;
    FSerializedInventoryData::StaticStruct()->SerializeItem(Ar, &Data, nullptr);

    FSerializedInventoryData::Deserialize(ContainerInventory, Data);
}
