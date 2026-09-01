// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/CommonBoundActionButton.h"
#include "MythicBoundActionButton.generated.h"

class UMythicInputGlyph;
class UInputAction;

UCLASS(Abstract, meta = (DisableNativeTick))
class MYTHIC_API UMythicBoundActionButton : public UCommonBoundActionButton {
    GENERATED_BODY()

public:
    UMythicBoundActionButton(const FObjectInitializer &ObjectInitializer);

    virtual void SetRepresentedAction(FUIActionBindingHandle InBindingHandle) override;

    /** Returns whether represented CommonUI hold mappings authoritatively drive this button's hold behavior. */
    bool IsRepresentedHoldLinked() const {
        return bLinkRequiresHoldToBindingHold;
    }

    /**
     * Overrides the represented action's display name without changing the binding or its device glyph.
     * Use this for contextual verbs such as Back becoming Cancel Changes while a transaction is dirty.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    void SetLabelOverride(const FText &InLabel);

    /** Returns the button to the localized display name supplied by its represented action. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    void ClearLabelOverride();

    /**
     * Prompt-only entries are decorative action-bar legends and cannot take focus. Interactive entries
     * are real buttons that support mouse, keyboard and gamepad focus while retaining the same shortcut.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    void SetActionBarPromptOnly(bool bInPromptOnly);

    /** Shows the current device glyph for an Enhanced Input action without creating a second UI binding. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Input")
    void SetGlyphEnhancedAction(const UInputAction *InAction);

protected:
    virtual void UpdateInputActionWidget() override;

    /** Optional: a button that only wants a label simply leaves this out. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UMythicInputGlyph> Glyph;

    /** True for compact global prompts; false for buttons placed inside a screen's interactive footer. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Mythic|Input")
    bool bActionBarPromptOnly = true;

private:
    void RefreshInteractionMode();

    FUIActionBindingHandle MythicBindingHandle;

    FText LabelOverride;
    bool bHasLabelOverride = false;
};
