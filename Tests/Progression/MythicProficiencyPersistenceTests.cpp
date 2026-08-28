#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "Misc/ScopeExit.h"
#include "Player/MythicPlayerState.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "Rewards/AttributeReward.h"
#include "Stats/MythicStatDefinition.h"
#include "Subsystem/SaveSystem/Character/SavedProficiency.h"

#include <limits>

namespace {
FGameplayTag RegisteredProficiencyTestTag(const TCHAR *Name) {
    return FGameplayTag::RequestGameplayTag(FName(Name), true);
}

UMythicStatDefinition *MakeStatDefinition(
    const TCHAR *TagName,
    const FGameplayAttribute &Attribute) {
    UMythicStatDefinition *Stat = NewObject<UMythicStatDefinition>();
    Stat->StatTag = RegisteredProficiencyTestTag(TagName);
    Stat->Attribute = Attribute;
    return Stat;
}

UProficiencyDefinition *MakeProficiencyDefinition(
    const TCHAR *TrackTagName,
    const TCHAR *ProgressStatTagName,
    const FGameplayAttribute &ProgressAttribute) {
    UMythicStatDefinition *ProgressStat = MakeStatDefinition(
        ProgressStatTagName, ProgressAttribute);
    UProficiencyDefinition *Definition = NewObject<UProficiencyDefinition>();
    Definition->TrackTag = RegisteredProficiencyTestTag(TrackTagName);
    Definition->ProgressStat.SetAsset(ProgressStat);
    return Definition;
}

FSerializedProficiencyData MakeSavedEntry(
    UProficiencyDefinition *Definition,
    const float CurrentXP) {
    FSerializedProficiencyData Entry;
    Entry.ProficiencyDefinition = Definition;
    Entry.CurrentXP = CurrentXP;
    return Entry;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTypedProficiencySaveTest,
    "Mythic.Player.Proficiency.Persistence.TypedRoundTrip",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicTypedProficiencySaveTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) {
        return false;
    }

    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    ON_SCOPE_EXIT { GameInstance->Shutdown(); };
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) {
        return false;
    }

    AMythicPlayerState *PlayerState = World->SpawnActor<AMythicPlayerState>();
    if (!TestNotNull(TEXT("authoritative player state spawned"), PlayerState)) {
        return false;
    }
    UMythicAbilitySystemComponent *AbilitySystem =
        PlayerState->GetMythicAbilitySystemComponent();
    if (!TestNotNull(TEXT("player state owns an ability system"), AbilitySystem)) {
        return false;
    }
    if (!AbilitySystem->IsRegistered()) {
        AbilitySystem->RegisterComponent();
    }
    AbilitySystem->InitAbilityActorInfo(PlayerState, PlayerState);
    AbilitySystem->AddAttributeSetSubobject(
        NewObject<UMythicAttributeSet_Proficiencies>(PlayerState));

    UProficiencyComponent *Component = NewObject<UProficiencyComponent>(PlayerState);
    PlayerState->AddInstanceComponent(Component);
    if (!TestEqual(TEXT("save component is owned by the ASC actor"),
                   Component->GetOwner(), static_cast<AActor *>(PlayerState))) {
        return false;
    }

    UProficiencyDefinition *Combat = MakeProficiencyDefinition(
        TEXT("Proficiency.Combat"), TEXT("Stat.Attribute.CombatProficiency"),
        UMythicAttributeSet_Proficiencies::GetCombatProficiencyAttribute());
    UProficiencyDefinition *Mining = MakeProficiencyDefinition(
        TEXT("Proficiency.Mining"), TEXT("Stat.Attribute.MiningProficiency"),
        UMythicAttributeSet_Proficiencies::GetMiningProficiencyAttribute());
    FProficiency CombatProficiency;
    CombatProficiency.Definition = Combat;
    Component->Proficiencies.Add(CombatProficiency);
    FProficiency MiningProficiency;
    MiningProficiency.Definition = Mining;
    Component->Proficiencies.Add(MiningProficiency);

    AbilitySystem->SetNumericAttributeBase(
        UMythicAttributeSet_Proficiencies::GetCombatProficiencyAttribute(), 123.5f);
    AbilitySystem->SetNumericAttributeBase(
        UMythicAttributeSet_Proficiencies::GetMiningProficiencyAttribute(), 456.0f);

    TArray<FSerializedProficiencyData> Saved;
    TestTrue(TEXT("typed proficiency roster serializes"),
             FSerializedProficiencyHelper::Serialize(Component, Saved));
    TestEqual(TEXT("serialize emits exactly the authored roster"), Saved.Num(), 2);
    if (Saved.Num() != 2) {
        return false;
    }
    TestEqual(TEXT("first save entry retains the definition reference"),
              Saved[0].ProficiencyDefinition.Get(), Combat);
    TestEqual(TEXT("second save entry retains the definition reference"),
              Saved[1].ProficiencyDefinition.Get(), Mining);
    TestTrue(TEXT("combat XP is read from its definition-derived GAS attribute"),
             FMath::IsNearlyEqual(Saved[0].CurrentXP, 123.5f));
    TestTrue(TEXT("mining XP is read from its definition-derived GAS attribute"),
             FMath::IsNearlyEqual(Saved[1].CurrentXP, 456.0f));

    Component->Proficiencies[0].SavedXP = 1.0f;
    Component->Proficiencies[1].SavedXP = 2.0f;
    TestTrue(TEXT("typed save payload deserializes"),
             FSerializedProficiencyHelper::Deserialize(Component, Saved));
    TestTrue(TEXT("round trip restores combat XP by definition"),
             FMath::IsNearlyEqual(Component->Proficiencies[0].SavedXP, 123.5f));
    TestTrue(TEXT("round trip restores mining XP by definition"),
             FMath::IsNearlyEqual(Component->Proficiencies[1].SavedXP, 456.0f));

    TArray<FSerializedProficiencyData> PreviousOutput = Saved;
    FProficiency DuplicateCombatProficiency;
    DuplicateCombatProficiency.Definition = Combat;
    Component->Proficiencies.Add(MoveTemp(DuplicateCombatProficiency));
    AddExpectedErrorPlain(
        TEXT("SavedProficiency::Serialize - rejected track without a typed, installed Progress Stat:"),
        EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("serialize rejects a duplicate authored definition"),
              FSerializedProficiencyHelper::Serialize(Component, Saved));
    TestEqual(TEXT("failed serialize preserves the caller's previous payload size"),
              Saved.Num(), PreviousOutput.Num());
    if (Saved.Num() == PreviousOutput.Num()) {
        for (int32 Index = 0; Index < Saved.Num(); ++Index) {
            TestTrue(TEXT("failed serialize preserves typed identity"),
                     Saved[Index].ProficiencyDefinition.ToSoftObjectPath()
                         == PreviousOutput[Index].ProficiencyDefinition.ToSoftObjectPath());
            TestEqual(TEXT("failed serialize preserves XP"),
                      Saved[Index].CurrentXP, PreviousOutput[Index].CurrentXP);
        }
    }
    Component->Proficiencies.Pop();

    auto ResetSentinelXp = [Component]() {
        Component->Proficiencies[0].SavedXP = 11.0f;
        Component->Proficiencies[1].SavedXP = 22.0f;
    };
    auto TestSentinelXp = [this, Component](const TCHAR *Context) {
        TestTrue(*FString::Printf(TEXT("%s preserves the complete roster"), Context),
                 FMath::IsNearlyEqual(Component->Proficiencies[0].SavedXP, 11.0f)
                 && FMath::IsNearlyEqual(Component->Proficiencies[1].SavedXP, 22.0f));
    };

    ResetSentinelXp();
    const TArray<FSerializedProficiencyData> DuplicateEntries{Saved[0], Saved[0]};
    AddExpectedErrorPlain(
        TEXT("SavedProficiency::Deserialize - rejected invalid or duplicate typed entry:"),
        EAutomationExpectedErrorFlags::Contains, 3);
    TestFalse(TEXT("duplicate save definition is rejected atomically"),
              FSerializedProficiencyHelper::Deserialize(Component, DuplicateEntries));
    TestSentinelXp(TEXT("duplicate save rejection"));

    ResetSentinelXp();
    FSerializedProficiencyData NonFinite = Saved[0];
    NonFinite.CurrentXP = std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("non-finite saved XP is rejected atomically"),
              FSerializedProficiencyHelper::Deserialize(Component, {NonFinite}));
    TestSentinelXp(TEXT("non-finite XP rejection"));

    ResetSentinelXp();
    UProficiencyDefinition *Unknown = MakeProficiencyDefinition(
        TEXT("Proficiency.Cooking"), TEXT("Stat.Attribute.CookingProficiency"),
        UMythicAttributeSet_Proficiencies::GetCookingProficiencyAttribute());
    const TArray<FSerializedProficiencyData> UnknownEntry{
        MakeSavedEntry(Unknown, 50.0f)};
    TestFalse(TEXT("save data cannot inject an unauthored proficiency"),
              FSerializedProficiencyHelper::Deserialize(Component, UnknownEntry));
    TestSentinelXp(TEXT("unknown definition rejection"));

    ResetSentinelXp();
    const TArray<FSerializedProficiencyData> PartialRoster{Saved[0]};
    TestTrue(TEXT("a valid partial save payload is accepted"),
             FSerializedProficiencyHelper::Deserialize(Component, PartialRoster));
    TestTrue(TEXT("present track restores its XP"),
             FMath::IsNearlyEqual(Component->Proficiencies[0].SavedXP, 123.5f));
    TestEqual(TEXT("absent track deterministically resets to zero"),
              Component->Proficiencies[1].SavedXP, 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicProficiencyTrackRebuildTest,
    "Mythic.Player.Proficiency.Track.DeterministicRebuild",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicProficiencyTrackRebuildTest::RunTest(const FString &Parameters) {
    UProficiencyDefinition *Definition = MakeProficiencyDefinition(
        TEXT("Proficiency.Combat"), TEXT("Stat.Attribute.CombatProficiency"),
        UMythicAttributeSet_Proficiencies::GetCombatProficiencyAttribute());
    Definition->MaxLevel = 4;
    Definition->KeyMilestones.AddDefaulted_GetRef().Name = FText::FromString(TEXT("Capstone"));

    UMythicStatDefinition *Armor = MakeStatDefinition(
        TEXT("Stat.Attribute.Armor"),
        UMythicAttributeSet_Defense::GetArmorAttribute());
    UMythicStatDefinition *Strength = MakeStatDefinition(
        TEXT("Stat.Attribute.Strength"),
        UMythicAttributeSet_Defense::GetStrengthAttribute());
    Definition->AttributeGoals.Add(
        FAttributeGoal(FMythicStatDefinitionHandle(), 20.0f, EGameplayModOp::AddBase));
    Definition->AttributeGoals.Last().TargetStat.SetAsset(Armor);
    Definition->AttributeGoals.Add(
        FAttributeGoal(FMythicStatDefinitionHandle(), 8.0f, EGameplayModOp::AddBase));
    Definition->AttributeGoals.Last().TargetStat.SetAsset(Strength);

    FProficiency Proficiency;
    Proficiency.Definition = Definition;
    Proficiency.GenerateTrack();
    TestEqual(TEXT("compiled track contains one row per level"),
              Proficiency.Track.Num(), Definition->MaxLevel);
    if (Proficiency.Track.Num() != Definition->MaxLevel) {
        return false;
    }

    TArray<FGuid> FirstSourceGuids;
    TMap<FPrimaryAssetId, float> TotalsByTarget;
    TSet<FGuid> UniqueSources;
    for (int32 LevelIndex = 0; LevelIndex < Proficiency.Track.Num(); ++LevelIndex) {
        const FMilestone &Milestone = Proficiency.Track[LevelIndex];
        TestEqual(*FString::Printf(TEXT("level %d has exactly one generated reward"), LevelIndex + 1),
                  Milestone.Rewards.Num(), 1);
        if (Milestone.Rewards.Num() != 1) {
            continue;
        }
        const UAttributeReward *Reward = Cast<UAttributeReward>(Milestone.Rewards[0]);
        if (!TestNotNull(*FString::Printf(TEXT("level %d reward is typed"), LevelIndex + 1),
                         Reward)) {
            continue;
        }
        TestTrue(*FString::Printf(TEXT("level %d reward has a stable source identity"), LevelIndex + 1),
                 Reward->PermanentSourceGuid.IsValid());
        TestFalse(*FString::Printf(TEXT("level %d reward source is unique"), LevelIndex + 1),
                  UniqueSources.Contains(Reward->PermanentSourceGuid));
        UniqueSources.Add(Reward->PermanentSourceGuid);
        FirstSourceGuids.Add(Reward->PermanentSourceGuid);
        TotalsByTarget.FindOrAdd(Reward->TargetStat.GetPrimaryAssetId()) += Reward->Magnitude;
    }
    TestEqual(TEXT("all generated reward sources are unique"),
              UniqueSources.Num(), Definition->MaxLevel);
    TestTrue(TEXT("distributed armor rewards sum to the authored goal"),
             FMath::IsNearlyEqual(TotalsByTarget.FindRef(Armor->GetPrimaryAssetId()), 20.0f));
    TestTrue(TEXT("distributed strength rewards sum to the authored goal"),
             FMath::IsNearlyEqual(TotalsByTarget.FindRef(Strength->GetPrimaryAssetId()), 8.0f));

    Proficiency.GenerateTrack();
    TestEqual(TEXT("rebuild replaces rather than appends track rows"),
              Proficiency.Track.Num(), Definition->MaxLevel);
    for (int32 LevelIndex = 0; LevelIndex < Proficiency.Track.Num(); ++LevelIndex) {
        const FMilestone &Milestone = Proficiency.Track[LevelIndex];
        TestEqual(*FString::Printf(TEXT("rebuilt level %d still has one reward"), LevelIndex + 1),
                  Milestone.Rewards.Num(), 1);
        const UAttributeReward *Reward = Milestone.Rewards.Num() == 1
            ? Cast<UAttributeReward>(Milestone.Rewards[0]) : nullptr;
        if (Reward && FirstSourceGuids.IsValidIndex(LevelIndex)) {
            TestTrue(*FString::Printf(TEXT("rebuilt level %d derives the same source identity"), LevelIndex + 1),
                     Reward->PermanentSourceGuid == FirstSourceGuids[LevelIndex]);
        }
    }

    Definition->KeyMilestones.Reset();
    AddExpectedErrorPlain(
        TEXT("Proficiency: Missing required data"),
        EAutomationExpectedErrorFlags::Contains, 1);
    Proficiency.GenerateTrack();
    TestTrue(TEXT("an invalid rebuild clears the previous compiled reward graph"),
             Proficiency.Track.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAuthoredProficiencyRewardMaterializationTest,
    "Mythic.Player.Proficiency.Track.AuthoredAttributeRewardMaterialization",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAuthoredProficiencyRewardMaterializationTest::RunTest(const FString &Parameters) {
    UProficiencyDefinition *Definition = MakeProficiencyDefinition(
        TEXT("Proficiency.Combat"), TEXT("Stat.Attribute.CombatProficiency"),
        UMythicAttributeSet_Proficiencies::GetCombatProficiencyAttribute());
    Definition->MaxLevel = 4;

    UMythicStatDefinition *Armor = MakeStatDefinition(
        TEXT("Stat.Attribute.Armor"),
        UMythicAttributeSet_Defense::GetArmorAttribute());
    Definition->AttributeGoals.Add(
        FAttributeGoal(FMythicStatDefinitionHandle(), 20.0f, EGameplayModOp::AddBase));
    Definition->AttributeGoals.Last().TargetStat.SetAsset(Armor);

    UAttributeReward *SharedAuthoredReward = NewObject<UAttributeReward>(Definition);
    SharedAuthoredReward->PermanentSourceGuid = FGuid(11, 22, 33, 44);
    SharedAuthoredReward->TargetStat.SetAsset(Armor);
    SharedAuthoredReward->Modifier = EGameplayModOp::AddBase;
    SharedAuthoredReward->Magnitude = 7.5f;

    UAttributeReward *InvalidIdentityAuthoredReward = NewObject<UAttributeReward>(Definition);
    InvalidIdentityAuthoredReward->TargetStat.SetAsset(Armor);
    InvalidIdentityAuthoredReward->Modifier = EGameplayModOp::Override;
    InvalidIdentityAuthoredReward->Magnitude = 99.0f;

    FMilestone &Capstone = Definition->KeyMilestones.AddDefaulted_GetRef();
    Capstone.Name = FText::FromString(TEXT("Capstone"));
    Capstone.Rewards.Add(SharedAuthoredReward);
    Capstone.Rewards.Add(SharedAuthoredReward);
    Capstone.Rewards.Add(InvalidIdentityAuthoredReward);

    FProficiency Proficiency;
    Proficiency.Definition = Definition;
    Proficiency.GenerateTrack();

    TestEqual(TEXT("authored rewards compile into the capstone plus its generated goal reward"),
              Proficiency.Track.Num() == Definition->MaxLevel
                  ? Proficiency.Track.Last().Rewards.Num() : 0,
              4);
    if (Proficiency.Track.Num() != Definition->MaxLevel
        || Proficiency.Track.Last().Rewards.Num() != 4) {
        return false;
    }

    TArray<FGuid> FirstBuildSources;
    TSet<FGuid> UniqueSources;
    for (int32 TrackIndex = 0; TrackIndex < Proficiency.Track.Num(); ++TrackIndex) {
        const FMilestone &Milestone = Proficiency.Track[TrackIndex];
        for (int32 RewardSlot = 0; RewardSlot < Milestone.Rewards.Num(); ++RewardSlot) {
            const UAttributeReward *RuntimeReward = Cast<UAttributeReward>(Milestone.Rewards[RewardSlot]);
            if (!TestNotNull(
                    *FString::Printf(TEXT("track %d slot %d remains an attribute reward"),
                                     TrackIndex, RewardSlot),
                    RuntimeReward)) {
                continue;
            }
            TestTrue(
                *FString::Printf(TEXT("track %d slot %d has a contextual source identity"),
                                 TrackIndex, RewardSlot),
                RuntimeReward->PermanentSourceGuid.IsValid());
            TestFalse(
                *FString::Printf(TEXT("track %d slot %d source identity is unique"),
                                 TrackIndex, RewardSlot),
                UniqueSources.Contains(RuntimeReward->PermanentSourceGuid));
            UniqueSources.Add(RuntimeReward->PermanentSourceGuid);
            FirstBuildSources.Add(RuntimeReward->PermanentSourceGuid);
        }
    }

    const FMilestone &CompiledCapstone = Proficiency.Track.Last();
    const UAttributeReward *FirstClone = Cast<UAttributeReward>(CompiledCapstone.Rewards[0]);
    const UAttributeReward *SecondClone = Cast<UAttributeReward>(CompiledCapstone.Rewards[1]);
    const UAttributeReward *InvalidIdentityClone = Cast<UAttributeReward>(CompiledCapstone.Rewards[2]);
    const UAttributeReward *LevelZeroGeneratedReward =
        Cast<UAttributeReward>(Proficiency.Track[0].Rewards[0]);
    const UAttributeReward *LevelThreeGeneratedReward =
        Cast<UAttributeReward>(CompiledCapstone.Rewards[3]);
    TestTrue(TEXT("each authored placement is materialized as an independent transient instance"),
             FirstClone && SecondClone && InvalidIdentityClone
             && FirstClone != SharedAuthoredReward && SecondClone != SharedAuthoredReward
             && InvalidIdentityClone != InvalidIdentityAuthoredReward
             && FirstClone != SecondClone
             && FirstClone->HasAnyFlags(RF_Transient)
             && SecondClone->HasAnyFlags(RF_Transient)
             && InvalidIdentityClone->HasAnyFlags(RF_Transient));
    TestTrue(TEXT("materialization preserves the authored reward payload"),
             FirstClone && FirstClone->TargetStat.GetAsset() == Armor
             && FirstClone->Modifier == SharedAuthoredReward->Modifier
             && FMath::IsNearlyEqual(FirstClone->Magnitude, SharedAuthoredReward->Magnitude)
             && InvalidIdentityClone && InvalidIdentityClone->TargetStat.GetAsset() == Armor
             && InvalidIdentityClone->Modifier == InvalidIdentityAuthoredReward->Modifier
             && FMath::IsNearlyEqual(
                    InvalidIdentityClone->Magnitude, InvalidIdentityAuthoredReward->Magnitude));
    TestEqual(TEXT("materialization does not rewrite a valid authored source identity"),
              SharedAuthoredReward->PermanentSourceGuid, FGuid(11, 22, 33, 44));
    TestFalse(TEXT("materialization does not write an identity into the definition-owned reward"),
              InvalidIdentityAuthoredReward->PermanentSourceGuid.IsValid());
    TestTrue(TEXT("authored and generated placements with coordinate zero occupy distinct identity namespaces"),
             FirstClone && LevelZeroGeneratedReward
             && FirstClone->PermanentSourceGuid != LevelZeroGeneratedReward->PermanentSourceGuid);

    const TArray<FGuid> AuthoredSourcesBeforeRelocation{
        FirstClone ? FirstClone->PermanentSourceGuid : FGuid(),
        SecondClone ? SecondClone->PermanentSourceGuid : FGuid(),
        InvalidIdentityClone ? InvalidIdentityClone->PermanentSourceGuid : FGuid()};
    const FGuid LevelZeroGeneratedSource = LevelZeroGeneratedReward
        ? LevelZeroGeneratedReward->PermanentSourceGuid : FGuid();
    const FGuid LevelThreeGeneratedSource = LevelThreeGeneratedReward
        ? LevelThreeGeneratedReward->PermanentSourceGuid : FGuid();

    Proficiency.GenerateTrack();
    TArray<FGuid> SecondBuildSources;
    for (const FMilestone &Milestone : Proficiency.Track) {
        for (const URewardBase *Reward : Milestone.Rewards) {
            if (const UAttributeReward *AttributeReward = Cast<UAttributeReward>(Reward)) {
                SecondBuildSources.Add(AttributeReward->PermanentSourceGuid);
            }
        }
    }
    TestEqual(TEXT("recompiling preserves the full typed placement identity roster"),
              SecondBuildSources.Num(), FirstBuildSources.Num());
    if (SecondBuildSources.Num() == FirstBuildSources.Num()) {
        for (int32 Index = 0; Index < FirstBuildSources.Num(); ++Index) {
            TestEqual(*FString::Printf(TEXT("recompiled source %d is stable"), Index),
                      SecondBuildSources[Index], FirstBuildSources[Index]);
        }
    }

    Definition->MaxLevel = 6;
    Proficiency.GenerateTrack();
    TestEqual(TEXT("MaxLevel retuning relocates the capstone without losing its compiled rewards"),
              Proficiency.Track.Num() == 6 ? Proficiency.Track.Last().Rewards.Num() : 0,
              4);
    if (Proficiency.Track.Num() == 6 && Proficiency.Track.Last().Rewards.Num() == 4) {
        for (int32 RewardSlot = 0; RewardSlot < AuthoredSourcesBeforeRelocation.Num(); ++RewardSlot) {
            const UAttributeReward *RelocatedReward = Cast<UAttributeReward>(
                Proficiency.Track.Last().Rewards[RewardSlot]);
            TestTrue(
                *FString::Printf(TEXT("authored reward slot %d keeps its source when MaxLevel moves the milestone"),
                                 RewardSlot),
                RelocatedReward
                && RelocatedReward->PermanentSourceGuid == AuthoredSourcesBeforeRelocation[RewardSlot]);
        }
    }
    const UAttributeReward *RetunedLevelZeroReward = Proficiency.Track.Num() == 6
        ? Cast<UAttributeReward>(Proficiency.Track[0].Rewards[0]) : nullptr;
    const UAttributeReward *RetunedLevelThreeReward = Proficiency.Track.Num() == 6
        ? Cast<UAttributeReward>(Proficiency.Track[3].Rewards[0]) : nullptr;
    TestTrue(TEXT("generated source identity remains attached to its stable level-zero placement"),
             RetunedLevelZeroReward
             && RetunedLevelZeroReward->PermanentSourceGuid == LevelZeroGeneratedSource);
    TestTrue(TEXT("generated source identity remains attached to level three after the milestone relocates"),
             RetunedLevelThreeReward
             && RetunedLevelThreeReward->PermanentSourceGuid == LevelThreeGeneratedSource);
    return true;
}

#endif
