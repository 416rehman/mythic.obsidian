// Copyright Stellar Games. All Rights Reserved.

#include "MythicTabListWidget.h"

#include "CommonButtonBase.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"

void UMythicTabListWidget::HandleTabCreation_Implementation(FName TabNameID, UCommonButtonBase *TabButton) {
    Super::HandleTabCreation_Implementation(TabNameID, TabButton);

    if (!TabButtonBox || !TabButton) {
        return;
    }
    UPanelSlot *AddedSlot = TabButtonBox->AddChild(TabButton);
    if (UHorizontalBoxSlot *HSlot = Cast<UHorizontalBoxSlot>(AddedSlot)) {
        HSlot->SetPadding(FMargin(0.0f, 0.0f, TabSpacing, 0.0f));
    }
}

void UMythicTabListWidget::HandleTabRemoval_Implementation(FName TabNameID, UCommonButtonBase *TabButton) {
    if (TabButtonBox && TabButton) {
        TabButtonBox->RemoveChild(TabButton);
    }
    Super::HandleTabRemoval_Implementation(TabNameID, TabButton);
}
