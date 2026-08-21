
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "MythicCrowdControl.generated.h"


UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_IMMUNE_HARDCC);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_SETBYCALLER_CCIMMUNE_DURATION);

USTRUCT()
struct FMythicCcEscalationConfig {
    GENERATED_BODY()

    UPROPERTY()
    float ThresholdEscalationStep = 0.25f;

    UPROPERTY()
    int32 ImmunityTriggerCount = 8;

    UPROPERTY()
    float RollingWindowSeconds = 6.0f;

    UPROPERTY()
    float ImmuneSeconds = 2.0f;
};

USTRUCT()
struct FMythicCcTrackState {
    GENERATED_BODY()

    UPROPERTY()
    int32 TriggersInWindow = 0;

    UPROPERTY()
    float WindowStartTime = 0.0f;

    UPROPERTY()
    float LastTriggerTime = 0.0f;
};

struct FMythicCcResolution {
    float EffectiveThresholdMultiplier = 1.0f;
    bool bGrantImmunity = false;
    FMythicCcTrackState NextState;
};

struct FMythicCrowdControlRules {
    static FMythicCcResolution ResolveCcTrigger(const FMythicCcTrackState &Prior, const FMythicCcEscalationConfig &Cfg, float Now) {
        FMythicCcResolution Out;

        const float Window = FMath::Max(0.0f, Cfg.RollingWindowSeconds);
        const bool bHasActiveWindow = (Prior.TriggersInWindow > 0);
        const bool bWithinWindow = bHasActiveWindow && ((Now - Prior.WindowStartTime) <= Window);

        int32 CountThisTrigger;
        float WindowStart;
        if (bWithinWindow) {
            CountThisTrigger = Prior.TriggersInWindow + 1;
            WindowStart = Prior.WindowStartTime;
        }
        else {
            CountThisTrigger = 1;
            WindowStart = Now;
        }

        const float Step = FMath::Max(0.0f, Cfg.ThresholdEscalationStep);
        Out.EffectiveThresholdMultiplier = 1.0f + Step * static_cast<float>(CountThisTrigger - 1);

        const int32 ImmunityAt = FMath::Max(1, Cfg.ImmunityTriggerCount);
        Out.NextState.LastTriggerTime = Now;
        if (CountThisTrigger >= ImmunityAt) {
            Out.bGrantImmunity = true;
            Out.NextState.TriggersInWindow = 0;
            Out.NextState.WindowStartTime = 0.0f;
        }
        else {
            Out.NextState.TriggersInWindow = CountThisTrigger;
            Out.NextState.WindowStartTime = WindowStart;
        }
        return Out;
    }

    static FMythicCcEscalationConfig ConfigForTier(int32 EnemyTierInt) {
        FMythicCcEscalationConfig Cfg;
        switch (EnemyTierInt) {
            case 5:
                Cfg.ThresholdEscalationStep = 1.0f;
                Cfg.ImmunityTriggerCount = 2;
                Cfg.RollingWindowSeconds = 12.0f;
                Cfg.ImmuneSeconds = 8.0f;
                break;
            case 4:
                Cfg.ThresholdEscalationStep = 0.75f;
                Cfg.ImmunityTriggerCount = 3;
                Cfg.RollingWindowSeconds = 10.0f;
                Cfg.ImmuneSeconds = 6.0f;
                break;
            case 3:
                Cfg.ThresholdEscalationStep = 0.5f;
                Cfg.ImmunityTriggerCount = 4;
                Cfg.RollingWindowSeconds = 10.0f;
                Cfg.ImmuneSeconds = 5.0f;
                break;
            case 2:
                Cfg.ThresholdEscalationStep = 0.35f;
                Cfg.ImmunityTriggerCount = 6;
                Cfg.RollingWindowSeconds = 8.0f;
                Cfg.ImmuneSeconds = 3.5f;
                break;
            case 1:
            default:
                Cfg.ThresholdEscalationStep = 0.25f;
                Cfg.ImmunityTriggerCount = 8;
                Cfg.RollingWindowSeconds = 6.0f;
                Cfg.ImmuneSeconds = 2.0f;
                break;
        }
        return Cfg;
    }
};

UCLASS()
class MYTHIC_API UMythicGE_CCImmune : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_CCImmune();
    virtual void PostInitProperties() override;
};
