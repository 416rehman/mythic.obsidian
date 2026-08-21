#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "MythicGA_Drink.generated.h"

UCLASS()
class MYTHIC_API UMythicGA_Drink : public UMythicGameplayAbility {
    GENERATED_BODY()

public:
    UMythicGA_Drink();

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

protected:
    // Hydration restored per successful drink (additive; the survival set clamps to [0, MaxHydration]).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drink", meta = (ClampMin = "0.0"))
    float HydrationRestored = 25.0f;

    // Max distance (cm) from the drinker's feet down to a water surface to permit a drink.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drink", meta = (ClampMin = "0.0"))
    float MaxDrinkDistance = 250.0f;

    // Depth (cm) of the downward water trace from the drinker.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drink", meta = (ClampMin = "0.0"))
    float WaterTraceDepth = 400.0f;

    // Water surface at/below this distance (cm) counts as feet-in-water → a minimal immersion Wetness bump on drink.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drink", meta = (ClampMin = "0.0"))
    float FeetInWaterDistance = 70.0f;

    // Wetness applied when drinking with feet in the water (0 disables the immersion bump). Rain wetness is owned by the
    // survival component — this is only the drink-from-water immersion edge.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Drink", meta = (ClampMin = "0.0"))
    float ImmersionWetness = 10.0f;
};
