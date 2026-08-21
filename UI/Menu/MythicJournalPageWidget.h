// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Narrative/MythicQuestJournalComponent.h"
#include "MythicJournalPageWidget.generated.h"

class UButton;
class UCommonTextBlock;
class UMythicJournalPageWidget;
class UMythicQuestDefinition;
class UMythicQuestJournalComponent;
class UObjectiveTracker;
class UPanelWidget;

UCLASS()
class MYTHIC_API UMythicQuestClickProxy : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY()
    TWeakObjectPtr<UMythicJournalPageWidget> Page;

    UPROPERTY()
    TObjectPtr<UMythicQuestDefinition> Quest;

    UFUNCTION()
    void HandleClicked();
};

USTRUCT()
struct FMythicJournalQuestRow {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UWidget> Button;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Title;

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> State;

    UPROPERTY()
    TObjectPtr<UMythicQuestClickProxy> Proxy;
};

USTRUCT()
struct FMythicJournalTaskRow {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UCommonTextBlock> Text;
};

UCLASS()
class MYTHIC_API UMythicJournalPageWidget : public UCommonActivatableWidget {
    GENERATED_BODY()

public:
    /** Show a quest in the detail pane. Called by a row's click proxy. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Journal")
    void SelectQuest(UMythicQuestDefinition *Quest);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;
    virtual void NativeDestruct() override;

    /** Quest rows go here. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> QuestList;

    /** Task lines for the selected quest go here. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> TaskList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_QuestTitle;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_QuestBody;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_QuestState;

    /** Shown when the player has no quests at all. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCommonTextBlock> Txt_Empty;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Journal", meta = (ClampMin = "0"))
    int32 PrewarmQuestCount = 24;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Journal", meta = (ClampMin = "0"))
    int32 PrewarmTaskCount = 12;

private:
    UMythicQuestJournalComponent *GetJournal() const;
    UObjectiveTracker *GetTracker() const;

    void Bind();
    void Unbind();

    UFUNCTION()
    void HandleJournalChanged();

    void RefreshQuestList();
    void RefreshDetail();

    FMythicJournalQuestRow &GetOrCreateQuestRow(int32 Index);
    FMythicJournalTaskRow &GetOrCreateTaskRow(int32 Index);

    static FText DescribeState(EMythicQuestState State);

    UPROPERTY()
    TArray<FMythicJournalQuestRow> QuestPool;

    UPROPERTY()
    TArray<FMythicJournalTaskRow> TaskPool;

    UPROPERTY()
    TObjectPtr<UMythicQuestDefinition> SelectedQuest;

    bool bBound = false;
    bool bPoolsBuilt = false;
};
