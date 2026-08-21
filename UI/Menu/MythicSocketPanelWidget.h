// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "MythicSocketPanelWidget.generated.h"

class UButton;
class UCommonTextBlock;
class UMythicInventoryComponent;
class UMythicItemInstance;
class UMythicSocketPanelWidget;
class UPanelWidget;

UCLASS()
class MYTHIC_API UMythicSocketClickProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicSocketPanelWidget> Panel;

    UPROPERTY()
    TWeakObjectPtr<UMythicItemInstance> HostItem;

    UPROPERTY()
    int32 SocketIndex = INDEX_NONE;

    UFUNCTION()
    void HandleClicked();
};

UCLASS()
class MYTHIC_API UMythicGemClickProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicSocketPanelWidget> Panel;

    UPROPERTY()
    TWeakObjectPtr<UMythicItemInstance> Gem;

    UFUNCTION()
    void HandleClicked();
};

USTRUCT()
struct FMythicSocketChip {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UWidget> Button;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Label;

    UPROPERTY()
    TObjectPtr<UMythicSocketClickProxy> Proxy;
};

USTRUCT()
struct FMythicSocketItemRow {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UPanelWidget> Box;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> ItemName;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> SocketSummary;

    UPROPERTY()
    TObjectPtr<UPanelWidget> ChipStrip;

    UPROPERTY()
    TArray<FMythicSocketChip> Chips;
};

USTRUCT()
struct FMythicGemRow {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UWidget> Button;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Label;

    UPROPERTY()
    TObjectPtr<UMythicGemClickProxy> Proxy;
};

UCLASS()
class MYTHIC_API UMythicSocketPanelWidget : public UCommonActivatableWidget {
    GENERATED_BODY()

public:
    void SelectGem(UMythicItemInstance *Gem);

    void ActivateSocket(UMythicItemInstance *HostItem, int32 SocketIndex);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;

    /** Equipped socketed items go here. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> ItemList;

    /** Loose gems go here. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> GemList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Hint;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_NoGems;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_NoItems;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets", meta = (ClampMin = "0"))
    int32 PrewarmItemRows = 8;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets", meta = (ClampMin = "0"))
    int32 PrewarmGemRows = 16;

    /** Socket chips built per item row. Matches the socket hard cap so a row never has to grow. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets", meta = (ClampMin = "1"))
    int32 ChipsPerRow = 6;

private:
    void Bind();
    void Unbind();

    UFUNCTION()
    void HandleSlotUpdated(int32 UpdatedSlotIndex);

    void RequestRefresh();
    void Refresh();

    FMythicSocketItemRow &GetOrCreateItemRow(int32 Index);
    FMythicGemRow &GetOrCreateGemRow(int32 Index);

    static FText ShortGemName(const FGameplayTag &GemType);


    class UMythicSocketComponent *GetSocketComponent() const;

    UPROPERTY()
    TArray<FMythicSocketItemRow> ItemPool;

    UPROPERTY()
    TArray<FMythicGemRow> GemPool;

    UPROPERTY()
    TWeakObjectPtr<UMythicItemInstance> SelectedGem;

    UPROPERTY()
    TArray<TWeakObjectPtr<UMythicInventoryComponent>> WatchedInventories;

    bool bRefreshArmed = false;
    bool bPoolsBuilt = false;
};
