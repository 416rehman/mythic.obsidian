
#include "MythicPlayerState.h"
#include "Net/UnrealNetwork.h"

AMythicPlayerState::AMythicPlayerState() {
	MythicAbilitySystemComponent = CreateDefaultSubobject<UMythicAbilitySystemComponent_Player>(TEXT("MythicAbilitySystemComponent"));
	MythicAbilitySystemComponent->SetIsReplicated(true);
	MythicAbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
    UE_LOG(LogTemp, Warning, TEXT("MythicAbilitySystemComponent %s created"), *MythicAbilitySystemComponent->GetName());

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