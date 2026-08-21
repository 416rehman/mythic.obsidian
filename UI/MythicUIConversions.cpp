// Copyright Stellar Games. All Rights Reserved.

#include "MythicUIConversions.h"

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
