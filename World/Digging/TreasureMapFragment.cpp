
#include "Itemization/Inventory/Fragments/Passive/TreasureMapFragment.h"

#include "World/Digging/MythicDiggingSubsystem.h"
#include "World/Digging/MythicDigSite.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/InventoryProviderInterface.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Mythic/Mythic.h"

void UTreasureMapFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);

    if (!IsTreasureMap() || !ItemInstance) {
        return;
    }
    AActor *Owner = ItemInstance->GetInventoryOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    if (TargetAnchor.IsNearlyZero()) {
        UWorld *World = Owner->GetWorld();
        UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
        if (UMythicDiggingSubsystem *Dig = GI ? GI->GetSubsystem<UMythicDiggingSubsystem>() : nullptr) {
            if (const UMythicDigSiteRegistry *Reg = Dig->GetRegistry()) {
                FMythicDigSiteEntry Entry;
                if (Reg->FindSiteById(TargetDigSiteId, Entry)) {
                    TargetAnchor = Entry.Anchor;
                    ToleranceRadius = Entry.ToleranceRadius;
                }
            }
        }
    }

    UE_LOG(Myth, Log, TEXT("TreasureMap: read map to dig site %d (anchor %s)."), TargetDigSiteId, *TargetAnchor.ToString());
}

bool UTreasureMapFragment::ConsumeMatchingMap(APlayerController *PC, int32 SiteId) {
    if (!PC || !PC->HasAuthority() || SiteId < 0) {
        return false;
    }
    const IInventoryProviderInterface *Provider = Cast<IInventoryProviderInterface>(PC);
    if (!Provider) {
        return false;
    }
    for (UMythicInventoryComponent *Inv : Provider->GetAllInventoryComponents()) {
        if (!Inv) {
            continue;
        }
        for (const FMythicInventorySlotEntry &Slot : Inv->GetAllSlots()) {
            UMythicItemInstance *Item = Slot.SlottedItemInstance;
            if (!Item) {
                continue;
            }
            const UTreasureMapFragment *Frag = Item->GetFragment<UTreasureMapFragment>();
            if (Frag && Frag->TargetDigSiteId == SiteId) {
                Inv->ServerRemoveItem(Item, 1);
                UE_LOG(Myth, Log, TEXT("TreasureMap: consumed map for dug site %d."), SiteId);
                return true;
            }
        }
    }
    return false;
}
