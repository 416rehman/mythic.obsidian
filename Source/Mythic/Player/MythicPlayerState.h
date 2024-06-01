

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GAS/MythicAbilitySystemComponent_Player.h"
#include "GameFramework/PlayerState.h"
#include "MythicPlayerState.generated.h"

class UMythicAttributeSet;
/**
 * 
 */
UCLASS()
class MYTHIC_API AMythicPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "Ability System")
    TObjectPtr<UMythicAbilitySystemComponent_Player> MythicAbilitySystemComponent;

	AMythicPlayerState();

public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UMythicAbilitySystemComponent_Player* GetMythicAbilitySystemComponent() const;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
