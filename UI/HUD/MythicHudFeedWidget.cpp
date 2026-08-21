// Copyright Stellar Games. All Rights Reserved.

#include "MythicHudFeedWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "CommonTextBlock.h"
#include "UI/MythicUIStyle.h"
#include "Player/MythicPlayerController.h"
#include "TimerManager.h"

namespace {
constexpr float FeedTickInterval = 0.1f;
}

void UMythicHudFeedWidget::NativeConstruct() {
    Super::NativeConstruct();

    if (FeedList && Entries.Num() < MaxLines) {
        Entries.Reserve(MaxLines);
        for (int32 i = Entries.Num(); i < MaxLines; ++i) {
            FMythicFeedEntry Entry;
            Entry.Text = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
            if (LineFont.HasValidFont()) {
                Entry.Text->SetFont(LineFont);
            }
            Entry.Text->SetVisibility(ESlateVisibility::Collapsed);
            FeedList->AddChild(Entry.Text);
            Entries.Add(Entry);
        }
    }

    if (!bBound) {
        if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer())) {
            PC->OnHudNotice.AddDynamic(this, &UMythicHudFeedWidget::HandleNotice);
            bBound = true;
        }
    }
}

void UMythicHudFeedWidget::NativeDestruct() {
    if (bBound) {
        if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer())) {
            PC->OnHudNotice.RemoveDynamic(this, &UMythicHudFeedWidget::HandleNotice);
        }
        bBound = false;
    }
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(FeedTimer);
    }
    Super::NativeDestruct();
}

void UMythicHudFeedWidget::HandleNotice(const FMythicHudNotice &Notice) {
    if (!FMythicHudNoticeRules::GoesToFeed(Notice.Kind)) {
        return;
    }
    Push(Notice);
    Rebuild();
    SetTicking(true);
}

void UMythicHudFeedWidget::Push(const FMythicHudNotice &Notice) {
    for (FMythicFeedEntry &Entry : Entries) {
        if (Entry.bInUse && FMythicHudNoticeRules::CanMerge(Entry.Notice, Notice)) {
            const int32 Merged = Entry.Notice.Count + Notice.Count;
            Entry.Notice = Notice;
            Entry.Notice.Count = Merged;
            Entry.Remaining = FMythicHudNoticeRules::LifetimeFor(Notice.Kind);
            return;
        }
    }

    FMythicFeedEntry *Target = nullptr;
    for (FMythicFeedEntry &Entry : Entries) {
        if (!Entry.bInUse) {
            Target = &Entry;
            break;
        }
    }
    if (!Target) {
        float Lowest = TNumericLimits<float>::Max();
        for (FMythicFeedEntry &Entry : Entries) {
            if (Entry.Remaining < Lowest) {
                Lowest = Entry.Remaining;
                Target = &Entry;
            }
        }
    }
    if (!Target) {
        return;
    }

    Target->Notice = Notice;
    Target->Remaining = FMythicHudNoticeRules::LifetimeFor(Notice.Kind);
    Target->bInUse = true;
}

void UMythicHudFeedWidget::Tick10Hz() {
    bool bAnyAlive = false;
    for (FMythicFeedEntry &Entry : Entries) {
        if (!Entry.bInUse) {
            continue;
        }
        Entry.Remaining -= FeedTickInterval;
        if (Entry.Remaining <= 0.0f) {
            Entry.bInUse = false;
        }
        else {
            bAnyAlive = true;
        }
    }
    Rebuild();
    if (!bAnyAlive) {
        SetTicking(false);
    }
}

void UMythicHudFeedWidget::SetTicking(bool bEnabled) {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    if (bEnabled) {
        if (!World->GetTimerManager().IsTimerActive(FeedTimer)) {
            World->GetTimerManager().SetTimer(FeedTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { Tick10Hz(); }),
                                              FeedTickInterval, true);
        }
    }
    else {
        World->GetTimerManager().ClearTimer(FeedTimer);
    }
}

void UMythicHudFeedWidget::Rebuild() {
    for (FMythicFeedEntry &Entry : Entries) {
        if (!Entry.Text) {
            continue;
        }
        if (!Entry.bInUse) {
            Entry.Text->SetVisibility(ESlateVisibility::Collapsed);
            continue;
        }

        const FText Line = Entry.Notice.Count > 1 && !Entry.Notice.StackKey.IsNone()
                               ? FText::Format(NSLOCTEXT("Mythic", "FeedStacked", "{0}  x{1}"), Entry.Notice.Text,
                                               FText::AsNumber(Entry.Notice.Count))
                               : Entry.Notice.Text;
        Entry.Text->SetText(Line);

        const float Life = FMythicHudNoticeRules::LifetimeFor(Entry.Notice.Kind);
        const float FadeStart = Life * FadeFraction;
        const float Alpha = FadeStart > 0.0f ? FMath::Clamp(Entry.Remaining / FadeStart, 0.0f, 1.0f) : 1.0f;

        FLinearColor Colour = Entry.Notice.Accent;
        Colour.A = Alpha;
        Entry.Text->SetColorAndOpacity(FSlateColor(Colour));
        Entry.Text->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
}
