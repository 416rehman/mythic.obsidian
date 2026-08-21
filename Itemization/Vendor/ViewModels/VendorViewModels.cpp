
#include "VendorViewModels.h"

#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Vendor/MythicVendor.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Player/MythicFactionStandingComponent.h"
#include "Player/MythicPlayerController.h"

#define VENDOR_VM_SETTER(Class, Type, Field)                     \
    void Class::Set##Field(Type In) {                            \
        if (UE_MVVM_SET_PROPERTY_VALUE(Field, In)) {             \
            UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Field);        \
        }                                                        \
    }

VENDOR_VM_SETTER(UVendorStockLineVM, UTexture2D *, StockIcon)
VENDOR_VM_SETTER(UVendorStockLineVM, FText, BuyAllLabel)
VENDOR_VM_SETTER(UVendorStockLineVM, bool, BuyHasStack)
VENDOR_VM_SETTER(UVendorStockLineVM, FText, StockName)
VENDOR_VM_SETTER(UVendorStockLineVM, FText, StockQtyText)
VENDOR_VM_SETTER(UVendorStockLineVM, int32, Quantity)
VENDOR_VM_SETTER(UVendorStockLineVM, int32, UnitPrice)
VENDOR_VM_SETTER(UVendorStockLineVM, FText, PriceText)
VENDOR_VM_SETTER(UVendorStockLineVM, bool, Affordable)
VENDOR_VM_SETTER(UVendorStockLineVM, int32, Shortfall)
VENDOR_VM_SETTER(UVendorStockLineVM, FLinearColor, RarityColor)
VENDOR_VM_SETTER(UVendorStockLineVM, int32, SlotIndex)

VENDOR_VM_SETTER(UVendorSellLineVM, UTexture2D *, SellIcon)
VENDOR_VM_SETTER(UVendorSellLineVM, FText, SellName)
VENDOR_VM_SETTER(UVendorSellLineVM, FText, SellQtyText)
VENDOR_VM_SETTER(UVendorSellLineVM, FText, SellPriceText)
VENDOR_VM_SETTER(UVendorSellLineVM, bool, Sellable)
VENDOR_VM_SETTER(UVendorSellLineVM, FLinearColor, SellRarityColor)
VENDOR_VM_SETTER(UVendorSellLineVM, FText, SellAllLabel)
VENDOR_VM_SETTER(UVendorSellLineVM, bool, SellHasStack)

VENDOR_VM_SETTER(UVendorMenuVM, bool, SellEmpty)
VENDOR_VM_SETTER(UVendorMenuVM, FText, VendorName)
VENDOR_VM_SETTER(UVendorMenuVM, int32, Wallet)
VENDOR_VM_SETTER(UVendorMenuVM, FText, WalletText)
VENDOR_VM_SETTER(UVendorMenuVM, FText, StandingText)
VENDOR_VM_SETTER(UVendorMenuVM, FLinearColor, StandingColor)
VENDOR_VM_SETTER(UVendorMenuVM, bool, CanSell)
VENDOR_VM_SETTER(UVendorMenuVM, bool, CanRepair)
VENDOR_VM_SETTER(UVendorMenuVM, bool, StockEmpty)

void UVendorMenuVM::SetStockLines(TArray<TObjectPtr<UVendorStockLineVM>> In) {
    StockLines = MoveTemp(In);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StockLines);
}

void UVendorMenuVM::SetSellLines(TArray<TObjectPtr<UVendorSellLineVM>> In) {
    SellLines = MoveTemp(In);
    UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SellLines);
}

#undef VENDOR_VM_SETTER

void UVendorSellLineVM::RequestSellAll() {
    RequestSell(StackCount);
}

void UVendorSellLineVM::RequestSell(int32 Count) {
    AMythicVendor *V = Vendor.Get();
    AMythicPlayerController *PC = Patron.Get();
    UMythicInventoryComponent *Inv = SourceInventory.Get();
    if (!V || !PC || !Inv || SourceSlotIndex < 0 || Count <= 0) {
        return;
    }
    PC->ServerVendorSell(V, Inv, SourceSlotIndex, Count);
}

void UVendorStockLineVM::RequestBuyAll() {
    RequestBuy(Quantity);
}

void UVendorStockLineVM::RequestBuy(int32 Count) {
    AMythicVendor *V = Vendor.Get();
    AMythicPlayerController *PC = Patron.Get();
    if (!V || !PC || SlotIndex < 0 || Count <= 0) {
        return;
    }
    PC->ServerVendorBuy(V, SlotIndex, Count);
}

void UMythicVendorStockRowBase::NativeOnListItemObjectSet(UObject *ListItemObject) {
    Line = Cast<UVendorStockLineVM>(ListItemObject);
    OnLineAssigned(Line);
}

void UMythicVendorSellRowBase::NativeOnListItemObjectSet(UObject *ListItemObject) {
    SellLine = Cast<UVendorSellLineVM>(ListItemObject);
    OnSellLineAssigned(SellLine);
}

namespace {
    const FLinearColor StandingGood(0.35f, 0.80f, 0.40f, 1.0f);
    const FLinearColor StandingBad(0.85f, 0.30f, 0.28f, 1.0f);
    const FLinearColor StandingPlain(0.75f, 0.72f, 0.65f, 1.0f);
}

UVendorMenuVM *UVendorMenuVM::CreateVendorMenuVM(UObject *Owner) {
    return NewObject<UVendorMenuVM>(Owner ? Owner : GetTransientPackage());
}

void UVendorMenuVM::RefreshFromVendor(AMythicVendor *Vendor, AMythicPlayerController *Patron) {
    if (!Vendor) {
        SetStockLines({});
        SetStockEmpty(true);
        return;
    }

    SetVendorName(Vendor->VendorDisplayName);

    const int32 Purse = Patron ? Patron->GetCarriedCurrency() : 0;
    SetWallet(Purse);
    SetWalletText(FText::AsNumber(Purse));
    SetCanSell(Vendor->CanVendorBuyFromPlayers());
    SetCanRepair(Vendor->CanVendorRepair());

    if (Vendor->HasReputationPricing()) {
        switch (Vendor->ResolvePatronTier(Patron)) {
        case EMythicStandingTier::Friendly:
            SetStandingText(NSLOCTEXT("Vendor", "StandingFriendly", "Friendly — you pay less, and they pay you more"));
            SetStandingColor(StandingGood);
            break;
        case EMythicStandingTier::Hostile:
            SetStandingText(NSLOCTEXT("Vendor", "StandingHostile", "Hostile — you pay more, and they pay you less"));
            SetStandingColor(StandingBad);
            break;
        default:
            SetStandingText(NSLOCTEXT("Vendor", "StandingNeutral", "Neutral — standard prices"));
            SetStandingColor(StandingPlain);
            break;
        }
    }
    else {
        SetStandingText(FText::GetEmpty());
        SetStandingColor(StandingPlain);
    }

    UMythicInventoryComponent *Stock = Vendor->GetContainerInventory();
    if (!Stock) {
        SetStockLines({});
        SetStockEmpty(true);
        return;
    }

    TArray<TObjectPtr<UVendorStockLineVM>> Lines = StockLines;
    int32 Written = 0;

    const TArray<FMythicInventorySlotEntry> &Slots = Stock->GetAllSlots();
    for (int32 i = 0; i < Slots.Num(); ++i) {
        UMythicItemInstance *Item = Slots[i].SlottedItemInstance;
        if (!Item) {
            continue;
        }
        const UItemDefinition *Def = Item->GetItemDefinition();
        if (!Def) {
            continue;
        }

        UVendorStockLineVM *Line = Lines.IsValidIndex(Written) ? Lines[Written].Get() : nullptr;
        if (!Line) {
            Line = NewObject<UVendorStockLineVM>(this);
            if (Lines.IsValidIndex(Written)) {
                Lines[Written] = Line;
            }
            else {
                Lines.Add(Line);
            }
        }

        const int32 Price = Vendor->GetBuyPriceForSlot(i, 1, Patron);
        const bool bAfford = Purse >= Price;

        Line->Vendor = Vendor;
        Line->Patron = Patron;
        Line->SetSlotIndex(i);
        Line->SetStockIcon(Def->Icon2d.LoadSynchronous());
        Line->SetStockName(Def->Name);
        Line->SetStockQtyText(FText::AsNumber(Item->GetStacks()));
        Line->SetQuantity(Item->GetStacks());
        Line->SetUnitPrice(Price);
        Line->SetPriceText(FText::AsNumber(Price));
        Line->SetAffordable(bAfford);
        Line->SetShortfall(bAfford ? 0 : Price - Purse);
        Line->SetRarityColor(UItemDefinition::GetRarityColor(Def->Rarity));
        const int32 Stacks = Item->GetStacks();
        Line->SetBuyHasStack(Stacks > 1);
        Line->SetBuyAllLabel(FText::Format(NSLOCTEXT("Vendor", "AllOf", "All ({0})"), FText::AsNumber(Stacks)));
        ++Written;
    }

    if (Lines.Num() > Written) {
        Lines.SetNum(Written);
    }
    SetStockLines(Lines);
    SetStockEmpty(Written == 0);

    RebuildSellLines(Vendor, Patron);
}

void UVendorMenuVM::RebuildSellLines(AMythicVendor *Vendor, AMythicPlayerController *Patron) {
    if (!Vendor || !Patron || !Vendor->CanVendorBuyFromPlayers()) {
        SetSellLines({});
        SetSellEmpty(true);
        return;
    }

    TArray<TObjectPtr<UVendorSellLineVM>> Lines = SellLines;
    int32 Written = 0;

    for (UMythicInventoryComponent *Inv : Patron->GetAllInventoryComponents()) {
        if (!Inv) {
            continue;
        }
        const TArray<FMythicInventorySlotEntry> &Slots = Inv->GetAllSlots();
        for (int32 i = 0; i < Slots.Num(); ++i) {
            const FMythicInventorySlotEntry &Slot = Slots[i];
            if (Slot.bEquipmentSlot || !Slot.bCanPlayerTake) {
                continue;
            }
            UMythicItemInstance *Item = Slot.SlottedItemInstance;
            if (!Item) {
                continue;
            }
            const UItemDefinition *Def = Item->GetItemDefinition();
            if (!Def) {
                continue;
            }
            if (Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
                continue;
            }

            UVendorSellLineVM *Line = Lines.IsValidIndex(Written) ? Lines[Written].Get() : nullptr;
            if (!Line) {
                Line = NewObject<UVendorSellLineVM>(this);
                if (Lines.IsValidIndex(Written)) {
                    Lines[Written] = Line;
                }
                else {
                    Lines.Add(Line);
                }
            }

            const int32 Price = Vendor->GetSalePriceForItem(Item, 1, Patron);

            Line->Vendor = Vendor;
            Line->Patron = Patron;
            Line->SourceInventory = Inv;
            Line->SourceSlotIndex = i;
            Line->SetSellIcon(Def->Icon2d.LoadSynchronous());
            Line->SetSellName(Def->Name);
            Line->SetSellQtyText(FText::AsNumber(Item->GetStacks()));
            Line->SetSellPriceText(FText::AsNumber(Price));
            Line->SetSellable(Price > 0);
            Line->SetSellRarityColor(UItemDefinition::GetRarityColor(Def->Rarity));
            const int32 Stacks = Item->GetStacks();
            Line->StackCount = FMath::Max(1, Stacks);
            Line->SetSellHasStack(Stacks > 1);
            Line->SetSellAllLabel(FText::Format(NSLOCTEXT("Vendor", "AllOf", "All ({0})"), FText::AsNumber(Stacks)));
            ++Written;
        }
    }

    if (Lines.Num() > Written) {
        Lines.SetNum(Written);
    }
    SetSellLines(Lines);
    SetSellEmpty(Written == 0);
}
