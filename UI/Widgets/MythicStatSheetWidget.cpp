// Copyright Stellar Games. All Rights Reserved.

#include "MythicStatSheetWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "CommonTextBlock.h"
#include "UI/MythicUIStyle.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "UI/ViewModels/MythicStatSheetViewModel.h"

namespace {
constexpr float ValueColumnWidth = 84.0f;
constexpr float BonusColumnWidth = 64.0f;

const TCHAR *Sheet_BarMaterialPath = TEXT("/Game/Mythic/UI/Globals/materials/M_UI_BarHand.M_UI_BarHand");
constexpr float Sheet_BarHeight = 5.0f;
constexpr float Sheet_BarMinPercent = 0.024f;
const FName Sheet_Bar_Percent(TEXT("Percent"));
const FName Sheet_Bar_ChipPercent(TEXT("ChipPercent"));
const FName Sheet_Bar_ChipAlpha(TEXT("ChipAlpha"));
const FName Sheet_Bar_FillStart(TEXT("FillColorStart"));
const FName Sheet_Bar_FillEnd(TEXT("FillColorEnd"));
const FName Sheet_Bar_Background(TEXT("BackgroundColor"));
const FLinearColor Sheet_BarFillStart(0.36f, 0.25f, 0.075f, 1.0f);
const FLinearColor Sheet_BarFillEnd(0.42f, 0.29f, 0.09f, 1.0f);
const FLinearColor Sheet_BarTrack(0.016f, 0.013f, 0.010f, 1.0f);

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
            Row.Box->SetVisibility(ESlateVisibility::Collapsed);
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

void UMythicStatSheetWidget::ToggleShowUnmodified() {
    if (ViewModel) {
        ViewModel->SetShowUnmodified(!ViewModel->GetShowUnmodified());
    }
}

void UMythicStatSheetWidget::HandleFieldChanged(UObject *Object, UE::FieldNotification::FFieldId FieldId) {
    Rebuild();
}

FMythicStatRowWidgets &UMythicStatSheetWidget::GetOrCreateRow(int32 Index) {
    if (RowPool.IsValidIndex(Index)) {
        return RowPool[Index];
    }

    FMythicStatRowWidgets Row;
    Row.Box = WidgetTree->ConstructWidget<UHorizontalBox>();

    Row.Label = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
    if (UHorizontalBoxSlot *LabelSlot = Cast<UHorizontalBoxSlot>(Row.Box->AddChild(Row.Label))) {
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    }

    Row.ValueBox = WidgetTree->ConstructWidget<UOverlay>();
    Row.ValueBox->SetVisibility(ESlateVisibility::HitTestInvisible);

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
    if (UOverlaySlot *BarSlot = Row.ValueBox->AddChildToOverlay(Row.Bar)) {
        BarSlot->SetHorizontalAlignment(HAlign_Fill);
        // Under the number, not through it. Centred, the bar's own centreline landed in the middle of a
        // top-aligned value and every gauged stat read as struck-through text.
        BarSlot->SetVerticalAlignment(VAlign_Bottom);
    }

    Row.Value = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
    Row.Value->SetJustification(ETextJustify::Right);
    Row.Value->SetMinDesiredWidth(ValueColumnWidth);
    if (UOverlaySlot *ValueSlot = Row.ValueBox->AddChildToOverlay(Row.Value)) {
        ValueSlot->SetHorizontalAlignment(HAlign_Right);
        ValueSlot->SetVerticalAlignment(VAlign_Top);
        // Clears the gauge sitting on the row's baseline.
        ValueSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, Sheet_BarHeight + 2.0f));
    }
    if (UHorizontalBoxSlot *BoxSlot = Cast<UHorizontalBoxSlot>(Row.Box->AddChild(Row.ValueBox))) {
        BoxSlot->SetHorizontalAlignment(HAlign_Right);
    }

    Row.Bonus = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
    Row.Bonus->SetJustification(ETextJustify::Right);
    Row.Bonus->SetMinDesiredWidth(BonusColumnWidth);
    if (UHorizontalBoxSlot *BonusSlot = Cast<UHorizontalBoxSlot>(Row.Box->AddChild(Row.Bonus))) {
        BonusSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
        BonusSlot->SetHorizontalAlignment(HAlign_Right);
    }

    Row.Box->SetVisibility(ESlateVisibility::HitTestInvisible);

    if (StatList) {
        StatList->AddChild(Row.Box);
    }
    RowPool.Add(Row);
    return RowPool.Last();
}

void UMythicStatSheetWidget::ApplyHeading(FMythicStatRowWidgets &Row, const FText &Heading) {
    Row.Box->SetVisibility(ESlateVisibility::HitTestInvisible);
    SetRowGap(Row, HeadingTopPadding);
    Row.Label->SetText(Heading);
    Row.Label->SetColorAndOpacity(FSlateColor(HeadingColor));
    if (HeadingFont.HasValidFont()) {
        Row.Label->SetFont(HeadingFont);
    }
    if (Row.ValueBox) {
        Row.ValueBox->SetVisibility(ESlateVisibility::Collapsed);
    }
    Row.Value->SetVisibility(ESlateVisibility::Collapsed);
    Row.Bonus->SetVisibility(ESlateVisibility::Collapsed);
}

void UMythicStatSheetWidget::SetRowGap(FMythicStatRowWidgets &Row, float TopGap) {
    if (!Row.Box) {
        return;
    }
    if (UVerticalBoxSlot *BoxSlot = Cast<UVerticalBoxSlot>(Row.Box->Slot)) {
        BoxSlot->SetPadding(FMargin(0.0f, TopGap, 0.0f, 0.0f));
    }
}

void UMythicStatSheetWidget::ApplyLine(FMythicStatRowWidgets &Row, const FMythicStatLine &Line) {
    Row.Box->SetVisibility(ESlateVisibility::HitTestInvisible);
    SetRowGap(Row, 0.0f);

    Row.Label->SetText(Line.Label);
    Row.Label->SetColorAndOpacity(FSlateColor(LabelColor));
    if (RowFont.HasValidFont()) {
        Row.Label->SetFont(RowFont);
    }

    if (Row.ValueBox) {
        Row.ValueBox->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    Row.Value->SetVisibility(ESlateVisibility::HitTestInvisible);
    Row.Value->SetText(Line.Value);
    Row.Value->SetColorAndOpacity(FSlateColor(ValueColor));
    if (RowFont.HasValidFont()) {
        Row.Value->SetFont(RowFont);
    }

    if (Row.Bar) {
        if (Line.BarPercent >= 0.0f) {
            Row.Bar->SetVisibility(ESlateVisibility::HitTestInvisible);
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

    Row.Bonus->SetVisibility(ESlateVisibility::HitTestInvisible);
    Row.Bonus->SetText(Line.BonusText);
    Row.Bonus->SetColorAndOpacity(FSlateColor(Line.BonusValue < 0.0f ? PenaltyColor : BonusColor));
    if (RowFont.HasValidFont()) {
        Row.Bonus->SetFont(RowFont);
    }
}

void UMythicStatSheetWidget::Rebuild() {
    if (!ViewModel || !StatList) {
        return;
    }

    int32 RowIndex = 0;
    for (const FMythicStatSection &Section : ViewModel->GetSections()) {
        if (Section.Lines.Num() == 0) {
            continue;
        }
        ApplyHeading(GetOrCreateRow(RowIndex++), Section.Heading);
        for (const FMythicStatLine &Line : Section.Lines) {
            ApplyLine(GetOrCreateRow(RowIndex++), Line);
        }
    }

    for (int32 i = RowIndex; i < RowPool.Num(); ++i) {
        if (RowPool[i].Box) {
            RowPool[i].Box->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (SummaryText) {
        const int32 Count = ViewModel->GetModifiedStatCount();
        SummaryText->SetText(Count > 0
                                 ? FText::Format(NSLOCTEXT("Mythic", "StatsModified", "{0} stats modified"), FText::AsNumber(Count))
                                 : NSLOCTEXT("Mythic", "StatsUnmodified", "No stats modified"));
        SummaryText->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}
