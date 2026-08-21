// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/MythicHudNotice.h"
#include "MythicObjectiveTrackerWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UPanelWidget;
class UTextBlock;

USTRUCT()
struct FMythicTrackedObjective {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UWidget> Row;

    UPROPERTY()
    TObjectPtr<UTextBlock> Text;

    UPROPERTY()
    TObjectPtr<UTextBlock> Count;

    UPROPERTY()
    TObjectPtr<UImage> Strike;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> StrikeMID;

    UPROPERTY()
    FName Key;

    bool bInUse = false;

    bool bDone = false;

    float DoneAge = -1.0f;
};

UCLASS()
class MYTHIC_API UMythicObjectiveTrackerWidget : public UUserWidget {
    GENERATED_BODY()

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** The whole pin: title, thread, list. Collapsed when nothing is tracked so an empty tracker leaves no furniture. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> TrackerRoot;

    /** The quest's name over the thread. Collapsed when the notice carried no quest. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_Title;

    /** The stone and its thread (MI_UI_Thread_Pin). The thread redraws itself when the quest changes. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Img_Thread;

    /** The rows live here. With the authored Row_N widgets missing, plain text rows are pooled into it instead. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> ObjectiveList;

    /** Pooled rows, and the hard cap on how many objectives show at once. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD", meta = (ClampMin = "1", ClampMax = "6"))
    int32 MaxTracked = 3;

private:
    UFUNCTION()
    void HandleNotice(const FMythicHudNotice &Notice);

    void BuildRows();
    void ResetRow(FMythicTrackedObjective &Line);
    bool AnyRowInUse() const;
    void RefreshRoot();

    void TickMotion(float DeltaSeconds);
    void SetMotionTicking(bool bEnabled);

    UPROPERTY()
    TArray<FMythicTrackedObjective> Lines;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ThreadMID;

    FText LastTitle;

    float ArriveAge = -1.0f;
    float LeaveAge = -1.0f;

    FTimerHandle MotionTimer;
    bool bRowsBuilt = false;
    bool bBound = false;
};
