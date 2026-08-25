
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicSkillDefinition.generated.h"

class UTexture2D;

UCLASS(BlueprintType)
class MYTHIC_API UMythicSkillDefinition : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    FText Name;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    TSoftObjectPtr<UTexture2D> Icon;

    // Drawn by a RichTextBlock, so style markup in the authored string is live.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill", meta = (MultiLine = true))
    FText Description;

    // Active. The slot it sits in decides which key fires it.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    TSubclassOf<UGameplayAbility> Ability;

    // The deed that earns this skill. INVALID = no deed; the skill is available from the first hour.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    FGameplayTag RequiredTag;

    // Shown while the skill is locked: how RequiredTag is earned.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill", meta = (MultiLine = true))
    FText Hint;

    // Skill.Kind.* — AoE, Defensive, Ranged, Projectile, Summon, Movement. Runes, talents and affixes key off these.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Skill")
    FGameplayTagContainer ClassificationTags;

    bool HasPayload() const { return Ability != nullptr; }
};
