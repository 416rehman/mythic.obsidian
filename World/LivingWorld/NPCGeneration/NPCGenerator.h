
#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "Mass/Fragments/MythicMassFragments.h"

class MYTHIC_API FMythicNPCGenerator {
public:
    static uint32 HashStep(uint32 Seed);


    static uint32 GenerateNameHash(uint8 FactionIndex, const FMythicCellCoord& Cell, int32 SpawnIndex);

    static FName ReconstructNameFromHash(uint32 NameHash, uint8 FactionIndex);


    static uint8 GenerateDemographicFlags(uint32 NameHash, bool bHasCivilians);

    static uint8 GenerateVisualArchetype(uint32 NameHash, uint8 MaxArchetypes = 8);


    static FMythicPersonalityFragment GeneratePersonality(
        uint32 NameHash,
        const FMythicIdeologyProfile& Ideology,
        const FGameplayTag& RoleTag = FGameplayTag());

private:

    static const TCHAR* GetSyllable(uint8 CultureIndex, int32 SyllableIndex, bool bIsFirst);

    static int32 GetSyllableCount(uint8 CultureIndex, bool bIsFirst);
};
