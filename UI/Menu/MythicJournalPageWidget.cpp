// Copyright Stellar Games. All Rights Reserved.

#include "MythicJournalPageWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "GameFramework/PlayerController.h"
#include "Narrative/MythicQuestDefinition.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "UI/MythicUIStyle.h"

void UMythicQuestClickProxy::HandleClicked() {
    if (UMythicJournalPageWidget *Owner = Page.Get()) {
        Owner->SelectQuest(Quest);
    }
}


void UMythicJournalPageWidget::NativeConstruct() {
    if (!bPoolsBuilt) {
        bPoolsBuilt = true;

        if (QuestList) {
            for (int32 i = 0; i < PrewarmQuestCount; ++i) {
                GetOrCreateQuestRow(i);
            }
        }
        if (TaskList) {
            for (int32 i = 0; i < PrewarmTaskCount; ++i) {
                GetOrCreateTaskRow(i);
            }
        }
    }

    Super::NativeConstruct();
}

void UMythicJournalPageWidget::NativeOnActivated() {
    Super::NativeOnActivated();
    Bind();
    RefreshQuestList();
    RefreshDetail();
}

void UMythicJournalPageWidget::NativeOnDeactivated() {
    Unbind();
    Super::NativeOnDeactivated();
}

void UMythicJournalPageWidget::NativeDestruct() {
    Unbind();
    Super::NativeDestruct();
}

UMythicQuestJournalComponent *UMythicJournalPageWidget::GetJournal() const {
    if (const APlayerController *PC = GetOwningPlayer()) {
        if (AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>()) {
            return PS->GetQuestJournal();
        }
    }
    return nullptr;
}

UObjectiveTracker *UMythicJournalPageWidget::GetTracker() const {
    if (const AMythicPlayerController *PC = Cast<AMythicPlayerController>(GetOwningPlayer())) {
        return PC->GetObjectiveTracker();
    }
    return nullptr;
}

void UMythicJournalPageWidget::Bind() {
    if (bBound) {
        return;
    }
    if (UMythicQuestJournalComponent *Journal = GetJournal()) {
        Journal->OnQuestsChanged.AddDynamic(this, &UMythicJournalPageWidget::HandleJournalChanged);
    }
    if (UObjectiveTracker *Tracker = GetTracker()) {
        Tracker->OnObjectivesChanged.AddDynamic(this, &UMythicJournalPageWidget::HandleJournalChanged);
    }
    bBound = true;
}

void UMythicJournalPageWidget::Unbind() {
    if (!bBound) {
        return;
    }
    if (UMythicQuestJournalComponent *Journal = GetJournal()) {
        Journal->OnQuestsChanged.RemoveDynamic(this, &UMythicJournalPageWidget::HandleJournalChanged);
    }
    if (UObjectiveTracker *Tracker = GetTracker()) {
        Tracker->OnObjectivesChanged.RemoveDynamic(this, &UMythicJournalPageWidget::HandleJournalChanged);
    }
    bBound = false;
}

void UMythicJournalPageWidget::HandleJournalChanged() {
    RefreshQuestList();
    RefreshDetail();
}


FMythicJournalQuestRow &UMythicJournalPageWidget::GetOrCreateQuestRow(int32 Index) {
    if (QuestPool.IsValidIndex(Index)) {
        return QuestPool[Index];
    }

    FMythicJournalQuestRow Row;
    UCommonTextBlock *Title = nullptr;
    UCommonTextBlock *State = nullptr;
    Row.Button = FMythicUIStyle::MakeButton(this, EMythicTextRole::Body, Title, &State);
    Row.Title = Title;
    Row.State = State;

    Row.Proxy = NewObject<UMythicQuestClickProxy>(this);
    Row.Proxy->Page = this;
    FMythicUIStyle::BindButtonClicked(Row.Button, Row.Proxy,
                                      GET_FUNCTION_NAME_CHECKED(UMythicQuestClickProxy, HandleClicked));

    Row.Button->SetVisibility(ESlateVisibility::Collapsed);
    QuestList->AddChild(Row.Button);

    QuestPool.Add(Row);
    return QuestPool.Last();
}

FMythicJournalTaskRow &UMythicJournalPageWidget::GetOrCreateTaskRow(int32 Index) {
    if (TaskPool.IsValidIndex(Index)) {
        return TaskPool[Index];
    }

    FMythicJournalTaskRow Row;
    Row.Text = FMythicUIStyle::MakeText(this, EMythicTextRole::Body);
    Row.Text->SetVisibility(ESlateVisibility::Collapsed);
    TaskList->AddChild(Row.Text);

    TaskPool.Add(Row);
    return TaskPool.Last();
}


FText UMythicJournalPageWidget::DescribeState(EMythicQuestState State) {
    switch (State) {
        case EMythicQuestState::Active:
            return NSLOCTEXT("Mythic", "QuestActive", "In hand");
        case EMythicQuestState::Completed:
            return NSLOCTEXT("Mythic", "QuestDone", "Done");
        case EMythicQuestState::Failed:
            return NSLOCTEXT("Mythic", "QuestFailed", "Lost");
        default:
            return NSLOCTEXT("Mythic", "QuestNotStarted", "Not begun");
    }
}

void UMythicJournalPageWidget::RefreshQuestList() {
    if (!QuestList) {
        return;
    }
    const UMythicQuestJournalComponent *Journal = GetJournal();
    const TArray<FMythicQuestJournalEntry> Entries =
        Journal ? Journal->GetQuests() : TArray<FMythicQuestJournalEntry>();

    TArray<const FMythicQuestJournalEntry *> Ordered;
    Ordered.Reserve(Entries.Num());
    for (const FMythicQuestJournalEntry &E : Entries) {
        if (E.Quest) {
            Ordered.Add(&E);
        }
    }
    Ordered.Sort([](const FMythicQuestJournalEntry &A, const FMythicQuestJournalEntry &B) {
        const auto Rank = [](EMythicQuestState S) {
            return S == EMythicQuestState::Active ? 0 : (S == EMythicQuestState::NotStarted ? 1 : 2);
        };
        const int32 RA = Rank(A.State);
        const int32 RB = Rank(B.State);
        if (RA != RB) {
            return RA < RB;
        }
        return A.Quest->JournalTitle.CompareTo(B.Quest->JournalTitle) < 0;
    });

    int32 Used = 0;
    bool bSelectionStillListed = false;
    for (const FMythicQuestJournalEntry *E : Ordered) {
        FMythicJournalQuestRow &Row = GetOrCreateQuestRow(Used++);
        Row.Proxy->Quest = E->Quest;
        const bool bActive = (E->State == EMythicQuestState::Active);
        Row.Title->SetText(E->Quest->JournalTitle);
        Row.Title->SetColorAndOpacity(FSlateColor(bActive ? FMythicUIStyle::Get().Ink
                                                          : FMythicUIStyle::Get().InkSubtle));
        FMythicUIStyle::SetOptionalText(Row.State, DescribeState(E->State),
                                        bActive ? FMythicUIStyle::Get().Caution : FMythicUIStyle::Get().InkSubtle);
        Row.Button->SetVisibility(ESlateVisibility::Visible);

        if (E->Quest == SelectedQuest) {
            bSelectionStillListed = true;
        }
    }

    for (int32 i = Used; i < QuestPool.Num(); ++i) {
        QuestPool[i].Button->SetVisibility(ESlateVisibility::Collapsed);
        QuestPool[i].Proxy->Quest = nullptr;
    }

    if (!bSelectionStillListed) {
        SelectedQuest = Ordered.Num() > 0 ? Ordered[0]->Quest : nullptr;
    }

    const bool bEmpty = Ordered.Num() == 0;
    if (Txt_Empty) {
        Txt_Empty->SetText(NSLOCTEXT("Mythic", "JournalEmpty",
                                     "Undertakings you accept are written here, with what remains of each."));
        Txt_Empty->SetVisibility(bEmpty ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    FMythicUIStyle::ShowEmptyState(this, TEXT("EmptyState_Journal"), bEmpty);
    if (UWidget *Detail = GetWidgetFromName(TEXT("DetailRail"))) {
        Detail->SetVisibility(bEmpty ? ESlateVisibility::Collapsed : ESlateVisibility::SelfHitTestInvisible);
    }
}

void UMythicJournalPageWidget::RefreshDetail() {
    const UMythicQuestDefinition *Quest = SelectedQuest;

    if (Txt_QuestTitle) {
        Txt_QuestTitle->SetText(Quest ? Quest->JournalTitle
                                      : NSLOCTEXT("Mythic", "JournalNoQuestTitle", "Nothing taken on"));
    }
    if (Txt_QuestBody) {
        Txt_QuestBody->SetText(Quest ? Quest->JournalText
                                     : NSLOCTEXT("Mythic", "JournalNoQuestBody",
                                                 "Undertakings you accept are written here, with what remains of each."));
    }
    if (Txt_QuestState) {
        const UMythicQuestJournalComponent *Journal = GetJournal();
        Txt_QuestState->SetText(Quest && Journal ? DescribeState(Journal->GetQuestState(Quest)) : FText::GetEmpty());
    }

    if (!TaskList) {
        return;
    }

    const UObjectiveTracker *Tracker = GetTracker();
    int32 Used = 0;
    if (Quest) {
        for (const TObjectPtr<UObjectiveDefinition> &Task : Quest->Tasks) {
            if (!Task) {
                continue;
            }
            FMythicJournalTaskRow &Row = GetOrCreateTaskRow(Used++);

            FObjectiveProgress Prog;
            const bool bTracked = Tracker && Tracker->FindObjectiveProgress(Task.Get(), Prog);
            const bool bDone = bTracked && Prog.bCompleted;

            FText Line = Task->DisplayText;
            if (bTracked && !bDone && Task->RequiredCount > 1) {
                Line = FText::Format(NSLOCTEXT("Mythic", "TaskProgress", "{0}   {1}/{2}"), Line,
                                     FText::AsNumber(Prog.CurrentCount), FText::AsNumber(Task->RequiredCount));
            }
            Row.Text->SetText(Line);
            Row.Text->SetColorAndOpacity(FSlateColor(bDone ? FMythicUIStyle::Get().Positive
                                                           : (bTracked ? FMythicUIStyle::Get().Ink
                                                                       : FMythicUIStyle::Get().InkSubtle)));
            Row.Text->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
    }

    for (int32 i = Used; i < TaskPool.Num(); ++i) {
        TaskPool[i].Text->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UMythicJournalPageWidget::SelectQuest(UMythicQuestDefinition *Quest) {
    if (SelectedQuest == Quest) {
        return;
    }
    SelectedQuest = Quest;
    RefreshDetail();
}
