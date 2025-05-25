// 


#include "ProficiencyComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Mythic.h"
#include "ProficiencyDefinition.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "Net/UnrealNetwork.h"
#include "Rewards/AttributeReward.h"

void FProficiency::GenerateTrack() {
    const int NumKeyMilestones = this->Definition->KeyMilestones.Num();
    const int NumGoals = this->Definition->AttributeGoals.Num();
    const int MaxLevel = this->Definition->MaxLevel;

    // Ensure there are milestones, goals, and max level defined
    if (NumGoals <= 0 || NumKeyMilestones <= 0 || MaxLevel <= 0) {
        UE_LOG(Mythic, Error, TEXT("Proficiency: Missing required data"));
        return;
    }

    // Initialize proficiency track with MaxLevel elements
    this->Track.SetNum(MaxLevel);

    // Rewards per goal
    const int GoalRewardSplitCount = MaxLevel / NumGoals;

    // Milestone interval (how often key milestones appear)
    const int MilestoneInterval = MaxLevel / NumKeyMilestones;
    int milestones_added = 0;

    // Reverse loop to ensure the last level always has a key milestone
    for (int32 i = MaxLevel - 1; i >= 0; --i) {
        // Calculate the level for the current milestone
        if (milestones_added < NumKeyMilestones) {
            const int milestone_level = MaxLevel - 1 - (milestones_added * MilestoneInterval);

            // If the current level matches the milestone level, set the key milestone
            if (i == milestone_level) {
                this->Track[i] = this->Definition->KeyMilestones[NumKeyMilestones - milestones_added - 1];
                milestones_added++;
            }
        }

        // Distribute rewards for each goal
        const auto &ChosenGoal = this->Definition->AttributeGoals[i % NumGoals];

        // Create the attribute reward for this level
        UAttributeReward *AttributeReward = NewObject<UAttributeReward>();
        AttributeReward->Attribute = ChosenGoal.Attribute;
        AttributeReward->Modifier = ChosenGoal.Modifier;
        AttributeReward->Magnitude = ChosenGoal.Goal / GoalRewardSplitCount;

        // Add the reward to the level's milestone if it doesn't already exist
        if (!this->Track[i].Rewards.Contains(AttributeReward)) {
            this->Track[i].Rewards.Add(AttributeReward);
        }
    }
}

void FProficiency::Instantiate() {
    if (!this->Definition) {
        UE_LOG(Mythic, Error, TEXT("Proficiency: Missing Definition"));
        return;
    }

    GenerateTrack();
}

void UProficiencyComponent::OnAttributeChanged(const FOnAttributeChangeData &OnAttributeChangeData) {
    auto NewValue = OnAttributeChangeData.NewValue;
    auto OldValue = OnAttributeChangeData.OldValue;
    auto AddedValue = NewValue - OldValue;

    // Get the Proficiency that depends on this progression attribute
    auto Proficiency = this->Proficiencies.FindByPredicate([&OnAttributeChangeData](const FProficiency &Proficiency) {
        return Proficiency.ProgressAttribute == OnAttributeChangeData.Attribute;
    });
    if (!Proficiency) {
        UE_LOG(Mythic, Error, TEXT("Proficiency: Missing Proficiency"));
        return;
    }

    // If the attribute changed to a value greater than the current level
    auto OldLevel = UProficiencyDefinition::CalcLevelAtXP(OldValue, Proficiency->Definition);
    auto NewLevel = UProficiencyDefinition::CalcLevelAtXP(NewValue, Proficiency->Definition);

    auto LevelsGained = NewLevel - OldLevel;
    if (LevelsGained <= 0) {
        UE_LOG(Mythic, Log, TEXT("Proficiency: No level gained"));
        return;
    }

    UE_LOG(Mythic, Log, TEXT("%s Proficiency: Leveled up %d levels"), *Proficiency->Definition->Name.ToString(), LevelsGained);

    // If the new level is greater than the current level, give rewards
    APlayerController *Owner = Cast<APlayerController>(this->GetOwner());
    if (!Owner) {
        UE_LOG(Mythic, Error, TEXT("Proficiency: Missing Owner"));
        return;
    }

    auto context = FRewardContext(Owner);
    for (int i = 0; i < LevelsGained; ++i) {
        auto Level = OldLevel + i;
        auto Milestone = Proficiency->Track[Level];
        UE_LOG(Mythic, Log, TEXT("%s Proficiency Reward: Level %d: %s"), *Proficiency->Definition->Name.ToString(), Level + 1, *Milestone.Name.ToString());
        for (auto Reward : Milestone.Rewards) {
            Reward->Give(context);
        }
    }

    UE_LOG(Mythic, Log, TEXT("%s Proficiency: Old XP: %f (Level %d) --(%f)--> New XP: %f (Level %d)"), *Proficiency->Definition->Name.ToString(), OldValue,
           OldLevel, AddedValue, NewValue, NewLevel);
}

// Sets the Maximum value for the progression attribute to be the max level's XP.
// This is done to ensure that the attribute is always at least the max level's XP.
// Starts listening for changes to the attribute for the proficiency's progression attribute, the listener function then gives rewards when the attribute changes.
void UProficiencyComponent::ConfigureProgressionAttribute(FProficiency &Proficiency) {
    auto Def = Proficiency.Definition;
    if (!Def) {
        return;
    }

    // Get the attribute set class from the attribute
    auto AttributeSetClass = Proficiency.ProgressAttribute.GetAttributeSetClass();
    if (!AttributeSetClass) {
        UE_LOG(Mythic, Error, TEXT("Proficiency: Progress Attribute is NOT from MythicAttributeSet_Proficiencies"));
        return;
    }

    // Get the attribute set from the ASC. Throw away the constness so we can "SetMaximumValue".
    auto AttributeSet = const_cast<UMythicAttributeSet_Proficiencies *>(Cast<UMythicAttributeSet_Proficiencies>(ASC->GetAttributeSet(AttributeSetClass)));
    if (!AttributeSet) {
        UE_LOG(Mythic, Error, TEXT("Proficiency: Progress Attribute is NOT from MythicAttributeSet_Proficiencies"));
        return;
    }

    // Set the maximum value for the attribute.

    auto MaxXP = ceil(UProficiencyDefinition::CalcCumulativeXPForLevel(Def->MaxLevel, Def));

    AttributeSet->SetMaximumValue(Proficiency.ProgressAttribute.AttributeName, MaxXP);

    // Register the attribute change callback

    ASC->GetGameplayAttributeValueChangeDelegate(Proficiency.ProgressAttribute).AddUObject(this, &UProficiencyComponent::OnAttributeChanged);
    UE_LOG(Mythic, Log, TEXT("Proficiency: Proficiency Bound to %s (Max XP: %f)"), *Proficiency.ProgressAttribute.AttributeName, MaxXP);
}

// Called when the game starts
void UProficiencyComponent::BeginPlay() {
    Super::BeginPlay();

    // if  not authority, we don't need to do anything
    auto Owner = GetOwner();
    if (!Owner->HasAuthority()) {
        return;
    }

    // Get ASC from owner using their ASC interface
    ASC = Cast<IAbilitySystemInterface>(GetOwner())->GetAbilitySystemComponent();
    checkf(ASC, TEXT("The parent actor of the ProficiencyComponent must implement IAbilitySystemInterface"));

    // For each proficiency, build the proficiency track
    for (auto &Proficiency : this->Proficiencies) {
        Proficiency.Instantiate();
        ConfigureProgressionAttribute(Proficiency);
    }
}

void UProficiencyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate the Proficiencies array after it has been generated
    DOREPLIFETIME_CONDITION(UProficiencyComponent, Proficiencies, COND_OwnerOnly);
}
