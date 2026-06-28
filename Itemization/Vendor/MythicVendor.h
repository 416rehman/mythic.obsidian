// Mythic — merchant vendor (the Acquire→vendors verb).
// A vendor IS-A storage container (interaction, opener registry, range gate, replicated stock inventory, save) PLUS a
// server-authoritative currency-gated buy/sell. Pricing is data-driven: items are priced from UItemDefinition::Value,
// marked up by BuyPriceMultiplier on buy and discounted by SellRate on sell; sale proceeds are paid in
// CurrencyItemDefinition. Standard ARPG vendor: infinite vendor funds, finite stock, fungible def-based goods
// (per-instance affix/durability preservation on resale is a logged follow-up). The transaction is INITIATED through
// AMythicPlayerController::ServerVendorBuy/Sell (the client owns its PC); this class executes it server-side once the
// PC has authorized access (opener + range, reusing CanPlayerAccessInventory on the stock inventory).

#pragma once

#include "CoreMinimal.h"
#include "Itemization/Storage/MythicStorageContainer.h"
#include "MythicVendor.generated.h"

class UItemDefinition;
class UMythicItemInstance;
class UMythicInventoryComponent;
class AMythicPlayerController;

UCLASS()
class MYTHIC_API AMythicVendor : public AMythicStorageContainer {
    GENERATED_BODY()

public:
    AMythicVendor();

    // --- Pricing accessors for the vendor UI (BlueprintPure): predict the price client-side so the shop can gray out
    //     unaffordable buys / show the payout. The server independently re-validates on the actual transaction. ---

    // What it costs the player to buy Quantity units of the item in StockSlotIndex (0 if the slot is empty/unpriced).
    UFUNCTION(BlueprintPure, Category = "Vendor")
    int32 GetBuyPriceForSlot(int32 StockSlotIndex, int32 Quantity) const;

    // What this vendor pays the player for Quantity units of Item (0 if worthless/unsellable or no currency def set).
    UFUNCTION(BlueprintPure, Category = "Vendor")
    int32 GetSalePriceForItem(const UMythicItemInstance *Item, int32 Quantity) const;

    // True iff this vendor can pay out (has a currency definition assigned). UI can hide the Sell tab when false.
    UFUNCTION(BlueprintPure, Category = "Vendor")
    bool CanVendorBuyFromPlayers() const { return CurrencyItemDefinition != nullptr; }

    // SERVER: buy Quantity units of StockSlotIndex for Buyer. Clamps to stock, affordability, and the buyer's free
    // space; charges the buyer's currency; delivers the goods. Returns the quantity actually sold (0 on any reject).
    // Authority + access are pre-checked by the calling PC RPC.
    int32 Server_ExecuteBuy(AMythicPlayerController *Buyer, int32 StockSlotIndex, int32 Quantity);

    // SERVER: sell Quantity units from Seller's PlayerSlotIndex in PlayerInventory to this vendor. Clamps to the stack;
    // removes the goods (absorbed into stock when bAbsorbSoldItems + space, else consumed); pays proceeds in
    // CurrencyItemDefinition. Returns the quantity actually sold (0 on any reject).
    int32 Server_ExecuteSell(AMythicPlayerController *Seller, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex, int32 Quantity);

protected:
    // Price multiplier applied to an item's Value when a player BUYS from this vendor (1.0 = at value, >1.0 = margin).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor", meta = (ClampMin = "0.0"))
    float BuyPriceMultiplier = 1.25f;

    // Fraction of an item's Value this vendor PAYS when a player SELLS to it (0.5 = half value). Clamped [0,1] by the
    // sale-price decision so a vendor never pays above value (no buy-low-sell-high money pump at sane knobs).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SellRate = 0.4f;

    // The currency item this vendor mints to pay sale proceeds (the project "gold" definition). MUST be assigned for
    // selling; when null this vendor only sells goods (player sells are rejected — honest, never a fake payout).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor")
    TObjectPtr<UItemDefinition> CurrencyItemDefinition = nullptr;

    // When true (default) items a player sells are added to this vendor's stock so it can resell them; when the stock
    // can't accept them they are consumed and the player is still paid. Pure policy knob.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor")
    bool bAbsorbSoldItems = true;
};
