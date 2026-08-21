// Copyright Stellar Games. All Rights Reserved.

#include "MythicBoundActionButton.h"

#include "CommonTextBlock.h"
#include "UI/Widgets/MythicInputGlyph.h"

void UMythicBoundActionButton::SetRepresentedAction(FUIActionBindingHandle InBindingHandle) {
    MythicBindingHandle = InBindingHandle;

    Super::SetRepresentedAction(InBindingHandle);

    SetIsFocusable(false);

    if (Glyph) {
        Glyph->SetActionBinding(InBindingHandle);
    }
}

void UMythicBoundActionButton::UpdateInputActionWidget() {
    Super::UpdateInputActionWidget();

    if (Text_ActionName) {
        Text_ActionName->SetText(MythicBindingHandle.GetDisplayName());
    }
    if (Glyph) {
        Glyph->SetActionBinding(MythicBindingHandle);
    }
}
