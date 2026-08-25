// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "GameplayTagContainer.h"
#include "MythicSocketRowWidget.generated.h"

class UImage;
class UMythicGemPickerWidget;
class UMythicInventoryComponent;
class UMythicItemInstance;
class UMythicSectionHeader;
class UMythicSocketRowWidget;
class UPanelWidget;
class UTextBlock;
class UTexture2D;
class UWidget;

UCLASS()
class MYTHIC_API UMythicSocketWellClickProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicSocketRowWidget> Row;

    UPROPERTY()
    int32 SocketIndex = INDEX_NONE;

    UFUNCTION()
    void HandleClicked();
};

/** What a gem type looks like on a well. Authored so a new gem is a row, not a code change. */
USTRUCT(BlueprintType)
struct FMythicGemMark {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Sockets", meta = (Categories = "Itemization.Gem"))
    FGameplayTag GemType;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Sockets")
    TSoftObjectPtr<UTexture2D> Mark;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Sockets")
    FLinearColor Colour = FLinearColor::White;
};

USTRUCT()
struct FMythicSocketWell {
    GENERATED_BODY()

    UPROPERTY(Transient)
    TObjectPtr<UWidget> Root;

    UPROPERTY(Transient)
    TObjectPtr<UImage> Plate;

    UPROPERTY(Transient)
    TObjectPtr<UImage> Mark;

    UPROPERTY(Transient)
    TObjectPtr<UMythicSocketWellClickProxy> Proxy;
};

/**
 * An item's sockets, as a row on the detail card beside its affixes and talents.
 *
 * Sockets were the one item-owned system behind its own tab, which meant the screen that says what an item
 * IS did not show the wells that change it most. The row only displays and routes: the picker it opens
 * commits through UMythicSocketComponent's server RPC, which re-runs every socketing rule.
 *
 * A well that cannot take a gem - already filled, or nothing in the bags fits its colour - does not answer a
 * click, because a control that responds and does nothing teaches the player that it lies.
 */
UCLASS(Blueprintable)
class MYTHIC_API UMythicSocketRowWidget : public UCommonUserWidget {
    GENERATED_BODY()

public:
    /** Re-points the row at another item. Wells are pooled, so this re-texts rather than rebuilds. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Sockets")
    void SetItem(UMythicItemInstance *Item);

    /** The picker calls this after it commits, so the wells redraw without waiting on a bag event. */
    void NotifySocketsChanged();

    /** Opens the gem picker against one well. Called by the well's hit area. */
    void OpenPickerFor(int32 SocketIndex);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** Where the wells go. Built once to MaxSockets, then shown and hidden. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> WellHost;

    /** The house header, preferred over Txt_Label when the row authors one. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicSectionHeader> Header;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_Label;

    /** How many wells to build. An item rolling more than this logs rather than hiding them silently. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets", meta = (ClampMin = "1", ClampMax = "8"))
    int32 MaxSockets = 6;

    /** Catalogue id for the well plate, so a renamed material is a log line and not the engine grey plate. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets")
    FName WellComponentId = TEXT("SlotTex.Round");

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets")
    FVector2D WellSize = FVector2D(56.0, 56.0);

    /** Padding between the plate edge and the gem mark inside it. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets", meta = (ClampMin = "0.0"))
    float MarkInset = 12.0f;

    /** How far a well that cannot be filled is faded, so inert and fillable read apart without colour alone. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float InertWellOpacity = 0.4f;

    /** A colour-locked empty well shows the mark it wants, ghosted, so the restriction reads before the click. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RestrictedMarkOpacity = 0.35f;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets")
    TArray<FMythicGemMark> GemMarks;

    /** The gem library, opened against whichever well was clicked. One instance for the row's lifetime. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Sockets")
    TSubclassOf<UMythicGemPickerWidget> GemPickerClass;

private:
    void BuildWells();
    void RefreshWells();

    /** Coalesces a burst of bag events into one redraw. */
    void RequestRefresh();

    void BindInventories();
    void UnbindInventories();

    UFUNCTION()
    void HandleSlotUpdated(int32 UpdatedSlotIndex);

    const FMythicGemMark *FindMark(const FGameplayTag &GemType) const;

    UPROPERTY(Transient)
    TArray<FMythicSocketWell> Wells;

    UPROPERTY(Transient)
    TObjectPtr<UMythicGemPickerWidget> GemPicker;

    TWeakObjectPtr<UMythicItemInstance> HostItem;

    TArray<TWeakObjectPtr<UMythicInventoryComponent>> WatchedInventories;

    bool bRefreshArmed = false;
};
