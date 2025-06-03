// 

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AbilitySystemInterface.h"
#include "DetourCrowdAIController.h"
#include "MythicAIController.generated.h"

class UMythicAttributeSet_NPCCombat;
class UMythicAbilitySystemComponent;

/// Every NPC will have a name, and an Ability System Component with LifeAttributeSet
/// AIController's are not replicated, so values needed on the clients should go in the NPCCharacter
UCLASS(Blueprintable, BlueprintType, Abstract)
class MYTHIC_API AMythicAIController : public ADetourCrowdAIController, public IAbilitySystemInterface {
    GENERATED_BODY()

public:
    AMythicAIController();
    virtual void BeginPlay() override;

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

    //~ Begin IGenericTeamAgentInterface Interface
    virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor &Other) const override;
    //~ End IGenericTeamAgentInterface Interface
};
