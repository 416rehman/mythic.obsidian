// 


#include "ProficiencyComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Mythic.h"
#include "ProficiencyDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Rewards/AttributeReward.h"

void FProficiency::GenerateTrack() {
    const int NumKeyMilestones = this->Definition->KeyMilestones.Num();
    const int NumGoals = this->Definition->AttributeGoals.Num();
    const int MaxLevel = this->Definition->MaxLevel;

    // Ensure there are milestones, goals, and max level defined
    if (NumGoals <= 0 || NumKeyMilestones <= 0 || MaxLevel <= 0) {
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing required data"));
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
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing Definition"));
        return;
    }

    GenerateTrack();
}

void UProficiencyComponent::OnAttributeChanged(const FOnAttributeChangeData &OnAttributeChangeData) {
    auto NewValue = OnAttributeChangeData.NewValue;
    auto OldValue = OnAttributeChangeData.OldValue;

    // Get the Proficiency that depends on this progression attribute
    auto Proficiency = this->Proficiencies.FindByPredicate([&OnAttributeChangeData](const FProficiency &Proficiency) {
        return Proficiency.ProgressAttribute == OnAttributeChangeData.Attribute;
    });
    if (!Proficiency) {
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing Proficiency"));
        return;
    }

    // if XP exceeds MaxXP, clamp it and return (the clamp will trigger a new callback with correct value)
    if (Proficiency->MaxXP > 0.0f && NewValue > Proficiency->MaxXP) {
        UE_LOG(Myth, Log, TEXT("Proficiency %s: XP %f exceeds MaxXP %f. Clamping."),
               *Proficiency->Definition->Name.ToString(), NewValue, Proficiency->MaxXP);

        // use the base setter to force the value back down
        ASC->SetNumericAttributeBase(Proficiency->ProgressAttribute, Proficiency->MaxXP);
        return;
    }

    // Skip during restore - we handle rewards separately
    if (bIsRestoring) {
        return;
    }

    // Calculate levels from XP
    int32 OldLevel = UProficiencyDefinition::CalcLevelAtXP(OldValue, Proficiency->Definition);
    int32 NewLevel = UProficiencyDefinition::CalcLevelAtXP(NewValue, Proficiency->Definition);

    // No level change - nothing to do
    if (NewLevel <= OldLevel) {
        return;
    }

    APlayerController *Owner = Cast<APlayerController>(this->GetOwner());
    if (!Owner) {
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing Owner"));
        return;
    }

    auto Context = FRewardContext(Owner);

    // Give rewards for NEW levels only (OldLevel to NewLevel-1)
    for (int32 Level = OldLevel; Level < NewLevel && Level < Proficiency->Track.Num(); ++Level) {
        auto &Milestone = Proficiency->Track[Level];
        UE_LOG(Myth, Log, TEXT("%s Proficiency Reward: Level %d: %s"),
               *Proficiency->Definition->Name.ToString(), Level + 1, *Milestone.Name.ToString());

        for (auto Reward : Milestone.Rewards) {
            if (Reward) {
                Reward->Give(Context);
            }
        }
    }

    UE_LOG(Myth, Log, TEXT("%s Proficiency: XP: %.0f -> %.0f (Level %d -> %d)"),
           *Proficiency->Definition->Name.ToString(), OldValue, NewValue, OldLevel, NewLevel);
}

// Ensures the progression attribute set is spawned and binds the change delegate.
void UProficiencyComponent::ConfigureProgressionAttribute(FProficiency &Proficiency) {
    auto Def = Proficiency.Definition;
    if (!Def) {
        return;
    }

    // Get the attribute set class from the attribute
    auto HasAttribute = ASC->HasAttributeSetForAttribute(Proficiency.ProgressAttribute);
    if (!HasAttribute) {
        auto AttributeSet = Proficiency.ProgressAttribute.GetAttributeSetClass()->GetDefaultObject<UAttributeSet>();
        ASC->AddSpawnedAttribute(AttributeSet);
        UE_LOG(Myth, Warning, TEXT("Proficiency: AttributeSet for Attribute %s granted because it wasn't"), *Proficiency.ProgressAttribute.AttributeName)
    }

    // Register the attribute change callback
    ASC->GetGameplayAttributeValueChangeDelegate(Proficiency.ProgressAttribute).AddUObject(this, &UProficiencyComponent::OnAttributeChanged);

    // Cache the MaxXP for clamping logic
    Proficiency.MaxXP = ceil(UProficiencyDefinition::CalcCumulativeXPForLevel(Def->MaxLevel, Def));

    UE_LOG(Myth, Log, TEXT("Proficiency: Bound to %s (MaxXP: %.1f)"), *Proficiency.ProgressAttribute.AttributeName, Proficiency.MaxXP);
}

// Reapply only CanReapplyOnLoad rewards (attributes, abilities) for levels up to given level
void UProficiencyComponent::ReapplyRewardsForLevel(FProficiency &Proficiency, int32 TargetLevel) {
    if (!Proficiency.Definition) {
        return;
    }

    APlayerController *Owner = Cast<APlayerController>(GetOwner());
    if (!Owner) {
        return;
    }

    auto Context = FRewardContext(Owner);
    int32 ReappliedCount = 0;

    for (int32 Level = 0; Level < TargetLevel && Level < Proficiency.Track.Num(); ++Level) {
        auto &Milestone = Proficiency.Track[Level];
        for (auto Reward : Milestone.Rewards) {
            if (Reward && Reward->CanReapplyOnLoad()) {
                Reward->Give(Context);
                ReappliedCount++;
            }
        }
    }

    UE_LOG(Myth, Log, TEXT("Proficiency %s: Reapplied %d rewards for %d levels"),
           *Proficiency.Definition->Name.ToString(), ReappliedCount, TargetLevel);
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

        // If loaded from save, restore XP
        int32 Level = 0;
        if (Proficiency.SavedXP > 0.0f) {
            // Calculate level from saved XP
            Level = UProficiencyDefinition::CalcLevelAtXP(Proficiency.SavedXP, Proficiency.Definition);

            UE_LOG(Myth, Log, TEXT("Proficiency %s: Restoring XP=%.1f, Level=%d"),
                   *Proficiency.Definition->Name.ToString(), Proficiency.SavedXP, Level);

            // Set restoring flag to skip OnAttributeChanged rewards
            bIsRestoring = true;
        }

        // Configure attribute - may trigger callback but bIsRestoring skips it
        ConfigureProgressionAttribute(Proficiency);

        // Clamp SavedXP if it exceeds MaxXP (e.g. if definitions changed)
        if (Proficiency.MaxXP > 0.0f && Proficiency.SavedXP > Proficiency.MaxXP) {
            UE_LOG(Myth, Warning, TEXT("Proficiency %s: SavedXP %.1f exceeds MaxXP %.1f. Clamping."),
                   *Proficiency.Definition->Name.ToString(), Proficiency.SavedXP, Proficiency.MaxXP);
            Proficiency.SavedXP = Proficiency.MaxXP;
        }

        ASC->SetNumericAttributeBase(Proficiency.ProgressAttribute, Proficiency.SavedXP);

        // Reapply only CanReapplyOnLoad rewards (items already in inventory)
        ReapplyRewardsForLevel(Proficiency, Level);

        bIsRestoring = false;
    }
}

void UProficiencyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate the Proficiencies array after it has been generated
    DOREPLIFETIME_CONDITION(UProficiencyComponent, Proficiencies, COND_OwnerOnly);
}
