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
#include "Player/MythicPlayerState.h"                 // resolve the patron's standing component
#include "Player/MythicFactionStandingComponent.h"    // EMythicStandingTier + TierForStanding + GetStanding

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

float AMythicVendor::ComputeReputationAdjustedMultiplier(float BaseMultiplier, EMythicStandingTier Tier, float HostileMult, float NeutralMult, float FriendlyMult) {
    float TierMult;
    switch (Tier) {
    case EMythicStandingTier::Hostile:
        TierMult = HostileMult;
        break;
    case EMythicStandingTier::Friendly:
        TierMult = FriendlyMult;
        break;
    case EMythicStandingTier::Neutral:
    default:
        TierMult = NeutralMult;
        break;
    }
    return BaseMultiplier * FMath::Max(0.0f, TierMult);
}

float AMythicVendor::EnforceBuyAboveSellRate(float EffBuyMultiplier, float EffSellRate) {
    // The sell side pays floor(Value × clamp(EffSellRate,0,1)); flooring the buy multiplier at that same clamped rate
    // guarantees ceil(Value × BuyPriceMultiplier × EffBuyMultiplier) ≥ the sell payout, so buy ≥ sell (no money pump).
    return FMath::Max(EffBuyMultiplier, FMath::Clamp(EffSellRate, 0.0f, 1.0f));
}

EMythicStandingTier AMythicVendor::ResolvePatronTier(AMythicPlayerController *Patron) const {
    if (!Patron || !VendorFaction.IsValid()) {
        return EMythicStandingTier::Neutral; // no patron / unpriced vendor → reputation pricing is a no-op
    }
    const AMythicPlayerState *PS = Patron->GetPlayerState<AMythicPlayerState>();
    const UMythicFactionStandingComponent *Standing = PS ? PS->GetFactionStanding() : nullptr;
    if (!Standing) {
        return EMythicStandingTier::Neutral;
    }
    return Standing->TierForStanding(Standing->GetStanding(VendorFaction));
}

float AMythicVendor::ResolveEffectiveBuyMultiplier(AMythicPlayerController *Buyer) const {
    const EMythicStandingTier Tier = ResolvePatronTier(Buyer);
    const float EffBuy = ComputeReputationAdjustedMultiplier(BuyPriceMultiplier, Tier,
        ReputationPricing.HostileBuyMultiplier, ReputationPricing.NeutralBuyMultiplier, ReputationPricing.FriendlyBuyMultiplier);
    const float EffSell = ComputeReputationAdjustedMultiplier(SellRate, Tier,
        ReputationPricing.HostileSellMultiplier, ReputationPricing.NeutralSellMultiplier, ReputationPricing.FriendlySellMultiplier);
    return EnforceBuyAboveSellRate(EffBuy, EffSell); // same-tier buy>sell guard
}

float AMythicVendor::ResolveEffectiveSellRate(AMythicPlayerController *Seller) const {
    return ComputeReputationAdjustedMultiplier(SellRate, ResolvePatronTier(Seller),
        ReputationPricing.HostileSellMultiplier, ReputationPricing.NeutralSellMultiplier, ReputationPricing.FriendlySellMultiplier);
}

int32 AMythicVendor::GetBuyPriceForSlot(int32 StockSlotIndex, int32 Quantity, AMythicPlayerController *Buyer) const {
    if (UMythicInventoryComponent *Stock = GetContainerInventory()) {
        if (UMythicItemInstance *Item = Stock->GetItem(StockSlotIndex)) {
            if (const UItemDefinition *Def = Item->GetItemDefinition()) {
                return MythicCurrency::ComputeBuyPrice(Def->Value, Quantity, ResolveEffectiveBuyMultiplier(Buyer));
            }
        }
    }
    return 0;
}

int32 AMythicVendor::GetSalePriceForItem(const UMythicItemInstance *Item, int32 Quantity, AMythicPlayerController *Seller) const {
    if (!CurrencyItemDefinition || !Item) {
        return 0; // a vendor with no currency def can't buy from players
    }
    if (const UItemDefinition *Def = Item->GetItemDefinition()) {
        return MythicCurrency::ComputeSalePrice(Def->Value, Quantity, ResolveEffectiveSellRate(Seller));
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

    // Fold the buyer's reputation tier into the buy markup (server-authoritative; the display accessor mirrors this).
    // ResolveEffectiveBuyMultiplier also floors the result above the same-tier sell rate (no buy-low/resell money pump).
    const float EffBuyMult = ResolveEffectiveBuyMultiplier(Buyer);

    // Decide the outcome + quantity from live inputs (pure, unit-tested). Charges nothing on a hard reject.
    const FMythicTradePlan Plan = MythicTrade::PlanBuy(Quantity, Def->Value, EffBuyMult, StockItem->GetStacks(),
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

    // Fold the seller's reputation tier into the sell rate (server-authoritative). PlanSell still clamps the effective
    // rate to [0,1], so a friendly bonus never pays above item value (no money pump).
    const float EffSellRate = ResolveEffectiveSellRate(Seller);

    // Decide the outcome (pure). Gated inputs: stacks, value, sell rate, vendor-can-pay, take-rule, currency guard.
    const bool bIsCurrency = Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY);
    const FMythicTradePlan Plan = MythicTrade::PlanSell(Quantity, Item->GetStacks(), Def->Value, EffSellRate,
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
    if (Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
        Reject.Result = EMythicTradeResult::NothingToRepair; // currency is never a repair target (charging would free it)
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

FMythicTradePlan AMythicVendor::Server_ExecuteRepairAll(AMythicPlayerController *Payer, UMythicInventoryComponent *PlayerInventory) {
    FMythicTradePlan Reject;
    if (!HasAuthority() || !Payer || !PlayerInventory || !bCanRepair) {
        return Reject; // InvalidRequest (incl. a vendor that doesn't repair)
    }

    // Gather damaged, repairable items + their full-repair cost (mirrors the single-item GetRepairCostForItem).
    struct FRepairCandidate {
        const UDurabilityFragment *Dura = nullptr;
        int32 RestoreAmount = 0;
        int32 Cost = 0;
    };
    TArray<FRepairCandidate> Candidates;
    const int32 NumSlots = PlayerInventory->GetNumSlots();
    for (int32 i = 0; i < NumSlots; ++i) {
        UMythicItemInstance *Item = PlayerInventory->GetItem(i);
        if (!Item) {
            continue;
        }
        const UItemDefinition *Def = Item->GetItemDefinition();
        const UDurabilityFragment *Dura = Item->GetFragment<UDurabilityFragment>();
        if (!Def || !Dura) {
            continue;
        }
        if (Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
            continue; // never treat spendable currency as a repair target — the charge would destroy this very instance,
                      // freeing the fragment we'd then ServerRepair (mirrors the sell path's currency guard)
        }
        const int32 Cur = Dura->GetCurrentDurability();
        const int32 Max = Dura->GetMaxDurability();
        if (Max <= 0 || Cur >= Max) {
            continue; // no durability concept / already full
        }
        const int32 Cost = MythicCurrency::ComputeRepairCost(Cur, Max, Def->Value, RepairCostFraction);
        if (Cost <= 0) {
            continue; // valueless/free — left to the single-item path; the batch is for priced repairs
        }
        Candidates.Add({Dura, Max - Cur, Cost});
    }
    if (Candidates.Num() == 0) {
        Reject.Result = EMythicTradeResult::NothingToRepair;
        return Reject;
    }

    // Cheapest-first so a limited budget repairs the MOST items; the pure plan decides how many are affordable.
    Candidates.Sort([](const FRepairCandidate &A, const FRepairCandidate &B) { return A.Cost < B.Cost; });
    TArray<int32> CostsAscending;
    CostsAscending.Reserve(Candidates.Num());
    for (const FRepairCandidate &C : Candidates) {
        CostsAscending.Add(C.Cost);
    }
    const FMythicTradePlan Plan = MythicTrade::ComputeRepairAllPlan(CostsAscending, SumPlayerCurrency(Payer));
    if (Plan.Result != EMythicTradeResult::Success) {
        return Plan; // InsufficientFunds (can't afford even the cheapest) — nothing charged or mutated
    }

    // Charge the total once, then restore the cheapest Plan.Quantity items (synchronous; affordability decided this frame).
    ChargePlayerCurrency(Payer, Plan.TotalPrice);
    for (int32 i = 0; i < Plan.Quantity && i < Candidates.Num(); ++i) {
        const_cast<UDurabilityFragment *>(Candidates[i].Dura)->ServerRepair(Candidates[i].RestoreAmount);
    }
    return Plan;
}
