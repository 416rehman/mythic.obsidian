// 

#pragma once

#include "CoreMinimal.h"
#include "MythicCharacter.h"
#include "MythicNPCCharacter.generated.h"

class UMythicAttributeSet_NPCCombat;
class UMythicAttributeSet_Life;

// NPC Config
USTRUCT(BlueprintType, Blueprintable)
struct FMythicNPCConfig {
    GENERATED_BODY()

    // The Level of the NPC - This will be used as the key for the BaselineCurveTable to get the stats
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    uint8 Level = 1;
};

UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API AMythicNPCCharacter : public AMythicCharacter {
    GENERATED_BODY()

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FMythicNPCConfig Config;

    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
    UAbilitySystemComponent *AbilitySystemComponent;

    // The LifeAttributeSet for the NPC
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Stats")
    UMythicAttributeSet_Life *LifeAttributes;

    // The Combat Attribute Set for the NPC
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic NPC | Stats")
    UMythicAttributeSet_NPCCombat *CombatAttributes;

public:
    // Sets default values for this character's properties
    AMythicNPCCharacter();
    
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
    virtual void InitializeASC() override;

    virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty> &OutLifetimeProps) const override;
};
