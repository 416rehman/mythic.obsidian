// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Menu/MythicRuneSocketWidget.h"
#include "MythicRunePickerCellWidget.generated.h"

class UCommonButtonBase;
class UCommonTextBlock;
class UImage;
class UTexture2D;
class UWidget;
class UWidgetAnimation;

UENUM(BlueprintType)
enum class EMythicRuneWorn : uint8 {
    None,
    Here,
    Elsewhere,
};

USTRUCT(BlueprintType)
struct FMythicRuneCellState {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Runes")
    FText Name;

    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Runes")
    TObjectPtr<UTexture2D> Icon = nullptr;

    /** Category colour; drawn on the socket glow. */
    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Runes")
    FLinearColor Tint = FLinearColor::White;

    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Runes")
    bool bUnlocked = false;

    /** The "no rune" tile: draws an empty well and clears the socket instead of wearing anything. */
    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Runes")
    bool bClear = false;

    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Runes")
    EMythicRuneWorn Worn = EMythicRuneWorn::None;

    /** The socket the rune sits in when Worn is Elsewhere. */
    UPROPERTY(BlueprintReadWrite, Category = "Mythic|Runes")
    int32 WornSlot = INDEX_NONE;
};

/**
 * One cell of the picker grid: a bezeled icon with the rune's name beside it.
 *
 * A locked rune keeps its hover and focus - only the verb is withheld - because everything the mouse can read,
 * a pad must be able to reach. The cell never disables itself; the picker decides what a press means.
 */
UCLASS()
class MYTHIC_API UMythicRunePickerCellWidget : public UUserWidget {
    GENERATED_BODY()

public:
    void SetCellIndex(int32 InCellIndex) { CellIndex = InCellIndex; }
    int32 GetCellIndex() const { return CellIndex; }

    /** Re-texts the cell. Never toggles enabled. */
    void SetCellState(const FMythicRuneCellState &State);

    void PlayRefuse();

    UWidget *GetFocusWidget() const;

    /** Click or Confirm on the cell. */
    UPROPERTY(BlueprintAssignable, Category = "Mythic|Runes")
    FMythicOnRuneSocketIndex OnPressed;

    UPROPERTY(BlueprintAssignable, Category = "Mythic|Runes")
    FMythicOnRuneSocketFlag OnHoverChanged;

    UPROPERTY(BlueprintAssignable, Category = "Mythic|Runes")
    FMythicOnRuneSocketFlag OnFocusChanged;

protected:
    virtual void NativeOnInitialized() override;

    /** Kit Rule.RowIdle, swapped to RowHover / RowPress while the pointer rests or presses. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> CellBacking;

    /** The one node that takes the hit. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonButtonBase> Hit;

    /** Lit while the hit has focus. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> FocusRing;

    /** The inner socket. Non-interactive; the cell's own hit answers. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicRuneSocketWidget> Socket;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> NameText;

    /** A small gold pip for a rune worn in another socket. State reads from the bezel and this mark, never from text. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> WornMark;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> HoverAnim;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> Refuse;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    FName BackingIdleId = TEXT("Rule.RowIdle");

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    FName BackingHoverId = TEXT("Rule.RowHover");

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    FName BackingPressId = TEXT("Rule.RowPress");

private:
    void ApplyBacking();
    void ApplySocketSelection();
    void Play(UWidgetAnimation *Anim);

    UFUNCTION()
    void HandleAnimationFinished();

    int32 CellIndex = INDEX_NONE;
    EMythicRuneWorn Worn = EMythicRuneWorn::None;
    bool bHovered = false;
    bool bPressed = false;
    bool bFocused = false;
};
