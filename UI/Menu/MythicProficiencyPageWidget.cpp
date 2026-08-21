// Copyright Stellar Games. All Rights Reserved.

#include "MythicProficiencyPageWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Mythic.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBoxSlot.h"
#include "CommonTextBlock.h"
#include "UI/MythicUIStyle.h"
#include "GameFramework/PlayerState.h"
#include "GAS/MythicTags_GAS.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/MythicPlayerController.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"

namespace {
const FName Prof_Percent(TEXT("Percent"));
const FName Prof_ChipPercent(TEXT("ChipPercent"));
const FName Prof_FillStart(TEXT("FillColorStart"));
const FName Prof_FillEnd(TEXT("FillColorEnd"));
const FName Prof_Background(TEXT("BackgroundColor"));
const FName Prof_ChipAlpha(TEXT("ChipAlpha"));
const FLinearColor Prof_Track(0.030f, 0.024f, 0.018f, 1.0f);

constexpr float CardWidth = 432.0f;
constexpr float CardHeight = 132.0f;
constexpr float CardPadX = 18.0f;
constexpr float CardPadY = 16.0f;
constexpr float CardBarHeight = 9.0f;

FSlateBrush MakeCardBrush() {
    static const FSoftObjectPath CardPath(TEXT("/Game/Mythic/UI/Globals/materials/kit/MI_UI_Plate_Quiet.MI_UI_Plate_Quiet"));
    FSlateBrush Brush;
    if (UMaterialInterface *Card = Cast<UMaterialInterface>(CardPath.TryLoad())) {
        Brush.SetResourceObject(Card);
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.TintColor = FSlateColor(FLinearColor::White);
    }
    else {
        Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
    }
    return Brush;
}
}

void UMythicProficiencyPageWidget::NativeConstruct() {
    if (TrackList) {
        for (int32 i = RowPool.Num(); i < PrewarmRowCount; ++i) {
            FMythicProficiencyRow &Row = GetOrCreateRow(i);
            Row.Box->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    Super::NativeConstruct();
}

void UMythicProficiencyPageWidget::NativeOnActivated() {
    Super::NativeOnActivated();
    Bind();
    Refresh();
}

void UMythicProficiencyPageWidget::NativeOnDeactivated() {
    Unbind();
    Super::NativeOnDeactivated();
}

void UMythicProficiencyPageWidget::Bind() {
    if (bBound) {
        return;
    }
    APlayerController *PC = GetOwningPlayer();
    if (!PC) {
        return;
    }
    UAbilitySystemComponent *ASC = nullptr;
    if (APlayerState *PS = PC->PlayerState) {
        ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS);
    }
    if (!ASC) {
        return;
    }
    EventHandle = ASC->AddGameplayEventTagContainerDelegate(
        FGameplayTagContainer(GAS_EVENT_PROFICIENCY_GAINED),
        FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &UMythicProficiencyPageWidget::HandleProficiencyEvent));
    bBound = true;
}

void UMythicProficiencyPageWidget::Unbind() {
    if (!bBound) {
        return;
    }
    if (const APlayerController *PC = GetOwningPlayer()) {
        if (APlayerState *PS = PC->PlayerState) {
            if (UAbilitySystemComponent *ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS)) {
                ASC->RemoveGameplayEventTagContainerDelegate(FGameplayTagContainer(GAS_EVENT_PROFICIENCY_GAINED), EventHandle);
            }
        }
    }
    EventHandle.Reset();
    bBound = false;
}

void UMythicProficiencyPageWidget::HandleProficiencyEvent(FGameplayTag Tag, const FGameplayEventData *Payload) {
    Refresh();
}

FMythicProficiencyRow &UMythicProficiencyPageWidget::GetOrCreateRow(int32 Index) {
    if (RowPool.IsValidIndex(Index)) {
        return RowPool[Index];
    }

    FMythicProficiencyRow Row;
    USizeBox *Box = WidgetTree->ConstructWidget<USizeBox>();
    Box->SetWidthOverride(CardWidth);
    Box->SetHeightOverride(CardHeight);
    Row.Box = Box;

    UOverlay *Layers = WidgetTree->ConstructWidget<UOverlay>();
    Box->AddChild(Layers);

    UImage *Sheet = WidgetTree->ConstructWidget<UImage>();
    Sheet->SetBrush(MakeCardBrush());
    Sheet->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (UOverlaySlot *S = Cast<UOverlaySlot>(Layers->AddChild(Sheet))) {
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
    }

    UVerticalBox *Column = WidgetTree->ConstructWidget<UVerticalBox>();
    if (UOverlaySlot *S = Cast<UOverlaySlot>(Layers->AddChild(Column))) {
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetVerticalAlignment(VAlign_Fill);
        S->SetPadding(FMargin(CardPadX, CardPadY, CardPadX, CardPadY));
    }

    UHorizontalBox *Head = WidgetTree->ConstructWidget<UHorizontalBox>();
    Row.Name = FMythicUIStyle::MakeText(this, EMythicTextRole::Heading);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Head->AddChild(Row.Name))) {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetVerticalAlignment(VAlign_Center);
    }
    Row.Level = FMythicUIStyle::MakeText(this, EMythicTextRole::Heading);
    Row.Level->SetJustification(ETextJustify::Right);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Head->AddChild(Row.Level))) {
        S->SetVerticalAlignment(VAlign_Center);
    }
    Column->AddChild(Head);

    Row.Bar = WidgetTree->ConstructWidget<UImage>();
    if (BarMaterialAsset) {
        FSlateBrush Brush;
        Brush.SetResourceObject(BarMaterialAsset);
        Brush.ImageSize = FVector2D(BarWidth, CardBarHeight);
        Row.Bar->SetBrush(Brush);
        Row.BarMaterial = Row.Bar->GetDynamicMaterial();
        if (Row.BarMaterial) {
            Row.BarMaterial->SetVectorParameterValue(Prof_FillStart, BarFillStart);
            Row.BarMaterial->SetVectorParameterValue(Prof_FillEnd, BarFillEnd);
            Row.BarMaterial->SetVectorParameterValue(Prof_Background, Prof_Track);
            Row.BarMaterial->SetScalarParameterValue(Prof_ChipAlpha, 0.0f);
        }
    }
    if (UVerticalBoxSlot *S = Cast<UVerticalBoxSlot>(Column->AddChild(Row.Bar))) {
        S->SetHorizontalAlignment(HAlign_Fill);
        S->SetPadding(FMargin(0.0f, 14.0f, 0.0f, 12.0f));
    }

    UHorizontalBox *Foot = WidgetTree->ConstructWidget<UHorizontalBox>();
    Row.Progress = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Foot->AddChild(Row.Progress))) {
        S->SetVerticalAlignment(VAlign_Center);
    }
    Row.Milestone = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
    Row.Milestone->SetJustification(ETextJustify::Right);
    if (UHorizontalBoxSlot *S = Cast<UHorizontalBoxSlot>(Foot->AddChild(Row.Milestone))) {
        S->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        S->SetVerticalAlignment(VAlign_Center);
        S->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
    }
    Column->AddChild(Foot);

    Box->SetVisibility(ESlateVisibility::HitTestInvisible);
    if (TrackList) {
        if (UPanelSlot *Added = TrackList->AddChild(Box)) {
            if (UWrapBoxSlot *W = Cast<UWrapBoxSlot>(Added)) {
                W->SetPadding(FMargin(0.0f, 0.0f, RowGap, RowGap));
            }
            else if (UVerticalBoxSlot *V = Cast<UVerticalBoxSlot>(Added)) {
                V->SetPadding(FMargin(0.0f, 0.0f, 0.0f, RowGap));
                V->SetHorizontalAlignment(HAlign_Left);
            }
        }
    }

    RowPool.Add(Row);
    return RowPool.Last();
}

void UMythicProficiencyPageWidget::ApplyRow(FMythicProficiencyRow &Row, const FProficiencySummary &Summary) {
    Row.Box->SetVisibility(ESlateVisibility::HitTestInvisible);

    if (Row.Name) {
        Row.Name->SetText(Summary.Name);
        Row.Name->SetColorAndOpacity(FSlateColor(NameColor));
    }
    if (Row.Level) {
        Row.Level->SetText(FText::Format(NSLOCTEXT("Mythic", "ProfLevel", "Lv {0}"), FText::AsNumber(Summary.Level)));
        Row.Level->SetColorAndOpacity(FSlateColor(NameColor));
    }
    if (Row.BarMaterial) {
        const float Fraction = FMath::Clamp(Summary.ProgressFraction, 0.0f, 1.0f);
        Row.BarMaterial->SetScalarParameterValue(Prof_Percent, FMath::Max(Fraction, 0.015f));
        Row.BarMaterial->SetScalarParameterValue(Prof_ChipPercent, Fraction);
    }
    if (Row.Progress) {
        const float Into = Summary.CurrentXP - Summary.LevelXPStart;
        const float Needed = Summary.LevelXPEnd - Summary.LevelXPStart;
        Row.Progress->SetText(Needed > 0.0f
                                  ? FText::Format(NSLOCTEXT("Mythic", "ProfXP", "{0} / {1}"),
                                                  FText::AsNumber(FMath::FloorToInt(Into)),
                                                  FText::AsNumber(FMath::FloorToInt(Needed)))
                                  : NSLOCTEXT("Mythic", "ProfMaxed", "Mastered"));
        Row.Progress->SetColorAndOpacity(FSlateColor(SubtleColor));
    }
    if (Row.Milestone) {
        if (Summary.NextMilestoneName.IsEmpty()) {
            Row.Milestone->SetText(NSLOCTEXT("Mythic", "ProfNoMilestone", "All milestones earned"));
            Row.Milestone->SetColorAndOpacity(FSlateColor(SubtleColor));
        }
        else {
            Row.Milestone->SetText(FText::Format(NSLOCTEXT("Mythic", "ProfNextMilestone", "Next at Lv {1} — {0}"),
                                                 Summary.NextMilestoneName,
                                                 FText::AsNumber(Summary.NextMilestoneLevel)));
            Row.Milestone->SetColorAndOpacity(FSlateColor(SubtleColor));
        }
    }
}

void UMythicProficiencyPageWidget::Refresh() {
    if (!TrackList) {
        return;
    }
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer());
    if (!PC) {
        return;
    }

    const TArray<FProficiencySummary> Summaries = PC->GetProficiencySummaries();

    int32 RowIndex = 0;
    for (const FProficiencySummary &Summary : Summaries) {
        ApplyRow(GetOrCreateRow(RowIndex++), Summary);
    }
    for (int32 i = RowIndex; i < RowPool.Num(); ++i) {
        if (RowPool[i].Box) {
            RowPool[i].Box->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (Txt_Empty) {
        Txt_Empty->SetVisibility(Summaries.Num() == 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
}
