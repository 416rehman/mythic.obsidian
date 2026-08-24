// Copyright Stellar Games. All Rights Reserved.

#include "MythicUIConversions.h"

#include "Engine/Texture2D.h"
#include "Itemization/Inventory/ViewModels/ItemSlotVM.h"

namespace {
constexpr float SlotBackingAlpha = 0.7f;
}

FLinearColor UMythicUIConversions::SlateColorToLinear(FSlateColor Color) {
    return Color.GetSpecifiedColor();
}

FLinearColor UMythicUIConversions::SlotBackingTint(FSlateColor RarityColor) {
    FLinearColor Tint = RarityColor.GetSpecifiedColor();
    Tint.A = SlotBackingAlpha;
    return Tint;
}

FSlateBrush UMythicUIConversions::SlotIconBrush(UItemSlotVM *SlotVM) {
    FSlateBrush Brush;
    UTexture2D *Icon = SlotVM ? SlotVM->GetIcon() : nullptr;
    if (!Icon) {
        Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
        return Brush;
    }
    Brush.SetResourceObject(Icon);
    Brush.ImageSize = FVector2D(Icon->GetSizeX(), Icon->GetSizeY());
    return Brush;
}
