
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "MVVMViewModelBase.h"
#include "VendorViewModels.generated.h"

class AMythicVendor;
class AMythicPlayerController;
class UTexture2D;
class UMythicInventoryComponent;

UCLASS(BlueprintType)
class MYTHIC_API UVendorStockLineVM : public UMVVMViewModelBase {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    TObjectPtr<UTexture2D> StockIcon;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FText StockName;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    int32 Quantity = 0;
    /**
     * Pre-formatted stack count for the row. Blueprint exposes no int-to-text node here, so the VM does it.
     * Named StockQtyText, not QuantityText: the Blueprint DSL resolves a node by its BARE function name and
     * ignores the class, so a getter sharing a name with any other class (WBPInventoryItemSlot::GetQuantityText)
     * can silently bind to the wrong function. Every getter on these view models must be globally unique.
     */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FText StockQtyText;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    int32 UnitPrice = 0;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FText PriceText;
    /** False greys the buy control. The row still SHOWS the price — hiding it would hide the goal. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    bool Affordable = false;
    /** How much more coin the player needs. 0 when affordable. Lets the row say "need 12 more" instead of "no". */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    int32 Shortfall = 0;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FLinearColor RarityColor = FLinearColor::White;
    /** The vendor stock slot this line came from — pass straight to ServerVendorBuy. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    int32 SlotIndex = INDEX_NONE;
    /** "All (44)" for the whole-stack button. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FText BuyAllLabel;
    /** False for a single item, so the row can hide a stack button that would do nothing different. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    bool BuyHasStack = false;

public:
    void SetStockIcon(UTexture2D *In);
    UTexture2D *GetStockIcon() const { return StockIcon; }
    void SetStockName(FText In);
    FText GetStockName() const { return StockName; }
    void SetStockQtyText(FText In);
    FText GetStockQtyText() const { return StockQtyText; }
    void SetQuantity(int32 In);
    int32 GetQuantity() const { return Quantity; }
    void SetUnitPrice(int32 In);
    int32 GetUnitPrice() const { return UnitPrice; }
    void SetPriceText(FText In);
    FText GetPriceText() const { return PriceText; }
    void SetAffordable(bool In);
    bool GetAffordable() const { return Affordable; }
    void SetShortfall(int32 In);
    int32 GetShortfall() const { return Shortfall; }
    void SetRarityColor(FLinearColor In);
    FLinearColor GetRarityColor() const { return RarityColor; }
    void SetSlotIndex(int32 In);
    int32 GetSlotIndex() const { return SlotIndex; }
    void SetBuyAllLabel(FText In);
    FText GetBuyAllLabel() const { return BuyAllLabel; }
    void SetBuyHasStack(bool In);
    bool GetBuyHasStack() const { return BuyHasStack; }

    /** The vendor this line belongs to. Carried on the line so a list row needs nothing but its item object. */
    UPROPERTY(BlueprintReadOnly, Category = "Vendor")
    TWeakObjectPtr<AMythicVendor> Vendor;

    /** The controller doing the buying. Weak: the screen outlives no player, but a stale line must not resurrect one. */
    UPROPERTY(BlueprintReadOnly, Category = "Vendor")
    TWeakObjectPtr<AMythicPlayerController> Patron;

    /**
     * Ask the server to buy Quantity of this line. One Blueprint node for the whole transaction — the row's Buy
     * button calls this and nothing else. Server-authoritative: this only sends the RPC, it never predicts the
     * result, so a client cannot show itself an item it did not get.
     */
    UFUNCTION(BlueprintCallable, Category = "Vendor")
    void RequestBuy(int32 Count = 1);

    /**
     * Buy the whole stack. The server clamps to whatever the player can actually afford and reports a partial,
     * so this reads as "buy as many as I can" rather than failing outright on a short purse.
     */
    UFUNCTION(BlueprintCallable, Category = "Vendor")
    void RequestBuyAll();
};

UCLASS(BlueprintType)
class MYTHIC_API UVendorSellLineVM : public UMVVMViewModelBase {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    TObjectPtr<UTexture2D> SellIcon;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FText SellName;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FText SellQtyText;
    /** What the vendor pays for ONE of these, after reputation, scarcity and haggling. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FText SellPriceText;
    /** False when this vendor pays nothing for it — contraband it refuses, or a worthless item. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    bool Sellable = false;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FLinearColor SellRarityColor = FLinearColor::White;
    /** "All (44)" for the whole-stack button. Without it a stack of 44 wood is 44 clicks. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FText SellAllLabel;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    bool SellHasStack = false;

public:
    void SetSellIcon(UTexture2D *In);
    UTexture2D *GetSellIcon() const { return SellIcon; }
    void SetSellName(FText In);
    FText GetSellName() const { return SellName; }
    void SetSellQtyText(FText In);
    FText GetSellQtyText() const { return SellQtyText; }
    void SetSellPriceText(FText In);
    FText GetSellPriceText() const { return SellPriceText; }
    void SetSellable(bool In);
    bool GetSellable() const { return Sellable; }
    void SetSellRarityColor(FLinearColor In);
    FLinearColor GetSellRarityColor() const { return SellRarityColor; }
    void SetSellAllLabel(FText In);
    FText GetSellAllLabel() const { return SellAllLabel; }
    void SetSellHasStack(bool In);
    bool GetSellHasStack() const { return SellHasStack; }

    UPROPERTY(BlueprintReadOnly, Category = "Vendor")
    TWeakObjectPtr<AMythicVendor> Vendor;
    UPROPERTY(BlueprintReadOnly, Category = "Vendor")
    TWeakObjectPtr<AMythicPlayerController> Patron;
    /** The player inventory this item lives in — the sell RPC is validated against the player's own bags. */
    UPROPERTY(BlueprintReadOnly, Category = "Vendor")
    TWeakObjectPtr<UMythicInventoryComponent> SourceInventory;
    UPROPERTY(BlueprintReadOnly, Category = "Vendor")
    int32 SourceSlotIndex = INDEX_NONE;

    /** Sell Count of this line. Sends the RPC only; the server decides and the screen refreshes from the result. */
    UFUNCTION(BlueprintCallable, Category = "Vendor")
    void RequestSell(int32 Count = 1);

    /** Sell the whole stack in one action. */
    UFUNCTION(BlueprintCallable, Category = "Vendor")
    void RequestSellAll();

    /** How many are in this stack — what RequestSellAll sends. */
    UPROPERTY(BlueprintReadOnly, Category = "Vendor")
    int32 StackCount = 1;
};

UCLASS(BlueprintType)
class MYTHIC_API UVendorMenuVM : public UMVVMViewModelBase {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FText VendorName;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    int32 Wallet = 0;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FText WalletText;

    /**
     * "Friendly — 10% off" / "Hostile — 35% surcharge" / empty when this vendor does not price on reputation.
     * Without this the whole reputation system is invisible: a player robbed the village and prices went up, and
     * nothing on screen ever said why. This line is what turns a hidden multiplier into a consequence.
     */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FText StandingText;
    /** Tints the standing line: green when standing helps, red when it costs, neutral otherwise. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    FLinearColor StandingColor = FLinearColor::White;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    bool CanSell = false;
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    bool CanRepair = false;
    /** True when the vendor has no stock at all, so the screen can show an empty state instead of a blank box. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    bool StockEmpty = true;

    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    TArray<TObjectPtr<UVendorStockLineVM>> StockLines;

    /** The player's own sellable goods, priced by this vendor. Empty when the vendor does not buy from players. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    TArray<TObjectPtr<UVendorSellLineVM>> SellLines;

    /** True when the player has nothing this vendor will take, so the screen can explain the empty pane. */
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, meta = (AllowPrivateAccess))
    bool SellEmpty = true;

public:
    void SetVendorName(FText In);
    FText GetVendorName() const { return VendorName; }
    void SetWallet(int32 In);
    int32 GetWallet() const { return Wallet; }
    void SetWalletText(FText In);
    FText GetWalletText() const { return WalletText; }
    void SetStandingText(FText In);
    FText GetStandingText() const { return StandingText; }
    void SetStandingColor(FLinearColor In);
    FLinearColor GetStandingColor() const { return StandingColor; }
    void SetCanSell(bool In);
    bool GetCanSell() const { return CanSell; }
    void SetCanRepair(bool In);
    bool GetCanRepair() const { return CanRepair; }
    void SetStockEmpty(bool In);
    bool GetStockEmpty() const { return StockEmpty; }
    void SetStockLines(TArray<TObjectPtr<UVendorStockLineVM>> In);
    TArray<TObjectPtr<UVendorStockLineVM>> GetStockLines() const { return StockLines; }
    void SetSellLines(TArray<TObjectPtr<UVendorSellLineVM>> In);
    TArray<TObjectPtr<UVendorSellLineVM>> GetSellLines() const { return SellLines; }
    void SetSellEmpty(bool In);
    bool GetSellEmpty() const { return SellEmpty; }

    /**
     * Rebuild every field from live vendor + player state. Safe to call every time the screen opens or the stock
     * inventory fires OnSlotUpdated. Reuses existing line objects where the slot count is unchanged so a refresh
     * does not churn UObjects while the player is scrolling.
     */
    UFUNCTION(BlueprintCallable, Category = "Vendor")
    void RefreshFromVendor(AMythicVendor *Vendor, AMythicPlayerController *Patron);

private:
    void RebuildSellLines(AMythicVendor *Vendor, AMythicPlayerController *Patron);

public:

    /**
     * Typed factory. Blueprint's generic "Construct Object from Class" returns a bare UObject that then needs a
     * cast at every call site; this hands back the concrete type, so opening a trade screen is two nodes instead
     * of four. Outer the VM to the widget that owns it so it dies with the screen.
     */
    UFUNCTION(BlueprintCallable, Category = "Vendor", meta = (DefaultToSelf = "Owner"))
    static UVendorMenuVM *CreateVendorMenuVM(UObject *Owner);
};

UCLASS(Abstract, BlueprintType)
class MYTHIC_API UMythicVendorStockRowBase : public UUserWidget, public IUserObjectListEntry {
    GENERATED_BODY()

public:
    /** The line this row currently shows. Valid from OnLineAssigned onward. */
    UPROPERTY(BlueprintReadOnly, Category = "Vendor")
    TObjectPtr<UVendorStockLineVM> Line;

    /** Fired when the list hands this row a line. Bind the row's visuals here. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Vendor")
    void OnLineAssigned(UVendorStockLineVM *InLine);

protected:
    virtual void NativeOnListItemObjectSet(UObject *ListItemObject) override;
};

UCLASS(Abstract, BlueprintType)
class MYTHIC_API UMythicVendorSellRowBase : public UUserWidget, public IUserObjectListEntry {
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Vendor")
    TObjectPtr<UVendorSellLineVM> SellLine;

    /** Fired when the list hands this row a sell line. Bind the row's visuals here. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Vendor")
    void OnSellLineAssigned(UVendorSellLineVM *InLine);

protected:
    virtual void NativeOnListItemObjectSet(UObject *ListItemObject) override;
};
