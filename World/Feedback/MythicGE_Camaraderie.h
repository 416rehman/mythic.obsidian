#pragma once

#include "CoreMinimal.h"
#include "GAS/Effects/MythicCombatBuffs.h"
#include "MythicGE_Camaraderie.generated.h"

UCLASS()
class MYTHIC_API UMythicGE_Camaraderie : public UMythicBuffGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_Camaraderie();

    static const FName BonusMagnitudeName;
};
