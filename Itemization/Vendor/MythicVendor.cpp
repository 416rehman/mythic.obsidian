// Mythic — merchant vendor implementation. See MythicVendor.h for the design contract.

#include "MythicVendor.h"

#include "Engine/GameInstance.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicCurrency.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Itemization/MythicTags_Inventory.h" // ITEMIZATION_TYPE_CURRENCY
#include "Player/MythicPlayerController.h"

namespace {
    // Sum a buyer's currency across all of their inventories (the player's wallet may span backpack + extra bags).
    int32 SumPlayerCurrency(AMythicPlayerController *Player) {
        int32 Total = 0;
        if (!Player) {
            return 0;
        }
        for (UMythicInventoryComponent *Inv : Player->GetAllInventoryComponents()) {
            if (Inv) {
                Total += Inv->GetTotalCurrency();
            }
        }
        return Total;
    }

    // Grant Amount currency to a player's inventory, minting it from CurrencyDef. Splits across multiple item instances
    // when Amount exceeds the currency's per-stack cap (Initialize clamps a single instance to StackSizeMax, so a naive
    // single mint of a large payout would silently lose the remainder). AddItem fires the "+N <Currency>" pickup callout.
    void GrantCurrency(UMythicInventoryComponent *Inv, AController *Recipient, int32 Amount, UItemDefinition *CurrencyDef,
                       UMythicLootManagerSubsystem *Loot) {
        if (!Inv || !CurrencyDef || !Loot || Amount <= 0) {
            return;
        }
        const int32 Cap = FMath::Max(1, CurrencyDef->StackSizeMax);
        int32 Remaining = Amount;
        int32 Guard = 0;
        while (Remaining > 0 && Guard++ < 4096) {
            const int32 Chunk = FMath::Min(Remaining, Cap);
            UMythicItemInstance *Coins = Loot->Create(CurrencyDef, Chunk, Recipient, 0);
            if (!Coins) {
                break;
            }
            Inv->AddItem(Coins, Recipient);
            Remaining -= Chunk;
        }
    }
} // namespace

AMythicVendor::AMythicVendor() {
    // All container infrastructure (SceneRoot/Mesh/ContainerInventory + replication + opener registry) is set up by the
    // AMythicStorageContainer constructor. The vendor adds only the priced trade behaviour on top.
}

int32 AMythicVendor::GetBuyPriceForSlot(int32 StockSlotIndex, int32 Quantity) const {
    if (UMythicInventoryComponent *Stock = GetContainerInventory()) {
        if (UMythicItemInstance *Item = Stock->GetItem(StockSlotIndex)) {
            if (const UItemDefinition *Def = Item->GetItemDefinition()) {
                return MythicCurrency::ComputeBuyPrice(Def->Value, Quantity, BuyPriceMultiplier);
            }
        }
    }
    return 0;
}

int32 AMythicVendor::GetSalePriceForItem(const UMythicItemInstance *Item, int32 Quantity) const {
    if (!CurrencyItemDefinition || !Item) {
        return 0; // a vendor with no currency def can't buy from players
    }
    if (const UItemDefinition *Def = Item->GetItemDefinition()) {
        return MythicCurrency::ComputeSalePrice(Def->Value, Quantity, SellRate);
    }
    return 0;
}

int32 AMythicVendor::Server_ExecuteBuy(AMythicPlayerController *Buyer, int32 StockSlotIndex, int32 Quantity) {
    UMythicInventoryComponent *Stock = GetContainerInventory();
    if (!HasAuthority() || !Buyer || !Stock || Quantity <= 0) {
        return 0;
    }

    UMythicItemInstance *StockItem = Stock->GetItem(StockSlotIndex);
    if (!StockItem) {
        return 0;
    }
    UItemDefinition *Def = StockItem->GetItemDefinition();
    if (!Def) {
        return 0;
    }
    const int32 Available = StockItem->GetStacks();
    if (Available <= 0) {
        return 0;
    }

    const int32 UnitPrice = MythicCurrency::ComputeBuyPrice(Def->Value, 1, BuyPriceMultiplier);
    if (UnitPrice <= 0) {
        return 0; // unpriced item — not for sale
    }

    // The goods factory must exist before we mutate anything, so a buy can never charge/remove without delivering.
    UGameInstance *GI = GetGameInstance();
    UMythicLootManagerSubsystem *Loot = GI ? GI->GetSubsystem<UMythicLootManagerSubsystem>() : nullptr;
    if (!Loot) {
        return 0;
    }

    // Find the first of the buyer's inventories that can accept this item type (the delivery target).
    UMythicInventoryComponent *Target = nullptr;
    for (UMythicInventoryComponent *Inv : Buyer->GetAllInventoryComponents()) {
        if (Inv && Inv->CanAcceptItemType(Def->ItemType)) {
            Target = Inv;
            break;
        }
    }
    if (!Target) {
        return 0; // no room / no inventory accepts this type
    }

    // Clamp the requested quantity to what's in stock and what the buyer can afford.
    const int32 BuyerCurrency = SumPlayerCurrency(Buyer);
    const int32 AffordableQty = BuyerCurrency / UnitPrice; // integer division — whole units only
    const int32 SellQty = FMath::Min3(Quantity, Available, AffordableQty);
    if (SellQty <= 0) {
        return 0; // can't afford even one unit, or nothing in stock
    }
    const int32 TotalPrice = UnitPrice * SellQty;

    // Charge the buyer across their inventories (affordability already verified this frame; server is single-threaded
    // so no concurrent spend can race this).
    int32 Remaining = TotalPrice;
    for (UMythicInventoryComponent *Inv : Buyer->GetAllInventoryComponents()) {
        if (Remaining <= 0) {
            break;
        }
        if (Inv) {
            Remaining -= Inv->SpendCurrency(Remaining);
        }
    }

    // Remove from stock and deliver fresh def-based goods (preserving the stock item's level). Fungible goods; per-
    // instance affix/durability preservation on resale is a logged follow-up.
    const int32 GoodsLevel = StockItem->GetItemLevel();
    Stock->ServerRemoveItem(StockItem, SellQty);

    if (UMythicItemInstance *Goods = Loot->Create(Def, SellQty, Buyer, GoodsLevel)) {
        Target->AddItem(Goods, Buyer); // fires the "+N <Item>" pickup callout + drives "collect N" objectives
    }
    return SellQty;
}

int32 AMythicVendor::Server_ExecuteSell(AMythicPlayerController *Seller, UMythicInventoryComponent *PlayerInventory,
                                        int32 PlayerSlotIndex, int32 Quantity) {
    if (!HasAuthority() || !Seller || !PlayerInventory || Quantity <= 0) {
        return 0;
    }
    if (!CurrencyItemDefinition) {
        return 0; // this vendor can't pay (no currency def) — reject rather than fake a payout
    }

    UMythicItemInstance *Item = PlayerInventory->GetItem(PlayerSlotIndex);
    if (!Item) {
        return 0;
    }
    if (!PlayerInventory->CanPlayerTakeFromSlot(PlayerSlotIndex)) {
        return 0; // equipped / bound item — not sellable
    }
    UItemDefinition *Def = Item->GetItemDefinition();
    if (!Def) {
        return 0;
    }
    if (Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
        return 0; // can't sell currency for currency
    }

    const int32 Available = Item->GetStacks();
    const int32 SellQty = FMath::Min(Quantity, Available);
    if (SellQty <= 0) {
        return 0;
    }

    const int32 Proceeds = MythicCurrency::ComputeSalePrice(Def->Value, SellQty, SellRate);
    if (Proceeds <= 0) {
        return 0; // worthless / unsellable — reject (no free item sink)
    }

    UGameInstance *GI = GetGameInstance();
    UMythicLootManagerSubsystem *Loot = GI ? GI->GetSubsystem<UMythicLootManagerSubsystem>() : nullptr;
    if (!Loot) {
        return 0;
    }

    // Remove the sold goods from the seller.
    PlayerInventory->ServerRemoveItem(Item, SellQty);

    // Absorb into stock for resale when enabled and the stock accepts the type; otherwise the goods are consumed (the
    // player is still paid — the vendor "melts them down").
    if (bAbsorbSoldItems) {
        if (UMythicInventoryComponent *Stock = GetContainerInventory()) {
            if (Stock->CanAcceptItemType(Def->ItemType)) {
                if (UMythicItemInstance *StockGoods = Loot->Create(Def, SellQty, nullptr, 0)) {
                    Stock->AddToAnySlot(StockGoods);
                }
            }
        }
    }

    // Pay the proceeds (minted — infinite vendor funds). AddItem inside GrantCurrency fires the "+N <Currency>" callout.
    GrantCurrency(PlayerInventory, Seller, Proceeds, CurrencyItemDefinition, Loot);
    return SellQty;
}
