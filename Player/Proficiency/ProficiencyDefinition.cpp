#include "ProficiencyDefinition.h"
#include "Mythic.h"
#include "ProficiencyComponent.h"
#include "Rewards/AttributeReward.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "Stats/MythicStatDefinition.h"
#include "System/MythicAssetManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/DataValidation.h"

const UProficiencyDefinition *UProficiencyDefinition::FindByProgressAttribute(const FGameplayAttribute &Attribute) {
    static TMap<FGameplayAttribute, TWeakObjectPtr<const UProficiencyDefinition>> Cache;
#if WITH_EDITOR
    // Editor asset replacement and property editing can invalidate either side of this derived cache.
    Cache.Reset();
#endif
    if (const TWeakObjectPtr<const UProficiencyDefinition> *Found = Cache.Find(Attribute)) {
        if (Found->IsValid()) {
            return Found->Get();
        }
    }
    const FAssetRegistryModule &Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> Assets;
    Registry.Get().GetAssetsByClass(UProficiencyDefinition::StaticClass()->GetClassPathName(), Assets, true);
    Assets.Sort([](const FAssetData &Left, const FAssetData &Right) {
        return Left.GetSoftObjectPath().ToString() < Right.GetSoftObjectPath().ToString();
    });
    const UProficiencyDefinition *Match = nullptr;
    for (const FAssetData &Asset : Assets) {
        const UProficiencyDefinition *Def = Cast<UProficiencyDefinition>(Asset.GetAsset());
        if (Def && Def->GetProgressAttribute() == Attribute) {
            if (Match) {
                UE_LOG(Myth, Error,
                       TEXT("Duplicate Proficiency Definitions map the same canonical Progress Stat: %s and %s."),
                       *Match->GetPathName(), *Def->GetPathName());
                return nullptr;
            }
            Match = Def;
        }
    }
    if (Match) {
        Cache.Add(Attribute, Match);
    }
    return Match;
}

FAttributeGoal::FAttributeGoal() {}

FAttributeGoal::FAttributeGoal(FMythicStatDefinitionHandle InStatDefinition,
                               const float InGoal,
                               const EGameplayModOp::Type InModifier)
    : TargetStat(MoveTemp(InStatDefinition)), Goal(InGoal), Modifier(InModifier) {}

UProficiencyDefinition::UProficiencyDefinition() {}

FPrimaryAssetId UProficiencyDefinition::GetPrimaryAssetId() const {
    return FPrimaryAssetId(UMythicAssetManager::ProficiencyDefinitionType,
                           GetFName());
}

const UMythicStatDefinition *UProficiencyDefinition::GetProgressStatDefinition() const {
    return ProgressStat.GetAsset();
}

FGameplayAttribute UProficiencyDefinition::GetProgressAttribute() const {
    const UMythicStatDefinition *Definition = GetProgressStatDefinition();
    return Definition ? Definition->Attribute : FGameplayAttribute();
}

FGameplayAttribute UProficiencyDefinition::GetProgressCapacityAttribute() const {
    const UMythicStatDefinition *Definition = GetProgressStatDefinition();
    const UMythicStatDefinition *Capacity = Definition
        ? Definition->PairedStat.GetAsset() : nullptr;
    return Capacity ? Capacity->Attribute : FGameplayAttribute();
}

float UProficiencyDefinition::CalcXPCostForLevelUp(int32 Level, const UProficiencyDefinition *Def) {
    if (!Def) {
        UE_LOG(Myth, Error, TEXT("ProficiencyDefinition::CalcXPCostForLevelUp: Def is null."));
        return 0.0f;
    }
    if (Level < 1) {
        return 0.0f;
    }
    return STARTING_XP * FMath::Pow(Def->GrowthRate, Level - 1);
}

float UProficiencyDefinition::CalcCumulativeXPForLevel(int32 Level, const UProficiencyDefinition *Def) {
    if (!Def) {
        UE_LOG(Myth, Error, TEXT("ProficiencyDefinition::CalcCumulativeXPForLevel: Def is null."));
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
        UE_LOG(Myth, Error, TEXT("ProficiencyDefinition::CalcLevelAtXP: Def is null."));
        return 1;
    }
    if (XP < 0.0f) {
        return 1;
    }
    const int32 MaxLvl = FMath::Max(1, Def->MaxLevel);
    int32 Level = 1;
    while (Level < MaxLvl && CalcCumulativeXPForLevel(Level + 1, Def) <= XP) {
        ++Level;
    }
    return Level;
}

float UProficiencyDefinition::CalcXPRemainingForLevel(float CurrentXP, int32 TargetLevel, const UProficiencyDefinition *Def) {
    if (TargetLevel <= CalcLevelAtXP(CurrentXP, Def)) {
        return 0.0f;
    }
    float RequiredXP = CalcCumulativeXPForLevel(TargetLevel, Def);
    return FMath::Max(RequiredXP - CurrentXP, 0.0f);
}
#if WITH_EDITOR
EDataValidationResult UProficiencyDefinition::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto Error = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };

    if (Name.IsEmptyOrWhitespace()) {
        Error(NSLOCTEXT("MythicProficiencyDefinition", "MissingName",
                        "A localized proficiency Name is required."));
    }
    if (!TrackTag.IsValid()) {
        Error(NSLOCTEXT("MythicProficiencyDefinition", "MissingTrackTag",
                        "A gameplay-semantic Track Tag is required."));
    }
    const UMythicStatDefinition *ProgressDefinition = ProgressStat.Asset.LoadSynchronous();
    const UMythicStatDefinition *CapacityDefinition = ProgressDefinition
        ? ProgressDefinition->PairedStat.Asset.LoadSynchronous() : nullptr;
    const bool bHasValidProgressPair =
        ProgressDefinition && ProgressStat.IsValid() && ProgressDefinition->Attribute.IsValid()
        && ProgressDefinition->PairRole == EMythicStatPairRole::Current
        && CapacityDefinition && CapacityDefinition->Attribute.IsValid()
        && CapacityDefinition->PairRole == EMythicStatPairRole::Capacity
        && CapacityDefinition->PairedStat.GetAsset() == ProgressDefinition;
    if (!bHasValidProgressPair) {
        Error(NSLOCTEXT("MythicProficiencyDefinition", "InvalidProgressStat",
                        "Progress Stat must directly reference a canonical current-value Stat Definition with a reciprocal capacity pair."));
    }
    else if (ProgressDefinition->SheetVisibility != EMythicStatSheetVisibility::Hidden
             || CapacityDefinition->SheetVisibility != EMythicStatSheetVisibility::Hidden) {
        Error(NSLOCTEXT(
            "MythicProficiencyDefinition", "ProgressStatVisibleOnCharacterSheet",
            "Proficiency current and capacity Stat Definitions must both be Hidden from the character stat sheet; proficiency progression belongs on its dedicated progression UI."));
    }
    if (MaxLevel < 1) {
        Error(NSLOCTEXT("MythicProficiencyDefinition", "InvalidMaxLevel",
                        "Max Level must be at least one."));
    }
    if (!FMath::IsFinite(GrowthRate) || GrowthRate <= 0.0f) {
        Error(NSLOCTEXT("MythicProficiencyDefinition", "InvalidGrowthRate",
                        "Growth Rate must be finite and greater than zero."));
    }
    if (!FMath::IsFinite(BaseXPPerAction) || BaseXPPerAction <= 0.0f) {
        Error(NSLOCTEXT("MythicProficiencyDefinition", "InvalidBaseXP",
                        "Base XP Per Action must be finite and greater than zero."));
    }
    if (AttributeGoals.IsEmpty()) {
        Error(NSLOCTEXT("MythicProficiencyDefinition", "MissingAttributeGoals",
                        "At least one typed Attribute Goal is required."));
    }
    if (KeyMilestones.IsEmpty()) {
        Error(NSLOCTEXT("MythicProficiencyDefinition", "MissingMilestones",
                        "At least one Key Milestone is required."));
    }
    else if (KeyMilestones.Num() > MaxLevel) {
        Error(NSLOCTEXT("MythicProficiencyDefinition", "TooManyMilestones",
                        "Key Milestone count cannot exceed Max Level."));
    }
    if (!AttributeGoals.IsEmpty()
        && (AttributeGoals.Num() > MaxLevel || MaxLevel % AttributeGoals.Num() != 0)) {
        Error(NSLOCTEXT("MythicProficiencyDefinition", "UnevenAttributeGoalDistribution",
                        "Max Level must divide evenly by Attribute Goal count so each authored goal is granted exactly."));
    }

    TSet<FSoftObjectPath> SeenStatDefinitions;
    for (int32 GoalIndex = 0; GoalIndex < AttributeGoals.Num(); ++GoalIndex) {
        const FAttributeGoal &Goal = AttributeGoals[GoalIndex];
        const FSoftObjectPath StatPath = Goal.TargetStat.Asset.ToSoftObjectPath();
        const UMythicStatDefinition *Definition = Goal.TargetStat.Asset.LoadSynchronous();
        if (!Definition || !Goal.TargetStat.IsValid() || !Definition->Attribute.IsValid()) {
            Error(FText::Format(
                NSLOCTEXT("MythicProficiencyDefinition", "InvalidGoalTarget",
                          "Attribute Goal {0} must directly reference a registered Stat Definition with a valid GAS attribute."),
                FText::AsNumber(GoalIndex)));
        }
        else if (SeenStatDefinitions.Contains(StatPath)) {
            Error(FText::Format(
                NSLOCTEXT("MythicProficiencyDefinition", "DuplicateGoalTarget",
                          "Attribute Goal {0} duplicates another goal's Stat Definition."),
                FText::AsNumber(GoalIndex)));
        }
        else {
            SeenStatDefinitions.Add(StatPath);
        }

        const EGameplayModOp::Type Operation = Goal.Modifier.GetValue();
        if (!MythicAffix::IsSupportedModifierOp(Operation)) {
            Error(FText::Format(
                NSLOCTEXT("MythicProficiencyDefinition", "InvalidGoalModifier",
                          "Attribute Goal {0} uses an operation unsupported by the permanent-stat ledger."),
                FText::AsNumber(GoalIndex)));
        }
        if (!FMath::IsFinite(Goal.Goal)) {
            Error(FText::Format(
                NSLOCTEXT("MythicProficiencyDefinition", "NonFiniteGoal",
                          "Attribute Goal {0} must be finite."),
                FText::AsNumber(GoalIndex)));
        }
        else if (MythicAffix::ModifierRequiresNonZeroMagnitude(Operation)
                 && FMath::IsNearlyZero(Goal.Goal)) {
            Error(FText::Format(
                NSLOCTEXT("MythicProficiencyDefinition", "ZeroMultiplicativeGoal",
                          "Attribute Goal {0} requires a non-zero value for its multiplicative or divisive operation."),
                FText::AsNumber(GoalIndex)));
        }
    }

    return Result;
}

FString UProficiencyDefinition::GetProgressionBreakdown() const {
    if (GrowthRate > 1.4f) {
        UE_LOG(Myth, Warning, TEXT("High growth rate of %.2f may result in very high XP requirements for later levels."), GrowthRate);
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

    if (Proficiency.Track.Num() < MaxLevel) {
        return Breakdown + FString::Printf(
                   TEXT("\n[no track] This definition cannot generate a progression track yet.\n")
                   TEXT("  AttributeGoals : %d  (must be >= 1)\n")
                   TEXT("  KeyMilestones  : %d  (must be >= 1)\n")
                   TEXT("  MaxLevel       : %d  (must be >= 1)\n")
                   TEXT("Fill all three, then re-edit any property to see the breakdown.\n"),
                   AttributeGoals.Num(), KeyMilestones.Num(), MaxLevel);
    }

    auto TotalActions = 0;
    for (int32 Level = 1; Level <= MaxLevel; ++Level) {
        float XPCost;
        float CumulativeXP;
        if (Level < MaxLevel) {
            XPCost = CalcXPCostForLevelUp(Level, this);
            CumulativeXP = CalcCumulativeXPForLevel(Level + 1, this);
        }
        else {
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
        bool bWroteAnyReward = false;
        for (int32 RewardIdx = 0; RewardIdx < Milestone.Rewards.Num(); ++RewardIdx) {
            const URewardBase *RewardDef = Milestone.Rewards[RewardIdx];
            if (!RewardDef) {
                continue;
            }

            if (bWroteAnyReward) {
                MilestoneText += TEXT(", ");
            }

            const UAttributeReward *AttributeRwdDef = Cast<UAttributeReward>(RewardDef);
            if (AttributeRwdDef) {
                const UMythicStatDefinition *Stat = AttributeRwdDef->TargetStat.GetAsset();
                MilestoneText += FString::Printf(TEXT(" (%s %s%.0f)"),
                                                 Stat ? *Stat->DeveloperName.ToString() : TEXT("Invalid Stat"),
                                                 AttributeRwdDef->Modifier == EGameplayModOp::Additive ? TEXT("+") : TEXT("*"),
                                                 AttributeRwdDef->Magnitude);
            }
            else {
                MilestoneText += RewardDef->GetName();
            }
            bWroteAnyReward = true;
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
    UE_LOG(Myth, Log, TEXT("%s"), *GetProgressionBreakdown());
}

#endif
