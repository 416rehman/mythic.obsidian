
#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MythicSkillProgressTypes.generated.h"

class UMythicSkillDefinition;
class UMythicSkillComponent;

/**
 * One skill's growth, keyed on the definition so a skill keeps what it earned whether or not it is currently bound
 * to a slot. The stored indices are positions in that definition's Modifiers array, which is why the array is
 * append-only: reorder it and every saved character wakes up holding a different choice.
 */
USTRUCT(BlueprintType)
struct FMythicSkillProgress {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, SaveGame) bool bLearned = false;
    UPROPERTY(BlueprintReadOnly, SaveGame) TArray<FName> ActiveModifierIds;
    UPROPERTY(SaveGame) int32 SchemaVersion = 0;

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

USTRUCT()
struct FMythicSkillProgressItem : public FFastArraySerializerItem {
    GENERATED_BODY()
    UPROPERTY() FMythicSkillProgress Data;
};

USTRUCT()
struct FMythicSkillProgressArray : public FFastArraySerializer {
    GENERATED_BODY()
    UPROPERTY() TArray<FMythicSkillProgressItem> Items;
    UPROPERTY(NotReplicated) TObjectPtr<UMythicSkillComponent> Owner;
    bool NetDeltaSerialize(FNetDeltaSerializeInfo& Info) {
        return FastArrayDeltaSerialize<FMythicSkillProgressItem, FMythicSkillProgressArray>(Items, Info, *this);
    }
    void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters& Parameters);
};

template<> struct TStructOpsTypeTraits<FMythicSkillProgressArray> : TStructOpsTypeTraitsBase2<FMythicSkillProgressArray> {
    enum { WithNetDeltaSerializer = true };
};

USTRUCT(BlueprintType)
struct FMythicAbilityCooldownSave {
    GENERATED_BODY()
    UPROPERTY(SaveGame) FSoftObjectPath Skill;
    UPROPERTY(SaveGame) float RemainingSeconds = 0;
};
