#include "UI/Widgets/MythicCombatUtilityOverlay.h"
#include "CommonTextBlock.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/Attention/MythicEntityAttentionSubsystem.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "SceneView.h"
#include "Slate/SGameLayerManager.h"
#include "Styling/CoreStyle.h"
#include "UI/MythicUIStyle.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"

#define LOCTEXT_NAMESPACE "MythicCombatUtilityOverlay"

UMythicCombatUtilityOverlay::UMythicCombatUtilityOverlay(const FObjectInitializer& Initializer):Super(Initializer) {
    SetIsFocusable(false);
    SetVisibility(ESlateVisibility::Collapsed);
}

void UMythicCombatUtilityOverlay::NativeOnInitialized() {
    Super::NativeOnInitialized();
    Plate=FMythicUIStyle::MakeSlotBrush();
    if (auto* Text=FMythicUIStyle::MakeText(this,EMythicTextRole::Subtle)) Font=Text->GetFont();
    Font.Size=14;
    Labels={LOCTEXT("GoHere","Go here"),LOCTEXT("Danger","Danger"),LOCTEXT("NeedHelp","Need help"),
        LOCTEXT("Loot","Loot"),LOCTEXT("Wait","Wait"),LOCTEXT("DefendHere","Defend here"),
        LOCTEXT("Regroup","Regroup"),LOCTEXT("Acknowledge","Acknowledged")};
    LabelStrings.Reserve(Labels.Num()); LabelSizes.Reserve(Labels.Num());
    for (const auto& Label:Labels) {
        LabelStrings.Add(Label.ToString());
        LabelSizes.Add(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label,Font));
    }
}

void UMythicCombatUtilityOverlay::NativeConstruct() {
    Super::NativeConstruct();
    Unbind();
    auto* PC=GetOwningPlayer(); auto* LP=PC ? PC->GetLocalPlayer() : nullptr;
    if (!PC || !LP || !PC->IsLocalController()) { RefreshProjection(); return; }
    BoundUtility=PC->FindComponentByClass<UMythicCombatUtilityComponent>();
    if (auto* Utility=BoundUtility.Get()) Utility->OnUtilityProjectionChanged.AddUniqueDynamic(this,&ThisClass::RefreshProjection);
    BoundAttention=LP->GetSubsystem<UMythicEntityAttentionSubsystem>();
    if (auto* Attention=BoundAttention.Get()) AttentionHandle=Attention->OnAttentionUpdated.AddUObject(this,&ThisClass::HandleAttentionChanged);
    BoundRegistry=GetWorld()->GetSubsystem<UMythicEntityPresentationRegistry>();
    if (auto* Registry=BoundRegistry.Get()) {
        RegisteredHandle=Registry->OnPresentationRegistered.AddUObject(this,&ThisClass::HandleRegistryChanged);
        UnregisteredHandle=Registry->OnPresentationUnregistered.AddUObject(this,&ThisClass::HandleRegistryChanged);
    }
    RefreshProjection();
}

void UMythicCombatUtilityOverlay::Unbind() {
    if (auto* Utility=BoundUtility.Get()) Utility->OnUtilityProjectionChanged.RemoveDynamic(this,&ThisClass::RefreshProjection);
    if (auto* Attention=BoundAttention.Get()) Attention->OnAttentionUpdated.Remove(AttentionHandle);
    if (auto* Registry=BoundRegistry.Get()) {
        Registry->OnPresentationRegistered.Remove(RegisteredHandle);
        Registry->OnPresentationUnregistered.Remove(UnregisteredHandle);
    }
    BoundUtility.Reset(); BoundAttention.Reset(); BoundRegistry.Reset();
    AttentionHandle.Reset(); RegisteredHandle.Reset(); UnregisteredHandle.Reset();
    PingCount=0;
}

void UMythicCombatUtilityOverlay::NativeDestruct() {
    Unbind(); ForceVolatile(false); SetVisibility(ESlateVisibility::Collapsed);
    Super::NativeDestruct();
}

double UMythicCombatUtilityOverlay::ServerTime() const {
    const auto* World=GetWorld(); const auto* State=World ? World->GetGameState() : nullptr;
    return State ? State->GetServerWorldTimeSeconds() : World ? World->GetTimeSeconds() : 0;
}

int32 UMythicCombatUtilityOverlay::GetActivePingCount() const {
    const auto* Utility=BoundUtility.Get();
    if (!Utility || Utility->GetOwner()!=GetOwningPlayer() || Utility->GetWorld()!=GetWorld()) return 0;
    int32 Count=0; const double Now=ServerTime();
    for (int32 Index=0;Index<PingCount;++Index) if (Pings[Index].ExpiryServerTime>Now) ++Count;
    return Count;
}

void UMythicCombatUtilityOverlay::RefreshProjection() {
    PingCount=0;
    if (const auto* Utility=BoundUtility.Get(); Utility && Utility->GetOwner()==GetOwningPlayer()) {
        const auto Current=Utility->GetActivePings();
        for (const auto& Ping:Current) {
            if (PingCount==MaximumPings) break;
            if (!Ping.PingId.IsValid() || !Ping.SenderToken.IsValid() || Ping.Location.ContainsNaN()
                || !FMath::IsFinite(Ping.ExpiryServerTime) || Ping.ExpiryServerTime<=ServerTime()
                || !Labels.IsValidIndex(static_cast<int32>(Ping.Intent))) continue;
            Pings[PingCount++]=Ping;
        }
    }
    ForceVolatile(PingCount>0);
    SetVisibility(PingCount>0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    InvalidateLayoutAndVolatility();
}

void UMythicCombatUtilityOverlay::HandleAttentionChanged(uint64) {
    if (PingCount>0) RefreshProjection();
}
void UMythicCombatUtilityOverlay::HandleRegistryChanged(const FMythicEntityPresentationInstance&,UMythicEntityPresentationComponent*) {
    if (PingCount>0) RefreshProjection();
}

bool UMythicCombatUtilityOverlay::CanIdentifySubject(const FMythicEntityPresentationInstance& Instance) const {
    const auto* Registry=BoundRegistry.Get(); const auto* Attention=BoundAttention.Get();
    auto* Component=Registry ? Registry->ResolvePresentationComponent(Instance) : nullptr;
    FMythicEntityAttentionObservation Observation;
    return Component && Component->RepresentsInstance(Instance) && Attention
        && Attention->FindObservation(Instance,Observation) && Observation.bOnScreen && Observation.bHasLineOfSight;
}

int32 UMythicCombatUtilityOverlay::NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& CullingRect,
    FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& WidgetStyle,bool bParentEnabled) const {
    Layer=Super::NativePaint(Args,Geometry,CullingRect,Elements,Layer,WidgetStyle,bParentEnabled);
    if (GetActivePingCount()==0) return Layer;
    const auto* PC=GetOwningPlayer(); const auto* LP=PC ? PC->GetLocalPlayer() : nullptr;
    if (!PC || !PC->IsLocalController() || !LP || !LP->ViewportClient || !LP->ViewportClient->Viewport) return Layer;
    const auto Layers=LP->ViewportClient->GetGameLayerManager();
    FSceneViewProjectionData Projection;
    if (!Layers || !LP->GetProjectionData(LP->ViewportClient->Viewport,Projection)) return Layer;
    FVector2D Pixels; LP->ViewportClient->GetViewportSize(Pixels);
    const FGeometry Host=Layers->GetViewportWidgetHostPaintGeometry();
    if (Pixels.X<=0 || Pixels.Y<=0 || Host.GetLocalSize().X<=0 || Host.GetLocalSize().Y<=0) return Layer;
    const auto ToLocal=[&](FVector2D Pixel) {
        return Geometry.AbsoluteToLocal(Host.LocalToAbsolute(Pixel/Pixels*Host.GetLocalSize()));
    };
    const auto Rect=Projection.GetConstrainedViewRect();
    const FVector2D Min=ToLocal(FVector2D(Rect.Min)),Max=ToLocal(FVector2D(Rect.Max));
    const FVector2D Center=(Min+Max)*0.5;
    const FMatrix Matrix=Projection.ComputeViewProjectionMatrix();
    const auto& Tokens=FMythicUIStyle::Get();
    const auto* White=FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
    const double Now=ServerTime();
    const auto* Utility=BoundUtility.Get();
    const double Lifetime=Utility ? FMath::Max(1.0f,Utility->PingLifetime) : 8.0;
    for (int32 Index=0;Index<PingCount;++Index) {
        const auto& Ping=Pings[Index]; const int32 Kind=static_cast<int32>(Ping.Intent);
        if (Ping.ExpiryServerTime<=Now || !LabelSizes.IsValidIndex(Kind)) continue;
        // Always project the communicated point, never a private actor transform. Current attention permits only the subject mark.
        const FVector4 Clip=Matrix.TransformFVector4(FVector4(FVector(Ping.Location),1));
        if (Clip.ContainsNaN() || !FMath::IsFinite(Clip.W) || FMath::Abs(Clip.W)<UE_SMALL_NUMBER) continue;
        const FVector2D Ndc(Clip.X/FMath::Abs(Clip.W),-Clip.Y/FMath::Abs(Clip.W));
        const FVector2D Pixel(Rect.Min.X+(Ndc.X+1)*0.5*Rect.Width(),Rect.Min.Y+(Ndc.Y+1)*0.5*Rect.Height());
        const FVector2D Desired=ToLocal(Pixel);
        const FVector2D Size(LabelSizes[Kind].X+32,FMath::Max(28.0,LabelSizes[Kind].Y+10));
        const FVector2D Half=(Max-Min)*0.5-Size*0.5-FVector2D(12,12);
        if (Desired.ContainsNaN() || Half.X<=0 || Half.Y<=0) continue;
        FVector2D Offset=Desired-Center;
        const bool Edge=Clip.W<=0 || FMath::Abs(Offset.X)>Half.X || FMath::Abs(Offset.Y)>Half.Y;
        FVector2D Direction=Offset.GetSafeNormal();
        if (Edge) {
            if (Direction.IsNearlyZero()) Direction=FVector2D(0,1);
            const double Reach=FMath::Min(FMath::Abs(Direction.X)>UE_SMALL_NUMBER ? Half.X/FMath::Abs(Direction.X) : DBL_MAX,
                FMath::Abs(Direction.Y)>UE_SMALL_NUMBER ? Half.Y/FMath::Abs(Direction.Y) : DBL_MAX);
            Offset=Direction*Reach;
        }
        const FVector2D Point=Center+Offset,TopLeft=Point-Size*0.5;
        FLinearColor Ink=Ping.Intent==EMythicPingIntent::Danger ? Tokens.Negative
            : Ping.Intent==EMythicPingIntent::NeedHelp ? Tokens.Caution : Tokens.Ink;
        const FLinearColor Tint=WidgetStyle.GetColorAndOpacityTint();
        FSlateDrawElement::MakeBox(Elements,Layer+1,Geometry.ToPaintGeometry(Size,FSlateLayoutTransform(TopLeft)),&Plate,ESlateDrawEffect::None,Tint);
        FSlateDrawElement::MakeText(Elements,Layer+2,Geometry.ToPaintGeometry(LabelSizes[Kind],FSlateLayoutTransform(TopLeft+FVector2D(25,(Size.Y-LabelSizes[Kind].Y)*0.5))),LabelStrings[Kind],Font,ESlateDrawEffect::None,Ink*Tint);
        // A filled public-subject dot vs a hollow communicated-point mark; no names, IDs, health, faction or inventory.
        const FVector2D Dot=TopLeft+FVector2D(13,Size.Y*0.5);
        const bool Subject=!Edge && CanIdentifySubject(Ping.Subject);
        if (Subject) FSlateDrawElement::MakeBox(Elements,Layer+2,Geometry.ToPaintGeometry(FVector2D(5,5),FSlateLayoutTransform(Dot-FVector2D(2.5,2.5))),White,ESlateDrawEffect::None,Ink*Tint);
        else {
            for (const FVector2D Sign:{FVector2D(-1,0),FVector2D(1,0),FVector2D(0,-1),FVector2D(0,1)})
                FSlateDrawElement::MakeBox(Elements,Layer+2,Geometry.ToPaintGeometry(FVector2D(2,2),FSlateLayoutTransform(Dot+Sign*4-FVector2D(1,1))),White,ESlateDrawEffect::None,Ink*Tint);
        }
        const double Remaining=FMath::Clamp((Ping.ExpiryServerTime-Now)/Lifetime,0.0,1.0);
        FSlateDrawElement::MakeBox(Elements,Layer+2,Geometry.ToPaintGeometry(FVector2D((Size.X-8)*Remaining,2),FSlateLayoutTransform(TopLeft+FVector2D(4,Size.Y-3))),White,ESlateDrawEffect::None,Ink*Tint);
        if (Edge) {
            const FVector2D Tip=Point+Direction*FMath::Min(Size.X,Size.Y)*0.5;
            const FVector2D Side(-Direction.Y,Direction.X);
            TArray<FVector2D> Arrow; Arrow.Reserve(3);
            Arrow.Add(Tip-Direction*6+Side*4); Arrow.Add(Tip); Arrow.Add(Tip-Direction*6-Side*4);
            FSlateDrawElement::MakeLines(Elements,Layer+3,Geometry.ToPaintGeometry(),Arrow,ESlateDrawEffect::None,Ink*Tint,true,1.5f);
        }
    }
    return Layer+3;
}

#undef LOCTEXT_NAMESPACE
