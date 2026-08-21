// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonTabListWidgetBase.h"
#include "MythicTabListWidget.generated.h"

class UPanelWidget;

UCLASS()
class MYTHIC_API UMythicTabListWidget : public UCommonTabListWidgetBase {
    GENERATED_BODY()

protected:
    virtual void HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase *TabButton) override;
    virtual void HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase *TabButton) override;

    /** Where the tab buttons live. A horizontal box gives a conventional strip across the top. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> TabButtonBox;

    /** Gap between tabs. Applied to the button's slot so the strip spaces itself. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|Menu", meta = (ClampMin = "0"))
    float TabSpacing = 8.0f;
};
