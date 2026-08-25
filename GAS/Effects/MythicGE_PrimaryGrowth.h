// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "MythicGE_PrimaryGrowth.generated.h"

/**
 * The one effect that makes primary stats rise with character level. Infinite, applied with the player
 * state's default effects; both modifiers are additive magnitudes from the authored player-growth curves,
 * captured non-snapshot so a level-up moves Power and Strength - and everything derived from them - live.
 */
UCLASS()
class MYTHIC_API UMythicGE_PrimaryGrowth : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_PrimaryGrowth();
};
