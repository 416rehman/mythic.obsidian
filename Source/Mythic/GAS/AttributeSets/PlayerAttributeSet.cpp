#include "PlayerAttributeSet.h"

#include "Net/UnrealNetwork.h"

void UPlayerAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData &OldMaxHealth) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, MaxHealth, OldMaxHealth);
}

void UPlayerAttributeSet::OnRep_Health(const FGameplayAttributeData &OldHealth) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, Health, OldHealth);
}

void UPlayerAttributeSet::OnRep_MovementSpeed(const FGameplayAttributeData &OldMovementSpeed) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, MovementSpeed, OldMovementSpeed);
}

void UPlayerAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, Health, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, MovementSpeed, COND_None, REPNOTIFY_Always);
}

void UPlayerAttributeSet::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
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


