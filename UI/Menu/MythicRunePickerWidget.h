#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "UI/Menu/MythicRunePickerCellWidget.h"
#include "UI/MythicActivatableWidget.h"
#include "MythicRunePickerWidget.generated.h"

class UButton;
class UCommonButtonBase;
class UCommonTextBlock;
class UInputAction;
class UMythicCharacterPageWidget;
class UMythicInputGlyph;
class UMythicRuneDefinition;
class UMythicRuneSocketWidget;
class UMythicSectionHeader;
class UPanelWidget;
class URichTextBlock;
class UTexture2D;
class UUniformGridPanel;
class UWidgetAnimation;

/**
 * The rune library, opened against one socket.
 *
 * One panel: the four sockets across the top so the target can be re-chosen without closing, a grid of every
 * rune with its category carried by the glow, and a detail card that answers hover and pad focus alike. A
 * locked rune stays reachable, because a library the player cannot see the shape of is one they cannot plan
 * against; only the verb is withheld. Every write is a server request and every redraw comes from the
 * replicated answer.
 */
UCLASS()
class MYTHIC_API UMythicRunePickerWidget : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    /** Points the picker at a socket and shows it. Cells are pooled at initialise; opening only re-texts. */
    void OpenForSlot(int32 SlotIndex, UMythicCharacterPageWidget *InPage);

    /** Plays CloseAnim, leaves the layer and hands focus back to the socket the page opened from. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Runes")
    void Close();

    /** Re-targets the picker at another open socket without closing. */
    void SelectSocket(int32 SlotIndex);

    /** Equip, move or unequip by the cell's worn state; a locked cell shakes and shows its deed. */
    void ActivateCell(int32 CellIndex);

    /** Empties the selected socket. */
    void UnequipCurrent();

    /** Draws this component instead of the owning player's. Tests and spectating have no owning player. */
    void SetRuneSource(UMythicRuneComponent *Runes);

    /** The icon loaded for a library rune at initialise, so no redraw pays a synchronous load. */
    UTexture2D *FindPreloadedIcon(const UMythicRuneDefinition *Rune) const;

    bool IsOnLayer() const { return bOnLayer; }

    int32 GetSelectedSlot() const { return SelectedSlot; }

    int32 GetCellCount() const { return Cells.Num(); }

    UMythicRunePickerCellWidget *GetCellWidget(int32 CellIndex) const;

    UMythicRuneSocketWidget *GetStripSocket(int32 SlotIndex) const;

    /** The rune a pooled cell currently points at. */
    const UMythicRuneDefinition *GetCellRune(int32 CellIndex) const;

    bool IsCellUnlocked(int32 CellIndex) const;

    EMythicRuneWorn GetCellWorn(int32 CellIndex) const;

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual bool NativeOnHandleBackAction() override;
    virtual UWidget *NativeGetDesiredFocusTarget() const override;

    UFUNCTION()
    void HandleRunesChanged();

    UFUNCTION()
    void HandleRuneRefused(int32 SlotIndex, EMythicRuneRefusal Reason);

    UFUNCTION()
    void HandleCellPressed(int32 CellIndex);

    UFUNCTION()
    void HandleCellHoverChanged(int32 CellIndex, bool bOn);

    UFUNCTION()
    void HandleCellFocusChanged(int32 CellIndex, bool bOn);

    UFUNCTION()
    void HandleStripPressed(int32 SlotIndex);

    UFUNCTION()
    void HandleStripHoverChanged(int32 SlotIndex, bool bOn);

    UFUNCTION()
    void HandleStripFocusChanged(int32 SlotIndex, bool bOn);

    UFUNCTION()
    void HandleDismissClicked();

    UFUNCTION()
    void HandlePrimaryAction();

    UFUNCTION()
    void HandleActionsAction();

    UFUNCTION()
    void HandleOpenAnimFinished();

    UFUNCTION()
    void HandleCloseAnimFinished();

    /** WBP_RunePickerCell. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    TSubclassOf<UMythicRunePickerCellWidget> CellClass;

    /** WBP_RuneSocket, for the strip across the top. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    TSubclassOf<UMythicRuneSocketWidget> SocketClass;

    /** Sockets drawn in the strip. Matches the rune component's MaxSlots. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes", meta = (ClampMin = "1", ClampMax = "8"))
    int32 StripSocketCount = 4;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes", meta = (ClampMin = "1", ClampMax = "8"))
    int32 GridColumns = 4;

    /** IA_InventoryPrimary: Enter / A. Equip, Move here or Unequip by the focused cell. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes|Input")
    TSoftObjectPtr<UInputAction> PrimaryAction;

    /** IA_InventoryActions: F / X. Unequips the selected socket. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes|Input")
    TSoftObjectPtr<UInputAction> ActionsAction;

    /** The layer the picker is pushed onto. Activating a widget nothing parented puts it nowhere. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes", meta = (Categories = "UI.Layer"))
    FGameplayTag PickerLayerTag;

    /** How far a locked cell is faded. Chains and a dimmed icon carry the rest; the cell stays reachable. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes", meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float LockedCellOpacity = 0.55f;

    /** Emblem on the title plate. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    TSoftObjectPtr<UTexture2D> TitleEmblem =
        TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/Mythic/UI/Icons/Emblem/T_Emblem_Sockets.T_Emblem_Sockets")));

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> Card;

    /** The NoDraw button behind the card; a click on the scrim closes. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> Dismiss;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicSectionHeader> TitleHeader;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicSectionHeader> SocketHeader;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicSectionHeader> GridHeader;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonButtonBase> CloseButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonButtonBase> UnequipButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonButtonBase> FooterCloseButton;

    /** Where the strip sockets go. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> SocketStrip;

    /** Where the cells go, GridColumns wide. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UUniformGridPanel> CellGrid;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicRuneSocketWidget> DetailSocket;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> DetailName;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> DetailCategory;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<URichTextBlock> DetailDescription;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> DetailHint;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicInputGlyph> Glyph_Select;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicInputGlyph> Glyph_Actions;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicInputGlyph> Glyph_Back;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Lbl_Select;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Lbl_Actions;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Lbl_Back;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> OpenAnim;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> CloseAnim;

private:
    /** What a pooled cell points at this draw. Parallel to Cells; cells are re-pointed, never moved. */
    struct FMythicRuneCellEntry {
        TObjectPtr<UMythicRuneDefinition> Rune;
        bool bUnlocked = false;
        EMythicRuneWorn Worn = EMythicRuneWorn::None;
        int32 WornSlot = INDEX_NONE;
        // The first tile in the grid: wears nothing and empties the socket. It holds no rune by design.
        bool bClear = false;
    };

    void BuildLibrary();
    void BuildStrip();
    void BuildCells();

    void RefreshAll();
    void RefreshStrip();
    /**
     * Points the pooled cells at the library in display order. Runs when the target socket changes, never on a
     * state change, so nothing moves under focus or the pointer while the picker is open.
     */
    void SortCells();
    void RefreshCells();
    void RefreshHeaders();
    void RefreshDetail();
    void RefreshPrompts();

    void ShowDetailForRune(const UMythicRuneDefinition *Rune, bool bUnlocked);
    void ShowDetailForSocket(int32 SlotIndex);
    void SetSocketHeaderTrailing(const FText &Trailing);
    void ShowSealedNotice(int32 SlotIndex);
    void NoticeStrip(int32 SlotIndex, bool bOn);
    void RefuseLocally(int32 CellIndex, EMythicRuneRefusal Reason, int32 OtherSlot);

    void ShowOnLayer();
    void HideFromLayer();
    void FinishClose();
    void ReturnFocusToPage();

    void BindRunes();
    void UnbindRunes();
    UMythicRuneComponent *FindRuneComponent() const;

    /** Category rank from the page's authored order, then worn-here, unlocked, locked, then name. */
    int32 CategoryRank(const UMythicRuneDefinition *Rune) const;
    int32 FindWornSlot(const UMythicRuneComponent *Runes, const UMythicRuneDefinition *Rune) const;
    FText SocketCountText(const UMythicRuneComponent *Runes) const;

    UPROPERTY()
    TArray<TObjectPtr<UMythicRuneDefinition>> Library;

    /** Parallel to Library. Null where a rune has no icon. */
    UPROPERTY()
    TArray<TObjectPtr<UTexture2D>> PreloadedIcons;

    UPROPERTY()
    TArray<TObjectPtr<UMythicRunePickerCellWidget>> Cells;

    UPROPERTY()
    TArray<TObjectPtr<UMythicRuneSocketWidget>> StripSockets;

    TArray<FMythicRuneCellEntry> CellState;

    TArray<FSoftObjectPath> StripLastDrawn;
    TArray<bool> StripLastUnlocked;
    bool bStripDrawn = false;

    TWeakObjectPtr<UMythicCharacterPageWidget> Page;
    TWeakObjectPtr<UMythicRuneComponent> BoundRunes;
    TWeakObjectPtr<UMythicRuneComponent> RuneSource;

    FInputActionBindingHandle PrimaryBinding;
    FInputActionBindingHandle ActionsBinding;

    /** Registers or drops the dedicated unequip binding, which is how its action-bar prompt appears or hides. */
    void SetActionsBindingActive(bool bActive);

    int32 SelectedSlot = INDEX_NONE;
    int32 HoveredCell = INDEX_NONE;
    int32 FocusedCell = INDEX_NONE;
    int32 HoveredStrip = INDEX_NONE;
    int32 FocusedStrip = INDEX_NONE;

    bool bOnLayer = false;
    bool bClosing = false;
};
