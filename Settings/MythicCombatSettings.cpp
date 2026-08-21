
#include "Settings/MythicCombatSettings.h"

#include "GAS/MythicTags_GAS.h"

namespace {
FMythicHealthBand MakeBand(const FGameplayTag &Tag, float Min, float Max) {
    FMythicHealthBand Band;
    Band.Tag = Tag;
    Band.MinFraction = Min;
    Band.MaxFraction = Max;
    return Band;
}
}

UMythicCombatSettings::UMythicCombatSettings() {
    HealthBands.Bands = {
        MakeBand(GAS_STATE_HEALTH_CRITICAL, 0.0f, 0.20f),
        MakeBand(GAS_STATE_HEALTH_LOW, 0.0f, 0.50f),
        MakeBand(GAS_STATE_HEALTH_WOUNDED, 0.0f, 0.90f),
        MakeBand(GAS_STATE_HEALTH_UNHURT, 0.90f, 1.0f),
    };
}
