// Copyright Stellar Games. All Rights Reserved.

#include "MythicStatSheetWidget.h"

#include "UI/Widgets/MythicSectionHeader.h"
#include "Components/VerticalBox.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/PanelWidget.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "CommonTextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UI/MythicUIKit.h"
#include "UI/MythicUIStyle.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UI/ViewModels/MythicStatSheetViewModel.h"

namespace {
constexpr float ValueColumnWidth = 84.0f;
constexpr float BonusColumnWidth = 64.0f;

constexpr int32 SummaryCardPoolSize = 6;
constexpr int32 PrimaryTooltipPoolSize = 4;
constexpr int32 TooltipLinePoolSize = 6;
constexpr float SummaryIconSize = 28.0f;

FSlateBrush Sheet_NoDrawBrush() {
    FSlateBrush Brush;
    Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
    return Brush;
}

FSlateBrush Sheet_KitBrush(const TCHAR *Id, FVector2D Size) {
    if (const UMythicUIKit *Kit = UMythicUIKit::Get()) {
        return Kit->MakeBrush(FName(Id), EMythicUIState::Normal, Size);
    }
    return Sheet_NoDrawBrush();
}

const TCHAR *Sheet_BarMaterialPath = TEXT("/Game/Mythic/UI/Globals/materials/M_UI_BarHand.M_UI_BarHand");
constexpr float Sheet_BarHeight = 5.0f;
constexpr float Sheet_BarGaugeHeight = 8.0f;
constexpr float Sheet_RowGap = 6.0f;
constexpr float Sheet_BarMinPercent = 0.024f;
const FName Sheet_Bar_Percent(TEXT("Percent"));
const FName Sheet_Bar_ChipPercent(TEXT("ChipPercent"));
const FName Sheet_Bar_ChipAlpha(TEXT("ChipAlpha"));
const FName Sheet_Bar_FillStart(TEXT("FillColorStart"));
const FName Sheet_Bar_FillEnd(TEXT("FillColorEnd"));
const FName Sheet_Bar_Background(TEXT("BackgroundColor"));
const FLinearColor Sheet_PrimaryText(0.788f, 0.663f, 0.416f, 1.0f);
const FLinearColor Sheet_BarFillStart(0.70f, 0.58f, 0.36f, 1.0f);
const FLinearColor Sheet_BarFillEnd(0.788f, 0.663f, 0.416f, 1.0f);
const FLinearColor Sheet_BarTrack(0.078f, 0.071f, 0.059f, 1.0f);

UMaterialInterface *Sheet_BarMaterial() {
    static TWeakObjectPtr<UMaterialInterface> Cached;
    if (!Cached.IsValid()) {
        Cached = LoadObject<UMaterialInterface>(nullptr, Sheet_BarMaterialPath);
    }
    return Cached.Get();
}
}

void UMythicStatSheetWidget::NativeConstruct() {
    if (StatList && RowPool.Num() < PrewarmRowCount) {
        RowPool.Reserve(PrewarmRowCount);
        for (int32 i = RowPool.Num(); i < PrewarmRowCount; ++i) {
            FMythicStatRowWidgets &Row = GetOrCreateRow(i);
            Row.Backing->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    BuildSummaryCardPool();
    BuildTooltipPool();

    if (!bCollapseInitialized) {
        bCollapseInitialized = true;
        for (const FGameplayTag CategoryTag : DefaultCollapsedCategoryTags) {
            if (CategoryTag.IsValid()) {
                CollapsedSections.Add(CategoryTag);
            }
        }
    }

    Super::NativeConstruct();

    BindIfVisible();
}

void UMythicStatSheetWidget::NativeOnActivated() {
    Super::NativeOnActivated();
    BindIfVisible();
}

void UMythicStatSheetWidget::NativeOnDeactivated() {
    Unbind();
    Super::NativeOnDeactivated();
}

void UMythicStatSheetWidget::SetVisibility(ESlateVisibility InVisibility) {
    Super::SetVisibility(InVisibility);
    if (InVisibility == ESlateVisibility::Collapsed || InVisibility == ESlateVisibility::Hidden) {
        Unbind();
    }
    else {
        BindIfVisible();
    }
}

void UMythicStatSheetWidget::NativeDestruct() {
    Unbind();
    Super::NativeDestruct();
}

void UMythicStatSheetWidget::BindIfVisible() {
    if (bBound) {
        return;
    }
    const ESlateVisibility Vis = GetVisibility();
    if (Vis == ESlateVisibility::Collapsed || Vis == ESlateVisibility::Hidden) {
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
        if (APawn *Pawn = PC->GetPawn()) {
            ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
        }
    }
    if (!ASC) {
        return;
    }

    if (!ViewModel) {
        ViewModel = NewObject<UMythicStatSheetViewModel>(this);
    }

    const INotifyFieldValueChanged::FFieldValueChangedDelegate Delegate =
        INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UMythicStatSheetWidget::HandleFieldChanged);
    using FDesc = UMythicStatSheetViewModel::FFieldNotificationClassDescriptor;
    ViewModel->AddFieldValueChangedDelegate(FDesc::Sections, Delegate);
    ViewModel->AddFieldValueChangedDelegate(FDesc::Summaries, Delegate);
    ViewModel->AddFieldValueChangedDelegate(FDesc::ModifiedStatCount, Delegate);

    ViewModel->InitializeForASC(ASC);
    bBound = true;
    Rebuild();
}

void UMythicStatSheetWidget::Unbind() {
    if (!bBound) {
        return;
    }
    if (ViewModel) {
        ViewModel->RemoveAllFieldValueChangedDelegates(this);
        ViewModel->Shutdown();
    }
    bBound = false;
}

void UMythicStatSheetWidget::HandleFieldChanged(UObject *Object, UE::FieldNotification::FFieldId FieldId) {
    ScheduleRebuild();
}

void UMythicStatSheetWidget::ScheduleRebuild() {
    if (bRebuildScheduled) {
        return;
    }
    UWorld *World = GetWorld();
    if (!World) {
        Rebuild();
        return;
    }
    bRebuildScheduled = true;
    World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]() {
        bRebuildScheduled = false;
        if (bBound) {
            Rebuild();
        }
    }));
}

FMythicStatRowWidgets &UMythicStatSheetWidget::GetOrCreateRow(int32 Index) {
    if (RowPool.IsValidIndex(Index)) {
        return RowPool[Index];
    }

    FMythicStatRowWidgets Row;
    Row.Backing = WidgetTree->ConstructWidget<UBorder>();
    Row.Backing->SetBrush(Sheet_NoDrawBrush());
    Row.Backing->SetPadding(FMargin(0.0f));
    Row.Backing->SetVisibility(ESlateVisibility::HitTestInvisible);

    Row.Box = WidgetTree->ConstructWidget<UHorizontalBox>();
    Row.Backing->SetContent(Row.Box);

    Row.Label = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
    if (UHorizontalBoxSlot *LabelSlot = Cast<UHorizontalBoxSlot>(Row.Box->AddChild(Row.Label))) {
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    /**
     * Value over gauge, stacked - never overlaid.
     *
     * Sharing one cell put the bar's centreline through the number, so a gauged stat read as struck-through
     * text and the track's ends read as a box drawn round it. Stacked, the fill is unmistakably a fill.
     */
    Row.ValueBox = WidgetTree->ConstructWidget<UVerticalBox>();
    Row.ValueBox->SetVisibility(ESlateVisibility::HitTestInvisible);

    Row.Value = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
    Row.Value->SetJustification(ETextJustify::Right);
    Row.Value->SetMinDesiredWidth(ValueColumnWidth);
    if (UVerticalBoxSlot *ValueSlot = Cast<UVerticalBoxSlot>(Row.ValueBox->AddChild(Row.Value))) {
        ValueSlot->SetHorizontalAlignment(HAlign_Fill);
    }

    Row.Bar = WidgetTree->ConstructWidget<UImage>();
    {
        FSlateBrush BarBrush;
        if (UMaterialInterface *Mat = Sheet_BarMaterial()) {
            BarBrush.SetResourceObject(Mat);
            BarBrush.DrawAs = ESlateBrushDrawType::Image;
        }
        else {
            BarBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
        }
        BarBrush.ImageSize = FVector2D(ValueColumnWidth, Sheet_BarHeight);
        Row.Bar->SetBrush(BarBrush);
    }
    Row.Bar->SetVisibility(ESlateVisibility::Collapsed);
    if (UVerticalBoxSlot *BarSlot = Cast<UVerticalBoxSlot>(Row.ValueBox->AddChild(Row.Bar))) {
        BarSlot->SetHorizontalAlignment(HAlign_Fill);
        BarSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
    }

    if (UHorizontalBoxSlot *BoxSlot = Cast<UHorizontalBoxSlot>(Row.Box->AddChild(Row.ValueBox))) {
        BoxSlot->SetHorizontalAlignment(HAlign_Right);
        BoxSlot->SetVerticalAlignment(VAlign_Center);
    }

    Row.Bonus = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
    Row.Bonus->SetJustification(ETextJustify::Right);
    Row.Bonus->SetMinDesiredWidth(BonusColumnWidth);
    if (UHorizontalBoxSlot *BonusSlot = Cast<UHorizontalBoxSlot>(Row.Box->AddChild(Row.Bonus))) {
        BonusSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
        BonusSlot->SetHorizontalAlignment(HAlign_Right);
    }

    Row.Box->SetVisibility(ESlateVisibility::HitTestInvisible);

    RowPool.Add(Row);
    return RowPool.Last();
}

UMythicSectionHeader *UMythicStatSheetWidget::GetOrCreateHeader(int32 Index) {
    if (HeaderPool.IsValidIndex(Index)) {
        return HeaderPool[Index].Get();
    }
    if (!SectionHeaderClass || !GetOwningPlayer()) {
        return nullptr;
    }
    UMythicSectionHeader *Header = CreateWidget<UMythicSectionHeader>(GetOwningPlayer(), SectionHeaderClass);
    Header->OnToggled.AddDynamic(this, &UMythicStatSheetWidget::HandleSectionToggled);
    HeaderPool.Add(Header);
    return Header;
}

void UMythicStatSheetWidget::HandleSectionToggled(UMythicSectionHeader *Header) {
    const int32 Index = HeaderPool.IndexOfByKey(Header);
    if (!HeaderCategories.IsValidIndex(Index)) {
        return;
    }
    const FGameplayTag CategoryTag = HeaderCategories[Index];
    if (!CollapsedSections.Remove(CategoryTag)) {
        CollapsedSections.Add(CategoryTag);
    }
    Rebuild();
}

void UMythicStatSheetWidget::ReorderIfShapeChanged(const TArray<int32> &NewShape) {
    if (Shape == NewShape) {
        return;
    }
    Shape = NewShape;

    StatList->ClearChildren();
    int32 RowIndex = 0;
    for (int32 Section = 0; Section < NewShape.Num(); ++Section) {
        if (UMythicSectionHeader *Header = GetOrCreateHeader(Section)) {
            // A header breaks from the rows above it and belongs to the rows below: 2x the row gap up, 0.5x down.
            if (UVerticalBoxSlot *HeaderSlot = Cast<UVerticalBoxSlot>(StatList->AddChild(Header))) {
                HeaderSlot->SetPadding(FMargin(0.0f, Sheet_RowGap * 2.0f, 0.0f, Sheet_RowGap * 0.5f));
            }
        }
        for (int32 i = 0; i < NewShape[Section]; ++i) {
            if (RowPool.IsValidIndex(RowIndex)) {
                StatList->AddChild(RowPool[RowIndex].Backing);
            }
            ++RowIndex;
        }
    }
}

void UMythicStatSheetWidget::SetRowGap(FMythicStatRowWidgets &Row, float TopGap) {
    if (!Row.Backing) {
        return;
    }
    if (UVerticalBoxSlot *BoxSlot = Cast<UVerticalBoxSlot>(Row.Backing->Slot)) {
        BoxSlot->SetPadding(FMargin(0.0f, TopGap, 0.0f, 0.0f));
    }
}

void UMythicStatSheetWidget::ApplyLine(FMythicStatRowWidgets &Row, const FMythicStatLine &Line) {
    const bool bEmphasized = Line.bEmphasizeRow;
    const bool bHasDrilldown = Line.bEnableContributionDrilldown;
    const FSlateFontInfo &LineFont = bEmphasized && PrimaryRowFont.HasValidFont() ? PrimaryRowFont : RowFont;
    const bool bGauge = Line.BarPercent >= 0.0f;
    // Base 0 means the value and the bonus are the same number; printing both reads as a bug.
    const bool bAllBonus = Line.bHasBonus && FMath::IsNearlyZero(Line.BaseValue);

    if (Row.Backing) {
        // Only data-authored drill-down rows take hover input; every other row stays transparent to the mouse.
        Row.Backing->SetVisibility(bHasDrilldown ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
        Row.Backing->SetBrush(Sheet_NoDrawBrush());
        Row.Backing->SetPadding(bEmphasized ? FMargin(0.0f, 4.0f) : FMargin(0.0f));
        if (!bHasDrilldown) {
            Row.Backing->SetToolTip(nullptr);
        }
    }
    Row.Box->SetVisibility(ESlateVisibility::HitTestInvisible);
    SetRowGap(Row, Sheet_RowGap);

    Row.Label->SetText(Line.Label);
    Row.Label->SetColorAndOpacity(FSlateColor(LabelColor));
    if (LineFont.HasValidFont()) {
        Row.Label->SetFont(LineFont);
    }

    if (Row.ValueBox) {
        Row.ValueBox->SetVisibility(ESlateVisibility::HitTestInvisible);
        // Gauge rows hand the value column the row's spare width so the fill reads as a bar, not an underline.
        if (UHorizontalBoxSlot *ValueBoxSlot = Cast<UHorizontalBoxSlot>(Row.ValueBox->Slot)) {
            ValueBoxSlot->SetSize(FSlateChildSize(bGauge ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic));
            ValueBoxSlot->SetHorizontalAlignment(bGauge ? HAlign_Fill : HAlign_Right);
        }
    }
    Row.Value->SetVisibility(ESlateVisibility::HitTestInvisible);
    Row.Value->SetText(Line.Value);
    Row.Value->SetColorAndOpacity(FSlateColor(
        bEmphasized ? Sheet_PrimaryText
                 : bAllBonus ? (Line.BonusValue < 0.0f ? PenaltyColor : BonusColor) : ValueColor));
    if (LineFont.HasValidFont()) {
        Row.Value->SetFont(LineFont);
    }

    if (Row.Bar) {
        if (bGauge) {
            Row.Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
            Row.Bar->SetDesiredSizeOverride(FVector2D(ValueColumnWidth, Sheet_BarGaugeHeight));
            if (UMaterialInstanceDynamic *MID = Row.Bar->GetDynamicMaterial()) {
                const float Shown = FMath::Max(Line.BarPercent, Sheet_BarMinPercent);
                MID->SetScalarParameterValue(Sheet_Bar_Percent, Shown);
                MID->SetScalarParameterValue(Sheet_Bar_ChipPercent, Shown);
                MID->SetScalarParameterValue(Sheet_Bar_ChipAlpha, 0.0f);
                MID->SetVectorParameterValue(Sheet_Bar_FillStart, Sheet_BarFillStart);
                MID->SetVectorParameterValue(Sheet_Bar_FillEnd, Sheet_BarFillEnd);
                MID->SetVectorParameterValue(Sheet_Bar_Background, Sheet_BarTrack);
            }
        }
        else {
            Row.Bar->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // Kept visible when empty so every value on the sheet shares one right edge.
    Row.Bonus->SetVisibility(ESlateVisibility::HitTestInvisible);
    Row.Bonus->SetText(bAllBonus ? FText::GetEmpty() : Line.BonusText);
    Row.Bonus->SetColorAndOpacity(FSlateColor(Line.BonusValue < 0.0f ? PenaltyColor : BonusColor));
    if (RowFont.HasValidFont()) {
        Row.Bonus->SetFont(RowFont);
    }

    if (bHasDrilldown) {
        ApplyContributionTooltip(Row, Line);
    }
}

void UMythicStatSheetWidget::ApplyContributionTooltip(FMythicStatRowWidgets &Row, const FMythicStatLine &Line) {
    if (!Row.Backing || !ViewModel || !TooltipPool.IsValidIndex(UsedContributionTooltips)) {
        return;
    }
    FMythicStatTooltipWidgets &Tip = TooltipPool[UsedContributionTooltips++];

    if (Tip.Title) {
        Tip.Title->SetText(FText::Format(NSLOCTEXT("Mythic", "PrimaryContributes", "{0} contributes"), Line.Label));
    }

    // Re-texted on every rebuild, so the figures a hover shows are as fresh as the sheet itself — and
    // generated from the same authored rows gameplay reads, never typed here.
    const TArray<FMythicStatContributionLine> Contributions = ViewModel->GetContributionsFor(Line.Attribute);
    for (int32 i = 0; i < Tip.LinePool.Num(); ++i) {
        FMythicStatTooltipLineWidgets &TipLine = Tip.LinePool[i];
        if (!TipLine.Box) {
            continue;
        }
        if (!Contributions.IsValidIndex(i)) {
            TipLine.Box->SetVisibility(ESlateVisibility::Collapsed);
            continue;
        }
        TipLine.Box->SetVisibility(ESlateVisibility::HitTestInvisible);
        TipLine.Label->SetText(Contributions[i].Label);
        TipLine.Value->SetText(Contributions[i].Value);
        TipLine.Diminished->SetVisibility(Contributions[i].bDiminished ? ESlateVisibility::HitTestInvisible
                                                                       : ESlateVisibility::Collapsed);
    }

    Row.Backing->SetToolTip(Tip.Plate);
}

void UMythicStatSheetWidget::BuildSummaryCardPool() {
    if (!SummaryCards || !WidgetTree || CardPool.Num() >= SummaryCardPoolSize) {
        return;
    }
    CardPool.Reserve(SummaryCardPoolSize);
    for (int32 i = CardPool.Num(); i < SummaryCardPoolSize; ++i) {
        FMythicSummaryCardWidgets Card;
        Card.Plate = WidgetTree->ConstructWidget<UBorder>();
        Card.Plate->SetBrush(Sheet_NoDrawBrush());
        Card.Plate->SetPadding(FMargin(0.0f, 8.0f));
        Card.Plate->SetVisibility(ESlateVisibility::Collapsed);

        UVerticalBox *Column = WidgetTree->ConstructWidget<UVerticalBox>();
        Card.Plate->SetContent(Column);

        UHorizontalBox *ValueRow = WidgetTree->ConstructWidget<UHorizontalBox>();
        if (UVerticalBoxSlot *RowSlot = Cast<UVerticalBoxSlot>(Column->AddChild(ValueRow))) {
            RowSlot->SetHorizontalAlignment(HAlign_Left);
        }

        Card.Icon = WidgetTree->ConstructWidget<UImage>();
        Card.Icon->SetVisibility(ESlateVisibility::Collapsed);
        if (UHorizontalBoxSlot *IconSlot = Cast<UHorizontalBoxSlot>(ValueRow->AddChild(Card.Icon))) {
            IconSlot->SetVerticalAlignment(VAlign_Center);
            IconSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
        }

        Card.Value = FMythicUIStyle::MakeText(this, EMythicTextRole::Title);
        Card.Value->SetColorAndOpacity(FSlateColor(FLinearColor(0.788f, 0.663f, 0.416f, 1.0f)));
        Card.Value->SetVisibility(ESlateVisibility::HitTestInvisible);
        ValueRow->AddChild(Card.Value);

        Card.Label = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
        Card.Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.910f, 0.886f, 0.839f, 0.55f)));
        Card.Label->SetVisibility(ESlateVisibility::HitTestInvisible);
        Column->AddChild(Card.Label);

        if (UHorizontalBoxSlot *CardSlot = Cast<UHorizontalBoxSlot>(SummaryCards->AddChild(Card.Plate))) {
            CardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            CardSlot->SetPadding(FMargin(i == 0 ? 0.0f : 8.0f, 0.0f, 0.0f, 0.0f));
        }
        CardPool.Add(Card);
    }
}

void UMythicStatSheetWidget::BuildTooltipPool() {
    if (!WidgetTree || TooltipPool.Num() >= PrimaryTooltipPoolSize) {
        return;
    }
    TooltipPool.Reserve(PrimaryTooltipPoolSize);
    for (int32 i = TooltipPool.Num(); i < PrimaryTooltipPoolSize; ++i) {
        FMythicStatTooltipWidgets Tip;
        Tip.Plate = WidgetTree->ConstructWidget<UBorder>();
        Tip.Plate->SetBrush(Sheet_KitBrush(TEXT("Plate.Card"), FVector2D(240.0, 120.0)));
        Tip.Plate->SetPadding(FMargin(14.0f, 10.0f));
        Tip.Plate->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        Tip.Lines = WidgetTree->ConstructWidget<UVerticalBox>();
        Tip.Plate->SetContent(Tip.Lines);

        Tip.Title = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
        Tip.Title->SetVisibility(ESlateVisibility::HitTestInvisible);
        if (UVerticalBoxSlot *TitleSlot = Cast<UVerticalBoxSlot>(Tip.Lines->AddChild(Tip.Title))) {
            TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
        }

        Tip.LinePool.Reserve(TooltipLinePoolSize);
        for (int32 LineIdx = 0; LineIdx < TooltipLinePoolSize; ++LineIdx) {
            FMythicStatTooltipLineWidgets TipLine;
            TipLine.Box = WidgetTree->ConstructWidget<UHorizontalBox>();
            TipLine.Box->SetVisibility(ESlateVisibility::Collapsed);

            TipLine.Label = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
            TipLine.Label->SetColorAndOpacity(FSlateColor(LabelColor));
            if (UHorizontalBoxSlot *LabelSlot = Cast<UHorizontalBoxSlot>(TipLine.Box->AddChild(TipLine.Label))) {
                LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
            }

            TipLine.Value = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
            TipLine.Value->SetColorAndOpacity(FSlateColor(ValueColor));
            TipLine.Value->SetJustification(ETextJustify::Right);
            if (UHorizontalBoxSlot *ValueSlot = Cast<UHorizontalBoxSlot>(TipLine.Box->AddChild(TipLine.Value))) {
                ValueSlot->SetPadding(FMargin(14.0f, 0.0f, 0.0f, 0.0f));
            }

            TipLine.Diminished = FMythicUIStyle::MakeText(this, EMythicTextRole::Subtle);
            TipLine.Diminished->SetText(NSLOCTEXT("Mythic", "ContributionDiminished", "(diminished)"));
            TipLine.Diminished->SetVisibility(ESlateVisibility::Collapsed);
            if (UHorizontalBoxSlot *DimSlot = Cast<UHorizontalBoxSlot>(TipLine.Box->AddChild(TipLine.Diminished))) {
                DimSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
                DimSlot->SetVerticalAlignment(VAlign_Center);
            }

            Tip.Lines->AddChild(TipLine.Box);
            Tip.LinePool.Add(TipLine);
        }
        TooltipPool.Add(Tip);
    }
}

void UMythicStatSheetWidget::ApplySummaries() {
    if (!SummaryCards) {
        return;
    }
    static const TArray<FMythicStatSummaryLine> NoSummaries;
    const TArray<FMythicStatSummaryLine> &Summaries = ViewModel ? ViewModel->GetSummaries() : NoSummaries;
    SummaryCards->SetVisibility(Summaries.Num() > 0 ? ESlateVisibility::SelfHitTestInvisible
                                                    : ESlateVisibility::Collapsed);
    for (int32 i = 0; i < CardPool.Num(); ++i) {
        FMythicSummaryCardWidgets &Card = CardPool[i];
        if (!Card.Plate) {
            continue;
        }
        if (!Summaries.IsValidIndex(i)) {
            Card.Plate->SetVisibility(ESlateVisibility::Collapsed);
            continue;
        }
        const FMythicStatSummaryLine &Line = Summaries[i];
        // The plate takes the hit for its description tooltip and nothing else; the card stays visually inert.
        Card.Plate->SetVisibility(ESlateVisibility::Visible);
        Card.Plate->SetToolTipText(Line.Description);
        Card.Value->SetText(Line.Value);
        Card.Label->SetText(Line.Label);
        UTexture2D *IconTexture = Line.Icon.IsNull() ? nullptr : Line.Icon.LoadSynchronous();
        if (IconTexture) {
            FSlateBrush IconBrush;
            IconBrush.SetResourceObject(IconTexture);
            IconBrush.ImageSize = FVector2D(SummaryIconSize, SummaryIconSize);
            Card.Icon->SetBrush(IconBrush);
            Card.Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else {
            Card.Icon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UMythicStatSheetWidget::Rebuild() {
    if (!ViewModel || !StatList) {
        return;
    }

    // Tooltips are re-texted from index zero each pass; a stale cursor would strand hovers on old figures.
    UsedContributionTooltips = 0;
    ApplySummaries();

    // Rows first, so the ordering pass has every widget it needs to parent. A collapsed section's rows are
    // hidden in the visibility band only - the shape keeps counting them, so no reparenting ever fires.
    TArray<int32> NewShape;
    int32 RowIndex = 0;
    for (const FMythicStatSection &Section : ViewModel->GetSections()) {
        if (Section.Lines.Num() == 0) {
            continue;
        }
        NewShape.Add(Section.Lines.Num());
        const bool bSectionCollapsed = CollapsedSections.Contains(Section.CategoryTag);
        for (const FMythicStatLine &Line : Section.Lines) {
            FMythicStatRowWidgets &Row = GetOrCreateRow(RowIndex++);
            ApplyLine(Row, Line);
            if (bSectionCollapsed && Row.Backing) {
                Row.Backing->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    int32 SectionIndex = 0;
    HeaderCategories.Reset();
    for (const FMythicStatSection &Section : ViewModel->GetSections()) {
        if (Section.Lines.Num() == 0) {
            continue;
        }
        if (UMythicSectionHeader *Header = GetOrCreateHeader(SectionIndex)) {
            const bool bSectionCollapsed = CollapsedSections.Contains(Section.CategoryTag);
            // Open sections carry no trailing count - it reads as a stat in its own right. A closed drawer
            // says what it holds, in words, so "4" can never read as a value.
            Header->SetHeader(Section.Heading,
                              bSectionCollapsed
                                  ? FText::Format(NSLOCTEXT("Mythic", "SectionHiddenCount", "{0} hidden"),
                                                  FText::AsNumber(Section.Lines.Num()))
                                  : FText::GetEmpty(),
                              nullptr);
            Header->SetCollapsible(true);
            Header->SetCollapsed(bSectionCollapsed);
            Header->SetVisibility(ESlateVisibility::Visible);
        }
        HeaderCategories.Add(Section.CategoryTag);
        ++SectionIndex;
    }

    ReorderIfShapeChanged(NewShape);

    for (int32 i = RowIndex; i < RowPool.Num(); ++i) {
        if (RowPool[i].Box) {
            RowPool[i].Box->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (SummaryText) {
        const int32 Count = ViewModel->GetModifiedStatCount();
        // Short enough to sit inline on the title row when the Blueprint hosts both in TitleRow.
        SummaryText->SetText(Count > 0
                                 ? FText::Format(NSLOCTEXT("Mythic", "StatsModifiedShort", "{0} modified"), FText::AsNumber(Count))
                                 : NSLOCTEXT("Mythic", "StatsUnmodified", "No stats modified"));
        SummaryText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}

