#include "MythicAbilityReticleWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Slate/SGameLayerManager.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicAbilityIntentComponent.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

UMythicAbilityReticleWidget::UMythicAbilityReticleWidget(const FObjectInitializer& Initializer):Super(Initializer) {
    SetIsFocusable(false); SetVisibility(ESlateVisibility::HitTestInvisible); ForceVolatile(true);
}

int32 UMythicAbilityReticleWidget::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bParentEnabled) const {
    Layer=Super::NativePaint(Args,Geometry,CullingRect,Elements,Layer,Style,bParentEnabled);
    auto* PC=GetOwningPlayer<AMythicPlayerController>();
    auto* LP=PC ? PC->GetLocalPlayer() : nullptr;
    FVector2D Pixel; bool bHardware=false;
    if (!LP || !LP->ViewportClient || !PC->AbilityIntentComponent
        || !PC->AbilityIntentComponent->GetReticleViewportPixel(Pixel,bHardware) || bHardware) return Layer;
    FVector2D Pixels; LP->ViewportClient->GetViewportSize(Pixels);
    if (Pixels.X<=0 || Pixels.Y<=0) return Layer;
    const auto Layers=LP->ViewportClient->GetGameLayerManager();
    if (!Layers) return Layer;
    // NativePaint receives window-space geometry. Tick geometry includes the detached
    // PIE window's desktop offset and would paint the reticle outside this viewport.
    const FGeometry Viewport=Layers->GetViewportWidgetHostPaintGeometry();
    if (Viewport.GetLocalSize().X<=0 || Viewport.GetLocalSize().Y<=0) return Layer;
    const FVector2D Point=Geometry.AbsoluteToLocal(Viewport.LocalToAbsolute(Pixel/Pixels*Viewport.GetLocalSize()));
    if (Point.ContainsNaN()) return Layer;
    const auto* Brush=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    const FLinearColor Ink(0.94f,0.88f,0.73f,0.92f),Shadow(0.015f,0.01f,0.008f,0.85f);
    const auto Box=[&](FVector2D Offset,FVector2D Size) {
        FSlateDrawElement::MakeBox(Elements,Layer+1,Geometry.ToPaintGeometry(Size+FVector2D(2,2),FSlateLayoutTransform(Point+Offset-FVector2D(1,1))),Brush,ESlateDrawEffect::None,Shadow);
        FSlateDrawElement::MakeBox(Elements,Layer+2,Geometry.ToPaintGeometry(Size,FSlateLayoutTransform(Point+Offset)),Brush,ESlateDrawEffect::None,Ink);
    };
    Box(FVector2D(-1,-1),FVector2D(2,2));
    Box(FVector2D(-1,-10),FVector2D(2,4)); Box(FVector2D(-1,6),FVector2D(2,4));
    Box(FVector2D(-10,-1),FVector2D(4,2)); Box(FVector2D(6,-1),FVector2D(4,2));
    return Layer+2;
}
