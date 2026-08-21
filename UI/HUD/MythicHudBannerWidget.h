// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/MythicHudNotice.h"
#include "MythicHudBannerWidget.generated.h"

class UTextBlock;
class UWidget;

UCLASS()
class MYTHIC_API UMythicHudBannerWidget : public UUserWidget {
    GENERATED_BODY()

public:
    /** Play a banner directly, for beats raised locally rather than over the notice delegate. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|HUD")
    void ShowBanner(const FMythicHudNotice &Notice);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_Title;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Txt_Detail;

    /** Root of the visible banner. Collapsed between beats. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> BannerRoot;

    /** Blueprint hook for the entrance animation. The C++ owns WHEN a banner shows; the Blueprint owns how it moves. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|HUD")
    void OnBannerShown(const FMythicHudNotice &Notice);

    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|HUD")
    void OnBannerHidden();

    /** Beats waiting behind the current one. Bounded so a burst cannot queue forever. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD", meta = (ClampMin = "1"))
    int32 MaxQueued = 4;

private:
    UFUNCTION()
    void HandleNotice(const FMythicHudNotice &Notice);

    void PlayNext();
    void HideCurrent();

    UPROPERTY()
    TArray<FMythicHudNotice> Queue;

    FTimerHandle BannerTimer;
    bool bBound = false;
    bool bShowing = false;
};
