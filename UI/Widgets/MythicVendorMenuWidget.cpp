// Copyright Stellar Games. All Rights Reserved.

#include "MythicVendorMenuWidget.h"

#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "Itemization/Vendor/MythicVendor.h"
#include "Itemization/Vendor/ViewModels/VendorViewModels.h"
#include "Player/MythicPlayerController.h"

void UMythicVendorMenuWidget::NativeConstruct() {
    Super::NativeConstruct();
    OpenForActiveVendor();
}

bool UMythicVendorMenuWidget::OpenForActiveVendor() {
    AMythicPlayerController *PC = GetOwningPlayer<AMythicPlayerController>();
    if (!PC) {
        return false;
    }
    AMythicVendor *Vendor = Cast<AMythicVendor>(PC->ActiveContainer.Get());
    if (!Vendor) {
        return false;
    }
    OpenForVendor(Vendor, PC);
    return true;
}

void UMythicVendorMenuWidget::OpenForVendor(AMythicVendor *Vendor, AMythicPlayerController *Patron) {
    if (!Vendor || !Patron) {
        return;
    }
    UnbindInventories();
    BoundVendor = Vendor;
    BoundPatron = Patron;

    if (!ViewModel) {
        ViewModel = UVendorMenuVM::CreateVendorMenuVM(this);

        const INotifyFieldValueChanged::FFieldValueChangedDelegate Delegate =
            INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UMythicVendorMenuWidget::HandleFieldChanged);

        using FDesc = UVendorMenuVM::FFieldNotificationClassDescriptor;
        ViewModel->AddFieldValueChangedDelegate(FDesc::VendorName, Delegate);
        ViewModel->AddFieldValueChangedDelegate(FDesc::WalletText, Delegate);
        ViewModel->AddFieldValueChangedDelegate(FDesc::StandingText, Delegate);
        ViewModel->AddFieldValueChangedDelegate(FDesc::StockLines, Delegate);
        ViewModel->AddFieldValueChangedDelegate(FDesc::SellLines, Delegate);
        ViewModel->AddFieldValueChangedDelegate(FDesc::StockEmpty, Delegate);
        ViewModel->AddFieldValueChangedDelegate(FDesc::SellEmpty, Delegate);
    }

    BindInventories();
    RefreshFromServer();
}

void UMythicVendorMenuWidget::BindInventories() {
    auto Watch = [this](UMythicInventoryComponent *Inv) {
        if (!Inv || WatchedInventories.Contains(Inv)) {
            return;
        }
        Inv->OnSlotUpdated.AddDynamic(this, &UMythicVendorMenuWidget::HandleSlotUpdated);
        WatchedInventories.Add(Inv);
    };

    if (BoundVendor.IsValid()) {
        Watch(BoundVendor->GetContainerInventory());
    }
    if (BoundPatron.IsValid()) {
        for (UMythicInventoryComponent *Inv : BoundPatron->GetAllInventoryComponents()) {
            Watch(Inv);
        }
    }
}

void UMythicVendorMenuWidget::UnbindInventories() {
    for (const TWeakObjectPtr<UMythicInventoryComponent> &Weak : WatchedInventories) {
        if (UMythicInventoryComponent *Inv = Weak.Get()) {
            Inv->OnSlotUpdated.RemoveDynamic(this, &UMythicVendorMenuWidget::HandleSlotUpdated);
        }
    }
    WatchedInventories.Reset();
}

void UMythicVendorMenuWidget::HandleSlotUpdated(int32 UpdatedSlotIndex) {
    RequestRefresh();
}

void UMythicVendorMenuWidget::RequestRefresh() {
    UWorld *World = GetWorld();
    if (!World || bRefreshArmed) {
        return;
    }
    bRefreshArmed = true;
    World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]() {
        bRefreshArmed = false;
        RefreshFromServer();
    }));
}

void UMythicVendorMenuWidget::RefreshFromServer() {
    if (ViewModel && BoundVendor.IsValid() && BoundPatron.IsValid()) {
        ViewModel->RefreshFromVendor(BoundVendor.Get(), BoundPatron.Get());
    }
}

void UMythicVendorMenuWidget::NativeDestruct() {
    Unbind();
    Super::NativeDestruct();
}

void UMythicVendorMenuWidget::Unbind() {
    if (ViewModel) {
        ViewModel->RemoveAllFieldValueChangedDelegates(this);
    }
    UnbindInventories();
    bRefreshArmed = false;
    BoundVendor.Reset();
    BoundPatron.Reset();
}

void UMythicVendorMenuWidget::HandleFieldChanged(UObject *Object, UE::FieldNotification::FFieldId FieldId) {
    Refresh();
}

void UMythicVendorMenuWidget::Refresh() {
    if (!ViewModel) {
        return;
    }

    if (Txt_VendorName) {
        Txt_VendorName->SetText(ViewModel->GetVendorName());
    }
    if (Txt_Wallet) {
        Txt_Wallet->SetText(ViewModel->GetWalletText());
    }
    if (Txt_Standing) {
        Txt_Standing->SetText(ViewModel->GetStandingText());
    }

    if (StockList) {
        TArray<UObject *> Items;
        const TArray<TObjectPtr<UVendorStockLineVM>> &Lines = ViewModel->GetStockLines();
        Items.Reserve(Lines.Num());
        for (const TObjectPtr<UVendorStockLineVM> &Line : Lines) {
            if (Line) {
                Items.Add(Line);
            }
        }
        StockList->SetListItems(Items);
    }

    if (SellList) {
        TArray<UObject *> Items;
        const TArray<TObjectPtr<UVendorSellLineVM>> &Lines = ViewModel->GetSellLines();
        Items.Reserve(Lines.Num());
        for (const TObjectPtr<UVendorSellLineVM> &Line : Lines) {
            if (Line) {
                Items.Add(Line);
            }
        }
        SellList->SetListItems(Items);
    }

    const bool bStockEmpty = ViewModel->GetStockEmpty();
    if (Txt_StockEmpty) {
        Txt_StockEmpty->SetVisibility(bStockEmpty ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (StockList) {
        StockList->SetVisibility(bStockEmpty ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    const bool bSellEmpty = ViewModel->GetSellEmpty();
    if (Txt_SellEmpty) {
        Txt_SellEmpty->SetVisibility(bSellEmpty ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (SellList) {
        SellList->SetVisibility(bSellEmpty ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }
}
