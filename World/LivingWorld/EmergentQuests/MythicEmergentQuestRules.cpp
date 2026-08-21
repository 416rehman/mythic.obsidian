
#include "World/LivingWorld/EmergentQuests/MythicEmergentQuestRules.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/LivingWorldTypes.h"

FMythicEmergentQuestRule::FMythicEmergentQuestRule()
    : ReqPlayerRelationToPrimary(EMythicFactionRelation::Hostile),
      DeliveryReserveAxis(EMythicResourceType::Food) {}

bool FMythicEmergentQuestRule::IsDeliveryRow() const {
    return DeliveryUnits > 0 && DeliveryItemTag.IsValid();
}

FMythicWorldEventSnapshot::FMythicWorldEventSnapshot()
    : PlayerRelation(EMythicFactionRelation::Neutral) {}

namespace MythicEmergentQuestRules {
bool PassesRelationGate(EMythicFactionRelation PlayerRelation, EMythicFactionRelation Required) {
    return static_cast<uint8>(PlayerRelation) <= static_cast<uint8>(Required);
}

int32 SelectQuestRuleForEvent(const FMythicWorldEventSnapshot &Snapshot,
                              TConstArrayView<FMythicEmergentQuestRule> Rules,
                              TConstArrayView<FGameplayTag> AlreadyActiveKinds,
                              uint32 Seed) {
    if (Rules.Num() == 0) {
        return INDEX_NONE;
    }

    FGameplayTagContainer EventTagContainer;
    if (Snapshot.EventTag.IsValid()) {
        EventTagContainer.AddTag(Snapshot.EventTag);
    }

    TArray<int32, TInlineAllocator<16>> Candidates;
    for (int32 i = 0; i < Rules.Num(); ++i) {
        const FMythicEmergentQuestRule &Rule = Rules[i];

        if (Rule.EventTagQuery.IsEmpty() || !Rule.EventTagQuery.Matches(EventTagContainer)) {
            continue;
        }
        if (Snapshot.Significance < Rule.MinSignificance) {
            continue;
        }
        if (!PassesRelationGate(Snapshot.PlayerRelation, Rule.ReqPlayerRelationToPrimary)) {
            continue;
        }
        bool bAlreadyActive = false;
        for (const FGameplayTag &ActiveKind : AlreadyActiveKinds) {
            if (Rule.QuestKind.MatchesTagExact(ActiveKind)) {
                bAlreadyActive = true;
                break;
            }
        }
        if (bAlreadyActive) {
            continue;
        }
        Candidates.Add(i);
    }

    if (Candidates.Num() == 0) {
        return INDEX_NONE;
    }
    return Candidates[static_cast<int32>(Seed % static_cast<uint32>(Candidates.Num()))];
}

FMythicEmergentReward ComputeEmergentReward(const FMythicEmergentQuestRule &Rule, int32 DangerTier, float FactionStrength) {
    const int32 SafeDanger = FMath::Max(0, DangerTier);
    const float SafeStrength = FMath::Max(0.0f, FactionStrength);
    const float SafeMultiplier = FMath::Max(0.0f, Rule.RewardTierMultiplier);

    FMythicEmergentReward Out;
    Out.QuestCount = FMath::Max(1, Rule.BaseCount + SafeDanger);
    Out.RewardTier = FMath::Max(0, FMath::RoundToInt(SafeMultiplier * (1.0f + SafeDanger) * (1.0f + SafeStrength)));
    return Out;
}
}
