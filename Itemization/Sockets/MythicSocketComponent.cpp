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
    if (!HostItem || !Gem) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: null host or gem."));
        return;
    }

    USocketsFragment *SocketsFrag = GetSocketsFragment(HostItem);
    if (!SocketsFrag) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: host item has no sockets."));
        return;
    }
    if (!SocketsFrag->Sockets.IsValidIndex(SocketIndex)) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: socket index %d out of range."), SocketIndex);
        return;
    }
    if (SocketsFrag->Sockets[SocketIndex].bFilled) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: socket %d already filled."), SocketIndex);
        return;
    }

    const UMythicGemFragment *GemFrag = Gem->GetFragment<UMythicGemFragment>();
    if (!GemFrag || !GemFrag->IsGem()) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: item is not a gem."));
        return;
    }

    const FGameplayTag SocketColor = SocketsFrag->Sockets[SocketIndex].SocketColor;
    if (!FMythicSocketMath::IsGemCompatible(GemFrag->GetGemType(), SocketColor)) {
        UE_LOG(Myth, Warning, TEXT("SocketComponent::Socket: gem %s incompatible with socket color %s."),
               *GemFrag->GetGemType().ToString(), *SocketColor.ToString());
        return;
    }

    SocketsFrag->ServerSocketGem(SocketIndex, GemFrag->GetGemType(), GemFrag->GrantedAffixes);
    Gem->ConsumeItem(1);
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
