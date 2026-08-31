#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Nameplate/MythicNameplateTypes.h"
#include "MythicNameplateLayer.generated.h"

class UCanvasPanel;
class UMythicNameplateActionRailWidget;
class UMythicNameplatePolicy;
class UMythicNameplateVisualStyle;
class UMythicNameplateViewModel;
class UMythicNameplateWidget;

/** HUD-owned fixed widget pool and screen-space placement layer for one local player. */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UMythicNameplateLayer : public UUserWidget {
    GENERATED_BODY()

public:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /**
     * Claims/updates one fixed pool slot for a presentable projection at split-screen-relative pixels. Returns false
     * for Silent/invalid input, absent canvas/class, or exhausted pool; it never allocates after prewarm.
     */
    bool ApplyProjection(const FMythicNameplateProjection &Projection,
                         FVector2D ScreenPosition, float PresentationAlpha = 1.0f,
                         float PresentationScale = 1.0f);

    /**
     * Moves an already-claimed exact instance without copying its semantic projection or notifying Blueprint. This is
     * the allocation-free per-frame path; false means the decision layer must rebuild or release the stale claim.
     */
    bool UpdateProjectionPlacement(const FMythicEntityPresentationInstance &Instance,
                                   FVector2D ScreenPosition,
                                   float PresentationAlpha = 1.0f,
                                   float PresentationScale = 1.0f);

    /** Applies the available-only action row for the focused exact subject at the same world anchor. */
    bool ApplyActionRailProjection(
        const FMythicNameplateActionRailProjection &Projection,
        FVector2D ScreenPosition, float PresentationAlpha = 1.0f,
        float PresentationScale = 1.0f);

    /** Moves the already-prewarmed action rail without semantic copies, allocation, or layout mutation. */
    bool UpdateActionRailPlacement(
        const FMythicEntityPresentationInstance &Instance,
        FVector2D ScreenPosition, float PresentationAlpha = 1.0f,
        float PresentationScale = 1.0f);

    /** Releases the action rail only when it still represents the supplied exact handle-generation pair. */
    void ReleaseActionRailProjection(
        const FMythicEntityPresentationInstance &Instance);

    /** Releases the slot held by this exact handle/generation; stale instances cannot evict a reused slot. */
    void ReleaseProjection(const FMythicEntityPresentationInstance &Instance);

    /** Releases every fixed slot during HUD teardown, world restore, or local-player reset. */
    void ReleaseAllProjections();

    /**
     * Applies local render-only accessibility preferences to every prewarmed slot and the action rail. This never
     * changes projection entitlement, pool capacity, semantic dwell, or replicated state.
     */
    void SetRenderPreferences(
        const FMythicNameplateRenderPreferences &InPreferences);

    /** Returns the number of currently claimed visible/fading pool slots for performance diagnostics. */
    int32 GetClaimedSlotCount() const;

    /** Returns the number of widgets created during the one initialization prewarm. */
    int32 GetPrewarmedSlotCount() const { return PooledWidgets.Num(); }

    /** Returns raw player-screen pixels per authored logical pixel, including local accessibility scale. */
    float GetScreenPixelsPerLogicalPixel() const;

    /** Returns the exact resident visual style used by the pooled widgets, or the native validated fallback. */
    const UMythicNameplateVisualStyle *GetVisualStyle() const;

    /** Returns the authored policy supplied to this layer, or null when native shipping defaults are active. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Nameplate")
    UMythicNameplatePolicy *GetNameplatePolicy() const { return Policy; }

protected:
    /** Canvas authored by WBP_NameplateLayer; each pooled widget is added once and only moved/collapsed afterward. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Nameplate", meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> PlateCanvas;

    /** Blueprint visual class instantiated only during initialization; null uses the native allocation-stable base. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Nameplate")
    TSubclassOf<UMythicNameplateWidget> NameplateWidgetClass;

    /** Visual class instantiated once for the focused entity's separate one-line contextual action rail. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Nameplate")
    TSubclassOf<UMythicNameplateActionRailWidget> ActionRailWidgetClass;

    /** Optional policy asset supplying pool size/caps; null uses the shipping default of sixteen slots. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Nameplate")
    TObjectPtr<UMythicNameplatePolicy> Policy;

    /** Project visual tokens applied to every prewarmed plate and the action rail; policy may supply the same asset. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Nameplate")
    TObjectPtr<UMythicNameplateVisualStyle> VisualStyle;

private:
    friend struct FMythicNameplateLayerTestAccess;

    int32 FindClaimedSlot(const FMythicEntityPresentationInstance &Instance) const;
    int32 FindFreeSlot() const;
    void PrewarmPool();
    void PrewarmActionRail();
    FVector2D ToLocalSlatePosition(FVector2D ScreenPosition) const;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMythicNameplateWidget>> PooledWidgets;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMythicNameplateViewModel>> PooledViewModels;

    /** Precreated action-rail renderer; deliberate Inspect replaces it with a separate CommonUI page. */
    UPROPERTY(Transient)
    TObjectPtr<UMythicNameplateActionRailWidget> ActionRailWidget;

    TArray<FMythicEntityPresentationInstance> ClaimedInstances;
    FMythicEntityPresentationInstance ActionRailInstance;
    FMythicNameplateRenderPreferences RenderPreferences;
};
