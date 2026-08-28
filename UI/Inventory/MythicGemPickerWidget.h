// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicGemPickerWidget.generated.h"

class APlayerController;
class UMythicGemPickerWidget;
class UMythicItemInstance;
class UMythicSocketRowWidget;
class UPanelWidget;
class UTextBlock;
class UWidget;

UCLASS()
class MYTHIC_API UMythicGemPickerRowProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicGemPickerWidget> Picker;

    UPROPERTY()
    int32 RowIndex = INDEX_NONE;

    UFUNCTION()
    void HandleClicked();
};

USTRUCT()
struct FMythicGemPickerRow {
    GENERATED_BODY()

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> Widget;

    UPROPERTY(Transient)
    TObjectPtr<UMythicGemPickerRowProxy> Proxy;

    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicItemInstance> Gem;
};

/**
 * The gems you are carrying, opened against one socket.
 *
 * A picker rather than a page because socketing is a verb performed on a thing that lives elsewhere - the
 * same reasoning that took sockets off their own tab. The filter here keeps the list short; the server
 * re-runs the rules and owns the answer.
 *
 * When nothing in the bags fits, the picker says which gem the socket wants instead of opening empty: a
 * list with no rows and no reason is a dead end.
 */
UCLASS(Blueprintable)
class MYTHIC_API UMythicGemPickerWidget : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    /** Points the picker at one socket and shows it. Rows are pooled - repeated opens re-text them. */
    void OpenForSocket(UMythicItemInstance *HostItem, int32 SocketIndex, UMythicSocketRowWidget *Opener);

    /** Commits the gem on one row through the socket component's server RPC. */
    void SocketRow(int32 RowIndex);

    /** Closes the picker and returns focus to the socket row that opened it. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Sockets")
    void Close();

    /** Every gem the player is carrying, in bag order. The one place the UI scans for gems. */
    static void CollectGems(const APlayerController *PC, TArray<UMythicItemInstance *> &OutGems);

    /** The gem-type an inventory item carries, or an invalid tag when the item is not a usable gem. */
    static FGameplayTag GetGemType(UMythicItemInstance *Item);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeOnDeactivated() override;
    virtual UWidget *NativeGetDesiredFocusTarget() const override;

    /** Where the rows go. One row per gem that fits. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> RowHost;

    /** Names the socket being filled, so the player is never guessing which of six they opened. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> SlotLabel;

    /** Shown instead of an empty list, carrying the reason nothing fits. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> EmptyState;

    /** Explains why no carried gem can be shown when the empty-state panel is visible. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_Empty;

    /** One row widget. Must expose the text blocks named below. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets")
    TSubclassOf<UUserWidget> RowClass;

    /** Rows built at startup. The pool grows on demand past this, never per click. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets", meta = (ClampMin = "0", ClampMax = "64"))
    int32 PrewarmRows = 12;

    /** Name of the row text widget that displays the candidate gem's player-facing name. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets")
    FName RowNameText = TEXT("NameText");

    /** Carries what the gem grants, which is the only thing that makes one row worth picking over another. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets")
    FName RowDescriptionText = TEXT("DescriptionText");

    /** Name of the row text widget that displays how many copies of the candidate gem are carried. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets")
    FName RowCountText = TEXT("CountText");

    /** Name of the row image widget that displays the candidate gem icon. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets")
    FName RowIconImage = TEXT("IconImage");

    /** Layer the picker is pushed to. Unset leaves placement to whoever parented the widget. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets", meta = (Categories = "UI.Layer"))
    FGameplayTag PickerLayerTag;

private:
    void BuildRows(int32 Count);

    FMythicGemPickerRow &GetOrCreateRow(int32 Index);

    void ShowOnLayer();

    UPROPERTY(Transient)
    TArray<FMythicGemPickerRow> Rows;

    UPROPERTY(Transient)
    TObjectPtr<UMythicSocketRowWidget> Opener;

    TWeakObjectPtr<UMythicItemInstance> Host;

    int32 SocketIndex = INDEX_NONE;

    FGameplayTag SocketColor;

    bool bOnLayer = false;
};
