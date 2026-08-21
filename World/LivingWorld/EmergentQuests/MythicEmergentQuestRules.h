
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Containers/ArrayView.h"

enum class EMythicFactionRelation : uint8;

enum class EMythicResourceType : uint8;

struct FMythicEmergentQuestRule {
    FGameplayTagQuery EventTagQuery;

    float MinSignificance = 0.0f;

    EMythicFactionRelation ReqPlayerRelationToPrimary;

    FGameplayTag QuestKind;

    FGameplayTag ObjectiveTriggerTag;
    FGameplayTag ObjectivePayloadTag;
    bool bCountByMagnitude = false;
    int32 BaseCount = 1;
    float RewardTierMultiplier = 1.0f;
    float FactionStandingReward = 0.0f;

    FText Headline;

    FGameplayTagContainer GrantStoryTagsOnComplete;

    FGameplayTag DeliveryItemTag;
    int32 DeliveryUnits = 0;
    EMythicResourceType DeliveryReserveAxis;

    bool IsDeliveryRow() const;

    FMythicEmergentQuestRule();
};

struct FMythicWorldEventSnapshot {
    FGameplayTag EventTag;
    float Significance = 0.0f;
    int32 PrimaryFactionId = -1;
    EMythicFactionRelation PlayerRelation;
    FIntPoint Cell = FIntPoint::ZeroValue;
    int32 DangerTier = 0;

    FMythicWorldEventSnapshot();
};

struct FMythicEmergentReward {
    int32 QuestCount = 0;
    int32 RewardTier = 0;
};

namespace MythicEmergentQuestRules {
MYTHIC_API bool PassesRelationGate(EMythicFactionRelation PlayerRelation, EMythicFactionRelation Required);

MYTHIC_API int32 SelectQuestRuleForEvent(const FMythicWorldEventSnapshot &Snapshot,
                                         TConstArrayView<FMythicEmergentQuestRule> Rules,
                                         TConstArrayView<FGameplayTag> AlreadyActiveKinds,
                                         uint32 Seed);

MYTHIC_API FMythicEmergentReward ComputeEmergentReward(const FMythicEmergentQuestRule &Rule, int32 DangerTier, float FactionStrength);
}
