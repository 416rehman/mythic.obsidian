
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"
#include "World/Death/MythicDeathStakeTypes.h"
#include "MythicDeathStakeSettings.generated.h"

class AMythicPlayerGravestone;

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Mythic Death Stake"))
class MYTHIC_API UMythicDeathStakeSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override { return FName("Game"); }

    // The stake curve + gravestone lifetime. All fields default to sensible code values (see FMythicDeathStakeConfig).
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Death Stake")
    FMythicDeathStakeConfig Config;

    // The gravestone actor spawned at a player's death site. Unset (default) → the C++ AMythicPlayerGravestone (runs
    // unauthored); assign a BP subclass for authored headstone visuals + a recovery VFX cue.
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Death Stake")
    TSoftClassPtr<AMythicPlayerGravestone> GravestoneClass;
};
