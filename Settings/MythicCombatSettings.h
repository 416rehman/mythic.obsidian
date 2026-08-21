
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GAS/Executions/MythicDamageCompose.h"
#include "GAS/MythicWeatherCombatRules.h"
#include "MythicCombatSettings.generated.h"

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Mythic Combat"))
class MYTHIC_API UMythicCombatSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    UMythicCombatSettings();

    virtual FName GetCategoryName() const override { return FName("Game"); }

    // Increased-vs-More damage bucket configuration. Empty buckets (default) = the compose layer is inert.
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Damage Compose")
    FMythicDamageComposeConfig DamageCompose;

    // Weather×combat elemental coupling (J1). Empty Mods (default) = weather never touches damage (byte-identical
    // pipeline). Authored rows couple the live Environment.Weather.* state to damage: e.g. rain smothers fire hits
    // (×0.75) while conducting shock (×1.25 + bonus Slowed buildup) — see MythicWeatherCombatRules.h for the documented
    // sample set. Resolved ONCE per damage application (a cheap subsystem getter — no Tick, no polling).
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Weather Combat")
    FMythicWeatherCombatConfig WeatherCombat;
};
