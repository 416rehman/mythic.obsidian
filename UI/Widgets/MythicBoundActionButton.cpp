// Copyright Stellar Games. All Rights Reserved.

#include "MythicBoundActionButton.h"

#include "CommonTextBlock.h"
#include "UI/Widgets/MythicInputGlyph.h"

UMythicBoundActionButton::UMythicBoundActionButton(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
    // A represented CommonUI hold binding owns the timing. Mirroring it into the button keeps pointer
    // interaction, Blueprint hold state, and glyph presentation on the same authoritative definition.
    bLinkRequiresHoldToBindingHold = true;
}

void UMythicBoundActionButton::SetRepresentedAction(FUIActionBindingHandle InBindingHandle) {
    MythicBindingHandle = InBindingHandle;

    // Older Widget Blueprints serialized CommonUI's old false default. Enforce the project-wide contract
    // at the boundary so stale content cannot silently turn a hold prompt back into a tap-style button.
    bLinkRequiresHoldToBindingHold = true;

    Super::SetRepresentedAction(InBindingHandle);

    RefreshInteractionMode();

    if (Glyph) {
        Glyph->SetActionBinding(InBindingHandle);
    }
}

void UMythicBoundActionButton::SetLabelOverride(const FText &InLabel) {
    LabelOverride = InLabel;
    bHasLabelOverride = true;
    UpdateInputActionWidget();
}

void UMythicBoundActionButton::ClearLabelOverride() {
    LabelOverride = FText::GetEmpty();
    bHasLabelOverride = false;
    UpdateInputActionWidget();
}

void UMythicBoundActionButton::SetActionBarPromptOnly(bool bInPromptOnly) {
    bActionBarPromptOnly = bInPromptOnly;
    RefreshInteractionMode();
}

void UMythicBoundActionButton::RefreshInteractionMode() {
    SetIsFocusable(!bActionBarPromptOnly);
    // The shared WBP defaults to prompt visibility for the global action bar. Screen-local instances
    // must opt back into hit testing or they render like buttons while every mouse click passes through.
    SetVisibility(bActionBarPromptOnly ? ESlateVisibility::SelfHitTestInvisible
                                       : ESlateVisibility::Visible);
}

void UMythicBoundActionButton::SetGlyphEnhancedAction(const UInputAction *InAction) {
    if (Glyph) {
        Glyph->SetEnhancedAction(InAction);
        // SetEnhancedAction intentionally no-ops for the same pointer. A screen reopen still needs a
        // fresh key lookup because the active mapping profile or input device may have changed meanwhile.
        Glyph->RefreshGlyph();
    }
    RefreshInteractionMode();
}

void UMythicBoundActionButton::UpdateInputActionWidget() {
    Super::UpdateInputActionWidget();

    if (Text_ActionName) {
        Text_ActionName->SetText(bHasLabelOverride ? LabelOverride : MythicBindingHandle.GetDisplayName());
    }
    if (Glyph) {
        Glyph->SetActionBinding(MythicBindingHandle);
    }
}
