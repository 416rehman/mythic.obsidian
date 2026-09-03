
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "MythicCrowdControl.generated.h"


UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_IMMUNE_HARDCC);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_IMMUNE_FALLDAMAGE);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_SETBYCALLER_CCIMMUNE_DURATION);

// How hard one enemy tier resists repeat crowd control. CC feel is among the most-retuned numbers in an ARPG, so
// these are authored data (a per-tier row in Mythic Combat settings), never a constant in C++.
USTRUCT(BlueprintType)
struct FMythicCcEscalationConfig {
    GENERATED_BODY()

    // Each repeat CC within the window raises the buildup threshold by this fraction (0.25 = +25% per stack).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CrowdControl", meta = (ClampMin = "0.0"))
    float ThresholdEscalationStep = 0.25f;

    // Reaching this many triggers within the window grants hard-CC immunity instead of applying the status.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CrowdControl", meta = (ClampMin = "1"))
    int32 ImmunityTriggerCount = 8;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CrowdControl", meta = (ClampMin = "0.0"))
    float RollingWindowSeconds = 6.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CrowdControl", meta = (ClampMin = "0.0"))
    float ImmuneSeconds = 2.0f;
};

// One authored row of the CC escalation ladder: the tier (AI tier as an int, 1..5) and how it resists.
USTRUCT(BlueprintType)
struct FMythicCcTierEscalation {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CrowdControl", meta = (ClampMin = "1"))
    int32 Tier = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CrowdControl")
    FMythicCcEscalationConfig Config;
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

    // The escalation rules for an enemy tier, read from the authored table. An unlisted tier falls back to the
    // gentlest defaults (the struct's own initialisers) rather than resisting nothing, so a new tier is playable
    // before it is tuned.
    static FMythicCcEscalationConfig ConfigForTier(TConstArrayView<FMythicCcTierEscalation> Table, int32 EnemyTierInt) {
        for (const FMythicCcTierEscalation &Row : Table) {
            if (Row.Tier == EnemyTierInt) {
                return Row.Config;
            }
        }
        return FMythicCcEscalationConfig();
    }
};

UCLASS()
class MYTHIC_API UMythicGE_CCImmune : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_CCImmune();
    virtual void PostInitProperties() override;
};
