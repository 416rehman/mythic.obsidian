

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GAS/MythicAbilitySystemComponent_Player.h"
#include "Player/LyraPlayerState.h"
#include "MythicPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class MYTHIC_API AMythicPlayerState : public ALyraPlayerState
{
	GENERATED_BODY()

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ability System")
    TObjectPtr<UMythicAbilitySystemComponent_Player> MythicAbilitySystemComponent;
    
	UPROPERTY(Transient)
	class UMythicAttributeSet* AttributeSet;

	// default abilities
	UPROPERTY(EditDefaultsOnly, Category = "Ability System")
	TArray<TSubclassOf<class UGameplayAbility>> DefaultAbilities;

	AMythicPlayerState();

public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UMythicAbilitySystemComponent_Player* GetMythicAbilitySystemComponent() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
};
