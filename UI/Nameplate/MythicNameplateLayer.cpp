#include "UI/Nameplate/MythicNameplateLayer.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/LocalPlayer.h"
#include "UI/Nameplate/MythicNameplateActionRailWidget.h"
#include "UI/Nameplate/MythicNameplateDirector.h"
#include "UI/Nameplate/MythicNameplatePolicy.h"
#include "UI/Nameplate/MythicNameplateViewModel.h"
#include "UI/Nameplate/MythicNameplateVisualStyle.h"
#include "UI/Nameplate/MythicNameplateWidget.h"

namespace {
float ResolvePresentationAlpha(
    const FMythicNameplateRenderPreferences &Preferences,
    const float PresentationAlpha) {
    (void)Preferences;
    return FMath::Clamp(PresentationAlpha, 0.0f, 1.0f);
}

float ResolvePresentationScale(
    const FMythicNameplateRenderPreferences &Preferences,
    const float PresentationScale) {
    return FMath::Clamp(Preferences.Scale, 0.75f, 1.5f)
        * FMath::Clamp(PresentationScale, 0.75f, 1.0f);
}

int32 ResolveSemanticZOrder(const EMythicNameplateLane Lane) {
    switch (Lane) {
    case EMythicNameplateLane::Focus:
        return 400;
    case EMythicNameplateLane::Safety:
        return 300;
    case EMythicNameplateLane::Opportunity:
        return 200;
    case EMythicNameplateLane::Awareness:
    default:
        return 100;
    }
}
} // namespace

void UMythicNameplateLayer::NativeOnInitialized() {
    Super::NativeOnInitialized();
    if (!NameplateWidgetClass) {
        NameplateWidgetClass = UMythicNameplateWidget::StaticClass();
    }
    if (!ActionRailWidgetClass) {
        ActionRailWidgetClass =
            UMythicNameplateActionRailWidget::StaticClass();
    }
    if (!VisualStyle) {
        VisualStyle = Policy && Policy->VisualStyle
            ? Policy->VisualStyle.Get()
            : GetMutableDefault<UMythicNameplateVisualStyle>();
    }
    PrewarmPool();
    PrewarmActionRail();
}

void UMythicNameplateLayer::NativeConstruct() {
    Super::NativeConstruct();
    // NativeOnInitialized is one-shot, while a HUD layer may be removed and
    // re-added during travel or a layout swap.
    if (!VisualStyle) {
        VisualStyle = Policy && Policy->VisualStyle
            ? Policy->VisualStyle.Get()
            : GetMutableDefault<UMythicNameplateVisualStyle>();
    }
    PrewarmPool();
    PrewarmActionRail();
    if (ULocalPlayer *LocalPlayer = GetOwningLocalPlayer()) {
        if (UMythicNameplateDirector *Director =
                LocalPlayer->GetSubsystem<UMythicNameplateDirector>()) {
            Director->AttachPresentationLayer(this, Policy);
        }
    }
}

void UMythicNameplateLayer::NativeDestruct() {
    if (ULocalPlayer *LocalPlayer = GetOwningLocalPlayer()) {
        if (UMythicNameplateDirector *Director =
                LocalPlayer->GetSubsystem<UMythicNameplateDirector>()) {
            Director->DetachPresentationLayer(this);
        }
    }
    ReleaseAllProjections();
    // Keep the fixed UObject pool and panel children alive. Reconstructing the
    // Slate tree must not strand or duplicate the prewarmed children.
    Super::NativeDestruct();
}

bool UMythicNameplateLayer::ApplyProjection(
    const FMythicNameplateProjection &Projection,
    const FVector2D ScreenPosition, const float PresentationAlpha,
    const float PresentationScale) {
    if (!Projection.IsPresentable() || !PlateCanvas) {
        return false;
    }

    int32 SlotIndex = FindClaimedSlot(Projection.Instance);
    if (SlotIndex == INDEX_NONE) {
        SlotIndex = FindFreeSlot();
        if (SlotIndex == INDEX_NONE) {
            return false;
        }
        ClaimedInstances[SlotIndex] = Projection.Instance;
    }

    UMythicNameplateWidget *Widget = PooledWidgets[SlotIndex];
    UMythicNameplateViewModel *ViewModel = PooledViewModels[SlotIndex];
    if (!Widget || !ViewModel) {
        ClaimedInstances[SlotIndex].Reset();
        return false;
    }

    const FVector2D LocalPosition = ToLocalSlatePosition(ScreenPosition);
    ViewModel->Apply(Projection, LocalPosition, PresentationAlpha);
    if (UCanvasPanelSlot *CanvasSlot =
            Cast<UCanvasPanelSlot>(Widget->Slot)) {
        CanvasSlot->SetZOrder(ResolveSemanticZOrder(Projection.Lane));
    }
    Widget->SetRenderTranslation(LocalPosition);
    Widget->SetRenderScale(FVector2D(ResolvePresentationScale(
        RenderPreferences, PresentationScale)));
    const float ResolvedAlpha = ResolvePresentationAlpha(
        RenderPreferences, PresentationAlpha);
    Widget->SetRenderOpacity(ResolvedAlpha);
    Widget->SetVisibility(ResolvedAlpha > 0.0f
        ? ESlateVisibility::SelfHitTestInvisible
        : ESlateVisibility::Hidden);
    Widget->NotifyProjectionChanged();
    return true;
}

bool UMythicNameplateLayer::UpdateProjectionPlacement(
    const FMythicEntityPresentationInstance &Instance,
    const FVector2D ScreenPosition, const float PresentationAlpha,
    const float PresentationScale) {
    const int32 SlotIndex = FindClaimedSlot(Instance);
    if (SlotIndex == INDEX_NONE || !PooledWidgets.IsValidIndex(SlotIndex)
        || !PooledViewModels.IsValidIndex(SlotIndex)) {
        return false;
    }

    UMythicNameplateWidget *Widget = PooledWidgets[SlotIndex];
    UMythicNameplateViewModel *ViewModel = PooledViewModels[SlotIndex];
    if (!Widget || !ViewModel) {
        return false;
    }

    const FVector2D LocalPosition = ToLocalSlatePosition(ScreenPosition);
    ViewModel->ApplyPlacement(LocalPosition, PresentationAlpha);
    Widget->SetRenderTranslation(LocalPosition);
    Widget->SetRenderScale(FVector2D(ResolvePresentationScale(
        RenderPreferences, PresentationScale)));
    const float ResolvedAlpha = ResolvePresentationAlpha(
        RenderPreferences, PresentationAlpha);
    Widget->SetRenderOpacity(ResolvedAlpha);
    Widget->SetVisibility(ResolvedAlpha > 0.0f
        ? ESlateVisibility::SelfHitTestInvisible
        : ESlateVisibility::Hidden);
    return true;
}

bool UMythicNameplateLayer::ApplyActionRailProjection(
    const FMythicNameplateActionRailProjection &Projection,
    const FVector2D ScreenPosition, const float PresentationAlpha,
    const float PresentationScale) {
    if (!ActionRailWidget) {
        return false;
    }
    if (!Projection.IsPresentable()) {
        ActionRailInstance.Reset();
        ActionRailWidget->ResetForPool();
        return false;
    }

    ActionRailInstance = Projection.Instance;
    ActionRailWidget->ApplyProjection(Projection);
    ActionRailWidget->SetRenderTranslation(
        ToLocalSlatePosition(ScreenPosition));
    ActionRailWidget->SetRenderScale(FVector2D(ResolvePresentationScale(
        RenderPreferences, PresentationScale)));
    const float ResolvedAlpha = ResolvePresentationAlpha(
        RenderPreferences, PresentationAlpha);
    ActionRailWidget->SetRenderOpacity(ResolvedAlpha);
    ActionRailWidget->SetVisibility(ResolvedAlpha > 0.0f
        ? ESlateVisibility::SelfHitTestInvisible
        : ESlateVisibility::Hidden);
    return true;
}

void UMythicNameplateLayer::SetActionRailBindings(
    const TMap<FGameplayTag, FUIActionBindingHandle> &InActionBindings,
    const FUIActionBindingHandle InInspectBinding) {
    if (ActionRailWidget) {
        ActionRailWidget->SetActionBindings(InActionBindings,
                                            InInspectBinding);
    }
}

bool UMythicNameplateLayer::UpdateActionRailPlacement(
    const FMythicEntityPresentationInstance &Instance,
    const FVector2D ScreenPosition, const float PresentationAlpha,
    const float PresentationScale) {
    if (!Instance.IsValid() || Instance != ActionRailInstance
        || !ActionRailWidget) {
        return false;
    }

    ActionRailWidget->SetRenderTranslation(
        ToLocalSlatePosition(ScreenPosition));
    ActionRailWidget->SetRenderScale(FVector2D(ResolvePresentationScale(
        RenderPreferences, PresentationScale)));
    const float ResolvedAlpha = ResolvePresentationAlpha(
        RenderPreferences, PresentationAlpha);
    ActionRailWidget->SetRenderOpacity(ResolvedAlpha);
    ActionRailWidget->SetVisibility(ResolvedAlpha > 0.0f
        ? ESlateVisibility::SelfHitTestInvisible
        : ESlateVisibility::Hidden);
    return true;
}

void UMythicNameplateLayer::ReleaseActionRailProjection(
    const FMythicEntityPresentationInstance &Instance) {
    if (!Instance.IsValid() || Instance != ActionRailInstance) {
        return;
    }
    ActionRailInstance.Reset();
    if (ActionRailWidget) {
        ActionRailWidget->ResetForPool();
    }
}

void UMythicNameplateLayer::ReleaseProjection(
    const FMythicEntityPresentationInstance &Instance) {
    const int32 SlotIndex = FindClaimedSlot(Instance);
    if (SlotIndex == INDEX_NONE) {
        ReleaseActionRailProjection(Instance);
        return;
    }
    ClaimedInstances[SlotIndex].Reset();
    if (PooledWidgets.IsValidIndex(SlotIndex) && PooledWidgets[SlotIndex]) {
        PooledWidgets[SlotIndex]->ResetForPool();
    }
    ReleaseActionRailProjection(Instance);
}

void UMythicNameplateLayer::ReleaseAllProjections() {
    for (int32 Index = 0; Index < ClaimedInstances.Num(); ++Index) {
        ClaimedInstances[Index].Reset();
        if (PooledWidgets.IsValidIndex(Index) && PooledWidgets[Index]) {
            PooledWidgets[Index]->ResetForPool();
        }
    }
    ActionRailInstance.Reset();
    if (ActionRailWidget) {
        ActionRailWidget->ResetForPool();
    }
}

void UMythicNameplateLayer::SetRenderPreferences(
    const FMythicNameplateRenderPreferences &InPreferences) {
    RenderPreferences = InPreferences;
    RenderPreferences.Scale = FMath::Clamp(RenderPreferences.Scale,
                                            0.75f, 1.5f);
    for (UMythicNameplateWidget *Widget : PooledWidgets) {
        if (Widget) {
            Widget->SetPresentationStyle(VisualStyle, RenderPreferences);
        }
    }
    if (ActionRailWidget) {
        ActionRailWidget->SetPresentationStyle(VisualStyle,
                                                RenderPreferences);
    }
}

int32 UMythicNameplateLayer::GetClaimedSlotCount() const {
    int32 ClaimedCount = 0;
    for (const FMythicEntityPresentationInstance &Instance :
         ClaimedInstances) {
        ClaimedCount += Instance.IsValid() ? 1 : 0;
    }
    return ClaimedCount;
}

float UMythicNameplateLayer::GetScreenPixelsPerLogicalPixel() const {
    return FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.05f)
        * FMath::Clamp(RenderPreferences.Scale, 0.75f, 1.5f);
}

const UMythicNameplateVisualStyle *
UMythicNameplateLayer::GetVisualStyle() const {
    return VisualStyle
        ? VisualStyle.Get()
        : GetDefault<UMythicNameplateVisualStyle>();
}

int32 UMythicNameplateLayer::FindClaimedSlot(
    const FMythicEntityPresentationInstance &Instance) const {
    return Instance.IsValid() ? ClaimedInstances.IndexOfByKey(Instance)
                              : INDEX_NONE;
}

int32 UMythicNameplateLayer::FindFreeSlot() const {
    return ClaimedInstances.IndexOfByPredicate(
        [](const FMythicEntityPresentationInstance &Instance) {
            return !Instance.IsValid();
        });
}

void UMythicNameplateLayer::PrewarmPool() {
    if (!PlateCanvas || !NameplateWidgetClass || !PooledWidgets.IsEmpty()) {
        return;
    }

    const int32 PoolSize = FMath::Clamp(
        Policy ? Policy->Capacity.PoolSize : 16, 1, 64);
    PooledWidgets.Reserve(PoolSize);
    PooledViewModels.Reserve(PoolSize);
    ClaimedInstances.Reserve(PoolSize);

    for (int32 Index = 0; Index < PoolSize; ++Index) {
        UMythicNameplateWidget *Widget =
            CreateWidget<UMythicNameplateWidget>(GetOwningPlayer(),
                                                 NameplateWidgetClass);
        if (!Widget) {
            break;
        }
        UMythicNameplateViewModel *ViewModel =
            NewObject<UMythicNameplateViewModel>(this);
        Widget->SetNameplateViewModel(ViewModel);
        Widget->SetPresentationStyle(VisualStyle, RenderPreferences);
        Widget->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
        Widget->SetRenderTranslation(FVector2D::ZeroVector);
        Widget->SetVisibility(ESlateVisibility::Collapsed);

        UCanvasPanelSlot *CanvasSlot = PlateCanvas->AddChildToCanvas(Widget);
        CanvasSlot->SetPosition(FVector2D::ZeroVector);
        CanvasSlot->SetAutoSize(true);
        CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));

        PooledWidgets.Add(Widget);
        PooledViewModels.Add(ViewModel);
        ClaimedInstances.AddDefaulted();
    }
}

void UMythicNameplateLayer::PrewarmActionRail() {
    if (!PlateCanvas || !ActionRailWidgetClass || ActionRailWidget) {
        return;
    }

    ActionRailWidget = CreateWidget<UMythicNameplateActionRailWidget>(
        GetOwningPlayer(), ActionRailWidgetClass);
    if (!ActionRailWidget) {
        return;
    }

    ActionRailWidget->SetPresentationStyle(VisualStyle,
                                            RenderPreferences);
    ActionRailWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.0f));
    ActionRailWidget->SetRenderTranslation(FVector2D::ZeroVector);
    ActionRailWidget->SetVisibility(ESlateVisibility::Collapsed);

    UCanvasPanelSlot *CanvasSlot =
        PlateCanvas->AddChildToCanvas(ActionRailWidget);
    CanvasSlot->SetPosition(FVector2D::ZeroVector);
    CanvasSlot->SetAutoSize(true);
    CanvasSlot->SetAlignment(FVector2D(0.5f, 0.0f));
    CanvasSlot->SetZOrder(450);
}

FVector2D UMythicNameplateLayer::ToLocalSlatePosition(
    const FVector2D ScreenPosition) const {
    if (ScreenPosition.ContainsNaN()) {
        return FVector2D::ZeroVector;
    }

    // ProjectWorldLocationToScreen(..., true) returns pixels relative to this
    // LocalPlayer. Convert through the player-screen geometry into absolute
    // Slate space before entering the authored canvas. This preserves both
    // split-screen origins and nonzero platform safe-zone offsets.
    const float ViewportScale = FMath::Max(
        UWidgetLayoutLibrary::GetViewportScale(this), 0.05f);
    const FVector2D PlayerLocalPosition = ScreenPosition / ViewportScale;
    APlayerController *PlayerController = GetOwningPlayer();
    const FGeometry PlayerGeometry =
        UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(
            PlayerController);
    const FGeometry CanvasGeometry = PlateCanvas
        ? PlateCanvas->GetCachedGeometry() : FGeometry();
    if (PlayerController
        && !PlayerGeometry.GetLocalSize().IsNearlyZero()
        && !CanvasGeometry.GetLocalSize().IsNearlyZero()) {
        return CanvasGeometry.AbsoluteToLocal(
            PlayerGeometry.LocalToAbsolute(PlayerLocalPosition));
    }
    return PlayerLocalPosition;
}
