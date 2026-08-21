
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "MythicCombatBuffs.generated.h"


UCLASS(Abstract)
class MYTHIC_API UMythicBuffGameplayEffect : public UGameplayEffect {
    GENERATED_BODY()

public:
    virtual void PostInitProperties() override;

protected:
    FGameplayTagContainer GrantedBuffTags;
};

UCLASS()
class MYTHIC_API UMythicGE_EvadeIFrames : public UMythicBuffGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_EvadeIFrames();

    static constexpr float IFrameDuration = 0.4f;
};

UCLASS()
class MYTHIC_API UMythicGE_Block : public UMythicBuffGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_Block();
};
