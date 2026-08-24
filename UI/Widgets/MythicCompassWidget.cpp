// Copyright Stellar Games. All Rights Reserved.

#include "MythicCompassWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Engine/LocalPlayer.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "TimerManager.h"
#include "UI/MythicHUDLayout.h"
#include "UI/MythicUIStyle.h"
#include "UI/WarMap/MythicCompass.h"
#include "UI/WarMap/MythicWarMapSubsystem.h"
#include "UObject/UObjectIterator.h"
#include "World/Feedback/MythicRegionTrackerComponent.h"


namespace {
const TCHAR *Compass_CardinalLetters[4] = {TEXT("N"), TEXT("E"), TEXT("S"), TEXT("W")};

constexpr float Compass_Culled = 0.0f;

constexpr float Compass_AxisV = 0.40f;

constexpr float Compass_LetterBottomOffsetPx = 21.5f;

constexpr float Compass_EndFadeStartPx = 10.0f;
constexpr float Compass_EndFadeRunPx = 28.0f;

constexpr float Compass_OverlapGonePx = 4.0f;
constexpr float Compass_OverlapRunPx = 3.0f;

constexpr float Compass_DistanceFade = 0.35f;

constexpr float Compass_DigitsYieldGonePx = 14.0f;
constexpr float Compass_DigitsYieldRunPx = 12.0f;

constexpr float Compass_LetterLogicalSize = 15.0f;
constexpr float Compass_LetterFloorPx = 11.0f;
constexpr float Compass_DigitsLogicalSize = 13.0f;
constexpr float Compass_DigitsFloorPx = 10.0f;

const FName Compass_YawParam(TEXT("Yaw"));
const FName Compass_HalfArcParam(TEXT("HalfArc"));

struct FCompassBeadPool {
    EMythicWarMapMarkerKind Kind;
    int32 Count;
    const TCHAR *MaterialPath;
};
const FCompassBeadPool Compass_BeadPools[] = {
    {EMythicWarMapMarkerKind::Settlement, 8, TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_CompassBead_Settlement.MI_UI_CompassBead_Settlement")},
    {EMythicWarMapMarkerKind::Capital, 2, TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_CompassBead_Capital.MI_UI_CompassBead_Capital")},
    {EMythicWarMapMarkerKind::Encounter, 6, TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_CompassBead_Encounter.MI_UI_CompassBead_Encounter")},
    {EMythicWarMapMarkerKind::Waypoint, 4, TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_CompassBead_Waypoint.MI_UI_CompassBead_Waypoint")},
    {EMythicWarMapMarkerKind::Objective, 4, TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_CompassBead_Objective.MI_UI_CompassBead_Objective")},
};
const EMythicWarMapMarkerKind Compass_PlacementOrder[] = {
    EMythicWarMapMarkerKind::Objective, EMythicWarMapMarkerKind::Encounter, EMythicWarMapMarkerKind::Waypoint,
    EMythicWarMapMarkerKind::Capital, EMythicWarMapMarkerKind::Settlement,
};

const TCHAR *Compass_RodPath = TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_CompassRod.MI_UI_CompassRod");
const TCHAR *Compass_LetterFontPath = TEXT("/Game/Mythic/UI/Fonts/Aleo/Aleo-Medium_Font.Aleo-Medium_Font");

bool Compass_IsCulled(float StripX) {
    return StripX < Compass_Culled;
}

float Compass_YawToBearing(float YawDeg) {
    float Bearing = FMath::Fmod(90.0f - YawDeg, 360.0f);
    if (Bearing < 0.0f) {
        Bearing += 360.0f;
    }
    return Bearing;
}

float Compass_Px(float Px, float Scale) {
    return Px / FMath::Max(Scale, 0.05f);
}

float Compass_EndFade(float StripX, float StripWidth, float Scale) {
    const float DxEnd = FMath::Min(StripX, StripWidth - StripX);
    return FMath::Clamp((DxEnd - Compass_Px(Compass_EndFadeStartPx, Scale)) / Compass_Px(Compass_EndFadeRunPx, Scale), 0.0f, 1.0f);
}

UMaterialInterface *Compass_LoadMaterial(const TCHAR *Path) {
    return LoadObject<UMaterialInterface>(nullptr, Path);
}

uint64 Compass_MarkerKey(EMythicWarMapMarkerKind Kind, const FVector2D &XY) {
    const int64 Mx = FMath::RoundToInt(XY.X / 100.0);
    const int64 My = FMath::RoundToInt(XY.Y / 100.0);
    return (static_cast<uint64>(static_cast<uint8>(Kind)) << 56) ^ (static_cast<uint64>(Mx & 0xFFFFFFF) << 28) ^ static_cast<uint64>(My & 0xFFFFFFF);
}
}


void UMythicCompassWidget::NativeConstruct() {
    BuildPools();
    Super::NativeConstruct();

    if (UImage *Plate = Cast<UImage>(GetWidgetFromName(TEXT("Plate")))) {
        // The rail is a static bronze band texture; the cardinal marks scroll on the Strip canvas above it
        // (see UpdateStrip / CompassStripX). The old material instance was mis-parented to M_UI_ItemSlot and
        // drew a flat grey bar, so the compass read as an unfinished placeholder.
        static const FSoftObjectPath BandPath(
            TEXT("/Game/Mythic/UI/Globals/textures/T_UI_CompassBand.T_UI_CompassBand"));
        if (UTexture2D *Band = Cast<UTexture2D>(BandPath.TryLoad())) {
            FSlateBrush Brush = Plate->GetBrush();
            Brush.SetResourceObject(Band);
            Brush.DrawAs = ESlateBrushDrawType::Image;
            Brush.TintColor = FSlateColor(FLinearColor::White);
            Plate->SetBrush(Brush);
            Plate->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }

    if (UMythicHUDLayout *Layout = FindHUDLayout()) {
        if (Txt_Bearing) {
            Layout->RegisterHUDElement(Txt_Bearing, EMythicHUDSalience::Hidden);
        }
        if (Txt_Region) {
            Layout->RegisterHUDElement(Txt_Region, EMythicHUDSalience::Dim);
        }
    }

    if (UMythicWarMapSubsystem *WarMap = GetWarMap()) {
        if (!bBoundToWarMap) {
            WarMap->OnWarMapChanged.AddDynamic(this, &UMythicCompassWidget::HandleWarMapChanged);
            bBoundToWarMap = true;
        }
    }
    RefreshMarkers();
    BindRegionTracker();
}

void UMythicCompassWidget::NativeDestruct() {
    if (bBoundToWarMap) {
        if (UMythicWarMapSubsystem *WarMap = GetWarMap()) {
            WarMap->OnWarMapChanged.RemoveDynamic(this, &UMythicCompassWidget::HandleWarMapChanged);
        }
        bBoundToWarMap = false;
    }
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(RegionBindTimer);
    }
    if (UMythicRegionTrackerComponent *Tracker = RegionTracker.Get()) {
        Tracker->OnRegionDangerChanged.RemoveDynamic(this, &UMythicCompassWidget::HandleRegionChanged);
    }
    if (UMythicHUDLayout *Layout = HUDLayout.Get()) {
        if (Txt_Bearing) {
            Layout->UnregisterHUDElement(Txt_Bearing);
        }
        if (Txt_Region) {
            Layout->UnregisterHUDElement(Txt_Region);
        }
    }
    Super::NativeDestruct();
}

void UMythicCompassWidget::HandleWarMapChanged() {
    RefreshMarkers();
}

void UMythicCompassWidget::BindRegionTracker() {
    constexpr int32 MaxRegionBindAttempts = 20;

    if (!Txt_Region) {
        return;
    }
    const APlayerController *PC = GetOwningPlayer();
    const APlayerState *PS = PC ? PC->PlayerState : nullptr;
    UMythicRegionTrackerComponent *Tracker = PS ? PS->FindComponentByClass<UMythicRegionTrackerComponent>() : nullptr;
    if (!Tracker) {
        // The component reaches the client after the PlayerState replicates; the widget can construct first.
        if (UWorld *World = GetWorld(); World && ++RegionBindAttempts <= MaxRegionBindAttempts) {
            World->GetTimerManager().SetTimer(RegionBindTimer, FTimerDelegate::CreateWeakLambda(this, [this]() {
                BindRegionTracker();
            }), 0.25f, false);
        }
        return;
    }

    RegionTracker = Tracker;
    Tracker->OnRegionDangerChanged.AddDynamic(this, &UMythicCompassWidget::HandleRegionChanged);
    // The initial replication never broadcasts, so the first readout must be pulled, not pushed.
    ApplyRegion(Tracker->GetCurrentRegionName(), Tracker->GetCurrentDangerTier());
}

void UMythicCompassWidget::ApplyRegion(const FText &Region, EMythicDangerTier Tier) const {
    if (!Txt_Region) {
        return;
    }
    Txt_Region->SetText(Region);
    Txt_Region->SetColorAndOpacity(FSlateColor(DangerColourForTier(Tier)));
}

FLinearColor UMythicCompassWidget::DangerColourForTier(EMythicDangerTier Tier) const {
    for (const FMythicCompassDangerStyle &Style : DangerStyles) {
        if (Style.Tier == Tier) {
            return Style.Colour;
        }
    }
    return FLinearColor::White;
}

void UMythicCompassWidget::HandleRegionChanged(FText Region, EMythicDangerTier Tier) {
    ApplyRegion(Region, Tier);
    if (UMythicHUDLayout *Layout = FindHUDLayout()) {
        Layout->PokeElement(this);
    }
}

UMythicWarMapSubsystem *UMythicCompassWidget::GetWarMap() const {
    const ULocalPlayer *LP = GetOwningLocalPlayer();
    return LP ? LP->GetSubsystem<UMythicWarMapSubsystem>() : nullptr;
}

UMythicHUDLayout *UMythicCompassWidget::FindHUDLayout() {
    if (UMythicHUDLayout *Cached = HUDLayout.Get()) {
        return Cached;
    }
    if (UMythicHUDLayout *Outer = GetTypedOuter<UMythicHUDLayout>()) {
        HUDLayout = Outer;
        return Outer;
    }
    if (const APlayerController *PC = GetOwningPlayer()) {
        for (TObjectIterator<UMythicHUDLayout> It; It; ++It) {
            UMythicHUDLayout *Layout = *It;
            if (IsValid(Layout) && !Layout->HasAnyFlags(RF_ClassDefaultObject) && Layout->GetOwningPlayer() == PC) {
                HUDLayout = Layout;
                return Layout;
            }
        }
    }
    return nullptr;
}


void UMythicCompassWidget::BuildPools() {
    if (bPoolsBuilt || !Strip) {
        return;
    }
    bPoolsBuilt = true;

    const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.05f);
    LastViewportScale = Scale;

    UObject *LetterFont = LoadObject<UObject>(nullptr, Compass_LetterFontPath);
    for (int32 i = 0; i < 4; ++i) {
        FMythicCompassTick &Tick = GetOrCreateTick(CardinalPool, nullptr, 0.0f, true);
        if (Tick.Label) {
            Tick.Label->SetText(FText::FromString(Compass_CardinalLetters[i]));
            FSlateFontInfo Font = Tick.Label->GetFont();
            if (LetterFont) {
                Font.FontObject = LetterFont;
                Font.TypefaceFontName = TEXT("Default");
            }
            Font.Size = FMath::Max(Compass_LetterLogicalSize, Compass_Px(Compass_LetterFloorPx, Scale));
            Font.LetterSpacing = 0;
            Font.OutlineSettings.OutlineSize = 1;
            Font.OutlineSettings.OutlineColor = FLinearColor(0.0056f, 0.0043f, 0.0031f, 0.70f);
            Tick.Label->SetFont(Font);
            Tick.Label->SetColorAndOpacity(FSlateColor(CardinalColour));
            Tick.Label->SetShadowOffset(FVector2D::ZeroVector);
            Tick.Label->SetShadowColorAndOpacity(FLinearColor::Transparent);
        }
    }

    for (const FCompassBeadPool &PoolDef : Compass_BeadPools) {
        UMaterialInterface *Brush = Compass_LoadMaterial(PoolDef.MaterialPath);
        const float Size = StyleForKind(PoolDef.Kind).Size * FMath::Max(1.0f, 1.0f / Scale);
        for (int32 i = 0; i < PoolDef.Count; ++i) {
            GetOrCreateTick(MarkerPool, Brush, Size, false);
        }
    }

    Strip->SetVisibility(ESlateVisibility::HitTestInvisible);
    Strip->ForceVolatile(true);
}

FMythicCompassTick &UMythicCompassWidget::GetOrCreateTick(TArray<FMythicCompassTick> &Pool, UMaterialInterface *Brush,
                                                          float IconSize, bool bWithLabel) {
    FMythicCompassTick Tick;

    if (IconSize > 0.0f) {
        Tick.Icon = WidgetTree->ConstructWidget<UImage>();
        FSlateBrush IconBrush;
        if (Brush) {
            IconBrush.SetResourceObject(Brush);
        }
        IconBrush.DrawAs = ESlateBrushDrawType::Image;
        IconBrush.ImageSize = FVector2D(IconSize, IconSize);
        Tick.Icon->SetBrush(IconBrush);
        Tick.Icon->SetVisibility(ESlateVisibility::Collapsed);
        if (UCanvasPanelSlot *S = Cast<UCanvasPanelSlot>(Strip->AddChild(Tick.Icon))) {
            S->SetAnchors(FAnchors(0.0f, 0.5f, 0.0f, 0.5f));
            S->SetAlignment(FVector2D(0.5f, 0.5f));
            S->SetAutoSize(true);
        }
    }

    if (bWithLabel) {
        Tick.Label = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
        Tick.Label->SetVisibility(ESlateVisibility::Collapsed);
        if (UCanvasPanelSlot *S = Cast<UCanvasPanelSlot>(Strip->AddChild(Tick.Label))) {
            S->SetAnchors(FAnchors(0.0f, 0.5f, 0.0f, 0.5f));
            S->SetAlignment(FVector2D(0.5f, 1.0f));
            S->SetAutoSize(true);
        }
    }

    Pool.Add(Tick);
    return Pool.Last();
}

const FMythicCompassKindStyle &UMythicCompassWidget::StyleForKind(EMythicWarMapMarkerKind Kind) const {
    for (const FMythicCompassKindStyle &Style : KindStyles) {
        if (Style.Kind == Kind) {
            return Style;
        }
    }
    static const FMythicCompassKindStyle Fallback;
    return Fallback;
}


void UMythicCompassWidget::RefreshMarkers() {
    WorldMarkers.Reset();
    SinceMarkerRefresh = 0.0f;

    UMythicWarMapSubsystem *WarMap = GetWarMap();
    if (!WarMap) {
        return;
    }

    FVector Here = FVector::ZeroVector;
    if (const APlayerController *PC = GetOwningPlayer()) {
        if (const APawn *Pawn = PC->GetPawn()) {
            Here = Pawn->GetActorLocation();
        }
    }

    TArray<FMythicWarMapMarker> Markers;
    WarMap->GetMarkers(Markers);

    const float MaxDistSq = MaxMarkerDistanceCm > 0.0f ? MaxMarkerDistanceCm * MaxMarkerDistanceCm : TNumericLimits<float>::Max();
    WorldMarkers.Reserve(Markers.Num());
    for (const FMythicWarMapMarker &M : Markers) {
        FVector2D WorldXY;
        if (!WarMap->GetMarkerWorldXY(M, WorldXY)) {
            continue;
        }
        const float DistSq = FVector2D::DistSquared(FVector2D(Here.X, Here.Y), WorldXY);
        if (DistSq > MaxDistSq) {
            continue;
        }
        FMythicCompassMarker Entry;
        Entry.WorldXY = WorldXY;
        Entry.Kind = M.Kind;
        Entry.DistanceCm = FMath::Sqrt(DistSq);
        WorldMarkers.Add(Entry);
    }
    WorldMarkers.Sort([](const FMythicCompassMarker &A, const FMythicCompassMarker &B) { return A.DistanceCm < B.DistanceCm; });

    TSet<uint64> Keys;
    Keys.Reserve(WorldMarkers.Num());
    bool bNews = false;
    for (const FMythicCompassMarker &Entry : WorldMarkers) {
        const uint64 Key = Compass_MarkerKey(Entry.Kind, Entry.WorldXY);
        Keys.Add(Key);
        if (!InRangeKeys.Contains(Key)) {
            bNews = true;
        }
    }
    const bool bFirstGather = InRangeKeys.Num() == 0 && Keys.Num() > 0;
    InRangeKeys = MoveTemp(Keys);
    if (bNews && !bFirstGather) {
        if (UMythicHUDLayout *Layout = FindHUDLayout()) {
            Layout->PokeElement(this);
        }
    }
}


void UMythicCompassWidget::PlaceTick(const FMythicCompassTick &Tick, float StripX, float Opacity, float YOffset) const {
    UWidget *Mark = Tick.Icon ? static_cast<UWidget *>(Tick.Icon) : static_cast<UWidget *>(Tick.Label);
    if (!Mark) {
        return;
    }
    if (Compass_IsCulled(StripX) || Opacity <= 0.004f) {
        if (Mark->GetVisibility() != ESlateVisibility::Collapsed) {
            Mark->SetVisibility(ESlateVisibility::Collapsed);
        }
        return;
    }
    if (Mark->GetVisibility() != ESlateVisibility::HitTestInvisible) {
        Mark->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    Mark->SetRenderTranslation(FVector2D(StripX, YOffset));
    if (Tick.Icon) {
        Tick.Icon->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, Opacity));
    }
    else if (Tick.Label) {
        Tick.Label->SetRenderOpacity(Opacity);
    }
}

void UMythicCompassWidget::NativeTick(const FGeometry &MyGeometry, float InDeltaTime) {
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!Strip) {
        return;
    }
    const APlayerController *PC = GetOwningPlayer();
    const APawn *Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn) {
        return;
    }

    SinceMarkerRefresh += InDeltaTime;
    if (SinceMarkerRefresh >= MarkerRefreshSeconds) {
        RefreshMarkers();
    }

    const float StripWidth = MyGeometry.GetLocalSize().X;
    if (StripWidth <= 1.0f) {
        return;
    }
    const float Scale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), 0.05f);

    if (!FMath::IsNearlyEqual(Scale, LastViewportScale, 0.001f)) {
        LastViewportScale = Scale;
        for (const FMythicCompassTick &Tick : CardinalPool) {
            if (Tick.Label) {
                FSlateFontInfo Font = Tick.Label->GetFont();
                Font.Size = FMath::Max(Compass_LetterLogicalSize, Compass_Px(Compass_LetterFloorPx, Scale));
                Tick.Label->SetFont(Font);
            }
        }
        int32 Index = 0;
        for (const FCompassBeadPool &PoolDef : Compass_BeadPools) {
            const float Size = StyleForKind(PoolDef.Kind).Size * FMath::Max(1.0f, 1.0f / Scale);
            for (int32 i = 0; i < PoolDef.Count && Index < MarkerPool.Num(); ++i, ++Index) {
                if (MarkerPool[Index].Icon) {
                    MarkerPool[Index].Icon->SetDesiredSizeOverride(FVector2D(Size, Size));
                }
            }
        }
        if (Txt_Bearing) {
            FSlateFontInfo Font = Txt_Bearing->GetFont();
            Font.Size = FMath::Max(Compass_DigitsLogicalSize, Compass_Px(Compass_DigitsFloorPx, Scale));
            Txt_Bearing->SetFont(Font);
        }
    }

    FRotator ViewRotation = FRotator::ZeroRotator;
    FVector ViewLocation = Pawn->GetActorLocation();
    if (PC) {
        PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
    }
    const float ViewYaw = ViewRotation.Yaw;
    const float ViewBearing = Compass_YawToBearing(ViewYaw);

    if (RodMID) {
        RodMID->SetScalarParameterValue(Compass_YawParam, ViewBearing);
    }

    const bool bDigits = Txt_Bearing && Txt_Bearing->GetVisibility() != ESlateVisibility::Collapsed &&
                         Txt_Bearing->GetRenderOpacity() > 0.01f;
    const float HalfWidth = StripWidth * 0.5f;
    const float AxisY = StripHeight * (Compass_AxisV - 0.5f);
    const float LetterY = AxisY + Compass_Px(Compass_LetterBottomOffsetPx, Scale);

    for (int32 i = 0; i < CardinalPool.Num() && i < 4; ++i) {
        const float WorldBearingDeg = static_cast<float>(i) * 90.0f;
        const float Rad = FMath::DegreesToRadians(WorldBearingDeg);
        const FVector Far = ViewLocation + FVector(FMath::Sin(Rad), FMath::Cos(Rad), 0.0f) * 100000.0f;
        const float Bearing = FMythicCompass::CompassBearingDegrees(ViewYaw, ViewLocation, Far);
        const float X = FMythicCompass::CompassStripX(Bearing, HalfArcDegrees, StripWidth);
        float Opacity = Compass_EndFade(X, StripWidth, Scale);
        if (bDigits) {
            Opacity *= FMath::Clamp((FMath::Abs(X - HalfWidth) - Compass_Px(Compass_DigitsYieldGonePx, Scale)) /
                                        Compass_Px(Compass_DigitsYieldRunPx, Scale), 0.0f, 1.0f);
        }
        PlaceTick(CardinalPool[i], X, Opacity, LetterY);
    }

    {
        int32 RangeStart[UE_ARRAY_COUNT(Compass_BeadPools)];
        int32 PoolCursor = 0;
        for (int32 p = 0; p < UE_ARRAY_COUNT(Compass_BeadPools); ++p) {
            RangeStart[p] = PoolCursor;
            PoolCursor += Compass_BeadPools[p].Count;
        }
        int32 Used[UE_ARRAY_COUNT(Compass_BeadPools)] = {0};
        TArray<float, TInlineAllocator<24>> PlacedX;

        for (const EMythicWarMapMarkerKind RankKind : Compass_PlacementOrder) {
            int32 PoolIndex = INDEX_NONE;
            for (int32 p = 0; p < UE_ARRAY_COUNT(Compass_BeadPools); ++p) {
                if (Compass_BeadPools[p].Kind == RankKind) {
                    PoolIndex = p;
                    break;
                }
            }
            if (PoolIndex == INDEX_NONE) {
                continue;
            }
            for (const FMythicCompassMarker &Entry : WorldMarkers) {
                if (Entry.Kind != RankKind) {
                    continue;
                }
                if (Used[PoolIndex] >= Compass_BeadPools[PoolIndex].Count) {
                    break;
                }
                const FVector Target(Entry.WorldXY.X, Entry.WorldXY.Y, ViewLocation.Z);
                const float Bearing = FMythicCompass::CompassBearingDegrees(ViewYaw, ViewLocation, Target);
                const float X = FMythicCompass::CompassStripX(Bearing, HalfArcDegrees, StripWidth);
                if (Compass_IsCulled(X)) {
                    continue;
                }
                float Alpha = Compass_EndFade(X, StripWidth, Scale);
                for (const float Xp : PlacedX) {
                    Alpha *= FMath::Clamp((FMath::Abs(X - Xp) - Compass_Px(Compass_OverlapGonePx, Scale)) /
                                              Compass_Px(Compass_OverlapRunPx, Scale), 0.0f, 1.0f);
                    if (Alpha <= 0.004f) {
                        break;
                    }
                }
                if (RankKind != EMythicWarMapMarkerKind::Objective && MaxMarkerDistanceCm > 0.0f) {
                    Alpha *= 1.0f - Compass_DistanceFade * FMath::Clamp(Entry.DistanceCm / MaxMarkerDistanceCm, 0.0f, 1.0f);
                }
                const int32 TickIndex = RangeStart[PoolIndex] + Used[PoolIndex]++;
                if (!MarkerPool.IsValidIndex(TickIndex)) {
                    break;
                }
                PlaceTick(MarkerPool[TickIndex], X, Alpha, AxisY);
                if (Alpha > 0.004f) {
                    PlacedX.Add(X);
                }
            }
        }
        for (int32 p = 0; p < UE_ARRAY_COUNT(Compass_BeadPools); ++p) {
            for (int32 i = Used[p]; i < Compass_BeadPools[p].Count; ++i) {
                const int32 TickIndex = RangeStart[p] + i;
                if (MarkerPool.IsValidIndex(TickIndex) && MarkerPool[TickIndex].Icon &&
                    MarkerPool[TickIndex].Icon->GetVisibility() != ESlateVisibility::Collapsed) {
                    MarkerPool[TickIndex].Icon->SetVisibility(ESlateVisibility::Collapsed);
                }
            }
        }
    }

    if (Txt_Bearing) {
        if (UMythicHUDLayout *Layout = FindHUDLayout()) {
            const bool bLit = Layout->GetElementSalience(this) == EMythicHUDSalience::Lit;
            if (bLit != bLastDigitsLit) {
                bLastDigitsLit = bLit;
                Layout->SetElementSalience(Txt_Bearing, bLit ? EMythicHUDSalience::Lit : EMythicHUDSalience::Hidden);
                LastPrintedBearing = MIN_int32;
            }
        }
        if (bDigits || bLastDigitsLit) {
            const int32 Bearing = FMath::RoundToInt(ViewBearing) % 360;
            if (Bearing != LastPrintedBearing) {
                LastPrintedBearing = Bearing;
                Txt_Bearing->SetText(FText::FromString(FString::Printf(TEXT("%03d"), Bearing)));
            }
        }
    }

    if (Txt_Region) {
        if (UMythicHUDLayout *Layout = FindHUDLayout()) {
            const bool bLit = Layout->GetElementSalience(this) == EMythicHUDSalience::Lit;
            if (bLit != bLastRegionLit) {
                bLastRegionLit = bLit;
                Layout->SetElementSalience(Txt_Region, bLit ? EMythicHUDSalience::Lit : EMythicHUDSalience::Dim);
            }
        }
    }
}
