// Copyright Stellar Games. All Rights Reserved.

#include "MythicObjectiveTrackerWidget.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "CommonTextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/App.h"
#include "Player/MythicPlayerController.h"
#include "TimerManager.h"
#include "UI/MythicHUDLayout.h"
#include "UI/MythicUIStyle.h"

namespace {
constexpr float Tracker_ArriveSeconds = 0.17f;
constexpr float Tracker_HoldSeconds = 2.5f;
constexpr float Tracker_FadeSeconds = 0.6f;
constexpr float Tracker_MotionInterval = 1.0f / 30.0f;
constexpr float Tracker_StrikeHeight = 8.0f;
constexpr float Tracker_DimTint = 0.60f;
const FLinearColor Tracker_InkCount(0.70f, 0.65f, 0.55f, 1.0f);
const FName Tracker_RevealStart(TEXT("RevealStart"));

float Tracker_MaterialNow() {
    return static_cast<float>(FApp::GetCurrentTime() - GStartTime);
}
}

void UMythicObjectiveTrackerWidget::NativeOnInitialized() {
    Super::NativeOnInitialized();
    BuildRows();
}

void UMythicObjectiveTrackerWidget::NativeConstruct() {
    Super::NativeConstruct();
    BuildRows();

    if (Img_Thread && !ThreadMID) {
        ThreadMID = Img_Thread->GetDynamicMaterial();
    }
    RefreshRoot();

    if (!bBound) {
        if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer())) {
            PC->OnHudNotice.AddDynamic(this, &UMythicObjectiveTrackerWidget::HandleNotice);
            bBound = true;
        }
    }

    if (UWorld *World = GetWorld()) {
        TWeakObjectPtr<UMythicObjectiveTrackerWidget> WeakThis(this);
        World->GetTimerManager().SetTimerForNextTick([WeakThis]() {
            if (UMythicObjectiveTrackerWidget *Self = WeakThis.Get()) {
                if (UMythicHUDLayout *Layout = Self->GetTypedOuter<UMythicHUDLayout>()) {
                    Layout->SetElementDimTint(Self, Tracker_DimTint);
                }
            }
        });
    }
}

void UMythicObjectiveTrackerWidget::NativeDestruct() {
    if (bBound) {
        if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer())) {
            PC->OnHudNotice.RemoveDynamic(this, &UMythicObjectiveTrackerWidget::HandleNotice);
        }
        bBound = false;
    }
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(MotionTimer);
    }
    Super::NativeDestruct();
}

void UMythicObjectiveTrackerWidget::BuildRows() {
    if (bRowsBuilt) {
        return;
    }
    bRowsBuilt = true;
    Lines.Reserve(MaxTracked);

    for (int32 i = 0; i < MaxTracked; ++i) {
        FMythicTrackedObjective Line;
        Line.Row = GetWidgetFromName(*FString::Printf(TEXT("Row_%d"), i));
        Line.Text = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("Txt_Line_%d"), i)));
        Line.Count = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("Txt_Count_%d"), i)));
        Line.Strike = Cast<UImage>(GetWidgetFromName(*FString::Printf(TEXT("Img_Strike_%d"), i)));
        if (Line.Strike) {
            Line.StrikeMID = Line.Strike->GetDynamicMaterial();
        }
        if (!Line.Row || !Line.Text) {
            if (!ObjectiveList) {
                break;
            }
            Line.Text = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
            Line.Text->SetJustification(ETextJustify::Right);
            ObjectiveList->AddChild(Line.Text);
            Line.Row = Line.Text;
            Line.Count = nullptr;
            Line.Strike = nullptr;
            Line.StrikeMID = nullptr;
        }
        ResetRow(Line);
        Lines.Add(Line);
    }
}

void UMythicObjectiveTrackerWidget::ResetRow(FMythicTrackedObjective &Line) {
    Line.Key = NAME_None;
    Line.bInUse = false;
    Line.bDone = false;
    Line.DoneAge = -1.0f;
    if (Line.Row) {
        Line.Row->SetVisibility(ESlateVisibility::Collapsed);
        Line.Row->SetRenderOpacity(1.0f);
    }
    if (Line.Text) {
        Line.Text->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().Ink));
    }
    if (Line.Count) {
        Line.Count->SetVisibility(ESlateVisibility::Collapsed);
        Line.Count->SetColorAndOpacity(FSlateColor(Tracker_InkCount));
    }
    if (Line.Strike) {
        Line.Strike->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (Line.StrikeMID) {
        Line.StrikeMID->SetScalarParameterValue(Tracker_RevealStart, -1.0f);
    }
}

bool UMythicObjectiveTrackerWidget::AnyRowInUse() const {
    for (const FMythicTrackedObjective &Line : Lines) {
        if (Line.bInUse) {
            return true;
        }
    }
    return false;
}

void UMythicObjectiveTrackerWidget::HandleNotice(const FMythicHudNotice &Notice) {
    if (Notice.Kind != EMythicNoticeKind::Objective || Lines.Num() == 0) {
        return;
    }
    const bool bWasEmpty = !AnyRowInUse();

    LeaveAge = -1.0f;
    SetRenderOpacity(1.0f);

    if (Txt_Title) {
        if (!Notice.Detail.IsEmpty()) {
            if (!Notice.Detail.EqualTo(LastTitle)) {
                LastTitle = Notice.Detail;
                Txt_Title->SetText(LastTitle);
                if (ThreadMID) {
                    ThreadMID->SetScalarParameterValue(Tracker_RevealStart, Tracker_MaterialNow());
                }
            }
            Txt_Title->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else if (LastTitle.IsEmpty()) {
            Txt_Title->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    FMythicTrackedObjective *Target = nullptr;
    for (FMythicTrackedObjective &Line : Lines) {
        if (Line.bInUse && Line.Key == Notice.StackKey) {
            Target = &Line;
            break;
        }
    }
    if (!Target) {
        for (FMythicTrackedObjective &Line : Lines) {
            if (!Line.bInUse) {
                Target = &Line;
                break;
            }
        }
    }
    if (!Target) {
        for (FMythicTrackedObjective &Line : Lines) {
            if (Line.bDone) {
                Target = &Line;
                break;
            }
        }
    }
    if (!Target) {
        Target = &Lines[0];
    }
    if (!Target->Text) {
        return;
    }
    if (!Target->bInUse || Target->bDone) {
        ResetRow(*Target);
    }
    Target->Key = Notice.StackKey;
    Target->bInUse = true;
    if (Target->Row) {
        Target->Row->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    if (!Target->Text->GetText().EqualTo(Notice.Text)) {
        Target->Text->SetText(Notice.Text);
    }
    if (Target->Count) {
        if (Notice.Total > 1) {
            Target->Count->SetText(FText::Format(NSLOCTEXT("Mythic", "ObjectiveCount", "{0} / {1}"),
                                                 FText::AsNumber(Notice.Count), FText::AsNumber(Notice.Total)));
            Target->Count->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else {
            Target->Count->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
    else {
        Target->Text->SetText(Notice.Total > 1
                                  ? FText::Format(NSLOCTEXT("Mythic", "ObjectiveLineCounted", "{0}  {1} / {2}"), Notice.Text,
                                                  FText::AsNumber(Notice.Count), FText::AsNumber(Notice.Total))
                                  : Notice.Text);
    }

    if (Notice.bTerminal && !Target->bDone) {
        Target->bDone = true;
        Target->DoneAge = 0.0f;
        Target->Text->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().InkSubtle));
        if (Target->Count) {
            Target->Count->SetColorAndOpacity(FSlateColor(FMythicUIStyle::Get().InkSubtle));
        }
        if (Target->Strike) {
            float Width = 80.0f;
            if (FSlateApplication::IsInitialized()) {
                const TSharedRef<FSlateFontMeasure> Measure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
                Width = FMath::Max(Measure->Measure(Target->Text->GetText(), Target->Text->GetFont()).X, 8.0f);
            }
            Target->Strike->SetDesiredSizeOverride(FVector2D(Width, Tracker_StrikeHeight));
            Target->Strike->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        if (Target->StrikeMID) {
            Target->StrikeMID->SetScalarParameterValue(Tracker_RevealStart, Tracker_MaterialNow());
        }
        bool bOtherInProgress = false;
        for (const FMythicTrackedObjective &Line : Lines) {
            if (Line.bInUse && !Line.bDone) {
                bOtherInProgress = true;
                break;
            }
        }
        if (!bOtherInProgress) {
            LeaveAge = 0.0f;
        }
        SetMotionTicking(true);
    }

    RefreshRoot();
    if (bWasEmpty) {
        ArriveAge = 0.0f;
        SetRenderOpacity(0.0f);
        SetMotionTicking(true);
    }
    if (UMythicHUDLayout *Layout = GetTypedOuter<UMythicHUDLayout>()) {
        Layout->PokeElement(this);
    }
}

void UMythicObjectiveTrackerWidget::RefreshRoot() {
    UWidget *Root = TrackerRoot ? TrackerRoot.Get() : ObjectiveList.Get();
    if (!Root) {
        return;
    }
    Root->SetVisibility(AnyRowInUse() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UMythicObjectiveTrackerWidget::TickMotion(float DeltaSeconds) {
    bool bAnyMoving = false;

    if (ArriveAge >= 0.0f) {
        ArriveAge += DeltaSeconds;
        const float Alpha = FMath::Clamp(ArriveAge / Tracker_ArriveSeconds, 0.0f, 1.0f);
        SetRenderOpacity(LeaveAge >= 0.0f ? 1.0f : Alpha);
        if (ArriveAge >= Tracker_ArriveSeconds) {
            ArriveAge = -1.0f;
        }
        else {
            bAnyMoving = true;
        }
    }

    if (LeaveAge >= 0.0f) {
        LeaveAge += DeltaSeconds;
        if (LeaveAge > Tracker_HoldSeconds) {
            SetRenderOpacity(1.0f - FMath::Clamp((LeaveAge - Tracker_HoldSeconds) / Tracker_FadeSeconds, 0.0f, 1.0f));
        }
        if (LeaveAge >= Tracker_HoldSeconds + Tracker_FadeSeconds) {
            LeaveAge = -1.0f;
            for (FMythicTrackedObjective &Line : Lines) {
                ResetRow(Line);
            }
            LastTitle = FText::GetEmpty();
            SetRenderOpacity(1.0f);
            RefreshRoot();
        }
        else {
            bAnyMoving = true;
        }
    }
    else {
        for (FMythicTrackedObjective &Line : Lines) {
            if (!Line.bInUse || !Line.bDone || Line.DoneAge < 0.0f) {
                continue;
            }
            Line.DoneAge += DeltaSeconds;
            if (Line.DoneAge > Tracker_HoldSeconds && Line.Row) {
                Line.Row->SetRenderOpacity(1.0f - FMath::Clamp((Line.DoneAge - Tracker_HoldSeconds) / Tracker_FadeSeconds, 0.0f, 1.0f));
            }
            if (Line.DoneAge >= Tracker_HoldSeconds + Tracker_FadeSeconds) {
                ResetRow(Line);
                RefreshRoot();
            }
            else {
                bAnyMoving = true;
            }
        }
    }

    if (!bAnyMoving) {
        SetMotionTicking(false);
    }
}

void UMythicObjectiveTrackerWidget::SetMotionTicking(bool bEnabled) {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    if (bEnabled) {
        if (!World->GetTimerManager().IsTimerActive(MotionTimer)) {
            World->GetTimerManager().SetTimer(MotionTimer, FTimerDelegate::CreateWeakLambda(this, [this]() {
                TickMotion(Tracker_MotionInterval);
            }), Tracker_MotionInterval, true);
        }
    }
    else {
        World->GetTimerManager().ClearTimer(MotionTimer);
    }
}
