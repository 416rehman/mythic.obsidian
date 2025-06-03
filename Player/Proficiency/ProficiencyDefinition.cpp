#include "ProficiencyDefinition.h"
#include "Mythic.h"
#include "ProficiencyComponent.h"
#include "Rewards/AttributeReward.h"

FAttributeGoal::FAttributeGoal() {}

FAttributeGoal::FAttributeGoal(FGameplayAttribute InAttribute, float InGoal, EGameplayModOp::Type InModifier): Attribute(InAttribute), Goal(InGoal),
    Modifier(InModifier) {}

UProficiencyDefinition::UProficiencyDefinition() {}

float UProficiencyDefinition::CalcXPCostForLevelUp(int32 Level, const UProficiencyDefinition *Def) {
    if (!Def) {
        UE_LOG(Mythic, Error, TEXT("ProficiencyDefinition::CalcCumulativeXPForLevel: Def is null."));
        return 0.0f;
    }
    if (Level < 1) {
        return 0.0f;
    }
    return STARTING_XP * FMath::Pow(Def->GrowthRate, Level - 1);
}

float UProficiencyDefinition::CalcCumulativeXPForLevel(int32 Level, const UProficiencyDefinition *Def) {
    if (!Def) {
        UE_LOG(Mythic, Error, TEXT("ProficiencyDefinition::CalcCumulativeXPForLevel: Def is null."));
        return 0.0f;
    }
    if (Level <= 1) {
        return 0.0f;
    }
    if (FMath::IsNearlyEqual(Def->GrowthRate, 1.0f)) {
        return STARTING_XP * (Level - 1);
    }
    return STARTING_XP * ((FMath::Pow(Def->GrowthRate, Level - 1) - 1.0f) / (Def->GrowthRate - 1.0f));
}

int32 UProficiencyDefinition::CalcLevelAtXP(float XP, const UProficiencyDefinition *Def) {
    if (!Def) {
        UE_LOG(Mythic, Error, TEXT("ProficiencyDefinition::CalcLevelAtXP: Def is null."));
        return 1;
    }
    if (XP < 0.0f) {
        return 1;
    }
    if (FMath::IsNearlyEqual(Def->GrowthRate, 1.0f)) {
        return FMath::FloorToInt(XP / STARTING_XP) + 1;
    }
    // Inverting the geometric series:
    //   XP = STARTING_XP * ((GrowthRate^(L - 1) - 1) / (GrowthRate - 1))
    //   => GrowthRate^(L - 1) = (XP*(GrowthRate - 1)/STARTING_XP) + 1
    //   => L = Floor( LogBase_GrowthRate( (XP*(GrowthRate - 1)/STARTING_XP) + 1 ) ) + 1
    int32 Level = FMath::FloorToInt(
        FMath::LogX(Def->GrowthRate, (XP * (Def->GrowthRate - 1.0f) / STARTING_XP) + 1.0f)
        ) + 1;
    return Level;
}

float UProficiencyDefinition::CalcXPRemainingForLevel(float CurrentXP, int32 TargetLevel, const UProficiencyDefinition *Def) {
    if (TargetLevel <= CalcLevelAtXP(CurrentXP, Def)) {
        return 0.0f;
    }
    float RequiredXP = CalcCumulativeXPForLevel(TargetLevel, Def);
    return FMath::Max(RequiredXP - CurrentXP, 0.0f);
}

FString UProficiencyDefinition::GetProgressionBreakdown() const {
    // Warn on high growth rates.
    if (GrowthRate > 1.4f) {
        UE_LOG(Mythic, Warning, TEXT("High growth rate of %.2f may result in very high XP requirements for later levels."), GrowthRate);
    }

    FString Breakdown;
    Breakdown += FString::Printf(TEXT("Proficiency Track: %s\n"), *Name.ToString());
    Breakdown += FString::Printf(TEXT("Max Level: %d, Growth Rate: %.2f, Base XP Per Action: %.2f\n\n"),
                                 MaxLevel, GrowthRate, BaseXPPerAction);
    Breakdown += TEXT("Level | XP Required | Cumulative XP | Actions Needed* | Total Actions Needed | Milestone\n");
    Breakdown += TEXT("-------------------------------------------------------------\n");

    FProficiency Proficiency;
    Proficiency.Definition = const_cast<UProficiencyDefinition *>(this);
    Proficiency.Instantiate();

    auto TotalActions = 0;
    for (int32 Level = 1; Level <= MaxLevel; ++Level) {
        float XPCost;
        float CumulativeXP;
        if (Level < MaxLevel) {
            XPCost = CalcXPCostForLevelUp(Level, this);
            CumulativeXP = CalcCumulativeXPForLevel(Level + 1, this);
        }
        else {
            // At MaxLevel, no further XP is required.
            XPCost = 0.0f;
            CumulativeXP = CalcCumulativeXPForLevel(Level, this);
        }
        int32 ActionsNeeded = (BaseXPPerAction > 0) ? FMath::CeilToInt(XPCost / BaseXPPerAction) : 0;
        TotalActions += ActionsNeeded;

        auto Milestone = Proficiency.Track[Level - 1];
        FString MilestoneText = Milestone.Name.ToString();

        if (!MilestoneText.IsEmpty()) {
            MilestoneText += TEXT("⭐");
        }
        if (Milestone.Rewards.Num() > 0) {
            for (auto Reward : Milestone.Rewards) {
                // Make sure the spec has a reward definition.
                auto RewardDef = Reward;
                if (!RewardDef) {
                    continue;
                }

                // If the reward is an attribute reward, print it.
                auto AttributeRwdDef = Cast<UAttributeReward>(RewardDef);
                if (AttributeRwdDef) {
                    MilestoneText += FString::Printf(TEXT(" (%s %s%.0f)"),
                                                     *AttributeRwdDef->Attribute.GetName(),
                                                     AttributeRwdDef->Modifier == EGameplayModOp::Additive ? TEXT("+") : TEXT("*"),
                                                     AttributeRwdDef->Magnitude);
                }
                else {
                    MilestoneText += RewardDef->GetName();
                }

                if (&Reward != &Milestone.Rewards.Last()) {
                    MilestoneText += TEXT(", ");
                }
            }
        }

        Breakdown += FString::Printf(TEXT("%3d   | %10.0f | %12.0f | %14d | %18d | %s\n"),
                                     Level, XPCost, CumulativeXP, ActionsNeeded, TotalActions, *MilestoneText);
    }

    Breakdown += TEXT("\n* Actions needed assumes base XP per action with no multipliers\n");
    return Breakdown;
}

FString UProficiencyDefinition::GetTimeToMaxLevelEstimate(float ActionsPerMinute) const {
    if (ActionsPerMinute <= 0.0f) {
        return TEXT("Invalid actions per minute.");
    }

    // Total XP required to reach max level is given by the cumulative XP for MaxLevel.
    float TotalXPNeeded = CalcCumulativeXPForLevel(MaxLevel, this);
    float TotalActionsNeeded = (BaseXPPerAction > 0) ? TotalXPNeeded / BaseXPPerAction : 0.0f;
    float MinutesNeeded = TotalActionsNeeded / ActionsPerMinute;
    float HoursNeeded = MinutesNeeded / 60.0f;
    float DaysNeeded = HoursNeeded / 24.0f;

    return FString::Printf(TEXT("Estimated Time to Max Level:\nMinutes: %.2f\nHours: %.2f\nDays: %.2f"),
                           MinutesNeeded, HoursNeeded, DaysNeeded);
}

void UProficiencyDefinition::PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) {
    Super::PostEditChangeProperty(PropertyChangedEvent);
    UE_LOG(Mythic, Log, TEXT("%s"), *GetProgressionBreakdown());
}
