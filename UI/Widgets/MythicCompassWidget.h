// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/WarMap/MythicWarMapTypes.h"
#include "World/LivingWorld/Territory/MythicDanger.h"
#include "MythicCompassWidget.generated.h"

class UCanvasPanel;
class UCommonTextBlock;
class UImage;
class UMaterialInterface;
class UMythicRegionTrackerComponent;
class UMythicWarMapSubsystem;

USTRUCT()
struct FMythicCompassTick {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UImage> Icon;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Label;
};

USTRUCT(BlueprintType)
struct FMythicCompassDangerStyle {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Compass")
    EMythicDangerTier Tier = EMythicDangerTier::Safe;

    UPROPERTY(EditDefaultsOnly, Category = "Compass")
    FLinearColor Colour = FLinearColor::White;
};

USTRUCT(BlueprintType)
struct FMythicCompassKindStyle {
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, Category = "Compass")
    EMythicWarMapMarkerKind Kind = EMythicWarMapMarkerKind::Settlement;

    UPROPERTY(EditDefaultsOnly, Category = "Compass")
    FLinearColor Colour = FLinearColor::White;

    /** Bigger for the things worth crossing the map for. */
    UPROPERTY(EditDefaultsOnly, Category = "Compass", meta = (ClampMin = "2.0"))
    float Size = 12.0f;
};

UCLASS()
class MYTHIC_API UMythicCompassWidget : public UUserWidget {
    GENERATED_BODY()

public:
    /** Re-read the marker list. Cheap enough to call on any world event; it never touches the widget tree. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|HUD")
    void RefreshMarkers();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual void NativeTick(const FGeometry &MyGeometry, float InDeltaTime) override;

    /** The canvas every tick and marker is placed into. Without it the compass draws nothing. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> Strip;

    /** Optional: the current region/settlement name under the strip, tinted by danger tier. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Region;

    /** Optional: the bearing readout under the strip, e.g. "NE  042". */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Bearing;

    /** How much of the world the strip spans, in degrees either side of centre. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Compass", meta = (ClampMin = "10.0", ClampMax = "180.0"))
    float HalfArcDegrees = 60.0f;

    /**
     * Ticks past this range are dropped. A marker on the far side of the world is noise, not navigation.
     *
     * 2.5km, not 600m: in an open world a settlement you cannot yet see is exactly the thing a compass is for, and
     * at 600m nothing but your own feet ever showed up on it.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Compass", meta = (ClampMin = "0.0"))
    float MaxMarkerDistanceCm = 250000.0f;

    /** Marker ticks in the pool. Also the hard cap on how many can show at once. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Compass", meta = (ClampMin = "1"))
    int32 PrewarmMarkerTicks = 24;

    /** Seconds between marker re-gathers. The war map's own broadcast refreshes sooner when something really moves. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Compass", meta = (ClampMin = "0.1"))
    float MarkerRefreshSeconds = 2.0f;

    /**
     * The pip every tick and marker is drawn with, tinted per kind by the widget.
     *
     * A material rather than a texture, to match the rest of this kit (the vital orbs, the progress bar and the
     * ability sigils are all procedural) — one shape that stays crisp at any size, with no atlas entry to manage.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Compass")
    TObjectPtr<UMaterialInterface> TickMaterial;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Compass")
    TArray<FMythicCompassKindStyle> KindStyles;

    /** Ink colour for Txt_Region per danger tier. Authored in the WBP; a tier with no row stays white. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Compass")
    TArray<FMythicCompassDangerStyle> DangerStyles;

    /** The eight cardinal/ordinal letters. Colour and size are shared; they are structure, not content. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Compass")
    FLinearColor CardinalColour = FLinearColor(0.93f, 0.88f, 0.76f, 1.0f);

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Compass")
    FLinearColor CardinalMinorColour = FLinearColor(0.68f, 0.62f, 0.52f, 1.0f);

    /**
     * Height of the strip's own row, used to place ticks vertically.
     *
     * Keep this in step with the Plate brush's ImageSize.Y in the WBP — the plate is what actually gives the band
     * its height; this is how far from the middle of that band the pips sit.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Compass", meta = (ClampMin = "8.0"))
    float StripHeight = 46.0f;

private:
    void BuildPools();

    FMythicCompassTick &GetOrCreateTick(TArray<FMythicCompassTick> &Pool, UMaterialInterface *Brush, float IconSize,
                                        bool bWithLabel);

    void PlaceTick(const FMythicCompassTick &Tick, float StripX, float Opacity, float YOffset) const;

    const FMythicCompassKindStyle &StyleForKind(EMythicWarMapMarkerKind Kind) const;

    UMythicWarMapSubsystem *GetWarMap() const;

    class UMythicHUDLayout *FindHUDLayout();

    UPROPERTY()
    TArray<FMythicCompassTick> CardinalPool;

    UPROPERTY()
    TArray<FMythicCompassTick> MarkerPool;

    struct FMythicCompassMarker {
        FVector2D WorldXY = FVector2D::ZeroVector;
        EMythicWarMapMarkerKind Kind = EMythicWarMapMarkerKind::Settlement;
        float DistanceCm = 0.0f;
    };

    TArray<FMythicCompassMarker> WorldMarkers;

    TSet<uint64> InRangeKeys;

    UPROPERTY(Transient)
    TWeakObjectPtr<class UMythicHUDLayout> HUDLayout;

    UPROPERTY(Transient)
    TObjectPtr<class UMaterialInstanceDynamic> RodMID;

    void BindRegionTracker();

    void ApplyRegion(const FText &Region, EMythicDangerTier Tier) const;

    FLinearColor DangerColourForTier(EMythicDangerTier Tier) const;

    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicRegionTrackerComponent> RegionTracker;

    FTimerHandle RegionBindTimer;
    int32 RegionBindAttempts = 0;

    float SinceMarkerRefresh = 0.0f;
    float LastViewportScale = -1.0f;
    bool bPoolsBuilt = false;
    bool bBoundToWarMap = false;
    bool bLastDigitsLit = false;
    bool bLastRegionLit = false;

    int32 LastPrintedBearing = MIN_int32;

    UFUNCTION()
    void HandleWarMapChanged();

    UFUNCTION()
    void HandleRegionChanged(FText Region, EMythicDangerTier Tier);
};
