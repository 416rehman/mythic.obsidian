#pragma once
#include "AbilitySystemComponent.h"
#include "ASCMythic_Player.generated.h"

UCLASS(ClassGroup=AbilitySystem, hidecategories=(Object,LOD,Lighting,Transform,Sockets,TextureStreaming), editinlinenew, meta=(BlueprintSpawnableComponent))
class MYTHIC_API UASCMythic_Player : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
    // Default Ability Set
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
    TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

    // BeginPlay
    virtual void BeginPlay() override;
    
	UFUNCTION(Server, Reliable)
	void Server_TryGrantAbility(const FGameplayAbilitySpec& AbilitySpec);
	void Server_TryGrantAbility_Implementation(const FGameplayAbilitySpec& AbilitySpec);

	UFUNCTION(Server, Reliable)
	void Server_TryRemoveAbility(const FGameplayAbilitySpecHandle& Handle);
	void Server_TryRemoveAbility_Implementation(const FGameplayAbilitySpecHandle& Handle);

    UFUNCTION(BlueprintCallable)
    void AddXp(int32 XpToAdd);
};
