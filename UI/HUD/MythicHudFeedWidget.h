// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/MythicHudNotice.h"
#include "MythicHudFeedWidget.generated.h"

class UPanelWidget;
class UTextBlock;

USTRUCT()
struct FMythicFeedEntry {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UTextBlock> Text;

    UPROPERTY()
    FMythicHudNotice Notice;

    float Remaining = 0.0f;
    bool bInUse = false;
};

UCLASS()
class MYTHIC_API UMythicHudFeedWidget : public UUserWidget {
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** Lines are added here, newest first. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> FeedList;

    /** How many lines can be on screen at once. Older lines drop off rather than growing the list forever. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD", meta = (ClampMin = "1"))
    int32 MaxLines = 6;

    /** Fraction of a line's life spent fading out. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FadeFraction = 0.35f;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD")
    FSlateFontInfo LineFont;

private:
    UFUNCTION()
    void HandleNotice(const FMythicHudNotice &Notice);

    void Tick10Hz();
    void SetTicking(bool bEnabled);
    void Rebuild();

    void Push(const FMythicHudNotice &Notice);

    UPROPERTY()
    TArray<FMythicFeedEntry> Entries;

    FTimerHandle FeedTimer;
    bool bBound = false;
};
