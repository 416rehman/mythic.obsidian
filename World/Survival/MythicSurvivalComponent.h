
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "MythicSurvivalComponent.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
class UMythicAttributeSet_Survival;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicSurvivalComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicSurvivalComponent();

    static void ClassifyWeather(const FGameplayTag &WeatherTag, bool &bOutCold, bool &bOutWet);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void ServerSurvivalTick();

    UAbilitySystemComponent *ResolvePlayerASC() const;

    void ApplyStatusDiff(UAbilitySystemComponent *ASC, uint8 NewMask);

    void ClearActiveStatusEffects(UAbilitySystemComponent *ASC);

    void NotifyStatus(const FText &StatusName, bool bOnset) const;

    struct FMappedStatus {
        uint8 Bit = 0;
        TSubclassOf<UGameplayEffect> Effect = nullptr;
        FText Name;
        FGameplayTag TermTag;
    };
    TArray<FMappedStatus> MappedStatuses;

    FTimerHandle SurvivalTimerHandle;

    uint8 ActiveStatusMask = 0;

    TMap<uint8, FActiveGameplayEffectHandle> ActiveStatusHandles;

    TSet<uint8> NotifiedStatuses;

    TWeakObjectPtr<UAbilitySystemComponent> HandlesOwnerASC;
};
