// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MythicRuneSocketWidget.generated.h"

class UCommonButtonBase;
class UImage;
class UTexture2D;
class UWidget;
class UWidgetAnimation;

UENUM(BlueprintType)
enum class EMythicRuneSocketState : uint8 {
    Sealed,
    Empty,
    Filled,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicOnRuneSocketIndex, int32, SlotIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMythicOnRuneSocketFlag, int32, SlotIndex, bool, bOn);

/**
 * One round rune socket: glow, well, painted mark, chains, bezel, focus ring and a single hit.
 *
 * The same cell sits on the character page plinth, in the picker's socket strip, inside every picker grid
 * cell and on the detail card, so the page and the picker share one visual language. Everything here is a
 * re-text of existing children; nothing is added or removed after construction.
 */
UCLASS()
class MYTHIC_API UMythicRuneSocketWidget : public UUserWidget {
    GENERATED_BODY()

public:
    void SetSlotIndex(int32 InSlotIndex) { SlotIndex = InSlotIndex; }
    int32 GetSlotIndex() const { return SlotIndex; }

    /** Re-texts brushes and tints only. A null icon collapses the mark; the tint goes on the glow, never the icon. */
    void SetState(EMythicRuneSocketState State, UTexture2D *Icon, FLinearColor Tint);

    /** Gold bezel while selected, iron otherwise. Focus borrows gold for as long as it lasts. */
    void SetSelected(bool bSelected);

    /** Collapses the hit so an inner socket in a grid cell never competes with the cell's own button. */
    void SetInteractive(bool bInteractive);

    /** Lifts the orb and lights the ring. A grid cell drives this for its inner socket, whose own hit is collapsed. */
    void SetHoverVisual(bool bInHovered);

    void PlayLand();
    void PlayUnland();
    void PlayRefuse();
    void PlayUnseal();

    UWidget *GetFocusWidget() const;

    EMythicRuneSocketState GetState() const { return CurrentState; }

    /** Click or Confirm on the hit. */
    UPROPERTY(BlueprintAssignable, Category = "Mythic|Runes")
    FMythicOnRuneSocketIndex OnPressed;

    /** Right-click on the socket. */
    UPROPERTY(BlueprintAssignable, Category = "Mythic|Runes")
    FMythicOnRuneSocketIndex OnAltPressed;

    UPROPERTY(BlueprintAssignable, Category = "Mythic|Runes")
    FMythicOnRuneSocketFlag OnHoverChanged;

    UPROPERTY(BlueprintAssignable, Category = "Mythic|Runes")
    FMythicOnRuneSocketFlag OnFocusChanged;

protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) override;

    /** Soft ring under the mark, tinted by the rune's category. Alpha 0 unless Filled. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Glow;

    /** The kit's round slot plate. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Well;

    /** The painted rune icon. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Mark;

    /** Chains drawn over a sealed socket or a locked rune. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Seal;

    /** Iron normally, gold when selected or focused. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Bezel;

    /** Lit while the hit has focus and for the length of Unseal. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> FocusRing;

    /** The one node that takes the hit. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonButtonBase> Hit;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> Land;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> Unland;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> Refuse;

    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    TObjectPtr<UWidgetAnimation> Unseal;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    TSoftObjectPtr<UTexture2D> BezelIron;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    TSoftObjectPtr<UTexture2D> BezelGold;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    TSoftObjectPtr<UTexture2D> SealTexture;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    TSoftObjectPtr<UTexture2D> GlowTexture;

    /** The recessed well behind the orb. Set, it replaces the kit plate, which reads see-through on the page. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    TSoftObjectPtr<UTexture2D> WellTexture;

    /** Catalogue id for the well when no WellTexture is set. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    FName WellKitId = TEXT("SlotTex.Round");

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes")
    FVector2D WellSize = FVector2D(64.0, 64.0);

    /** Grey the mark and well drop to under the chains. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SealedMarkDim = 0.35f;

    /**
     * How far a chosen socket's ring leans into the worn rune's category colour. The iron stays iron at 0 and
     * goes fully to the category hue at 1; a little lean reads as "this one, and it is a combat rune".
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SelectedRingCategoryLean = 0.75f;

    /** Strength of the hover bloom. The socket carries no glow at rest, so this is the whole effect. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Runes", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HoverGlowAlpha = 0.7f;

private:
    void Play(UWidgetAnimation *Anim);
    void ApplyBezel();

    // The rune's aura, lit only while the socket is under the cursor or pad focus.
    void ApplyGlow();

    // The worn rune's category colour, kept from SetState so selection can wear it.
    FLinearColor CategoryTint = FLinearColor::White;

    UFUNCTION()
    void HandleAnimationFinished();

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> LoadedBezelIron;

    UPROPERTY(Transient)
    TObjectPtr<UTexture2D> LoadedBezelGold;

    int32 SlotIndex = INDEX_NONE;
    EMythicRuneSocketState CurrentState = EMythicRuneSocketState::Empty;
    bool bSelected = false;
    bool bFocused = false;
    bool bHovered = false;
    bool bInteractive = true;

    void ApplyFocusRing();
};
