// 

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "MythicGameplayAbility.generated.h"

class APCMythic;
/**
 * 
 */
UCLASS()
class MYTHIC_API UMythicGameplayAbility : public UGameplayAbility {
    GENERATED_BODY()

public:
    // Whether this ability is a passive one
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
    bool bIsPassive = false;

    // Override OnAvatarSet
    virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

    // UFUNCTION(BlueprintCallable, Category = "Lyra|Ability")
    // APCMythic* GetMythicPlayerControllerFromActorInfo() const;
};
