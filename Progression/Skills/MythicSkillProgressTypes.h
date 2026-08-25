
#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "MythicSkillProgressTypes.generated.h"

class UMythicSkillDefinition;

/**
 * One skill's growth, keyed on the definition so a skill keeps what it earned whether or not it is currently bound
 * to a slot. The stored indices are positions in that definition's Modifiers array, which is why the array is
 * append-only: reorder it and every saved character wakes up holding a different choice.
 */
USTRUCT(BlueprintType)
struct FMythicSkillProgress {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Progression|Skills")
    TSoftObjectPtr<UMythicSkillDefinition> Skill;

    // Each level grants one point, so this doubles as the skill's point budget. A skill nobody has levelled is 1.
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Progression|Skills")
    int32 Level = 1;

    // Positions in Modifiers that are switched on. The point stays spent for as long as the index sits here.
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Progression|Skills")
    TArray<int32> ActiveModifiers;

    // Times the character has cast this skill. The authored ladder in MythicDeveloperSettings turns this into Level,
    // which is why nothing a client can call needs to raise a level: practice is the only currency.
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Progression|Skills")
    int32 Uses = 0;
};
