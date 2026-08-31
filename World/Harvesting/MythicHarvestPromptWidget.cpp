#include "World/Harvesting/MythicHarvestPromptWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "World/Harvesting/MythicHarvestToolTypeDefinition.h"

void UMythicHarvestPromptWidget::SetFocusPresentation(const FMythicHarvestFocusPresentation &InFocus) {
    // Collapsed rather than Hidden: a hidden prompt still costs a Slate pre-pass for every child on a surface the
    // player is not looking at.
    const ESlateVisibility TargetVisibility = InFocus.bHasFocus
        ? ESlateVisibility::HitTestInvisible
        : ESlateVisibility::Collapsed;
    if (GetVisibility() != TargetVisibility) {
        SetVisibility(TargetVisibility);
    }
    if (!InFocus.bHasFocus) {
        return;
    }

    if (PromptLabel) {
        PromptLabel->SetText(InFocus.PromptText);
    }
    if (ToolIcon) {
        const UMythicHarvestToolTypeDefinition *ToolType = InFocus.RequiredToolType;
        UObject *Icon = ToolType ? ToolType->Icon.Get() : nullptr;
        if (Icon) {
            ToolIcon->SetBrushResourceObject(Icon);
        }
        ToolIcon->SetVisibility(Icon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }

    OnFocusPresentationChanged(InFocus);
}
