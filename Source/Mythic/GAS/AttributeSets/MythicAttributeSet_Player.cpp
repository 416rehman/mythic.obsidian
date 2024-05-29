#include "MythicAttributeSet_Player.h"

#include "Net/UnrealNetwork.h"

void UMythicAttributeSet_Player::OnRep_MaxHealth(const FGameplayAttributeData &OldMaxHealth) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Player, MaxHealth, OldMaxHealth);
}

void UMythicAttributeSet_Player::OnRep_Health(const FGameplayAttributeData &OldHealth) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Player, Health, OldHealth);
}

void UMythicAttributeSet_Player::OnRep_MovementSpeed(const FGameplayAttributeData &OldMovementSpeed) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Player, MovementSpeed, OldMovementSpeed);
}

void UMythicAttributeSet_Player::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Player, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Player, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Player, MovementSpeed, COND_None, REPNOTIFY_Always);
}

void UMythicAttributeSet_Player::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);
    // Clamp Health
    if (Attribute == GetHealthAttribute()) {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    // // MaxHealth should always be greater than 0
    else if (Attribute == GetMaxHealthAttribute()) {
        NewValue = FMath::Max(NewValue, 1.0f);
    }
    // Clamp MovementSpeed
    // else if (Attribute == GetMovementSpeedAttribute()) {
    //     NewValue = FMath::Clamp(NewValue, 150.0f, 600.0f);
    // }
}


