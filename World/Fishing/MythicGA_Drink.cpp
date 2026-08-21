
#include "World/Fishing/MythicGA_Drink.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"

#include "World/Fishing/MythicTags_Fishing.h"
#include "World/Fishing/MythicWaterQuery.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Survival.h"
#include "Mythic.h"

UMythicGA_Drink::UMythicGA_Drink() {
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    ActivationPolicy = EMythicAbilityActivationPolicy::OnInputTriggered;
    ActivationGroup = EMythicAbilityActivationGroup::Independent;

    FAbilityTriggerData Trigger;
    Trigger.TriggerTag = TAG_FieldActivity_Drink;
    Trigger.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(Trigger);
}

void UMythicGA_Drink::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) {
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!ActorInfo || !ActorInfo->IsNetAuthority()) {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    AActor *Avatar = ActorInfo->AvatarActor.Get();
    UWorld *World = GetWorld();
    UAbilitySystemComponent *ASC = ActorInfo->AbilitySystemComponent.Get();
    if (!Avatar || !World || !ASC) {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    FHitResult Hit;
    const bool bHitWater = MythicWaterQuery::TraceWaterDown(World, Avatar->GetActorLocation(), WaterTraceDepth, Hit);
    const float Dist = bHitWater ? Hit.Distance : TNumericLimits<float>::Max();
    if (!MythicWaterQuery::IsWithinDrinkRange(bHitWater, Dist, MaxDrinkDistance)) {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    if (HydrationRestored > 0.0f) {
        ASC->ApplyModToAttribute(UMythicAttributeSet_Survival::GetHydrationAttribute(), EGameplayModOp::Additive, HydrationRestored);
    }

    const bool bFeetInWater = bHitWater && Hit.Distance <= FeetInWaterDistance;
    const float CurrentWetness = ASC->GetNumericAttribute(UMythicAttributeSet_Survival::GetWetnessAttribute());
    if (ImmersionWetness > 0.0f && MythicWaterQuery::WetnessShouldApply(bFeetInWater, CurrentWetness > 0.0f)) {
        ASC->ApplyModToAttribute(UMythicAttributeSet_Survival::GetWetnessAttribute(), EGameplayModOp::Additive, ImmersionWetness);
    }

    UE_LOG(Myth, Log, TEXT("Drink: %s drank at water (+%.0f hydration%s)"), *GetNameSafe(Avatar), HydrationRestored,
           bFeetInWater ? TEXT(", feet wet") : TEXT(""));

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
