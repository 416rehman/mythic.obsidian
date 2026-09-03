// Copyright Stellar Games. All Rights Reserved.

#include "UI/ViewModels/MythicRuneDescriber.h"

#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Progression/MythicAchievementDefinition.h"
#include "Progression/MythicAchievementSet.h"
#include "Progression/MythicUnlockRule.h"
#include "Progression/MythicUnlockRuleSet.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "Progression/Runes/MythicRuneDefinition.h"

FString UMythicRuneDescriber::SocketRuleId(int32 SlotIndex) {
    return FString::Printf(TEXT("Unlock.Rule.RuneSlot%d"), SlotIndex + 1);
}

FText UMythicRuneDescriber::DescribeBehaviour(const UMythicRuneDefinition *Rune, const UMythicRuneComponent *Owner) {
    if (!Rune) {
        return FText::GetEmpty();
    }
    if (Rune->Parameters.Num() == 0 || Rune->Description.IsEmpty()) {
        return Rune->Description;
    }

    const FString Source = Rune->Description.ToString();
    FAbilityDefinition Definition;
    Definition.RichText = Rune->Description;
    TArray<FRolledTagSpec> Rolled;
    for (const TPair<FGameplayTag, FRollDefinition> &Param : Rune->Parameters) {
        // GetRichText refuses a roll the text never mentions, so only the parameters the text draws go in.
        if (!Param.Key.IsValid() || !Source.Contains(FString::Printf(TEXT("<#%s>"), *Param.Key.ToString()))) {
            continue;
        }
        Definition.ParameterRolls.Add(Param.Key, Param.Value);
        float Value = 0.0f;
        if (!Owner || !Owner->GetRolledRuneValue(Rune, Param.Key, Value)) {
            Value = Rune->GetParameterMidpoint(Param.Key, 0.0f);
        }
        Rolled.Emplace(Param.Key, Value);
    }

    FText Drawn;
    if (Rolled.Num() == 0 || !Definition.GetRichText(Drawn, Rolled)) {
        return Rune->Description;
    }
    return Drawn;
}

FText UMythicRuneDescriber::DescribeSealedSocket(int32 SlotIndex, const UMythicUnlockRuleSet *Rules,
                                                 const UMythicAchievementSet *Achievements, FText &OutDeedName) {
    OutDeedName = FText::GetEmpty();
    const FString RuleId = SocketRuleId(SlotIndex);

    const UMythicUnlockRule *Rule = nullptr;
    if (Rules) {
        for (const UMythicUnlockRule *Candidate : Rules->Rules) {
            if (Candidate && Candidate->RuleId.IsValid() && Candidate->RuleId.ToString() == RuleId) {
                Rule = Candidate;
                break;
            }
        }
    }
    if (!Rule) {
        OutDeedName = FText::FromString(RuleId);
        return FText::Format(NSLOCTEXT("Mythic", "RuneSocketSealedNoRule", "Sealed - earn {0}"), OutDeedName);
    }

    TArray<FGameplayTag> Wanted;
    Rule->Precondition.RequireAll.GetGameplayTagArray(Wanted);
    TArray<FGameplayTag> Any;
    Rule->Precondition.RequireAny.GetGameplayTagArray(Any);
    Wanted.Append(Any);
    if (Wanted.Num() == 0) {
        OutDeedName = FText::FromString(RuleId);
        return FText::Format(NSLOCTEXT("Mythic", "RuneSocketSealedNoRule", "Sealed - earn {0}"), OutDeedName);
    }

    TArray<FText> Names;
    TArray<FText> Lines;
    for (const FGameplayTag &Tag : Wanted) {
        const UMythicAchievementDefinition *Deed = nullptr;
        if (Achievements) {
            for (const UMythicAchievementDefinition *Candidate : Achievements->Achievements) {
                if (Candidate && Candidate->AchievementTag == Tag) {
                    Deed = Candidate;
                    break;
                }
            }
        }
        const FText Name = (Deed && !Deed->DisplayName.IsEmpty()) ? Deed->DisplayName : FText::FromString(Tag.ToString());
        const FText Description = Deed ? Deed->Description : FText::GetEmpty();
        Names.Add(Name);
        Lines.Add(Description.IsEmpty()
                      ? FText::Format(NSLOCTEXT("Mythic", "RuneSocketSealedNoRule", "Sealed - earn {0}"), Name)
                      : FText::Format(NSLOCTEXT("Mythic", "RuneSocketSealed", "Sealed - earn {0}: {1}"), Name,
                                      Description));
    }
    OutDeedName = FText::Join(FText::FromString(TEXT(", ")), Names);
    return FText::Join(FText::FromString(TEXT("\n")), Lines);
}
