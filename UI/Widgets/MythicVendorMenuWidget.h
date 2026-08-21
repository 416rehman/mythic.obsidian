// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FieldNotification/FieldId.h"
#include "MythicVendorMenuWidget.generated.h"

class AMythicPlayerController;
class AMythicVendor;
class UListView;
class UMythicInventoryComponent;
class UTextBlock;
class UVendorMenuVM;

UCLASS()
class MYTHIC_API UMythicVendorMenuWidget : public UUserWidget {
    GENERATED_BODY()

public:
    /** Open the screen for a vendor. Creates the ViewModel on first use and refreshes from the server's view. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Vendor")
    void OpenForVendor(AMythicVendor *Vendor, AMythicPlayerController *Patron);

    /** Re-read the vendor and the player's bags. Call after a buy or sell completes. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Vendor")
    void RefreshFromServer();

    UFUNCTION(BlueprintPure, Category = "Mythic|Vendor")
    UVendorMenuVM *GetVendorViewModel() const { return ViewModel; }

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /**
     * Bind to whatever container the owning player last opened, if it is a vendor. Called on construct; safe to call
     * again. Returns false when the player is not standing at a vendor, which is not an error — a designer previewing
     * the widget just sees the empty state.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Vendor")
    bool OpenForActiveVendor();

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_VendorName;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_Wallet;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_Standing;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UListView> StockList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UListView> SellList;

    /** "Sold out" / "nothing this trader wants" — shown only when the matching list is genuinely empty. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_StockEmpty;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_SellEmpty;

private:
    void HandleFieldChanged(UObject *Object, UE::FieldNotification::FFieldId FieldId);
    void Refresh();
    void Unbind();

    void BindInventories();
    void UnbindInventories();

    UFUNCTION()
    void HandleSlotUpdated(int32 UpdatedSlotIndex);

    void RequestRefresh();

    UPROPERTY()
    TObjectPtr<UVendorMenuVM> ViewModel;

    UPROPERTY()
    TWeakObjectPtr<AMythicVendor> BoundVendor;

    UPROPERTY()
    TWeakObjectPtr<AMythicPlayerController> BoundPatron;

    UPROPERTY()
    TArray<TWeakObjectPtr<UMythicInventoryComponent>> WatchedInventories;

    bool bRefreshArmed = false;
};
