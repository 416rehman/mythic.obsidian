// Copyright Stellar Games. All Rights Reserved.

#include "MythicHudBannerWidget.h"

#include "Components/TextBlock.h"
#include "Player/MythicPlayerController.h"
#include "TimerManager.h"
#include "Components/RichTextBlock.h"

void UMythicHudBannerWidget::NativeConstruct() {
    Super::NativeConstruct();

    if (BannerRoot) {
        BannerRoot->SetVisibility(ESlateVisibility::Collapsed);
    }
    if (!bBound) {
        if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer())) {
            PC->OnHudNotice.AddDynamic(this, &UMythicHudBannerWidget::HandleNotice);
            bBound = true;
        }
    }
}

void UMythicHudBannerWidget::NativeDestruct() {
    if (bBound) {
        if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer())) {
            PC->OnHudNotice.RemoveDynamic(this, &UMythicHudBannerWidget::HandleNotice);
        }
        bBound = false;
    }
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(BannerTimer);
    }
    Super::NativeDestruct();
}

void UMythicHudBannerWidget::HandleNotice(const FMythicHudNotice &Notice) {
    if (!FMythicHudNoticeRules::GoesToBanner(Notice.Kind)) {
        return;
    }
    ShowBanner(Notice);
}

void UMythicHudBannerWidget::ShowBanner(const FMythicHudNotice &Notice) {
    if (Queue.Num() < MaxQueued) {
        Queue.Add(Notice);
    }
    if (!bShowing) {
        PlayNext();
    }
}

void UMythicHudBannerWidget::PlayNext() {
    if (Queue.Num() == 0) {
        HideCurrent();
        return;
    }

    const FMythicHudNotice Notice = Queue[0];
    Queue.RemoveAt(0);
    bShowing = true;

    if (Txt_Title) {
        Txt_Title->SetText(Notice.Text);
        Txt_Title->SetColorAndOpacity(FSlateColor(Notice.Accent));
        Txt_Title->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    const bool bUseRich = Rich_Detail != nullptr;
    if (Rich_Detail) {
        const bool bHasDetail = !Notice.Detail.IsEmpty();
        Rich_Detail->SetText(Notice.Detail);
        Rich_Detail->SetVisibility(bHasDetail ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (Txt_Detail) {
        const bool bHasDetail = !Notice.Detail.IsEmpty() && !bUseRich;
        Txt_Detail->SetText(bUseRich ? FText::GetEmpty() : Notice.Detail);
        Txt_Detail->SetVisibility(bHasDetail ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (BannerRoot) {
        BannerRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
    }

    OnBannerShown(Notice);

    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimer(BannerTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { PlayNext(); }),
                                          FMythicHudNoticeRules::LifetimeFor(Notice.Kind), false);
    }
}

void UMythicHudBannerWidget::HideCurrent() {
    bShowing = false;
    if (BannerRoot) {
        BannerRoot->SetVisibility(ESlateVisibility::Collapsed);
    }
    OnBannerHidden();
}
