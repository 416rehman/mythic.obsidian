

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
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Itemization/Affixes/MythicAffixApplicationComponent.h"
#include "Itemization/Affixes/MythicAffixRng.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Stats/MythicStatDefinition.h"
#include "UObject/UObjectGlobals.h"

namespace {
enum class EProficiencyRewardSourceKind : uint8 {
    AuthoredMilestone = 1,
    GeneratedAttributeGoal = 2
};

FGuid MakeProficiencyRewardSourceGuid(const UProficiencyDefinition &Definition,
                                      const EProficiencyRewardSourceKind SourceKind,
                                      const int32 PlacementIndex,
                                      const int32 RewardSlot) {
    const FPrimaryAssetId DefinitionId = Definition.GetPrimaryAssetId();
    if (!DefinitionId.IsValid() || PlacementIndex < 0 || RewardSlot < 0) {
        return FGuid();
    }

    // A source is the typed authored placement, not its current balance payload or derived final reward-array index.
    // Designers can retune a reward or move a key milestone through MaxLevel balancing without orphaning the prior
    // permanent-ledger source. The explicit kind prevents authored and generated placement coordinates from aliasing.
    FMythicAffixCanonicalWriter Writer("MYTHIC_PROFICIENCY_REWARD_SOURCE_PAYLOAD_V3");
    Writer.AddPrimaryAssetId(DefinitionId);
    Writer.AddUInt8(static_cast<uint8>(SourceKind));
    Writer.AddInt32(PlacementIndex);
    Writer.AddInt32(RewardSlot);
    return Writer.IsValid()
        ? FMythicAffixRngFactory::GuidFromCanonicalBytes(
              "MYTHIC_PROFICIENCY_REWARD_SOURCE_V3", Writer.GetBytes())
        : FGuid();
}

URewardBase *MaterializeProficiencyReward(const UProficiencyDefinition &Definition,
                                          const int32 AuthoredMilestoneIndex,
                                          const int32 RewardSlot,
                                          const URewardBase &AuthoredReward) {
    UObject *RuntimeOuter = GetTransientPackageAsObject();
    const FName RuntimeName = MakeUniqueObjectName(
        RuntimeOuter, AuthoredReward.GetClass(), FName(TEXT("ProficiencyReward")));
    URewardBase *RuntimeReward = DuplicateObject<URewardBase>(
        &AuthoredReward, RuntimeOuter, RuntimeName);
    if (!RuntimeReward) {
        return nullptr;
    }

    RuntimeReward->SetFlags(RF_Transient);
    RuntimeReward->ClearFlags(RF_Public | RF_Standalone);
    if (UAttributeReward *AttributeReward = Cast<UAttributeReward>(RuntimeReward)) {
        AttributeReward->PermanentSourceGuid = MakeProficiencyRewardSourceGuid(
            Definition, EProficiencyRewardSourceKind::AuthoredMilestone,
            AuthoredMilestoneIndex, RewardSlot);
        if (!AttributeReward->PermanentSourceGuid.IsValid()) {
            return nullptr;
        }
    }
    return RuntimeReward;
}

bool MaterializeProficiencyMilestone(const UProficiencyDefinition &Definition,
                                     const int32 TrackIndex,
                                     const int32 AuthoredMilestoneIndex,
                                     const FMilestone &AuthoredMilestone,
                                     FMilestone &OutMilestone) {
    FMilestone CompiledMilestone;
    CompiledMilestone.Icon = AuthoredMilestone.Icon;
    CompiledMilestone.Name = AuthoredMilestone.Name;
    CompiledMilestone.Rewards.Reserve(AuthoredMilestone.Rewards.Num());

    for (int32 RewardSlot = 0; RewardSlot < AuthoredMilestone.Rewards.Num(); ++RewardSlot) {
        const URewardBase *AuthoredReward = AuthoredMilestone.Rewards[RewardSlot];
        if (!AuthoredReward) {
            UE_LOG(Myth, Error,
                   TEXT("Proficiency %s contains a null authored reward at track index %d, reward slot %d."),
                   *GetNameSafe(&Definition), TrackIndex, RewardSlot);
            return false;
        }

        URewardBase *RuntimeReward = MaterializeProficiencyReward(
            Definition, AuthoredMilestoneIndex, RewardSlot, *AuthoredReward);
        if (!RuntimeReward) {
            UE_LOG(Myth, Error,
                   TEXT("Proficiency %s could not materialize track index %d, reward slot %d."),
                   *GetNameSafe(&Definition), TrackIndex, RewardSlot);
            return false;
        }
        CompiledMilestone.Rewards.Add(RuntimeReward);
    }

    OutMilestone = MoveTemp(CompiledMilestone);
    return true;
}
}

void FProficiency::GenerateTrack() {
    // Compiled rewards are disposable derivations. Any invalid rebuild must leave no stale track or definition-owned
    // reward references behind for save restore to consume.
    this->Track.Reset();
    if (!this->Definition) {
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing Definition"));
        return;
    }

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
                const int32 AuthoredMilestoneIndex =
                    NumKeyMilestones - milestones_added - 1;
                const FMilestone &AuthoredMilestone =
                    this->Definition->KeyMilestones[AuthoredMilestoneIndex];
                if (!MaterializeProficiencyMilestone(
                        *this->Definition, i, AuthoredMilestoneIndex,
                        AuthoredMilestone, this->Track[i])) {
                    this->Track.Reset();
                    return;
                }
                milestones_added++;
            }
        }

        const auto &ChosenGoal = this->Definition->AttributeGoals[i % NumGoals];

        constexpr int32 GeneratedRewardOrdinal = 0;
        UAttributeReward *AttributeReward = NewObject<UAttributeReward>(
            GetTransientPackageAsObject(), NAME_None, RF_Transient);
        AttributeReward->PermanentSourceGuid = MakeProficiencyRewardSourceGuid(
            *this->Definition, EProficiencyRewardSourceKind::GeneratedAttributeGoal,
            i, GeneratedRewardOrdinal);
        AttributeReward->TargetStat = ChosenGoal.TargetStat;
        AttributeReward->Modifier = ChosenGoal.Modifier;
        AttributeReward->Magnitude = ChosenGoal.Goal / GoalRewardSplitCount;

        if (!AttributeReward->PermanentSourceGuid.IsValid()) {
            UE_LOG(Myth, Error,
                   TEXT("Proficiency %s could not derive a permanent reward source identity for track index %d."),
                   *GetNameSafe(this->Definition), i);
            this->Track.Reset();
            return;
        }
        this->Track[i].Rewards.Add(AttributeReward);
    }
}

void FProficiency::Instantiate() {
    if (!this->Definition) {
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing Definition"));
        return;
    }

    GenerateTrack();
}

FGameplayAttribute FProficiency::GetProgressAttribute() const {
    return Definition ? Definition->GetProgressAttribute() : FGameplayAttribute();
}

FGameplayAttribute FProficiency::GetProgressCapacityAttribute() const {
    return Definition ? Definition->GetProgressCapacityAttribute() : FGameplayAttribute();
}

void UProficiencyComponent::OnAttributeChanged(const FOnAttributeChangeData &OnAttributeChangeData) {
    auto NewValue = OnAttributeChangeData.NewValue;
    auto OldValue = OnAttributeChangeData.OldValue;

    auto Proficiency = this->Proficiencies.FindByPredicate([&OnAttributeChangeData](const FProficiency &Proficiency) {
        return Proficiency.GetProgressAttribute() == OnAttributeChangeData.Attribute;
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
            ASC->SetNumericAttributeBase(Proficiency->GetProgressAttribute(), Proficiency->MaxXP);
        }
        return;
    }

    int32 OldLevel = UProficiencyDefinition::CalcLevelAtXP(OldValue, Proficiency->Definition);
    int32 NewLevel = UProficiencyDefinition::CalcLevelAtXP(NewValue, Proficiency->Definition);

    if (NewLevel <= OldLevel) {
        if (bClampToMax) {
            ASC->SetNumericAttributeBase(Proficiency->GetProgressAttribute(), Proficiency->MaxXP);
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
        ASC->SetNumericAttributeBase(Proficiency->GetProgressAttribute(), Proficiency->MaxXP);
    }
}

bool UProficiencyComponent::ConfigureProgressionAttribute(FProficiency &Proficiency) {
    UProficiencyDefinition *Def = Proficiency.Definition;
    UGameInstance *GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    const UMythicItemizationDataRegistrySubsystem *Registry = GameInstance
        ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
    const UMythicStatDefinition *ProgressStat = Def ? Def->GetProgressStatDefinition() : nullptr;
    const UMythicStatDefinition *CapacityStat = ProgressStat
        ? ProgressStat->PairedStat.GetAsset() : nullptr;
    const FGameplayAttribute ProgressAttribute = ProgressStat
        ? ProgressStat->Attribute : FGameplayAttribute();
    const FGameplayAttribute CapacityAttribute = CapacityStat
        ? CapacityStat->Attribute : FGameplayAttribute();
    if (!Def || !Registry || !Registry->IsCoreSemanticReady() || !ProgressStat || !CapacityStat
        || Registry->FindStat(ProgressStat->GetPrimaryAssetId()) != ProgressStat
        || Registry->FindStat(CapacityStat->GetPrimaryAssetId()) != CapacityStat
        || ProgressStat->PairRole != EMythicStatPairRole::Current
        || CapacityStat->PairRole != EMythicStatPairRole::Capacity
        || CapacityStat->PairedStat.GetAsset() != ProgressStat
        || !ProgressAttribute.IsValid() || !CapacityAttribute.IsValid()
        || !ASC->HasAttributeSetForAttribute(ProgressAttribute)
        || !ASC->HasAttributeSetForAttribute(CapacityAttribute)) {
        UE_LOG(Myth, Error,
               TEXT("Proficiency '%s' rejected an unavailable, unregistered, unpaired, or uninstalled Progress Stat."),
               *GetNameSafe(Def));
        return false;
    }

    ASC->GetGameplayAttributeValueChangeDelegate(ProgressAttribute).RemoveAll(this);
    ASC->GetGameplayAttributeValueChangeDelegate(ProgressAttribute).AddUObject(
        this, &UProficiencyComponent::OnAttributeChanged);

    Proficiency.MaxXP = ceil(UProficiencyDefinition::CalcCumulativeXPForLevel(Def->MaxLevel, Def));

    // Seed the explicitly paired capacity stat. Overall XP and primary growth consume this GAS value.
    ASC->SetNumericAttributeBase(CapacityAttribute, Proficiency.MaxXP);

    UE_LOG(Myth, Log, TEXT("Proficiency: Bound to %s / %s (MaxXP: %.1f)"),
           *ProgressAttribute.GetName(), *CapacityAttribute.GetName(), Proficiency.MaxXP);
    return true;
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
            // Attribute rewards are restored as one source-addressed ledger set before any other reward replays.
            if (Reward && !Reward->IsA<UAttributeReward>() && Reward->CanReapplyOnLoad()) {
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

    UGameInstance *GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UMythicItemizationDataRegistrySubsystem *Registry = GameInstance
        ? GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>() : nullptr;
    if (!Registry || !Registry->IsCoreSemanticReady()) {
        if (Registry && !bSemanticDataRequestPending) {
            bSemanticDataRequestPending = true;
            TWeakObjectPtr<UProficiencyComponent> WeakThis(this);
            Registry->RequestCoreSemanticDataAsync(FOnMythicItemizationDataReady::CreateLambda(
                [WeakThis](const bool bSuccess) {
                    if (!WeakThis.IsValid()) return;
                    WeakThis->bSemanticDataRequestPending = false;
                    if (bSuccess) {
                        WeakThis->ApplyLoadedProficiencies();
                    }
                    else {
                        UE_LOG(Myth, Error,
                               TEXT("Proficiency initialization failed because canonical Stat Definitions did not become ready."));
                    }
                }));
        }
        return;
    }

    UMythicAffixApplicationComponent *Application = nullptr;
    if (AActor *OwnerActor = ASC->GetOwnerActor()) {
        Application = OwnerActor->FindComponentByClass<UMythicAffixApplicationComponent>();
    }
    if (!Application) {
        if (AActor *AvatarActor = ASC->GetAvatarActor()) {
            Application = AvatarActor->FindComponentByClass<UMythicAffixApplicationComponent>();
        }
    }
    if (!Application) {
        UE_LOG(Myth, Error,
               TEXT("Proficiency initialization requires the authoritative permanent-stat application component."));
        return;
    }

    // Reject the complete roster before binding delegates or writing any GAS base. The roster is a semantic graph,
    // so duplicate definitions/tags/progress stats are a whole-graph failure rather than partially usable content.
    TSet<const UProficiencyDefinition *> SeenDefinitions;
    TSet<FGameplayTag> SeenTrackTags;
    TSet<const UMythicStatDefinition *> SeenProgressStats;
    for (const FProficiency &Proficiency : Proficiencies) {
        const UProficiencyDefinition *Definition = Proficiency.Definition;
        const UMythicStatDefinition *ProgressStat = Definition
            ? Definition->GetProgressStatDefinition() : nullptr;
        const UMythicStatDefinition *CapacityStat = ProgressStat
            ? ProgressStat->PairedStat.GetAsset() : nullptr;
        const FGameplayAttribute ProgressAttribute = ProgressStat
            ? ProgressStat->Attribute : FGameplayAttribute();
        const FGameplayAttribute CapacityAttribute = CapacityStat
            ? CapacityStat->Attribute : FGameplayAttribute();
        if (!Definition || SeenDefinitions.Contains(Definition)
            || !Definition->TrackTag.IsValid() || SeenTrackTags.Contains(Definition->TrackTag)
            || !ProgressStat || SeenProgressStats.Contains(ProgressStat) || !CapacityStat
            || Registry->FindStat(ProgressStat->GetPrimaryAssetId()) != ProgressStat
            || Registry->FindStat(CapacityStat->GetPrimaryAssetId()) != CapacityStat
            || ProgressStat->PairRole != EMythicStatPairRole::Current
            || CapacityStat->PairRole != EMythicStatPairRole::Capacity
            || CapacityStat->PairedStat.GetAsset() != ProgressStat
            || !ProgressAttribute.IsValid() || !CapacityAttribute.IsValid()
            || !ASC->HasAttributeSetForAttribute(ProgressAttribute)
            || !ASC->HasAttributeSetForAttribute(CapacityAttribute)
            || !FMath::IsFinite(Proficiency.SavedXP) || Proficiency.SavedXP < 0.0f) {
            UE_LOG(Myth, Error,
                   TEXT("Proficiency initialization rejected the complete roster at %s."),
                   *GetNameSafe(Definition));
            return;
        }
        SeenDefinitions.Add(Definition);
        SeenTrackTags.Add(Definition->TrackTag);
        SeenProgressStats.Add(ProgressStat);
    }

    TGuardValue<bool> RestoringGuard(bIsRestoring, true);
    TArray<TPair<FProficiency *, int32>> ToReapply;
    TArray<FGuid> OwnedPermanentSourceGuids;
    TArray<FMythicPermanentStatSourceSpec> DesiredPermanentSources;
    TSet<FGuid> SeenPermanentSourceGuids;

    for (FProficiency &Proficiency : Proficiencies) {
        Proficiency.Instantiate();
        if (Proficiency.Track.Num() != Proficiency.Definition->MaxLevel
            || !ConfigureProgressionAttribute(Proficiency)) {
            UE_LOG(Myth, Error,
                   TEXT("Proficiency initialization failed closed while compiling %s."),
                   *GetNameSafe(Proficiency.Definition));
            return;
        }

        if (Proficiency.MaxXP > 0.0f && Proficiency.SavedXP > Proficiency.MaxXP) {
            UE_LOG(Myth, Warning, TEXT("Proficiency %s: SavedXP %.1f exceeds MaxXP %.1f. Clamping."),
                   *Proficiency.Definition->Name.ToString(), Proficiency.SavedXP, Proficiency.MaxXP);
            Proficiency.SavedXP = Proficiency.MaxXP;
        }

        const int32 Level = UProficiencyDefinition::CalcLevelAtXP(
            Proficiency.SavedXP, Proficiency.Definition);
        UE_LOG(Myth, Log, TEXT("Proficiency %s: Restoring XP=%.1f, Level=%d"),
               *Proficiency.Definition->Name.ToString(), Proficiency.SavedXP, Level);
        ASC->SetNumericAttributeBase(Proficiency.GetProgressAttribute(), Proficiency.SavedXP);

        for (int32 TrackLevel = 0; TrackLevel < Proficiency.Track.Num(); ++TrackLevel) {
            for (const auto &Reward : Proficiency.Track[TrackLevel].Rewards) {
                const UAttributeReward *AttrReward = Cast<UAttributeReward>(Reward);
                if (!AttrReward) {
                    continue;
                }
                if (!AttrReward->PermanentSourceGuid.IsValid()
                    || SeenPermanentSourceGuids.Contains(AttrReward->PermanentSourceGuid)) {
                    UE_LOG(Myth, Error,
                           TEXT("Proficiency reward source identity is invalid or duplicated on %s."),
                           *GetNameSafe(Proficiency.Definition));
                    return;
                }
                SeenPermanentSourceGuids.Add(AttrReward->PermanentSourceGuid);
                OwnedPermanentSourceGuids.Add(AttrReward->PermanentSourceGuid);
                if (TrackLevel >= 1 && TrackLevel < Level) {
                    DesiredPermanentSources.Add(FMythicPermanentStatSourceSpec{
                        AttrReward->PermanentSourceGuid, AttrReward->TargetStat,
                        AttrReward->Modifier, AttrReward->Magnitude});
                }
            }
        }

        if (Level > 0) {
            ToReapply.Emplace(&Proficiency, Level);
        }
    }

    if (!Application->ReplacePermanentStatSourceSetTransactional(
            OwnedPermanentSourceGuids, DesiredPermanentSources)) {
        UE_LOG(Myth, Error,
               TEXT("Proficiency reward restore failed closed because its complete typed source set was rejected."));
        return;
    }

    for (const TPair<FProficiency *, int32> &Entry : ToReapply) {
        ReapplyRewardsForLevel(*Entry.Key, Entry.Value);
    }
}

FProficiency* UProficiencyComponent::FindCombatProficiency() {
    const FGameplayAttribute CombatAttr = UMythicAttributeSet_Proficiencies::GetCombatProficiencyAttribute();
    for (auto &Proficiency : Proficiencies) {
        if (Proficiency.GetProgressAttribute() == CombatAttr) {
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
    TryGrantProficiencyXPWithContext(Definition, Amount, ContextTags);
}

bool UProficiencyComponent::TryGrantProficiencyXPWithContext(
    UProficiencyDefinition *Definition, const float Amount,
    const FGameplayTagContainer &ContextTags) {
    if (!Definition || !FMath::IsFinite(Amount) || Amount <= 0.0f
        || !ASC || !GetOwner() || !GetOwner()->HasAuthority()) {
        return false;
    }

    FProficiency *Prof = nullptr;
    for (auto &P : Proficiencies) {
        if (P.Definition == Definition) {
            Prof = &P;
            break;
        }
    }
    const FGameplayAttribute ProgressAttribute = Prof
        ? Prof->GetProgressAttribute() : FGameplayAttribute();
    if (!Prof || !ProgressAttribute.IsValid()
        || !ASC->HasAttributeSetForAttribute(ProgressAttribute)) {
        UE_LOG(Myth, Warning, TEXT("ProficiencyComponent: no proficiency configured for %s, cannot grant XP"),
               *GetNameSafe(Definition));
        return false;
    }

    const float Current = ASC->GetNumericAttributeBase(ProgressAttribute);
    const float Updated = Current + Amount;
    if (!FMath::IsFinite(Current) || !FMath::IsFinite(Updated)) {
        return false;
    }
    ASC->SetNumericAttributeBase(ProgressAttribute, Updated);

    UE_LOG(Myth, Log, TEXT("ProficiencyComponent: granted %.1f %s XP (%.1f -> %.1f)"),
           Amount, *GetNameSafe(Definition), Current, Updated);

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
    return true;
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
    const FGameplayAttribute ProgressAttribute = CombatProf
        ? CombatProf->GetProgressAttribute() : FGameplayAttribute();
    if (!CombatProf || !ProgressAttribute.IsValid()
        || !ASC->HasAttributeSetForAttribute(ProgressAttribute)) {
        return;
    }

    const float OldXP = ASC->GetNumericAttributeBase(ProgressAttribute);
    const int32 OldLevel = UProficiencyDefinition::CalcLevelAtXP(OldXP, CombatProf->Definition);
    const float LevelFloorXP = UProficiencyDefinition::CalcCumulativeXPForLevel(OldLevel, CombatProf->Definition);
    const float NewXP = ComputeXpAfterDeathPenalty(OldXP, PenaltyFraction, LevelFloorXP);
    ASC->SetNumericAttributeBase(ProgressAttribute, NewXP);

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

    const FGameplayAttribute ProgressAttribute = Prof.GetProgressAttribute();
    if (ASC && ProgressAttribute.IsValid()
        && ASC->HasAttributeSetForAttribute(ProgressAttribute)) {
        Summary.CurrentXP = ASC->GetNumericAttributeBase(ProgressAttribute);
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

bool UProficiencyComponent::TryGetLevelForDefinition(
    const UProficiencyDefinition *Definition, int32 &OutLevel) const {
    OutLevel = 0;
    if (!Definition) {
        return false;
    }

    const FProficiency *Match = nullptr;
    for (const FProficiency &Proficiency : Proficiencies) {
        if (Proficiency.Definition != Definition) {
            continue;
        }
        if (Match) {
            return false;
        }
        Match = &Proficiency;
    }
    if (!Match) {
        return false;
    }

    float CurrentXP = Match->SavedXP;
    const FGameplayAttribute ProgressAttribute = Match->GetProgressAttribute();
    if (ASC && ProgressAttribute.IsValid()
        && ASC->HasAttributeSetForAttribute(ProgressAttribute)) {
        CurrentXP = ASC->GetNumericAttributeBase(ProgressAttribute);
    }
    if (!FMath::IsFinite(CurrentXP) || CurrentXP < 0.0f) {
        return false;
    }

    OutLevel = UProficiencyDefinition::CalcLevelAtXP(CurrentXP, Definition);
    return OutLevel >= 0;
}

void UProficiencyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(UProficiencyComponent, Proficiencies, COND_OwnerOnly);
}
