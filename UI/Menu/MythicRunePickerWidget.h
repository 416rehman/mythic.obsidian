#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicRunePickerWidget.generated.h"

class UMythicCharacterPageWidget;
class UMythicRuneDefinition;
class UMythicRunePickerWidget;
class UPanelWidget;
class UTextBlock;

UCLASS()
class MYTHIC_API UMythicRunePickerRowProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicRunePickerWidget> Picker;

    UPROPERTY()
    int32 RowIndex = INDEX_NONE;

    UFUNCTION()
    void HandleClicked();
};

/**
 * The rune library, opened against one socket.
 *
 * It is a picker rather than a page because socketing is a verb performed on a thing that lives elsewhere -
 * the same reasoning that removed the Sockets tab. A locked rune is shown with the deed that earns it rather
 * than hidden, because a library the player cannot see the shape of is one they cannot plan against.
 */
UCLASS()
class MYTHIC_API UMythicRunePickerWidget : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    /** Points the picker at a socket and shows it. Safe to call repeatedly - rows are pooled, never rebuilt. */
    void OpenForSlot(int32 InSlotIndex, UMythicCharacterPageWidget *InPage);

    void EquipRow(int32 RowIndex);

protected:
    virtual void NativeConstruct() override;

    /** Where the rows go. One row per rune in the library. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> RowHost;

    /** Names the socket being filled, so the player is never guessing which of four they opened. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> SlotLabel;

    /** One row widget. Must expose the text blocks named below. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    TSubclassOf<UUserWidget> RowClass;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    FName RowNameText = TEXT("NameText");

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    FName RowDescriptionText = TEXT("DescriptionText");

    /** Carries the hint on a locked rune, which is the only thing that makes a locked row actionable. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    FName RowHintText = TEXT("HintText");

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    FName RowIconImage = TEXT("IconImage");

    /** How far a locked row is faded, so locked and earned read apart without colour alone carrying it. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float LockedRowOpacity = 0.45f;

    /** The layer the picker is pushed onto. Activating a widget nothing parented puts it nowhere. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes", meta = (Categories = "UI.Layer"))
    FGameplayTag PickerLayerTag;

private:
    void ShowOnLayer();

    void HideFromLayer();

    bool bOnLayer = false;

    struct FMythicRuneRow {
        TObjectPtr<UUserWidget> Widget;
        TObjectPtr<UMythicRunePickerRowProxy> Proxy;
        TObjectPtr<UMythicRuneDefinition> Rune;
        bool bUnlocked = false;
    };

    void BuildRows();

    void RefreshRows();

    TArray<FMythicRuneRow> Rows;

    UPROPERTY()
    TArray<TObjectPtr<UMythicRuneDefinition>> Library;

    UPROPERTY()
    TObjectPtr<UMythicCharacterPageWidget> Page;

    int32 SlotIndex = INDEX_NONE;
};
