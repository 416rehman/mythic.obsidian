// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonInputTypeEnum.h"
#include "Components/Image.h"
#include "GameplayTagContainer.h"
#include "Input/UIActionBindingHandle.h"
#include "MythicInputGlyph.generated.h"

class UInputAction;
class UMaterialInstanceDynamic;

UCLASS()
class MYTHIC_API UMythicInputGlyph : public UImage {
    GENERATED_BODY()

public:
    UMythicInputGlyph();

    /** The UI action to show a key for. Same tag the binding uses. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|Input", meta = (Categories = "UI.Action"))
    FGameplayTag ActionTag;

    /** Drawn height. Width follows the source art's aspect, so wide keys like SHIFT stay wide. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|Input", meta = (ClampMin = "4.0"))
    float GlyphHeight = 30.0f;

    /** Changes the CommonUI action represented by this glyph and refreshes its current-device art. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    void SetActionTag(FGameplayTag InActionTag);

    /** Binds this glyph to a live CommonUI action, including its authoritative hold-progress delegate. */
    void SetActionBinding(FUIActionBindingHandle InHandle);

    /** Updates the bottom-to-top visual fill for a live hold action. Values are clamped to the 0..1 range. */
    void SetHoldProgress(float HeldPercent);

    /**
     * Show the key for a GAMEPLAY action — an Enhanced Input action from a mapping context, not a CommonUI menu
     * action. The HUD needs this: Q, E, dodge and block are Enhanced Input, and asking the ini for them finds
     * nothing. Keys come from the player's live subsystem, so a remapped key shows the new key with no extra work.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    void SetEnhancedAction(const UInputAction *InAction);

    /** Re-read the key and repaint. Cheap: brush swap only, no layout unless the aspect changed. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    void RefreshGlyph();

    /** Returns whether the shared glyph has a material and scalar parameter capable of rendering hold progress. */
    bool HasConfiguredHoldPresentation() const {
        return !HoldProgressMaterial.IsNull()
            && !HoldProgressParameterName.IsNone();
    }

    /**
     * Short label for a key, as it should read on a cap: "E", "LMB", "Shift".
     *
     * FKey::GetDisplayName gives "Left Mouse Button" and "Left Shift", which are correct sentences and useless on a
     * 22px chip. Public and static so anything else that needs to name a key short agrees with the caps.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Input")
    static FText GetShortKeyLabel(const FKey &Key);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void OnWidgetRebuilt() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;
    virtual void SynchronizeProperties() override;

#if WITH_EDITOR
    virtual const FText GetPaletteCategory() override;
#endif

private:
    void Listen(bool bListen);
    void ListenToActionBinding(bool bListen);
    void HandleInputMethodChanged(ECommonInputType NewType);
    void RefreshHoldPresentation();
    void UpdateHoldProgressBrushSize(const FVector2D &NewSize);
    bool IsCurrentActionHold() const;

    static bool KeyMatchesInputType(const FKey &Key, ECommonInputType InputType);

    void GatherKeys(TArray<FKey> &OutKeys) const;

    FUIActionBindingHandle BindingHandle;
    FDelegateHandle InputMethodHandle;
    bool bIsHoldAction = false;
    float HoldProgress = 0.0f;

    UPROPERTY()
    TObjectPtr<const UInputAction> EnhancedAction;

    TSharedPtr<class STextBlock> KeyLabel;
    TSharedPtr<class SImage> HoldProgressImage;

    UPROPERTY(Transient)
    TObjectPtr<class UMaterialInstanceDynamic> KeyCapMID;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> HoldProgressMID;

    FSlateBrush HoldProgressBrush;

protected:

    /** Hand-drawn cap the kit draws itself. Null falls back to the platform brush, so this can never blank a prompt. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|Input")
    TSoftObjectPtr<UMaterialInterface> KeyCapMaterial;

    /** The key letter. Slab serif to match the rest of the game; a UI sans here reads as a different product. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|Input")
    FSlateFontInfo KeyFont;

    /** Ink on the cap, and the cap's own line colour. One hue, two values. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|Input")
    FLinearColor KeyInk = FLinearColor(0.86f, 0.84f, 0.78f, 1.0f);

    /** Reusable CommonUI hold fill. The material must expose HoldProgressParameterName in the 0..1 range. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|Input|Hold")
    TSoftObjectPtr<UMaterialInterface> HoldProgressMaterial;

    /** Scalar parameter driven by CommonUI's live hold-progress delegate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|Input|Hold")
    FName HoldProgressParameterName = TEXT("percentage");

    /** Warm fill drawn behind the key or controller glyph while a hold is in progress. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|Input|Hold")
    FLinearColor HoldProgressTint = FLinearColor(0.92f, 0.55f, 0.16f, 0.90f);

    /** Persistent glyph tint that distinguishes hold actions from ordinary tap actions before input begins. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|Input|Hold")
    FLinearColor HoldGlyphTint = FLinearColor(0.98f, 0.76f, 0.34f, 1.0f);
};
