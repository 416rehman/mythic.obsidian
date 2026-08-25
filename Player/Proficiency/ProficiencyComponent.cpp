

#include "ProficiencyComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Mythic.h"
#include "ProficiencyDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Rewards/AttributeReward.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/MythicTags_GAS.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Engine/World.h"

void FProficiency::GenerateTrack() {
    const int NumKeyMilestones = this->Definition->KeyMilestones.Num();
    const int NumGoals = this->Definition->AttributeGoals.Num();
    const int MaxLevel = this->Definition->MaxLevel;

    if (NumGoals <= 0 || NumKeyMilestones <= 0 || MaxLevel <= 0) {
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing required data"));
        return;
    }

    this->Track.SetNum(MaxLevel);

    const int GoalRewardSplitCount = FMath::Max(1, MaxLevel / NumGoals);

    const int MilestoneInterval = MaxLevel / NumKeyMilestones;
    int milestones_added = 0;

    for (int32 i = MaxLevel - 1; i >= 0; --i) {
        if (milestones_added < NumKeyMilestones) {
            const int milestone_level = MaxLevel - 1 - (milestones_added * MilestoneInterval);

            if (i == milestone_level) {
                this->Track[i] = this->Definition->KeyMilestones[NumKeyMilestones - milestones_added - 1];
                milestones_added++;
            }
        }

        const auto &ChosenGoal = this->Definition->AttributeGoals[i % NumGoals];

        UAttributeReward *AttributeReward = NewObject<UAttributeReward>();
        AttributeReward->Attribute = ChosenGoal.Attribute;
        AttributeReward->Modifier = ChosenGoal.Modifier;
        AttributeReward->Magnitude = ChosenGoal.Goal / GoalRewardSplitCount;

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

    auto Proficiency = this->Proficiencies.FindByPredicate([&OnAttributeChangeData](const FProficiency &Proficiency) {
        return Proficiency.ProgressAttribute == OnAttributeChangeData.Attribute;
    });
    if (!Proficiency) {
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing Proficiency"));
        return;
    }

    bool bClampToMax = false;
    if (Proficiency->MaxXP > 0.0f && NewValue > Proficiency->MaxXP) {
        UE_LOG(Myth, Log, TEXT("Proficiency %s: XP %f exceeds MaxXP %f. Clamping (after granting crossed-level rewards)."),
               *Proficiency->Definition->Name.ToString(), NewValue, Proficiency->MaxXP);
        NewValue = Proficiency->MaxXP;
        bClampToMax = true;
    }

    if (bIsRestoring) {
        if (bClampToMax) {
            ASC->SetNumericAttributeBase(Proficiency->ProgressAttribute, Proficiency->MaxXP);
        }
        return;
    }

    int32 OldLevel = UProficiencyDefinition::CalcLevelAtXP(OldValue, Proficiency->Definition);
    int32 NewLevel = UProficiencyDefinition::CalcLevelAtXP(NewValue, Proficiency->Definition);

    if (NewLevel <= OldLevel) {
        if (bClampToMax) {
            ASC->SetNumericAttributeBase(Proficiency->ProgressAttribute, Proficiency->MaxXP);
        }
        return;
    }

    APlayerController *Owner = Cast<APlayerController>(this->GetOwner());
    if (!Owner) {
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing Owner"));
        return;
    }

    auto Context = FRewardContext(Owner);

    FText MilestoneName;
    for (int32 Level = OldLevel; Level < NewLevel && Level < Proficiency->Track.Num(); ++Level) {
        auto &Milestone = Proficiency->Track[Level];
        UE_LOG(Myth, Log, TEXT("%s Proficiency Reward: Level %d: %s"),
               *Proficiency->Definition->Name.ToString(), Level + 1, *Milestone.Name.ToString());

        if (!Milestone.Name.IsEmpty()) {
            MilestoneName = Milestone.Name;
        }
        for (auto Reward : Milestone.Rewards) {
            if (Reward) {
                Reward->Give(Context);
            }
        }
    }

    UE_LOG(Myth, Log, TEXT("%s Proficiency: XP: %.0f -> %.0f (Level %d -> %d)"),
           *Proficiency->Definition->Name.ToString(), OldValue, NewValue, OldLevel, NewLevel);

    if (Owner->HasAuthority()) {
        if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(Owner)) {
            MythicPC->ClientNotifyProficiencyLevel(Proficiency->Definition->Name, NewLevel, MilestoneName);
        }
    }

    if (bClampToMax) {
        ASC->SetNumericAttributeBase(Proficiency->ProgressAttribute, Proficiency->MaxXP);
    }
}

void UProficiencyComponent::ConfigureProgressionAttribute(FProficiency &Proficiency) {
    auto Def = Proficiency.Definition;
    if (!Def) {
        return;
    }

    auto HasAttribute = ASC->HasAttributeSetForAttribute(Proficiency.ProgressAttribute);
    if (!HasAttribute) {
        UAttributeSet *AttributeSet = NewObject<UAttributeSet>(ASC->GetOwner(), Proficiency.ProgressAttribute.GetAttributeSetClass());
        ASC->AddSpawnedAttribute(AttributeSet);
        UE_LOG(Myth, Warning, TEXT("Proficiency: AttributeSet for Attribute %s granted because it wasn't"), *Proficiency.ProgressAttribute.AttributeName)
    }

    ASC->GetGameplayAttributeValueChangeDelegate(Proficiency.ProgressAttribute).RemoveAll(this);
    ASC->GetGameplayAttributeValueChangeDelegate(Proficiency.ProgressAttribute).AddUObject(this, &UProficiencyComponent::OnAttributeChanged);

    Proficiency.MaxXP = ceil(UProficiencyDefinition::CalcCumulativeXPForLevel(Def->MaxLevel, Def));

    // Seed the paired *Max attribute. OverallXpMax is a weighted aggregate of these, and an unseeded 0
    // leaves the whole account at level 1 forever: the header lies and the primary-growth GE grants
    // nothing, because both divide by it.
    const FString MaxAttrName = Proficiency.ProgressAttribute.AttributeName + TEXT("Max");
    if (const UClass *SetClass = Proficiency.ProgressAttribute.GetAttributeSetClass()) {
        for (TFieldIterator<FProperty> It(SetClass); It; ++It) {
            if (It->GetName() == MaxAttrName) {
                const FGameplayAttribute MaxAttr(*It);
                if (ASC->HasAttributeSetForAttribute(MaxAttr)) {
                    ASC->SetNumericAttributeBase(MaxAttr, Proficiency.MaxXP);
                }
                break;
            }
        }
    }

    UE_LOG(Myth, Log, TEXT("Proficiency: Bound to %s (MaxXP: %.1f)"), *Proficiency.ProgressAttribute.AttributeName, Proficiency.MaxXP);
}

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

    for (int32 Level = 1; Level < TargetLevel && Level < Proficiency.Track.Num(); ++Level) {
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

void UProficiencyComponent::BeginPlay() {
    Super::BeginPlay();

    ApplyLoadedProficiencies();
}

void UProficiencyComponent::ApplyLoadedProficiencies() {
    auto Owner = GetOwner();
    if (!Owner->HasAuthority()) {
        return;
    }

    IAbilitySystemInterface *OwnerASI = Cast<IAbilitySystemInterface>(Owner);
    checkf(OwnerASI, TEXT("The parent actor of the ProficiencyComponent must implement IAbilitySystemInterface"));
    ASC = OwnerASI->GetAbilitySystemComponent();
    checkf(ASC, TEXT("The parent actor of the ProficiencyComponent returned a null AbilitySystemComponent"));

    TArray<TPair<FProficiency *, int32>> ToReapply;
    TSet<FGameplayAttribute> ResetAttrs;

    for (auto &Proficiency : this->Proficiencies) {
        Proficiency.Instantiate();

        int32 Level = 0;
        if (Proficiency.SavedXP > 0.0f) {
            Level = UProficiencyDefinition::CalcLevelAtXP(Proficiency.SavedXP, Proficiency.Definition);

            UE_LOG(Myth, Log, TEXT("Proficiency %s: Restoring XP=%.1f, Level=%d"),
                   *Proficiency.Definition->Name.ToString(), Proficiency.SavedXP, Level);

            bIsRestoring = true;
        }

        ConfigureProgressionAttribute(Proficiency);

        if (Proficiency.MaxXP > 0.0f && Proficiency.SavedXP > Proficiency.MaxXP) {
            UE_LOG(Myth, Warning, TEXT("Proficiency %s: SavedXP %.1f exceeds MaxXP %.1f. Clamping."),
                   *Proficiency.Definition->Name.ToString(), Proficiency.SavedXP, Proficiency.MaxXP);
            Proficiency.SavedXP = Proficiency.MaxXP;
        }

        ASC->SetNumericAttributeBase(Proficiency.ProgressAttribute, Proficiency.SavedXP);

        for (int32 TrackLevel = 1; TrackLevel < Level && TrackLevel < Proficiency.Track.Num(); ++TrackLevel) {
            for (const auto &Reward : Proficiency.Track[TrackLevel].Rewards) {
                const UAttributeReward *AttrReward = Cast<UAttributeReward>(Reward);
                if (!AttrReward || !AttrReward->Attribute.IsValid()) {
                    continue;
                }
                ResetAttrs.Add(AttrReward->Attribute);
            }
        }

        if (Level > 0) {
            ToReapply.Emplace(&Proficiency, Level);
        }
    }

    if (ASC) {
        for (const FGameplayAttribute &Attr : ResetAttrs) {
            if (ASC->HasAttributeSetForAttribute(Attr)) {
                const UAttributeSet *CDO = Attr.GetAttributeSetClass()->GetDefaultObject<UAttributeSet>();
                ASC->SetNumericAttributeBase(Attr, Attr.GetNumericValue(CDO));
            }
        }
    }

    for (const TPair<FProficiency *, int32> &Entry : ToReapply) {
        ReapplyRewardsForLevel(*Entry.Key, Entry.Value);
    }

    bIsRestoring = false;
}

FProficiency* UProficiencyComponent::FindCombatProficiency() {
    const FGameplayAttribute CombatAttr = UMythicAttributeSet_Proficiencies::GetCombatProficiencyAttribute();
    for (auto &Proficiency : Proficiencies) {
        if (Proficiency.ProgressAttribute == CombatAttr) {
            return &Proficiency;
        }
    }
    return nullptr;
}

void UProficiencyComponent::GrantCombatXP(float Amount) {
    FProficiency *CombatProf = FindCombatProficiency();
    if (!CombatProf) {
        UE_LOG(Myth, Warning, TEXT("ProficiencyComponent: no combat proficiency configured, cannot grant XP"));
        return;
    }

    // Combat is a proficiency like any other. Granting it by hand skipped GAS.Event.Proficiency.Gained, so a
    // talent that rewards work saw every kind of work except killing.
    GrantProficiencyXPWithContext(CombatProf->Definition, Amount, FGameplayTagContainer());
}

void UProficiencyComponent::GrantProficiencyXP(UProficiencyDefinition *Definition, float Amount) {
    GrantProficiencyXPWithContext(Definition, Amount, FGameplayTagContainer());
}

void UProficiencyComponent::GrantProficiencyXPWithContext(UProficiencyDefinition *Definition, float Amount,
                                                          FGameplayTagContainer ContextTags) {
    if (!Definition || Amount <= 0.0f) {
        return;
    }
    if (!ASC || !GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }

    FProficiency *Prof = nullptr;
    for (auto &P : Proficiencies) {
        if (P.Definition == Definition) {
            Prof = &P;
            break;
        }
    }
    if (!Prof || !Prof->ProgressAttribute.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("ProficiencyComponent: no proficiency configured for %s, cannot grant XP"),
               *GetNameSafe(Definition));
        return;
    }

    const float Current = ASC->GetNumericAttributeBase(Prof->ProgressAttribute);
    ASC->SetNumericAttributeBase(Prof->ProgressAttribute, Current + Amount);

    UE_LOG(Myth, Log, TEXT("ProficiencyComponent: granted %.1f %s XP (%.1f -> %.1f)"),
           Amount, *GetNameSafe(Definition), Current, Current + Amount);

    {
        FGameplayEventData Payload;
        Payload.EventTag = GAS_EVENT_PROFICIENCY_GAINED;
        Payload.Instigator = ASC->GetAvatarActor();
        Payload.Target = ASC->GetAvatarActor();
        Payload.OptionalObject = Definition;
        Payload.EventMagnitude = Amount;
        if (Definition->TrackTag.IsValid()) {
            Payload.InstigatorTags.AddTag(Definition->TrackTag);
        }
        Payload.InstigatorTags.AppendTags(ContextTags);
        ASC->HandleGameplayEvent(GAS_EVENT_PROFICIENCY_GAINED, &Payload);
    }
}

float UProficiencyComponent::ComputeXpOverflow(float CurrentXP, float Amount, float MaxXP) {
    if (MaxXP <= 0.0f || Amount <= 0.0f) {
        return 0.0f;
    }

    return FMath::Clamp((CurrentXP + Amount) - MaxXP, 0.0f, Amount);
}

void UProficiencyComponent::ApplyDeathPenalty(float PenaltyFraction) {
    if (PenaltyFraction <= 0.0f) {
        return;
    }
    if (!ASC || !GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }

    FProficiency *CombatProf = FindCombatProficiency();
    if (!CombatProf || !CombatProf->ProgressAttribute.IsValid()) {
        return;
    }

    const float OldXP = ASC->GetNumericAttributeBase(CombatProf->ProgressAttribute);
    const int32 OldLevel = UProficiencyDefinition::CalcLevelAtXP(OldXP, CombatProf->Definition);
    const float LevelFloorXP = UProficiencyDefinition::CalcCumulativeXPForLevel(OldLevel, CombatProf->Definition);
    const float NewXP = ComputeXpAfterDeathPenalty(OldXP, PenaltyFraction, LevelFloorXP);
    ASC->SetNumericAttributeBase(CombatProf->ProgressAttribute, NewXP);

    UE_LOG(Myth, Log, TEXT("ProficiencyComponent: death penalty, combat XP %.1f -> %.1f (%.0f%% loss, level %d floor %.1f)"),
           OldXP, NewXP, PenaltyFraction * 100.0f, OldLevel, LevelFloorXP);
}

float UProficiencyComponent::ComputeXpAfterDeathPenalty(float CurrentXP, float PenaltyFraction) {
    return ComputeXpAfterDeathPenalty(CurrentXP, PenaltyFraction, 0.0f);
}

float UProficiencyComponent::ComputeXpAfterDeathPenalty(float CurrentXP, float PenaltyFraction, float LevelFloorXP) {
    const float Frac = FMath::Clamp(PenaltyFraction, 0.0f, 1.0f);
    const float AfterPenalty = FMath::Max(0.0f, CurrentXP * (1.0f - Frac));
    return FMath::Max(LevelFloorXP, AfterPenalty);
}

FProficiencySummary UProficiencyComponent::GetSummary(int32 Index) const {
    FProficiencySummary Summary;
    if (!Proficiencies.IsValidIndex(Index)) {
        return Summary;
    }

    const FProficiency &Prof = Proficiencies[Index];
    if (!Prof.Definition) {
        return Summary;
    }

    Summary.Name = Prof.Definition->Name;
    Summary.TrackTag = Prof.Definition->TrackTag;
    Summary.Description = Prof.Definition->Description;
    Summary.Icon = Prof.Definition->Icon;

    if (ASC) {
        Summary.CurrentXP = ASC->GetNumericAttributeBase(Prof.ProgressAttribute);
    } else {
        Summary.CurrentXP = Prof.SavedXP;
    }

    Summary.Level = UProficiencyDefinition::CalcLevelAtXP(Summary.CurrentXP, Prof.Definition);

    Summary.LevelXPStart = UProficiencyDefinition::CalcCumulativeXPForLevel(Summary.Level, Prof.Definition);
    Summary.LevelXPEnd = UProficiencyDefinition::CalcCumulativeXPForLevel(Summary.Level + 1, Prof.Definition);

    if (Summary.Level >= Prof.Definition->MaxLevel) {
        Summary.ProgressFraction = 1.0f;
    } else {
        float Cost = Summary.LevelXPEnd - Summary.LevelXPStart;
        if (Cost > 0.0f) {
            Summary.ProgressFraction = FMath::Clamp((Summary.CurrentXP - Summary.LevelXPStart) / Cost, 0.0f, 1.0f);
        } else {
            Summary.ProgressFraction = 0.0f;
        }
    }

    for (int32 i = Summary.Level; i < Prof.Track.Num(); ++i) {
        if (!Prof.Track[i].Name.IsEmpty()) {
            Summary.NextMilestoneName = Prof.Track[i].Name;
            Summary.NextMilestoneLevel = i + 1;
            break;
        }
    }

    return Summary;
}

void UProficiencyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UProficiencyComponent, Proficiencies, COND_OwnerOnly);
}
