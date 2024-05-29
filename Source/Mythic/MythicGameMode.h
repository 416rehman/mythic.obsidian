// 

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MythicGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MYTHIC_API AMythicGameMode : public AGameModeBase {
    GENERATED_BODY()

public:
    // Curve Table to use for character progression
    UPROPERTY(EditDefaultsOnly, Category = "Ability System")
    UCurveTable* XPLevelsCurveTable;
};
