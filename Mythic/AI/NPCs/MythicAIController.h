// 

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AbilitySystemInterface.h"
#include "DetourCrowdAIController.h"
#include "GameplayTagContainer.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "MythicAIController.generated.h"

class UMythicAttributeSet_NPCCombat;
class UMythicAbilitySystemComponent;

/// Every NPC will have a name, and an Ability System Component with LifeAttributeSet
UCLASS(Blueprintable, BlueprintType, Abstract)
class MYTHIC_API AMythicAIController : public ADetourCrowdAIController, public IAbilitySystemInterface {
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    AMythicAIController();

protected:
    // Name of the NPC
    UPROPERTY(EditAnywhere, Category = "Mythic NPC",  Replicated)
    FText Name;

    // The role of the NPC in their faction
    UPROPERTY(EditAnywhere, Category = "Mythic NPC", Replicated)
    FGameplayTag NPCRole;

    // Affiliation of the NPC
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic NPC | Behavior", Replicated)
    FGameplayTag Faction;

    //replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

public:
    virtual void BeginPlay() override;
    
    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

    //~ Begin IGenericTeamAgentInterface Interface
    virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor &Other) const override;
    //~ End IGenericTeamAgentInterface Interface
};
