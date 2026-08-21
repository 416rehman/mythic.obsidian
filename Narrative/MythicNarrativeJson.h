
#pragma once

#include "CoreMinimal.h"

struct FMythicStoryConditionSpec {
    TArray<FString> RequireAll;
    TArray<FString> RequireAny;
    TArray<FString> BlockAny;

    bool operator==(const FMythicStoryConditionSpec &O) const {
        return RequireAll == O.RequireAll && RequireAny == O.RequireAny && BlockAny == O.BlockAny;
    }
    bool operator!=(const FMythicStoryConditionSpec &O) const { return !(*this == O); }
};

struct FMythicRewardsSpec {
    float XpPercentage = 0.0f;
    FString XpProficiency;
    FString ItemId;
    int32 ItemQuantity = 0;

    bool IsEmpty() const { return XpPercentage <= 0.0f && (ItemId.IsEmpty() || ItemQuantity <= 0); }

    bool operator==(const FMythicRewardsSpec &O) const {
        return XpPercentage == O.XpPercentage && XpProficiency == O.XpProficiency && ItemId == O.ItemId &&
               ItemQuantity == O.ItemQuantity;
    }
    bool operator!=(const FMythicRewardsSpec &O) const { return !(*this == O); }
};

struct FMythicBranchSpec {
    FString Outcome;
    TArray<FString> GrantFlags;
    TArray<FString> Next;
    TArray<FString> Cancel;

    bool operator==(const FMythicBranchSpec &O) const {
        return Outcome == O.Outcome && GrantFlags == O.GrantFlags && Next == O.Next && Cancel == O.Cancel;
    }
    bool operator!=(const FMythicBranchSpec &O) const { return !(*this == O); }
};

struct FMythicTaskSpec {
    FString Id;
    FString Display;
    FString TriggerTag;
    FString PayloadTag;
    int32 Count = 1;
    bool bOptional = false;
    FMythicStoryConditionSpec Precondition;
    TArray<FString> GrantStoryTags;
    TArray<FMythicBranchSpec> Branches;
    TArray<FString> Next;

    bool operator==(const FMythicTaskSpec &O) const {
        return Id == O.Id && Display == O.Display && TriggerTag == O.TriggerTag && PayloadTag == O.PayloadTag &&
               Count == O.Count && bOptional == O.bOptional && Precondition == O.Precondition &&
               GrantStoryTags == O.GrantStoryTags && Branches == O.Branches && Next == O.Next;
    }
    bool operator!=(const FMythicTaskSpec &O) const { return !(*this == O); }
};

struct FMythicOutcomeSpec {
    FString Outcome;
    FMythicStoryConditionSpec When;
    FMythicRewardsSpec Rewards;
    TArray<FString> GrantStoryTags;

    bool operator==(const FMythicOutcomeSpec &O) const {
        return Outcome == O.Outcome && When == O.When && Rewards == O.Rewards && GrantStoryTags == O.GrantStoryTags;
    }
    bool operator!=(const FMythicOutcomeSpec &O) const { return !(*this == O); }
};

struct FMythicQuestSpec {
    FString Id;
    FString Display;
    FMythicStoryConditionSpec UnlockCondition;
    TArray<FString> ExclusiveLockTags;
    bool bOptional = false;
    TArray<FMythicTaskSpec> Tasks;
    TArray<FMythicOutcomeSpec> Outcomes;
    FMythicRewardsSpec Rewards;
    TArray<FString> GrantStoryTags;

    bool operator==(const FMythicQuestSpec &O) const {
        return Id == O.Id && Display == O.Display && UnlockCondition == O.UnlockCondition &&
               ExclusiveLockTags == O.ExclusiveLockTags && bOptional == O.bOptional && Tasks == O.Tasks &&
               Outcomes == O.Outcomes && Rewards == O.Rewards && GrantStoryTags == O.GrantStoryTags;
    }
    bool operator!=(const FMythicQuestSpec &O) const { return !(*this == O); }
};

struct FMythicStorylineSpec {
    FString Id;
    FString Display;
    FString ArcTag;
    FMythicStoryConditionSpec ArcGate;
    TArray<FMythicQuestSpec> Quests;
    FMythicRewardsSpec Rewards;
    TArray<FString> GrantStoryTags;

    bool operator==(const FMythicStorylineSpec &O) const {
        return Id == O.Id && Display == O.Display && ArcTag == O.ArcTag && ArcGate == O.ArcGate && Quests == O.Quests &&
               Rewards == O.Rewards && GrantStoryTags == O.GrantStoryTags;
    }
    bool operator!=(const FMythicStorylineSpec &O) const { return !(*this == O); }
};

class MYTHIC_API FMythicNarrativeJson {
public:
    static bool ParseTaskSpec(const FString &Json, FMythicTaskSpec &Out);
    static FString SerializeTaskSpec(const FMythicTaskSpec &Spec);

    static bool ParseStorylineSpec(const FString &Json, FMythicStorylineSpec &Out);
    static FString SerializeStorylineSpec(const FMythicStorylineSpec &Spec);
};
