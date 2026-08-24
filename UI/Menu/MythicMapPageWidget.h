// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/WarMap/MythicWarMapScreen.h"
#include "MythicMapPageWidget.generated.h"

class UButton;
class UCanvasPanel;
class UCommonTextBlock;
class UImage;
class UPanelWidget;
class UTexture2D;
class UMaterialInterface;
class UMythicMapPageWidget;

UCLASS()
class MYTHIC_API UMythicTravelClickProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicMapPageWidget> Page;

    UPROPERTY()
    int32 POIId = INDEX_NONE;

    UFUNCTION()
    void HandleClicked();
};

USTRUCT()
struct FMythicMapPin {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UImage> Icon;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Label;
};

USTRUCT()
struct FMythicMapLegendRow {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UPanelWidget> Box;

    UPROPERTY()
    TObjectPtr<UImage> Swatch;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Name;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Count;
};

USTRUCT()
struct FMythicMapTravelRow {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UWidget> Button;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Label;

    UPROPERTY()
    TObjectPtr<UMythicTravelClickProxy> Proxy;
};

UCLASS()
class MYTHIC_API UMythicMapPageWidget : public UMythicWarMapScreen {
    GENERATED_BODY()

public:
    /** Ask the server to travel to a discovered landmark. Refusals come back as a HUD notice with a reason. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Map")
    void TravelTo(int32 POIId);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnActivated() override;

    virtual void OnWarMapTextureReady_Implementation(UTexture2D *Texture) override;

    /** Points the terrain layer at the live minimap, cropped to the grid region. Idempotent. */
    void SetupTerrainLayer();
    virtual void OnWarMapDataRefreshed_Implementation(const TArray<FMythicWarMapLegendEntry> &Legend,
                                                      const TArray<FMythicWarMapMarker> &Markers,
                                                      const FMythicWarMapMarker &PlayerMarker) override;

    // ── Bound layout ──
    /** The territory texture. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> MapImage;

    /**
     * The terrain layer: the live World Partition minimap, cropped to the grid and stylized to parchment.
     *
     * Dynamic by construction - it samples the minimap texture the level build produces, so rebuilding the
     * minimap after a level change updates the map with no UI work and no bake.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> MapTerrain;

    /** Pins are placed here in normalized space, so the map scales with the panel. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> MarkerCanvas;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> LegendBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> TravelList;

    /** Explains the travel rule, and says so plainly when nothing has been discovered yet. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_TravelHint;

    // ── Designer knobs ──
    /**
     * Fallback pin art, used only if a kit mark material is missing. The pins normally draw the kit's procedural
     * cartographer's marks (M_UI_MapMark, one instance per marker kind, tint = faction colour).
     */
    /** Runtime parchment stylizer fed the live minimap. Defaults to M_UI_MapParchment. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Map")
    TSoftObjectPtr<UMaterialInterface> MapTerrainMaterial;

    /** The grid whose bounds the terrain is cropped to. Defaults to DA_TerritoryGridSettings. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Map")
    TSoftObjectPtr<class UMythicTerritoryGridSettings> TerritoryGrid;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Map")
    TObjectPtr<UTexture2D> PinTexture;

    /** 18px is the smallest size at which the marks' strokes and halo still read. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Map")
    FVector2D PinSize = FVector2D(18.0f, 18.0f);

    /** Capitals, landmarks and the player read larger than ordinary settlements and encounters. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Map")
    FVector2D MajorPinSize = FVector2D(24.0f, 24.0f);

    /** The player is the chart's one bright accent: the kit's brass, on a diamond like the travel gems. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Map")
    FLinearColor PlayerPinColor = FLinearColor(0.95f, 0.78f, 0.40f, 1.0f);

    /** Pool sizes. Built once on construct; the pools still grow if a world exceeds them. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Map", meta = (ClampMin = "0"))
    int32 PrewarmPinCount = 96;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Map", meta = (ClampMin = "0"))
    int32 PrewarmLegendCount = 12;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Map", meta = (ClampMin = "0"))
    int32 PrewarmTravelCount = 32;

private:
    FMythicMapPin &GetOrCreatePin(int32 Index);
    FMythicMapLegendRow &GetOrCreateLegendRow(int32 Index);
    FMythicMapTravelRow &GetOrCreateTravelRow(int32 Index);

    static void PlaceOnCanvas(UWidget *Widget, const FVector2D &Normalized, const FVector2D &Size, float VerticalNudge);

    void ApplyMarkers(const TArray<FMythicWarMapMarker> &Markers, const FMythicWarMapMarker &PlayerMarker);
    void ApplyLegend(const TArray<FMythicWarMapLegendEntry> &Legend);
    void ApplyTravelList(const TArray<FMythicWarMapMarker> &Markers);

    UPROPERTY()
    TArray<FMythicMapPin> PinPool;

    UPROPERTY()
    TArray<FMythicMapLegendRow> LegendPool;

    UPROPERTY()
    TArray<FMythicMapTravelRow> TravelPool;

    UPROPERTY()
    TObjectPtr<UImage> PlayerPin;

    bool bPoolsBuilt = false;
};
