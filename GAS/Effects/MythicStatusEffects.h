
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "MythicStatusEffects.generated.h"


UCLASS(Abstract)
class MYTHIC_API UMythicDebuffGameplayEffect : public UGameplayEffect {
    GENERATED_BODY()

public:
    virtual void PostInitProperties() override;

protected:
    FGameplayTagContainer GrantedDebuffTags;

    FGameplayTagContainer BlockedApplicationTags;
};

UCLASS()
class MYTHIC_API UMythicGE_Burn : public UMythicDebuffGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_Burn();
};

UCLASS()
class MYTHIC_API UMythicGE_Poison : public UMythicDebuffGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_Poison();
};

UCLASS()
class MYTHIC_API UMythicGE_Bleed : public UMythicDebuffGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_Bleed();
};

UCLASS()
class MYTHIC_API UMythicGE_Slow : public UMythicDebuffGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_Slow();
};

UCLASS()
class MYTHIC_API UMythicGE_Freeze : public UMythicDebuffGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_Freeze();
};

UCLASS()
class MYTHIC_API UMythicGE_Stun : public UMythicDebuffGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_Stun();
};

UCLASS()
class MYTHIC_API UMythicGE_Weaken : public UMythicDebuffGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_Weaken();
};

UCLASS()
class MYTHIC_API UMythicGE_Terrify : public UMythicDebuffGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_Terrify();
};

struct MYTHIC_API FMythicStatusEffectResolver {
    static TSubclassOf<UGameplayEffect> ResolveDebuffGEForStatus(const FGameplayTag &StatusType);
};
