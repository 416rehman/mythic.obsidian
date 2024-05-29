#include "MythicAbilitySystemComponent_Player.h"

#include "Mythic/Mythic.h"

void UMythicAbilitySystemComponent_Player::BeginPlay() {
    Super::BeginPlay();

    // Grant default abilities
    for (TSubclassOf<UGameplayAbility> Ability : DefaultAbilities) {
        FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability, 1, INDEX_NONE, this);
        this->Server_TryGrantAbility(AbilitySpec);
    }
}

void UMythicAbilitySystemComponent_Player::Server_TryGrantAbility_Implementation(const FGameplayAbilitySpec& AbilitySpec) {
	// if the ability is not already granted
	if (this->FindAbilitySpecFromClass(AbilitySpec.Ability->GetClass()) == nullptr) {
		this->GiveAbility(AbilitySpec);
	} else {
		UE_LOG(Mythic, Warning, TEXT("UMythicAbilitySystemComponent_Player::Server_GrantAbility_Implementation: Ability already granted"));
	}
}

void UMythicAbilitySystemComponent_Player::Server_TryRemoveAbility_Implementation(const FGameplayAbilitySpecHandle& Handle) {
	this->ClearAbility(Handle);
}

void UMythicAbilitySystemComponent_Player::AddXp(int32 XpToAdd) {
    // Server only
    if (GetOwnerRole() != ROLE_Authority) {
        UE_LOG(Mythic, Error, TEXT("UMythicAbilitySystemComponent_Player::AddXp - Not authority"));
        return;
    }

    // Call tag event to add XP
    FGameplayEventData EventData;
    EventData.EventMagnitude = XpToAdd;
    this->HandleGameplayEvent(FGameplayTag::RequestGameplayTag(FName("GAS.Attribute.Progression.XpGain")), &EventData);
}
