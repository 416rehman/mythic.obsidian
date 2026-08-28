// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicDPSWidget.h"

#include "CommonTextBlock.h"

void UMythicDPSWidget::SetAttackDisplayData(
    const FMythicWeaponAttackViewData &InAttackDisplayData) {
    if (!InAttackDisplayData.bIsValid) {
        ClearAttackDisplayData();
        return;
    }

    AttackDisplayData = InAttackDisplayData;
    if (DamagePerSecondText) {
        DamagePerSecondText->SetText(AttackDisplayData.DamagePerSecondText);
    }
    if (DamagePerHitText) {
        DamagePerHitText->SetText(AttackDisplayData.DamagePerHitText);
    }
    if (AttacksPerSecondText) {
        AttacksPerSecondText->SetText(AttackDisplayData.AttacksPerSecondText);
    }
    OnAttackPresentationUpdated(AttackDisplayData);
}

void UMythicDPSWidget::ClearAttackDisplayData() {
    AttackDisplayData = FMythicWeaponAttackViewData();
    if (DamagePerSecondText) {
        DamagePerSecondText->SetText(FText::GetEmpty());
    }
    if (DamagePerHitText) {
        DamagePerHitText->SetText(FText::GetEmpty());
    }
    if (AttacksPerSecondText) {
        AttacksPerSecondText->SetText(FText::GetEmpty());
    }
    OnAttackPresentationUpdated(AttackDisplayData);
}
