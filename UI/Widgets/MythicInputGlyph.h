// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonInputTypeEnum.h"
#include "Components/Image.h"
#include "GameplayTagContainer.h"
#include "Input/UIActionBindingHandle.h"
#include "MythicInputGlyph.generated.h"

class UInputAction;

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

    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    void SetActionTag(FGameplayTag InActionTag);

    void SetActionBinding(FUIActionBindingHandle InHandle);

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
    void HandleInputMethodChanged(ECommonInputType NewType);

    static bool KeyMatchesInputType(const FKey &Key, ECommonInputType InputType);

    void GatherKeys(TArray<FKey> &OutKeys) const;

    FUIActionBindingHandle BindingHandle;
    FDelegateHandle InputMethodHandle;

    UPROPERTY()
    TObjectPtr<const UInputAction> EnhancedAction;

    TSharedPtr<class STextBlock> KeyLabel;

    UPROPERTY(Transient)
    TObjectPtr<class UMaterialInstanceDynamic> KeyCapMID;

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
};
