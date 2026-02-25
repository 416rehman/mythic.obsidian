// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MythicNPCData.h"
#include "Player/MythicCharacter.h"
#include "MythicNPCCharacter.generated.h"

class UMythicNPCManager;
class UMythicAttributeSet_NPCCombat;
class UMythicAttributeSet_Life;
class UMythicCognitiveBrainComponent;
struct FMassEntityHandle;

UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API AMythicNPCCharacter : public AMythicCharacter {
    GENERATED_BODY()

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FMythicNPCData NPCData;

    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
    UAbilitySystemComponent *AbilitySystemComponent;

    // The LifeAttributeSet for the NPC
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Stats")
    UMythicAttributeSet_Life *LifeAttributes;

    // The Combat Attribute Set for the NPC
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Stats")
    UMythicAttributeSet_NPCCombat *CombatAttributes;

    // Cognitive brain for Tier 2+ NPCs
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UMythicCognitiveBrainComponent* CognitiveBrain;

    // Get NPC Data
    UFUNCTION(BlueprintCallable, Category = "Mythic NPC | Data")
    const FMythicNPCData GetNPCData() const;

public:
    //~ Begin IPoolableNPC Interface
    virtual void OnSpawnedFromPool(const struct FMythicNPCData &InNPCData);
    virtual void OnReturnedToPool();
    //~ End IPoolableNPC Interface

    // Get the NPC Id
    UFUNCTION(BlueprintCallable, Category = "Mythic NPC | Data")
    const FGuid &GetNPCId() const;

    // Get the NPC Type
    UFUNCTION(BlueprintCallable, Category = "Mythic NPC | Data")
    const FGameplayTag &GetNPCType() const;

    /**
     * Initializes the character from its Mass entity counterpart upon promotion to a fully simulated actor.
     * Hooks up the Cognitive Brain, Faction bindings, and Personality.
     */
    void InitializeFromMassEntity(const FMassEntityHandle& InEntityHandle);

    // Sets default values for this character's properties
    AMythicNPCCharacter();

    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
    virtual void InitializeASC() override;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> &OutLifetimeProps) const override;

    // PossessedBy - Server side
    virtual void PossessedBy(AController *NewController) override;

    friend class UMythicNPCManager;
};
