// Copyright Stellar Games. All Rights Reserved.

#include "MythicSectionHeader.h"

#include "CommonTextBlock.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "UI/MythicUIKit.h"

void UMythicSectionHeader::SetHeader(const FText &InLabel, const FText &InTrailing, UTexture2D *InIcon) {
    LabelText = InLabel;
    TrailingText = InTrailing;
    IconTexture = InIcon;
    Apply();
}

void UMythicSectionHeader::SetCollapsible(bool bInCollapsible) {
    bCollapsible = bInCollapsible;
    Apply();
}

void UMythicSectionHeader::SetCollapsed(bool bInCollapsed) {
    bCollapsed = bInCollapsed;
    Apply();
}

void UMythicSectionHeader::NativePreConstruct() {
    Super::NativePreConstruct();
    Apply();
}

FReply UMythicSectionHeader::NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) {
    if (bCollapsible && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton) {
        OnToggled.Broadcast(this);
        return FReply::Handled();
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UMythicSectionHeader::NativeOnMouseEnter(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) {
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
    bPointerOver = true;
    ApplyHoverBacking();
}

void UMythicSectionHeader::NativeOnMouseLeave(const FPointerEvent &InMouseEvent) {
    Super::NativeOnMouseLeave(InMouseEvent);
    bPointerOver = false;
    ApplyHoverBacking();
}

void UMythicSectionHeader::ApplyHoverBacking() {
    if (!Img_HoverBacking) {
        return;
    }
    Img_HoverBacking->SetVisibility(bCollapsible && bPointerOver ? ESlateVisibility::HitTestInvisible
                                                                 : ESlateVisibility::Collapsed);
}

void UMythicSectionHeader::Apply() {
    if (Txt_Label) {
        Txt_Label->SetText(LabelText);
    }

    if (Img_Emblem) {
        if (IconTexture) {
            Img_Emblem->SetBrushFromTexture(IconTexture, false);
            Img_Emblem->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else {
            Img_Emblem->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (Txt_Trailing) {
        Txt_Trailing->SetText(TrailingText);
        Txt_Trailing->SetVisibility(TrailingText.IsEmpty() ? ESlateVisibility::Collapsed
                                                           : ESlateVisibility::HitTestInvisible);
    }

    if (Img_Rule) {
        if (const UMythicUIKit *Kit = UMythicUIKit::Get()) {
            Img_Rule->SetBrush(Kit->MakeBrush(TEXT("Rule.Section"), EMythicUIState::Normal,
                                              FVector2D(64.0, 2.0)));
        }
        Img_Rule->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    const bool bTexturedChevron = Img_Chevron && !ChevronTexture.IsNull();
    if (Img_Chevron) {
        if (bCollapsible && bTexturedChevron) {
            if (UTexture2D *Chevron = ChevronTexture.LoadSynchronous()) {
                Img_Chevron->SetBrushFromTexture(Chevron, false);
            }
            Img_Chevron->SetRenderTransformAngle(bCollapsed ? -90.0f : 0.0f);
            Img_Chevron->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else {
            Img_Chevron->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (Txt_Chevron) {
        if (bCollapsible && !bTexturedChevron) {
            // ASCII drawer marks: the hand-drawn face has no arrow glyphs, and its .notdef box reads as a keycap.
            Txt_Chevron->SetText(bCollapsed ? NSLOCTEXT("Mythic", "SectionClosed", "+")
                                            : NSLOCTEXT("Mythic", "SectionOpen", "-"));
            Txt_Chevron->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else {
            Txt_Chevron->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (Img_HoverBacking) {
        if (const UMythicUIKit *Kit = UMythicUIKit::Get()) {
            Img_HoverBacking->SetBrush(Kit->MakeBrush(TEXT("Focus.Row"), EMythicUIState::Normal,
                                                      FVector2D(360.0, 36.0)));
        }
        ApplyHoverBacking();
    }
}
