#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Nameplate/MythicNameplateTypes.h"
#include "MythicNameplateWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UImage;
class UMythicBarWidget;
class UMythicNameplateViewModel;
class UMythicNameplateVisualStyle;
class USizeBox;
class UTextBlock;

/** Blueprint-skinnable, allocation-stable renderer for one local nameplate pool slot. */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UMythicNameplateWidget : public UUserWidget {
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** Returns this pool slot's local view model, or null before the owning layer prewarms and binds it. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Nameplate")
    UMythicNameplateViewModel *GetNameplateViewModel() const {
        return ViewModel;
    }

    /** Binds the layer-owned allocation-stable view model; null releases visual state without gameplay side effects. */
    void SetNameplateViewModel(UMythicNameplateViewModel *InViewModel);

    /**
     * Returns local render-only accessibility preferences. They never alter disclosure or reveal additional
     * LivingWorld information.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Nameplate|Accessibility")
    const FMythicNameplateRenderPreferences &GetRenderPreferences() const {
        return RenderPreferences;
    }

    /** Applies project visual tokens and local accessibility preferences without querying gameplay. */
    void SetPresentationStyle(
        UMythicNameplateVisualStyle *InVisualStyle,
        const FMythicNameplateRenderPreferences &InPreferences);

    /** Applies updated local accessibility preferences while retaining the current project visual style. */
    void SetRenderPreferences(
        const FMythicNameplateRenderPreferences &InPreferences);

    /**
     * Called after native code applies a new immutable projection. Blueprint may animate existing children but must
     * not query gameplay or create, remove, or reorder widgets.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Nameplate")
    void OnNameplateProjectionChanged();

    /** Called when this fixed slot is released so Blueprint can clear animation and material state before reuse. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Nameplate")
    void OnNameplateReleased();

    /**
     * Called when local accessibility rendering preferences change. Blueprint may alter cosmetics but cannot
     * reinterpret the viewer-safe semantic projection.
     */
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Mythic|Nameplate|Accessibility")
    void OnNameplateRenderPreferencesChanged();

    /** Notifies the renderer after the layer updates the bound view model. */
    void NotifyProjectionChanged();

    /** Resets the renderer and collapses it before another subject claims this fixed pool slot. */
    void ResetForPool();

protected:
    /** Transparent structural container; world-space nameplates never paint a card background. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (BindWidgetOptional))
    TObjectPtr<UBorder> PlateSurface;

    /** Fixed maximum-bounds container selected from the resolved visual family. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (BindWidgetOptional))
    TObjectPtr<USizeBox> PlateSizeBox;

    /** Viewer-safe one-line identity label; native rendering enforces ellipsis and disables wrapping. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> NameText;

    /** Single learned peaceful subtitle already resolved by the local director. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> SubtitleText;

    /** Fixed bounds for the single native-selected cue, rank, or danger emblem. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (BindWidgetOptional))
    TObjectPtr<USizeBox> SemanticIconBox;

    /** Resident shape-and-color emblem selected by the mutually exclusive visual hierarchy. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (BindWidgetOptional))
    TObjectPtr<UImage> SemanticIcon;

    /** Already-localized compact level text; the widget never formats or infers combat level. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LevelText;

    /** Optional normalized health bar collapsed unless the projection explicitly permits health. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (BindWidgetOptional))
    TObjectPtr<UMythicBarWidget> HealthBar;

    /** Fixed health-band bounds selected from the current visual-family geometry. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (BindWidgetOptional))
    TObjectPtr<USizeBox> HealthSizeBox;

    /** Optional rounded health percentage for an already-entitled Focus/current-target read. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> HealthPercentText;

    /**
     * Host for four fixed status slots created during prewarm. It never receives runtime-created or reordered
     * children.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (BindWidgetOptional))
    TObjectPtr<UHorizontalBox> StatusBadgeHost;

    /** Fail-safe resolved status label used only when the authored fixed badge host is absent. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> StatusText;

private:
    friend struct FMythicNameplateWidgetTestAccess;

    static constexpr int32 StatusBadgeCapacity = 4;

    void RefreshNativeBindings();
    void ResetNativeBindings();
    void ApplyVisualStyle(const FMythicNameplateProjection &Projection);
    void PrewarmStatusBadges();
    void RefreshStatusBadges(
        const FMythicNameplateProjection &Projection);
    void ResetStatusBadges();

    /** Layer-owned view model; widgets never retain an actor or gameplay subsystem pointer. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMythicNameplateViewModel> ViewModel;

    /** Project visual tokens; this renderer never reads gameplay policy from the asset. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Mythic|Nameplate",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMythicNameplateVisualStyle> VisualStyle;

    /** Local render-only options copied from the owning LocalPlayer's fixed HUD layer. */
    UPROPERTY(Transient, BlueprintReadOnly,
              Category = "Mythic|Nameplate|Accessibility",
              meta = (AllowPrivateAccess = "true"))
    FMythicNameplateRenderPreferences RenderPreferences;

    /** Fixed status badge roots created once when the pooled widget is initialized. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UHorizontalBox>> StatusBadgeRoots;

    /** Fixed icon bounds paired by index with the status roots. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<USizeBox>> StatusBadgeIconBoxes;

    /** Fixed resident-only icon images paired by index with the status roots. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UImage>> StatusBadgeIcons;

    /** Fixed optional localized labels paired by index with the status roots. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> StatusBadgeLabels;

    /** Fixed bounded stack counters paired by index with the status roots. */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UTextBlock>> StatusBadgeStacks;

    /** Fixed overflow marker; it never reveals counts on tiers whose status cap is zero. */
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> StatusOverflowText;
};
