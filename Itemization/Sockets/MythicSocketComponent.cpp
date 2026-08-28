#include "Itemization/Sockets/MythicSocketComponent.h"

#include "Engine/GameInstance.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Itemization/Inventory/Fragments/Passive/SocketsFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Itemization/Inventory/Fragments/Passive/MythicGemFragment.h"
#include "Itemization/Sockets/MythicSocketTypes.h"
#include "Mythic/Mythic.h"
#include "Player/MythicPlayerController.h"

UMythicSocketComponent::UMythicSocketComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

AController *UMythicSocketComponent::GetOwningController() const {
    if (const APawn *Pawn = Cast<APawn>(GetOwner())) {
        return Pawn->GetController();
    }
    if (AController *AsController = Cast<AController>(GetOwner())) {
        return AsController;
    }
    return nullptr;
}

USocketsFragment *UMythicSocketComponent::GetSocketsFragment(UMythicItemInstance *Item) {
    if (!Item) {
        return nullptr;
    }
    return const_cast<USocketsFragment *>(Item->GetFragment<USocketsFragment>());
}

void UMythicSocketComponent::ServerSocketGem_Implementation(UMythicItemInstance *HostItem, int32 SocketIndex, UMythicItemInstance *Gem) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (!HostItem || !Gem || HostItem == Gem || Gem->GetStacks() < 1) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: null host or gem."));
        return;
    }

    // RPC object references are not authorization. Both physical items must still be in an inventory controlled by
    // the caller; otherwise a client could name another player's replicated item and consume/socket it.
    AController *Controller = GetOwningController();
    const AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(Controller);
    const TArray<UMythicInventoryComponent *> OwnedInventories = MythicPC
                                                                     ? MythicPC->GetAllInventoryComponents()
                                                                     : TArray<UMythicInventoryComponent *>();
    if (!MythicPC || !OwnedInventories.Contains(HostItem->GetInventoryComponent())
        || !OwnedInventories.Contains(Gem->GetInventoryComponent())) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: caller does not own both item instances."));
        return;
    }

    USocketsFragment *SocketsFrag = GetSocketsFragment(HostItem);
    if (!SocketsFrag) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: host item has no sockets."));
        return;
    }
    if (SocketIndex < 0 || SocketIndex >= SocketsFrag->GetSocketCount()) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: socket index %d out of range."), SocketIndex);
        return;
    }
    if (SocketsFrag->IsSocketFilled(SocketIndex)) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: socket %d already filled."), SocketIndex);
        return;
    }

    UMythicGemFragment *GemFrag = const_cast<UMythicGemFragment *>(Gem->GetFragment<UMythicGemFragment>());
    if (GemFrag) {
        GemFrag->RequestRuntimeData();
    }
    if (!GemFrag || !GemFrag->IsGem()) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: item is not a registry-ready gem."));
        return;
    }

    const FGameplayTag SocketColor = SocketsFrag->GetSocketColor(SocketIndex);
    if (!FMythicSocketMath::IsGemCompatible(GemFrag->GetGemType(), SocketColor)) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: gem %s incompatible with socket color %s."),
               *GemFrag->GetGemType().ToString(), *SocketColor.ToString());
        return;
    }

    TArray<FRolledAffix> GemSnapshots;
    GemFrag->GetGrantedAffixSnapshots(GemSnapshots);
    if (SocketsFrag->ServerSocketGem(SocketIndex, GemFrag->GetGemType(), Gem->GetItemInstanceGuid(),
                                     GemSnapshots)) {
        // Consumption is last: a failed provenance rekey, effect apply or socket-state commit never destroys a gem.
        Gem->ConsumeItem(1);
    }
}

void UMythicSocketComponent::ServerUnsocketGem_Implementation(UMythicItemInstance *HostItem, int32 SocketIndex) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (!HostItem) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Unsocket: null host."));
        return;
    }

    const AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(GetOwningController());
    if (!MythicPC || !MythicPC->GetAllInventoryComponents().Contains(HostItem->GetInventoryComponent())) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Unsocket: caller does not own the host item."));
        return;
    }

    USocketsFragment *SocketsFrag = GetSocketsFragment(HostItem);
    if (!SocketsFrag) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Unsocket: host item has no sockets."));
        return;
    }

    const FGameplayTag RemovedGemType = SocketsFrag->ServerUnsocketGem(SocketIndex);
    if (!RemovedGemType.IsValid()) {
        return;
    }

    if (bReturnGemOnUnsocket) {
        const TObjectPtr<UItemDefinition> *DefPtr = GemReturnDefs.Find(RemovedGemType);
        UItemDefinition *GemDef = DefPtr ? DefPtr->Get() : nullptr;
        if (!GemDef) {
            UE_LOG(Myth, Warning, TEXT("SocketComponent::Unsocket: no return ItemDefinition for gem %s; gem discarded."),
                   *RemovedGemType.ToString());
            return;
        }
        const UWorld *World = GetWorld();
        UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
        UMythicLootManagerSubsystem *Loot = GI ? GI->GetSubsystem<UMythicLootManagerSubsystem>() : nullptr;
        if (!Loot) {
            UE_LOG(Myth, Warning, TEXT("SocketComponent::Unsocket: LootManager unavailable; gem discarded."));
            return;
        }
        AController *Controller = GetOwningController();
        UMythicItemInstance *GemItem = Loot->Create(GemDef, 1, Controller, HostItem->GetItemLevel());
        if (GemItem) {
            if (UMythicInventoryComponent *HostInv = HostItem->GetInventoryComponent()) {
                HostInv->AddItem(GemItem, Controller);
            }
        }
    }
}
