
#include "MythicPlayerState.h"

#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "Net/UnrealNetwork.h"

AMythicPlayerState::AMythicPlayerState() {
	MythicAbilitySystemComponent = CreateDefaultSubobject<UMythicAbilitySystemComponent_Player>(TEXT("MythicAbilitySystemComponent"));
	MythicAbilitySystemComponent->SetIsReplicated(true);
	MythicAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UMythicAttributeSet>(TEXT("AttributeSet"));

	NetUpdateFrequency = 30.0f;
}

UAbilitySystemComponent* AMythicPlayerState::GetAbilitySystemComponent() const {
	return MythicAbilitySystemComponent;
}

UMythicAbilitySystemComponent_Player* AMythicPlayerState::GetMythicAbilitySystemComponent() const {
	return MythicAbilitySystemComponent;
}

void AMythicPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMythicPlayerState, MythicAbilitySystemComponent);
}

void AMythicPlayerState::BeginPlay() {
	Super::BeginPlay();

	// add default abilities
	for (TSubclassOf<class UGameplayAbility> Ability : DefaultAbilities) {
		MythicAbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(Ability, 1, 0, this));
	}
}
