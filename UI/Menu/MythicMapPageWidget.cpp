// Copyright Stellar Games. All Rights Reserved.

#include "MythicMapPageWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Player/MythicPlayerController.h"
#include "UI/MythicUIStyle.h"

namespace {
const TCHAR *MarkPaths[] = {
    TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_MapMark_Settlement.MI_UI_MapMark_Settlement"),
    TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_MapMark_Capital.MI_UI_MapMark_Capital"),
    TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_MapMark_Player.MI_UI_MapMark_Player"),
    TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_MapMark_Encounter.MI_UI_MapMark_Encounter"),
    TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_MapMark_Objective.MI_UI_MapMark_Objective"),
    TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_MapMark_Waypoint.MI_UI_MapMark_Waypoint"),
};
static_assert(UE_ARRAY_COUNT(MarkPaths) == static_cast<int32>(EMythicWarMapMarkerKind::COUNT),
              "one mark material per marker kind");

UMaterialInterface *MarkMaterial(EMythicWarMapMarkerKind Kind) {
    static TWeakObjectPtr<UMaterialInterface> Cache[UE_ARRAY_COUNT(MarkPaths)];
    const int32 Index = static_cast<int32>(Kind);
    if (Index < 0 || Index >= static_cast<int32>(UE_ARRAY_COUNT(MarkPaths))) {
        return nullptr;
    }
    if (!Cache[Index].IsValid()) {
        Cache[Index] = LoadObject<UMaterialInterface>(nullptr, MarkPaths[Index]);
    }
    return Cache[Index].Get();
}

const FLinearColor LabelInk(0.022f, 0.016f, 0.011f, 1.0f);
const FLinearColor LabelHalo(0.255f, 0.171f, 0.090f, 0.70f);
const FLinearColor MarkPaper(0.255f, 0.171f, 0.090f, 1.0f);

FLinearColor FactionWash(const FColor &Raw) {
    FLinearColor HSV = FLinearColor(Raw).LinearRGBToHSV();
    HSV.G = FMath::Min(HSV.G, 0.55f);
    HSV.B = FMath::Clamp(HSV.B, 0.18f, 0.42f);
    FLinearColor Out = HSV.HSVToLinearRGB();
    Out.A = 1.0f;
    return Out;
}

constexpr float LabelMinSeparation = 0.045f;

int32 LabelPriority(EMythicWarMapMarkerKind Kind) {
    switch (Kind) {
        case EMythicWarMapMarkerKind::Capital: return 4;
        case EMythicWarMapMarkerKind::Waypoint: return 3;
        case EMythicWarMapMarkerKind::Objective: return 2;
        case EMythicWarMapMarkerKind::Settlement: return 1;
        default: return 0;
    }
}
}

void UMythicTravelClickProxy::HandleClicked() {
    if (UMythicMapPageWidget *Owner = Page.Get()) {
        Owner->TravelTo(POIId);
    }
}


void UMythicMapPageWidget::NativeConstruct() {
    if (!bPoolsBuilt) {
        bPoolsBuilt = true;

        if (MarkerCanvas) {
            for (int32 i = 0; i < PrewarmPinCount; ++i) {
                GetOrCreatePin(i);
            }
            PlayerPin = WidgetTree->ConstructWidget<UImage>();
            FSlateBrush Brush;
            if (UMaterialInterface *Mark = MarkMaterial(EMythicWarMapMarkerKind::Player)) {
                Brush.SetResourceObject(Mark);
            }
            else if (PinTexture) {
                Brush.SetResourceObject(PinTexture);
            }
            Brush.ImageSize = MajorPinSize;
            PlayerPin->SetBrush(Brush);
            if (UMaterialInstanceDynamic *MID = PlayerPin->GetDynamicMaterial()) {
                MID->SetVectorParameterValue(TEXT("Fill"), PlayerPinColor);
                MID->SetVectorParameterValue(TEXT("Ink"), LabelInk);
                MID->SetVectorParameterValue(TEXT("Paper"), MarkPaper);
            }
            PlayerPin->SetColorAndOpacity(FLinearColor::White);
            PlayerPin->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
            PlayerPin->SetVisibility(ESlateVisibility::Collapsed);
            MarkerCanvas->AddChild(PlayerPin);
        }
        if (LegendBox) {
            for (int32 i = 0; i < PrewarmLegendCount; ++i) {
                GetOrCreateLegendRow(i);
            }
        }
        if (TravelList) {
            for (int32 i = 0; i < PrewarmTravelCount; ++i) {
                GetOrCreateTravelRow(i);
            }
        }
    }

    Super::NativeConstruct();
}

void UMythicMapPageWidget::NativeOnActivated() {
    Super::NativeOnActivated();
    SetupTerrainLayer();
    RefreshFromSubsystem();
}


FMythicMapPin &UMythicMapPageWidget::GetOrCreatePin(int32 Index) {
    if (PinPool.IsValidIndex(Index)) {
        return PinPool[Index];
    }

    FMythicMapPin Pin;

    Pin.Icon = WidgetTree->ConstructWidget<UImage>();
    FSlateBrush Brush;
    if (UMaterialInterface *Mark = MarkMaterial(EMythicWarMapMarkerKind::Settlement)) {
        Brush.SetResourceObject(Mark);
    }
    else if (PinTexture) {
        Brush.SetResourceObject(PinTexture);
    }
    Brush.ImageSize = PinSize;
    Pin.Icon->SetBrush(Brush);
    if (UMaterialInstanceDynamic *MID = Pin.Icon->GetDynamicMaterial()) {
        MID->SetVectorParameterValue(TEXT("Ink"), LabelInk);
        MID->SetVectorParameterValue(TEXT("Paper"), MarkPaper);
    }
    Pin.Icon->SetVisibility(ESlateVisibility::Collapsed);
    MarkerCanvas->AddChild(Pin.Icon);

    Pin.Label = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
    Pin.Label->SetColorAndOpacity(FSlateColor(LabelInk));
    Pin.Label->SetShadowOffset(FVector2D(1.0f, 1.0f));
    Pin.Label->SetShadowColorAndOpacity(LabelHalo);
    Pin.Label->SetVisibility(ESlateVisibility::Collapsed);
    MarkerCanvas->AddChild(Pin.Label);

    PinPool.Add(Pin);
    return PinPool.Last();
}

FMythicMapLegendRow &UMythicMapPageWidget::GetOrCreateLegendRow(int32 Index) {
    if (LegendPool.IsValidIndex(Index)) {
        return LegendPool[Index];
    }

    FMythicMapLegendRow Row;
    UHorizontalBox *Box = WidgetTree->ConstructWidget<UHorizontalBox>();
    Row.Box = Box;

    Row.Swatch = WidgetTree->ConstructWidget<UImage>();
    FSlateBrush SwatchBrush;
    if (UMaterialInterface *Mark = MarkMaterial(EMythicWarMapMarkerKind::Settlement)) {
        SwatchBrush.SetResourceObject(Mark);
    }
    SwatchBrush.ImageSize = FVector2D(14.0f, 14.0f);
    Row.Swatch->SetBrush(SwatchBrush);
    if (UMaterialInstanceDynamic *MID = Row.Swatch->GetDynamicMaterial()) {
        MID->SetVectorParameterValue(TEXT("Ink"), FMythicUIStyle::Get().Ink);
        MID->SetVectorParameterValue(TEXT("Paper"), FLinearColor(0.0f, 0.0f, 0.0f, 0.0f));
        MID->SetScalarParameterValue(TEXT("HaloPx"), 0.0f);
    }
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Box->AddChild(Row.Swatch))) {
        S->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
        S->SetVerticalAlignment(VAlign_Center);
    }

    Row.Name = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Box->AddChild(Row.Name))) {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    Row.Count = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Box->AddChild(Row.Count))) {
        S->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
    }

    Box->SetVisibility(ESlateVisibility::Collapsed);
    LegendBox->AddChild(Box);

    LegendPool.Add(Row);
    return LegendPool.Last();
}

FMythicMapTravelRow &UMythicMapPageWidget::GetOrCreateTravelRow(int32 Index) {
    if (TravelPool.IsValidIndex(Index)) {
        return TravelPool[Index];
    }

    FMythicMapTravelRow Row;
    UCommonTextBlock *Label = nullptr;
    Row.Button = FMythicUIStyle::MakeButton(this, EMythicTextRole::Body, Label);
    Row.Label = Label;

    Row.Proxy = NewObject<UMythicTravelClickProxy>(this);
    Row.Proxy->Page = this;
    FMythicUIStyle::BindButtonClicked(Row.Button, Row.Proxy,
                                      GET_FUNCTION_NAME_CHECKED(UMythicTravelClickProxy, HandleClicked));

    Row.Button->SetVisibility(ESlateVisibility::Collapsed);
    TravelList->AddChild(Row.Button);

    TravelPool.Add(Row);
    return TravelPool.Last();
}


void UMythicMapPageWidget::PlaceOnCanvas(UWidget *Widget, const FVector2D &Normalized, const FVector2D &Size,
                                         float VerticalNudge) {
    UCanvasPanelSlot *S = Cast<UCanvasPanelSlot>(Widget->Slot);
    if (!S) {
        return;
    }
    S->SetAnchors(FAnchors(Normalized.X, Normalized.Y, Normalized.X, Normalized.Y));
    S->SetAlignment(FVector2D(0.5f, 0.5f));
    S->SetAutoSize(false);
    S->SetOffsets(FMargin(0.0f, VerticalNudge, Size.X, Size.Y));
}

void UMythicMapPageWidget::SetupTerrainLayer() {
    if (!MapTerrain) {
        return;
    }

    // The grid region of the World Partition minimap, exported to a standalone asset by
    // scratchpad/export_minimap_crop.py after Build > Build Minimap. A standalone asset resolves in PIE and
    // packaged builds where the editor-only WorldPartitionMiniMap actor does not; rerun the export when the
    // level changes and the map follows, no bake and no hand-work. The material stylizes it to parchment.
    static const FSoftObjectPath TerrainPath(
        TEXT("/Game/Mythic/UI/Globals/textures/T_UI_WorldMinimap.T_UI_WorldMinimap"));
    UTexture2D *Texture = Cast<UTexture2D>(TerrainPath.TryLoad());
    UMaterialInterface *Base = MapTerrainMaterial.LoadSynchronous();
    if (!Texture || !Base) {
        MapTerrain->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    UMaterialInstanceDynamic *MID = Cast<UMaterialInstanceDynamic>(MapTerrain->GetBrush().GetResourceObject());
    if (!MID || MID->Parent != Base) {
        FSlateBrush Brush = MapTerrain->GetBrush();
        Brush.SetResourceObject(Base);
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.TintColor = FSlateColor(FLinearColor::White);
        MapTerrain->SetBrush(Brush);
        MID = MapTerrain->GetDynamicMaterial();
    }
    if (MID) {
        // The asset is already cropped to the grid, so the material samples its full extent.
        MID->SetTextureParameterValue(TEXT("MiniMap"), Texture);
        MID->SetVectorParameterValue(TEXT("UVMin"), FLinearColor(0.0f, 0.0f, 0.0f, 1.0f));
        MID->SetVectorParameterValue(TEXT("UVSize"), FLinearColor(1.0f, 1.0f, 0.0f, 1.0f));
    }
    MapTerrain->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMythicMapPageWidget::OnWarMapTextureReady_Implementation(UTexture2D *Texture) {
    if (MapImage && Texture) {
        static const FSoftObjectPath TerritoryPath(TEXT("/Game/Mythic/UI/Globals/materials/M_UI_MapTerritory.M_UI_MapTerritory"));
        UMaterialInterface *Territory = Cast<UMaterialInterface>(TerritoryPath.TryLoad());
        UMaterialInstanceDynamic *MID = nullptr;
        if (Territory) {
            if (!Cast<UMaterialInstanceDynamic>(MapImage->GetBrush().GetResourceObject())) {
                FSlateBrush Brush = MapImage->GetBrush();
                Brush.SetResourceObject(Territory);
                Brush.DrawAs = ESlateBrushDrawType::Image;
                Brush.TintColor = FSlateColor(FLinearColor::White);
                MapImage->SetBrush(Brush);
            }
            MID = MapImage->GetDynamicMaterial();
        }
        if (MID) {
            MID->SetTextureParameterValue(TEXT("Territory"), Texture);
            MID->SetVectorParameterValue(TEXT("TexSize"), FLinearColor(static_cast<float>(Texture->GetSizeX()),
                                                                       static_cast<float>(Texture->GetSizeY()), 0.0f, 1.0f));
            MID->SetVectorParameterValue(TEXT("Ink"), LabelInk);
            return;
        }
        MapImage->SetBrushFromTexture(Texture, false);
    }
}

void UMythicMapPageWidget::OnWarMapDataRefreshed_Implementation(const TArray<FMythicWarMapLegendEntry> &Legend,
                                                                const TArray<FMythicWarMapMarker> &Markers,
                                                                const FMythicWarMapMarker &PlayerMarker) {
    ApplyMarkers(Markers, PlayerMarker);
    ApplyLegend(Legend);
    ApplyTravelList(Markers);
}

void UMythicMapPageWidget::ApplyMarkers(const TArray<FMythicWarMapMarker> &Markers,
                                        const FMythicWarMapMarker &PlayerMarker) {
    if (!MarkerCanvas) {
        return;
    }

    TArray<FVector2D> LabelledAt;
    TArray<bool> ShowLabel;
    ShowLabel.SetNumZeroed(Markers.Num());
    {
        TArray<int32> Order;
        Order.Reserve(Markers.Num());
        for (int32 i = 0; i < Markers.Num(); ++i) {
            if (!Markers[i].Label.IsEmpty() && LabelPriority(Markers[i].Kind) > 0) {
                Order.Add(i);
            }
        }
        Order.Sort([&Markers](int32 A, int32 B) {
            return LabelPriority(Markers[A].Kind) > LabelPriority(Markers[B].Kind);
        });
        for (int32 i : Order) {
            const FVector2D &At = Markers[i].NormalizedPos;
            bool bClear = true;
            for (const FVector2D &Other : LabelledAt) {
                if (FVector2D::DistSquared(At, Other) < LabelMinSeparation * LabelMinSeparation) {
                    bClear = false;
                    break;
                }
            }
            const bool bMajorKind = Markers[i].Kind == EMythicWarMapMarkerKind::Capital ||
                                    Markers[i].Kind == EMythicWarMapMarkerKind::Waypoint;
            if (bClear || bMajorKind) {
                ShowLabel[i] = true;
                LabelledAt.Add(At);
            }
        }
    }

    int32 Used = 0;
    for (int32 Index = 0; Index < Markers.Num(); ++Index) {
        const FMythicWarMapMarker &M = Markers[Index];
        FMythicMapPin &Pin = GetOrCreatePin(Used++);

        const bool bMajor = M.Kind == EMythicWarMapMarkerKind::Capital || M.Kind == EMythicWarMapMarkerKind::Waypoint;
        const FVector2D Size = bMajor ? MajorPinSize : PinSize;

        if (UMaterialInstanceDynamic *MID = Pin.Icon->GetDynamicMaterial()) {
            MID->SetScalarParameterValue(TEXT("Kind"), static_cast<float>(static_cast<int32>(M.Kind)));
            const bool bFactionColoured = M.Kind == EMythicWarMapMarkerKind::Settlement ||
                                          M.Kind == EMythicWarMapMarkerKind::Capital ||
                                          M.Kind == EMythicWarMapMarkerKind::Encounter;
            const FLinearColor Fill = bFactionColoured ? FactionWash(M.Color) : FLinearColor(M.Color);
            MID->SetVectorParameterValue(TEXT("Fill"), Fill);
            MID->SetVectorParameterValue(TEXT("Paper"), FMath::Lerp(MarkPaper, Fill, bFactionColoured ? 0.35f : 0.0f));
        }
        Pin.Icon->SetColorAndOpacity(FLinearColor::White);
        Pin.Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
        PlaceOnCanvas(Pin.Icon, M.NormalizedPos, Size, 0.0f);

        const bool bLabelled = ShowLabel[Index];
        if (bLabelled) {
            Pin.Label->SetText(M.Label);
            Pin.Label->SetVisibility(ESlateVisibility::HitTestInvisible);
            PlaceOnCanvas(Pin.Label, M.NormalizedPos, FVector2D(140.0f, 18.0f), Size.Y * 0.5f + 9.0f + 3.0f);
        }
        else {
            Pin.Label->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    for (int32 i = Used; i < PinPool.Num(); ++i) {
        PinPool[i].Icon->SetVisibility(ESlateVisibility::Collapsed);
        PinPool[i].Label->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (PlayerPin) {
        PlayerPin->SetVisibility(ESlateVisibility::HitTestInvisible);
        PlaceOnCanvas(PlayerPin, PlayerMarker.NormalizedPos, MajorPinSize, 0.0f);
        if (const APlayerController *PC = GetOwningPlayer()) {
            if (const APawn *Pawn = PC->GetPawn()) {
                PlayerPin->SetRenderTransformAngle(90.0f - Pawn->GetActorRotation().Yaw);
            }
        }
    }
}

void UMythicMapPageWidget::ApplyLegend(const TArray<FMythicWarMapLegendEntry> &Legend) {
    if (!LegendBox) {
        return;
    }

    int32 Used = 0;
    for (const FMythicWarMapLegendEntry &E : Legend) {
        FMythicMapLegendRow &Row = GetOrCreateLegendRow(Used++);
        Row.Box->SetVisibility(ESlateVisibility::HitTestInvisible);
        if (UMaterialInstanceDynamic *MID = Row.Swatch->GetDynamicMaterial()) {
            MID->SetVectorParameterValue(TEXT("Fill"), FactionWash(E.Color));
        }
        Row.Swatch->SetColorAndOpacity(FLinearColor::White);
        Row.Name->SetText(E.DisplayName);
        Row.Count->SetText(FText::AsNumber(E.ControlledCellCount));
    }

    for (int32 i = Used; i < LegendPool.Num(); ++i) {
        LegendPool[i].Box->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (UWidget *Heading = GetWidgetFromName(TEXT("Hdr_Powers"))) {
        Heading->SetVisibility(Used > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}

void UMythicMapPageWidget::ApplyTravelList(const TArray<FMythicWarMapMarker> &Markers) {
    if (!TravelList) {
        return;
    }

    TArray<const FMythicWarMapMarker *> Destinations;
    for (const FMythicWarMapMarker &M : Markers) {
        if (M.Kind == EMythicWarMapMarkerKind::Waypoint && M.SourceId != INDEX_NONE) {
            Destinations.Add(&M);
        }
    }
    Destinations.Sort([](const FMythicWarMapMarker &A, const FMythicWarMapMarker &B) {
        return A.Label.CompareTo(B.Label) < 0;
    });

    int32 Used = 0;
    for (const FMythicWarMapMarker *M : Destinations) {
        FMythicMapTravelRow &Row = GetOrCreateTravelRow(Used++);
        Row.Proxy->POIId = M->SourceId;
        Row.Label->SetText(M->Label);
        Row.Button->SetVisibility(ESlateVisibility::Visible);
    }

    for (int32 i = Used; i < TravelPool.Num(); ++i) {
        TravelPool[i].Button->SetVisibility(ESlateVisibility::Collapsed);
        TravelPool[i].Proxy->POIId = INDEX_NONE;
    }

    const bool bNone = Destinations.Num() == 0;
    FMythicUIStyle::ShowEmptyState(this, TEXT("EmptyState_Travel"), bNone);
    if (Txt_TravelHint) {
        Txt_TravelHint->SetText(NSLOCTEXT("Mythic", "TravelRule", "Depart from any landmark you have found."));
        Txt_TravelHint->SetVisibility(bNone ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }
}


void UMythicMapPageWidget::TravelTo(int32 POIId) {
    if (POIId == INDEX_NONE) {
        return;
    }
    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer())) {
        PC->ServerFastTravelToPOI(POIId);
    }
}
