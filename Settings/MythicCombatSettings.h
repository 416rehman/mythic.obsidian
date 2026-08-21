
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GAS/Executions/MythicDamageCompose.h"
#include "GAS/MythicHealthBands.h"
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

    /**
     * Slices of the health bar an entity advertises as GAS.State.Health.* tags while inside them. This is the whole
     * mechanism behind "hits harder when the target is nearly dead": the band tag is a normal gameplay tag, so a
     * gameplay effect gates a modifier on it with the tag requirements GAS already has, and no C++ knows which
     * talent is asking. Bands nest deliberately - an entity at 10% carries Critical, Low and Wounded at once, so a
     * two-tier talent is two modifiers rather than a special case.
     *
     * Empty (never, unless deliberately cleared) = no band tags are published and every effect gated on one is inert.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Health Bands")
    FMythicHealthBandConfig HealthBands;
};
