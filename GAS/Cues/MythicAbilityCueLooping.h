#pragma once
#include "GameplayCueNotify_Looping.h"
#include "MythicAbilityCueLooping.generated.h"

/** Each ability cast owns its notify instance, even when another caster grants the same tag. */
UCLASS(Blueprintable)
class MYTHIC_API AMythicAbilityCueLooping : public AGameplayCueNotify_Looping {
    GENERATED_BODY()
public:
    AMythicAbilityCueLooping();
    virtual void HandleGameplayCue(AActor* Target,EGameplayCueEvent::Type Event,const FGameplayCueParameters& Parameters) override;
};
