// Mythic — merchant vendor implementation. See MythicVendor.h for the design contract.

#include "MythicVendor.h"

#include "Mythic.h" // Myth log category
#include "Engine/GameInstance.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicCurrency.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h" // repair target
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/MythicTrade.h" // PlanBuy / PlanSell / PlanRepair pure decisions
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

    // Spend Amount of the player's currency across all their inventories. The caller must have verified affordability
    // this frame (server is single-threaded, so no concurrent spend can race this). Shared by buy + repair.
    void ChargePlayerCurrency(AMythicPlayerController *Player, int32 Amount) {
        if (!Player || Amount <= 0) {
            return;
        }
        int32 Remaining = Amount;
        for (UMythicInventoryComponent *Inv : Player->GetAllInventoryComponents()) {
            if (Remaining <= 0) {
                break;
            }
            if (Inv) {
                Remaining -= Inv->SpendCurrency(Remaining);
            }
        }
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
        // Bound the loop by the chunks the payout actually needs (+ slack), NOT a fixed constant, so a large sale is
        // never silently truncated below the amount owed. int64 keeps the chunk count safe from overflow.
        const int64 MaxChunks = (static_cast<int64>(Amount) + Cap - 1) / Cap + 4;
        int32 Remaining = Amount;
        int64 Guard = 0;
        while (Remaining > 0 && Guard++ < MaxChunks) {
            const int32 Chunk = FMath::Min(Remaining, Cap);
            UMythicItemInstance *Coins = Loot->Create(CurrencyDef, Chunk, Recipient, 0);
            if (!Coins) {
                break;
            }
            Inv->AddItem(Coins, Recipient);
            Remaining -= Chunk;
        }
        if (Remaining > 0) {
            // A payout must never be silently lost (the "never a fake payout" contract). With the guard now sized to the
            // work, this is reachable only on a Create() failure (OOM/GC pathology).
            UE_LOG(Myth, Warning, TEXT("MythicVendor::GrantCurrency could not mint %d of %d currency for the payout"), Remaining, Amount);
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

FMythicTradePlan AMythicVendor::Server_ExecuteBuy(AMythicPlayerController *Buyer, int32 StockSlotIndex, int32 Quantity) {
    FMythicTradePlan Reject; // defaults to {InvalidRequest, 0, 0}
    UMythicInventoryComponent *Stock = GetContainerInventory();
    if (!HasAuthority() || !Buyer || !Stock || Quantity <= 0) {
        return Reject;
    }

    UMythicItemInstance *StockItem = Stock->GetItem(StockSlotIndex);
    if (!StockItem) {
        Reject.Result = EMythicTradeResult::OutOfStock; // empty slot
        return Reject;
    }
    UItemDefinition *Def = StockItem->GetItemDefinition();
    if (!Def) {
        return Reject; // malformed stock entry
    }

    // The goods factory must exist before we mutate anything, so a buy can never charge/remove without delivering.
    UGameInstance *GI = GetGameInstance();
    UMythicLootManagerSubsystem *Loot = GI ? GI->GetSubsystem<UMythicLootManagerSubsystem>() : nullptr;
    if (!Loot) {
        return Reject;
    }

    // Find the first of the buyer's inventories that can accept this item type (the delivery target).
    UMythicInventoryComponent *Target = nullptr;
    for (UMythicInventoryComponent *Inv : Buyer->GetAllInventoryComponents()) {
        if (Inv && Inv->CanAcceptItemType(Def->ItemType)) {
            Target = Inv;
            break;
        }
    }

    // Decide the outcome + quantity from live inputs (pure, unit-tested). Charges nothing on a hard reject.
    const FMythicTradePlan Plan = MythicTrade::PlanBuy(Quantity, Def->Value, BuyPriceMultiplier, StockItem->GetStacks(),
                                                       SumPlayerCurrency(Buyer), Target != nullptr);
    if (Plan.Quantity <= 0) {
        return Plan; // hard reject — nothing mutated; the result carries the reason
    }

    // Pre-create the goods (split across StackSizeMax) BEFORE any irreversible mutation: a creation failure then aborts
    // without charging or decrementing stock, and the delivered total always equals Plan.Quantity even if it exceeds a
    // single stack cap (Loot->Create clamps one instance to StackSizeMax). Mirrors the sell path's pre-mutation guard.
    const int32 GoodsLevel = StockItem->GetItemLevel();
    const int32 StackCap = FMath::Max(1, Def->StackSizeMax);
    TArray<UMythicItemInstance *, TInlineAllocator<8>> Goods;
    for (int32 ToMake = Plan.Quantity; ToMake > 0;) {
        const int32 Chunk = FMath::Min(ToMake, StackCap);
        UMythicItemInstance *G = Loot->Create(Def, Chunk, Buyer, GoodsLevel);
        if (!G) {
            // Creation failed (OOM/GC pathology) — undo the half-built goods and abort before any charge / removal.
            for (UMythicItemInstance *Made : Goods) {
                if (Made) {
                    Made->Destroy();
                }
            }
            return Reject;
        }
        Goods.Add(G);
        ToMake -= Chunk;
    }

    // Goods are ready — now the irreversible mutations. Charge the buyer (affordability already decided this frame).
    ChargePlayerCurrency(Buyer, Plan.TotalPrice);

    // Remove from stock and deliver the pre-created goods (fresh def-based; per-instance affix preservation on resale is
    // a logged follow-up). AddItem fires the "+N <Item>" pickup callout + drives "collect N" objectives.
    Stock->ServerRemoveItem(StockItem, Plan.Quantity);
    for (UMythicItemInstance *G : Goods) {
        Target->AddItem(G, Buyer);
    }
    return Plan;
}

FMythicTradePlan AMythicVendor::Server_ExecuteSell(AMythicPlayerController *Seller, UMythicInventoryComponent *PlayerInventory,
                                                   int32 PlayerSlotIndex, int32 Quantity) {
    FMythicTradePlan Reject;
    if (!HasAuthority() || !Seller || !PlayerInventory || Quantity <= 0) {
        return Reject;
    }

    UMythicItemInstance *Item = PlayerInventory->GetItem(PlayerSlotIndex);
    if (!Item) {
        Reject.Result = EMythicTradeResult::OutOfStock; // nothing in that slot
        return Reject;
    }
    UItemDefinition *Def = Item->GetItemDefinition();
    if (!Def) {
        return Reject; // malformed item
    }

    // Decide the outcome (pure). Gated inputs: stacks, value, sell rate, vendor-can-pay, take-rule, currency guard.
    const bool bIsCurrency = Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY);
    const FMythicTradePlan Plan = MythicTrade::PlanSell(Quantity, Item->GetStacks(), Def->Value, SellRate,
                                                        CurrencyItemDefinition != nullptr,
                                                        PlayerInventory->CanPlayerTakeFromSlot(PlayerSlotIndex), bIsCurrency);
    if (Plan.Quantity <= 0) {
        return Plan; // hard reject — nothing mutated
    }

    UGameInstance *GI = GetGameInstance();
    UMythicLootManagerSubsystem *Loot = GI ? GI->GetSubsystem<UMythicLootManagerSubsystem>() : nullptr;
    if (!Loot) {
        return Reject; // can't mint proceeds — abort BEFORE removing the goods
    }

    // Remove the sold goods from the seller.
    PlayerInventory->ServerRemoveItem(Item, Plan.Quantity);

    // Absorb into stock for resale when enabled and the stock accepts the type; otherwise the goods are consumed (the
    // player is still paid — the vendor "melts them down").
    if (bAbsorbSoldItems) {
        if (UMythicInventoryComponent *Stock = GetContainerInventory()) {
            if (Stock->CanAcceptItemType(Def->ItemType)) {
                if (UMythicItemInstance *StockGoods = Loot->Create(Def, Plan.Quantity, nullptr, 0)) {
                    Stock->AddToAnySlot(StockGoods);
                }
            }
        }
    }

    // Pay the proceeds (minted — infinite vendor funds). AddItem inside GrantCurrency fires the "+N <Currency>" callout.
    GrantCurrency(PlayerInventory, Seller, Plan.TotalPrice, CurrencyItemDefinition, Loot);
    return Plan;
}

int32 AMythicVendor::GetRepairCostForItem(UMythicItemInstance *Item) const {
    if (!bCanRepair || !Item) {
        return 0;
    }
    const UItemDefinition *Def = Item->GetItemDefinition();
    if (!Def) {
        return 0;
    }
    if (const UDurabilityFragment *Dura = Item->GetFragment<UDurabilityFragment>()) {
        return MythicCurrency::ComputeRepairCost(Dura->GetCurrentDurability(), Dura->GetMaxDurability(), Def->Value, RepairCostFraction);
    }
    return 0;
}

FMythicTradePlan AMythicVendor::Server_ExecuteRepair(AMythicPlayerController *Payer, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex) {
    FMythicTradePlan Reject;
    if (!HasAuthority() || !Payer || !PlayerInventory || !bCanRepair) {
        return Reject; // InvalidRequest (incl. a vendor that doesn't offer repair — the UI shouldn't surface it)
    }

    UMythicItemInstance *Item = PlayerInventory->GetItem(PlayerSlotIndex);
    if (!Item) {
        Reject.Result = EMythicTradeResult::NothingToRepair;
        return Reject;
    }
    const UItemDefinition *Def = Item->GetItemDefinition();
    const UDurabilityFragment *Dura = Item->GetFragment<UDurabilityFragment>();
    if (!Def || !Dura) {
        Reject.Result = EMythicTradeResult::NothingToRepair; // the item has no durability to repair
        return Reject;
    }

    const FMythicTradePlan Plan = MythicTrade::PlanRepair(Dura->GetCurrentDurability(), Dura->GetMaxDurability(),
                                                          Def->Value, RepairCostFraction, SumPlayerCurrency(Payer));
    if (Plan.Result != EMythicTradeResult::Success) {
        return Plan; // NothingToRepair / InsufficientFunds — nothing charged or mutated
    }

    // Charge then restore (affordability decided this frame; one synchronous server call, no async gap). const_cast
    // mirrors the wear chokepoint — the durability fragment's only writers are the authoritative wear/repair edges.
    ChargePlayerCurrency(Payer, Plan.TotalPrice);
    const_cast<UDurabilityFragment *>(Dura)->ServerRepair(Plan.Quantity); // restores Missing points, clears broken, replicates
    return Plan;
}
