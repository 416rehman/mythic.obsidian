#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Abilities/MythicAbilityActionTypes.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicTags_GAS.h"
#include "Tests/Progression/MythicAbilityWorldFixture.h"
#include "Internationalization/StringTableRegistry.h"
#include "Itemization/Affixes/MythicAffixApplicationComponent.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Misc/ScopeExit.h"
#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"

struct FMythicAffixApplicationTestAccessor {
    static bool ComputeWinners(const TMap<FGuid, FMythicAppliedAffixState> &Candidates,
                               TSet<FGuid> &OutActive) {
        return UMythicAffixApplicationComponent::ComputeStackingWinners(
            Candidates, OutActive);
    }

    static bool SnapshotsEquivalent(const FRolledAffix &A, const FRolledAffix &B) {
        return UMythicAffixApplicationComponent::SnapshotsEquivalent(A, B);
    }

    static void SetAbilitySystem(UMythicAffixApplicationComponent &Application,
                                 UMythicAbilitySystemComponent &AbilitySystem) {
        Application.AbilitySystemComponent = &AbilitySystem;
    }

    static bool Transition(UMythicAffixApplicationComponent &Application,
                           TMap<FGuid, FMythicAppliedAffixState> Desired) {
        return Application.TransitionLedgerTransactional(MoveTemp(Desired));
    }

    static void FailNextApply(UMythicAffixApplicationComponent &Application) {
        Application.TestApplyFailureCountdown = 0;
    }

    static void FailNextRemove(UMythicAffixApplicationComponent &Application) {
        Application.TestRemoveFailureCountdown = 0;
    }

    static const FMythicAppliedAffixState *FindState(
        const UMythicAffixApplicationComponent &Application, const FGuid RollGuid) {
        return Application.Ledger.Find(RollGuid);
    }

    static void QuarantineSemanticRevision(
        UMythicAffixApplicationComponent &Application, const uint64 SemanticRevision) {
        Application.QuarantineApplicationAfterSemanticReconciliationFailure(
            SemanticRevision);
    }
};

namespace {
FMythicAppliedAffixState MakeRankedState(
    const FGuid RollGuid, const FGuid SourceItemGuid,
    const EMythicAffixStackingRule Rule, const float Magnitude,
    const EMythicStatComparisonDirection Direction =
        EMythicStatComparisonDirection::HigherIsBetter) {
    FMythicAppliedAffixState State;
    State.RollGuid = RollGuid;
    State.SourceItemGuid = SourceItemGuid;
    State.Snapshot.RollGuid = RollGuid;
    State.Snapshot.Magnitude = Magnitude;
    State.TargetStatTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Stat.Attribute.Armor")), true);
    State.StackingGroup = AFFIX_SOURCE_SIGNATURE;
    State.StackingRule = Rule;
    State.ModifierOp = EGameplayModOp::AddBase;
    State.ComparisonDirection = Direction;
    State.NeutralValue = Direction == EMythicStatComparisonDirection::LowerIsBetter
        ? 1.0f : 0.0f;
    State.Magnitude = Magnitude;
    return State;
}

const FName ApplicationStringTableId(TEXT("MythicAffixApplicationTests"));

class FScopedApplicationStringTable {
public:
    FScopedApplicationStringTable() {
        FStringTableRegistry::Get().UnregisterStringTable(ApplicationStringTableId);
        FStringTableRegistry::Get().Internal_NewLocTable(
            ApplicationStringTableId, TEXT("MythicAffixApplicationTests"));
    }

    ~FScopedApplicationStringTable() {
        FStringTableRegistry::Get().UnregisterStringTable(ApplicationStringTableId);
    }

    FText Add(const TCHAR *Key, const TCHAR *Source) const {
        FStringTableRegistry::Get().Internal_SetLocTableEntry(
            ApplicationStringTableId, Key, Source);
        return FText::FromStringTable(ApplicationStringTableId, Key);
    }
};

FRolledAffix MakeApplicationSnapshot(UMythicAffixDefinition *Definition,
                                     const FGuid RollGuid,
                                     const FGuid SourceItemGuid,
                                     const float Magnitude) {
    FRolledAffix Snapshot;
    Snapshot.RollGuid = RollGuid;
    Snapshot.AffixDefinition.SetAsset(Definition);
    Snapshot.TierRank = 1;
    Snapshot.Magnitude = Magnitude;
    Snapshot.Provenance.RollGroup = AFFIX_ROLL_GROUP_PREFIX;
    Snapshot.Provenance.SourceKind = AFFIX_SOURCE_EXPLICIT;
    Snapshot.Provenance.SourceItemGuid = SourceItemGuid;
    Snapshot.Provenance.GeneratedItemLevel = 1;
    Snapshot.Provenance.AlgorithmVersion = 1;
    return Snapshot;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixStackingWinnerTest,
    "Mythic.Itemization.Affixes.Application.StackingWinners",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixStackingWinnerTest::RunTest(const FString &Parameters) {
    const FGuid ItemA(1, 0, 0, 0);
    const FGuid ItemB(2, 0, 0, 0);
    const FGuid WeakRoll(10, 0, 0, 0);
    const FGuid StrongRoll(20, 0, 0, 0);
    const FGuid OtherItemRoll(30, 0, 0, 0);

    TMap<FGuid, FMythicAppliedAffixState> Candidates;
    Candidates.Add(WeakRoll, MakeRankedState(
        WeakRoll, ItemA, EMythicAffixStackingRule::HighestPerItem, 5.0f));
    Candidates.Add(StrongRoll, MakeRankedState(
        StrongRoll, ItemA, EMythicAffixStackingRule::HighestPerItem, 9.0f));
    Candidates.Add(OtherItemRoll, MakeRankedState(
        OtherItemRoll, ItemB, EMythicAffixStackingRule::HighestPerItem, 7.0f));

    TSet<FGuid> Active;
    TestTrue(TEXT("HighestPerItem candidates produce a valid winner set"),
             FMythicAffixApplicationTestAccessor::ComputeWinners(Candidates, Active));
    TestFalse(TEXT("weaker candidate on one item is suppressed"), Active.Contains(WeakRoll));
    TestTrue(TEXT("stronger candidate wins within its source item"), Active.Contains(StrongRoll));
    TestTrue(TEXT("another source item has an independent winner"),
             Active.Contains(OtherItemRoll));

    for (TPair<FGuid, FMythicAppliedAffixState> &Pair : Candidates) {
        Pair.Value.StackingRule = EMythicAffixStackingRule::HighestOverall;
    }
    TestTrue(TEXT("HighestOverall candidates produce a valid winner set"),
             FMythicAffixApplicationTestAccessor::ComputeWinners(Candidates, Active));
    TestEqual(TEXT("HighestOverall activates exactly one candidate"), Active.Num(), 1);
    TestTrue(TEXT("HighestOverall chooses the greatest beneficial contribution"),
             Active.Contains(StrongRoll));

    Candidates.Reset();
    const FGuid LowerRoll(40, 0, 0, 0);
    const FGuid LowestRoll(50, 0, 0, 0);
    Candidates.Add(LowerRoll, MakeRankedState(
        LowerRoll, ItemA, EMythicAffixStackingRule::HighestOverall, 0.90f,
        EMythicStatComparisonDirection::LowerIsBetter));
    Candidates.Add(LowestRoll, MakeRankedState(
        LowestRoll, ItemB, EMythicAffixStackingRule::HighestOverall, 0.75f,
        EMythicStatComparisonDirection::LowerIsBetter));
    TestTrue(TEXT("lower-is-better candidates use stat comparison semantics"),
             FMythicAffixApplicationTestAccessor::ComputeWinners(Candidates, Active));
    TestTrue(TEXT("the greatest beneficial reduction wins"), Active.Contains(LowestRoll));

    Candidates.Reset();
    const FGuid DivideHigherWeak(51, 0, 0, 0);
    const FGuid DivideHigherStrong(52, 0, 0, 0);
    FMythicAppliedAffixState DivideHalf = MakeRankedState(
        DivideHigherWeak, ItemA, EMythicAffixStackingRule::HighestOverall, 2.0f);
    FMythicAppliedAffixState DivideDouble = MakeRankedState(
        DivideHigherStrong, ItemB, EMythicAffixStackingRule::HighestOverall, 0.5f);
    DivideHalf.ModifierOp = EGameplayModOp::DivideAdditive;
    DivideDouble.ModifierOp = EGameplayModOp::DivideAdditive;
    Candidates.Add(DivideHigherWeak, DivideHalf);
    Candidates.Add(DivideHigherStrong, DivideDouble);
    TestTrue(TEXT("DivideAdditive higher-is-better candidates normalize through the inverse"),
             FMythicAffixApplicationTestAccessor::ComputeWinners(Candidates, Active));
    TestTrue(TEXT("dividing by one half beats dividing by two for a higher-is-better stat"),
             Active.Contains(DivideHigherStrong));

    Candidates.Reset();
    const FGuid DivideLowerWeak(53, 0, 0, 0);
    const FGuid DivideLowerStrong(54, 0, 0, 0);
    FMythicAppliedAffixState DivideByOneQuarter = MakeRankedState(
        DivideLowerWeak, ItemA, EMythicAffixStackingRule::HighestOverall, 1.25f,
        EMythicStatComparisonDirection::LowerIsBetter);
    FMythicAppliedAffixState DivideByTwo = MakeRankedState(
        DivideLowerStrong, ItemB, EMythicAffixStackingRule::HighestOverall, 2.0f,
        EMythicStatComparisonDirection::LowerIsBetter);
    DivideByOneQuarter.ModifierOp = EGameplayModOp::DivideAdditive;
    DivideByTwo.ModifierOp = EGameplayModOp::DivideAdditive;
    Candidates.Add(DivideLowerWeak, DivideByOneQuarter);
    Candidates.Add(DivideLowerStrong, DivideByTwo);
    TestTrue(TEXT("DivideAdditive lower-is-better candidates normalize through the inverse"),
             FMythicAffixApplicationTestAccessor::ComputeWinners(Candidates, Active));
    TestTrue(TEXT("dividing by two beats dividing by one point two five for a lower-is-better stat"),
             Active.Contains(DivideLowerStrong));

    Candidates.Reset();
    const FGuid ConflictWinner(60, 0, 0, 0);
    const FGuid ConflictRunnerUp(70, 0, 0, 0);
    FMythicAppliedAffixState Winner = MakeRankedState(
        ConflictWinner, ItemA, EMythicAffixStackingRule::StackAll, 1.0f);
    FMythicAppliedAffixState RunnerUp = MakeRankedState(
        ConflictRunnerUp, ItemB, EMythicAffixStackingRule::StackAll, 999.0f);
    Winner.ConflictGroups.AddTag(AFFIX_SOURCE_IMPLICIT);
    RunnerUp.ConflictGroups.AddTag(AFFIX_SOURCE_IMPLICIT);
    Candidates.Add(ConflictRunnerUp, RunnerUp);
    Candidates.Add(ConflictWinner, Winner);
    TestTrue(TEXT("cross-item conflict groups produce a deterministic winner set"),
             FMythicAffixApplicationTestAccessor::ComputeWinners(Candidates, Active));
    TestEqual(TEXT("a conflict group activates exactly one candidate"), Active.Num(), 1);
    TestTrue(TEXT("conflicts use stable source identity rather than magnitude"),
             Active.Contains(ConflictWinner));

    FRolledAffix Changed = RunnerUp.Snapshot;
    Changed.Magnitude += 1.0f;
    TestFalse(TEXT("singular magnitude participates in snapshot equivalence"),
              FMythicAffixApplicationTestAccessor::SnapshotsEquivalent(
                  RunnerUp.Snapshot, Changed));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixPermanentApplicationRollbackTest,
    "Mythic.Itemization.Affixes.Application.PermanentStatTransactionRollback",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixPermanentApplicationRollbackTest::RunTest(const FString &Parameters) {
    if (!TestNotNull(TEXT("engine is available"), GEngine)) return false;
    UGameInstance *GameInstance = NewObject<UGameInstance>(GEngine);
    GameInstance->InitializeStandalone();
    ON_SCOPE_EXIT { GameInstance->Shutdown(); };
    UWorld *World = GameInstance->GetWorld();
    if (!TestNotNull(TEXT("standalone world exists"), World)) return false;

    FScopedApplicationStringTable Texts;
    UMythicStatCategoryDefinition *Category =
        NewObject<UMythicStatCategoryDefinition>(GetTransientPackage());
    Category->DeveloperName = TEXT("Defense");
    Category->DesignerPurpose = TEXT("Application fixture.");
    Category->CategoryTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Stat.Category.Defense")), true);
    Category->DisplayName = Texts.Add(TEXT("Defense"), TEXT("Defense"));

    UMythicStatDefinition *Stat =
        NewObject<UMythicStatDefinition>(GetTransientPackage());
    Stat->DeveloperName = TEXT("Armor");
    Stat->DesignerPurpose = TEXT("Application fixture.");
    Stat->StatTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Stat.Attribute.Armor")), true);
    Stat->Attribute = UMythicAttributeSet_Defense::GetArmorAttribute();
    Stat->DisplayName = Texts.Add(TEXT("Armor"), TEXT("Armor"));
    Stat->Category.SetAsset(Category);
    Stat->bCanBeAffixTarget = true;

    UMythicAffixDefinition *Definition =
        NewObject<UMythicAffixDefinition>(GetTransientPackage());
    Definition->DeveloperName = TEXT("Armor");
    Definition->DesignerPurpose = TEXT("Application fixture.");
    Definition->AffixTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Itemization.Affix.Armor")), true);
    Definition->DisplayNameTemplate = Texts.Add(
        TEXT("ArmorAffix"), TEXT("Armor"));
    Definition->TargetStat.SetAsset(Stat);
    Definition->ModifierOp = EGameplayModOp::AddBase;
    Definition->StackingRule = EMythicAffixStackingRule::StackAll;
    FMythicAffixTierProgressionDefinition &Progression =
        Definition->TierProgressions.AddDefaulted_GetRef();
    Progression.DeveloperName = TEXT("Fallback");
    Progression.TuningContext = TEXT("Core");
    FMythicAffixTierDefinition &Tier = Progression.Tiers.AddDefaulted_GetRef();
    Tier.DeveloperName = TEXT("Rank1");
    Tier.Magnitude.Min = 1.0f;
    Tier.Magnitude.Max = 10.0f;

    UMythicItemizationDataRegistrySubsystem *Registry =
        GameInstance->GetSubsystem<UMythicItemizationDataRegistrySubsystem>();
    if (!TestNotNull(TEXT("itemization registry exists"), Registry)) return false;
    TArray<UObject *> Assets{Category, Stat, Definition};
    TArray<FText> Errors;
    if (!TestTrue(TEXT("live Definition and Stat graph publishes"),
                  Registry->PublishCoreSemanticAssetsForTests(Assets, Errors))) return false;

    AActor *Owner = World->SpawnActor<AActor>();
    UMythicAbilitySystemComponent *AbilitySystem =
        NewObject<UMythicAbilitySystemComponent>(Owner);
    AbilitySystem->RegisterComponent();
    AbilitySystem->InitAbilityActorInfo(Owner, Owner);
    AbilitySystem->AddAttributeSetSubobject(
        NewObject<UMythicAttributeSet_Defense>(Owner));
    AbilitySystem->SetNumericAttributeBase(Stat->Attribute, 100.0f);

    UMythicAffixApplicationComponent *Application =
        NewObject<UMythicAffixApplicationComponent>(Owner);
    FMythicAffixApplicationTestAccessor::SetAbilitySystem(*Application, *AbilitySystem);

    const FGuid RollGuid(101, 102, 103, 104);
    const FGuid SourceGuid(201, 202, 203, 204);
    FMythicAppliedAffixState Initial;
    Initial.RollGuid = RollGuid;
    Initial.SourceItemGuid = SourceGuid;
    Initial.Snapshot = MakeApplicationSnapshot(
        Definition, RollGuid, SourceGuid, 5.0f);
    TMap<FGuid, FMythicAppliedAffixState> InitialLedger;
    InitialLedger.Add(RollGuid, Initial);
    TestTrue(TEXT("initial permanent affix applies"),
             FMythicAffixApplicationTestAccessor::Transition(
                 *Application, InitialLedger));
    TestTrue(TEXT("live stat base includes the applied affix"),
             FMath::IsNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Stat->Attribute), 105.0f));

    FMythicAppliedAffixState Replacement = Initial;
    Replacement.Snapshot.Magnitude = 9.0f;
    TMap<FGuid, FMythicAppliedAffixState> ReplacementLedger;
    ReplacementLedger.Add(RollGuid, Replacement);
    FMythicAffixApplicationTestAccessor::FailNextApply(*Application);
    TestFalse(TEXT("injected apply failure rejects the replacement"),
              FMythicAffixApplicationTestAccessor::Transition(
                  *Application, ReplacementLedger));
    const FMythicAppliedAffixState *AfterApplyFailure =
        FMythicAffixApplicationTestAccessor::FindState(*Application, RollGuid);
    TestTrue(TEXT("apply failure preserves the old snapshot and stat base"),
             AfterApplyFailure && AfterApplyFailure->Snapshot.Magnitude == 5.0f
             && FMath::IsNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Stat->Attribute), 105.0f));

    FMythicAffixApplicationTestAccessor::FailNextRemove(*Application);
    TestFalse(TEXT("injected removal failure rejects unequip"),
              FMythicAffixApplicationTestAccessor::Transition(*Application, {}));
    TestTrue(TEXT("remove failure preserves the composed base"),
             FMath::IsNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Stat->Attribute), 105.0f));

    AddExpectedErrorPlain(
        TEXT("quarantined 1 source item(s) because semantic revision 42 could not reconcile"),
        EAutomationExpectedErrorFlags::Contains, 1);
    FMythicAffixApplicationTestAccessor::QuarantineSemanticRevision(
        *Application, 42);
    TestTrue(TEXT("a rejected semantic revision visibly quarantines application"),
             Application->IsApplicationQuarantined());
    TestFalse(TEXT("semantic quarantine suppresses the previously active source"),
              Application->IsActive(RollGuid));
    TestTrue(TEXT("semantic quarantine removes the stale permanent stat contribution"),
             FMath::IsNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Stat->Attribute), 100.0f));

    TestTrue(TEXT("cleanup transition succeeds after one-shot failures"),
             FMythicAffixApplicationTestAccessor::Transition(*Application, {}));
    TestTrue(TEXT("cleanup restores the unmodified base"),
             FMath::IsNearlyEqual(
                 AbilitySystem->GetNumericAttributeBase(Stat->Attribute), 100.0f));
    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicActionAffixEligibilityTest,"Mythic.Itemization.Affixes.ActionEligibilityAndComposition",EAutomationTestFlags_ApplicationContextMask|EAutomationTestFlags::ProductFilter)
bool FMythicActionAffixEligibilityTest::RunTest(const FString&) {
    const auto Melee=FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Reach.Melee"));
    const auto Ranged=FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Reach.Ranged"));
    const auto Fire=FGameplayTag::RequestGameplayTag(TEXT("Ability.Action.Element.Fire"));
    const auto Power=UMythicAttributeSet_Offense::GetPowerAttribute();
    FMythicActionAffixSnapshot Snapshot; Snapshot.NonEquipmentBases.Add(Power,100);
    const FGuid Item(1,2,3,4);
    auto Add=[&](int32 Id,float Value,FGameplayTagQuery Query) {
        auto State=MakeRankedState(FGuid(Id,0,0,0),Item,EMythicAffixStackingRule::HighestOverall,Value);
        State.TargetAttribute=Power;
        if (!Query.IsEmpty()) { State.Application.Scope=EMythicAffixApplicationScope::Action; State.Application.ActionQuery=Query; }
        Snapshot.Candidates.Add(State.RollGuid,State);
    };
    Add(1,3,{}); Add(2,5,FGameplayTagQuery::MakeQuery_MatchTag(Melee)); Add(3,9,FGameplayTagQuery::MakeQuery_MatchTag(Ranged));
    FGameplayTagContainer WholeAbility; WholeAbility.AddTag(Melee); WholeAbility.AddTag(Ranged);
    Add(4,99,FGameplayTagQuery::MakeQuery_MatchAllTags(WholeAbility));
    TMap<FGameplayAttribute,float> MeleeBases,RangedBases;
    TestTrue(TEXT("melee equipment resolves"),UMythicAffixApplicationComponent::ResolveActionEquipmentBases(Snapshot,Melee.GetSingleTagContainer(),WholeAbility,{},MeleeBases));
    TestTrue(TEXT("ranged equipment resolves"),UMythicAffixApplicationComponent::ResolveActionEquipmentBases(Snapshot,Ranged.GetSingleTagContainer(),WholeAbility,{},RangedBases));
    TestEqual(TEXT("ineligible strongest affixes never suppress melee"),MeleeBases.FindRef(Power),105.0f);
    TestEqual(TEXT("ranged action selects its own strongest eligible affix"),RangedBases.FindRef(Power),109.0f);
    auto FireOnly=Snapshot.Candidates.FindChecked(FGuid(4,0,0,0)); FireOnly.Application.ActionQuery=FGameplayTagQuery::MakeQuery_MatchTag(Fire);
    Snapshot.Candidates[FireOnly.RollGuid]=FireOnly;
    TestTrue(TEXT("source state is a separate query domain"),UMythicAffixApplicationComponent::ResolveActionEquipmentBases(Snapshot,Melee.GetSingleTagContainer(),WholeAbility,Fire.GetSingleTagContainer(),MeleeBases));
    TestEqual(TEXT("fire on the source cannot turn a contact action into fire"),MeleeBases.FindRef(Power),105.0f);
    auto* Game=NewObject<UGameInstance>(GEngine); Game->InitializeStandalone(); ON_SCOPE_EXIT { DestroyOwnedAbilityTestWorld(Game); };
    auto* Actor=Game->GetWorld()->SpawnActor<AActor>(); auto* ASC=NewObject<UMythicAbilitySystemComponent>(Actor);
    Actor->AddInstanceComponent(ASC); ASC->RegisterComponent(); ASC->InitAbilityActorInfo(Actor,Actor);
    ASC->AddAttributeSetSubobject(NewObject<UMythicAttributeSet_Offense>(Actor)); ASC->SetNumericAttributeBase(Power,103);
    auto* Effect=NewObject<UGameplayEffect>(); Effect->DurationPolicy=EGameplayEffectDurationType::Infinite;
    auto& Mod=Effect->Modifiers.AddDefaulted_GetRef(); Mod.Attribute=Power; Mod.ModifierOp=EGameplayModOp::Multiplicitive;
    Mod.ModifierMagnitude=FGameplayEffectModifierMagnitude(FScalableFloat(2));
    ASC->ApplyGameplayEffectToSelf(Effect,1,ASC->MakeEffectContext());
    int32 Changes=0; const auto Listener=ASC->GetGameplayAttributeValueChangeDelegate(Power).AddLambda([&](const FOnAttributeChangeData&){++Changes;});
    const int32 EffectCount=ASC->GetActiveEffects(FGameplayEffectQuery()).Num();
    float MeleeValue=0,RangedValue=0;
    TestTrue(TEXT("GAS evaluates recomposed melee base without mutation"),FMythicAbilityActionSource::CaptureStat(ASC,Power,{},MeleeBases,MeleeValue));
    TestTrue(TEXT("GAS evaluates recomposed ranged base without mutation"),FMythicAbilityActionSource::CaptureStat(ASC,Power,{},RangedBases,RangedValue));
    TestEqual(TEXT("temporary multiplication applies above melee equipment"),MeleeValue,210.0f);
    TestEqual(TEXT("temporary multiplication applies above ranged equipment"),RangedValue,218.0f);
    TestEqual(TEXT("live equipment base is untouched"),ASC->GetNumericAttributeBase(Power),103.0f);
    TestEqual(TEXT("live aggregate is untouched"),ASC->GetNumericAttribute(Power),206.0f);
    TestEqual(TEXT("snapshot evaluation emitted no attribute change"),Changes,0);
    TestEqual(TEXT("snapshot evaluation added no gameplay effect"),ASC->GetActiveEffects(FGameplayEffectQuery()).Num(),EffectCount);
    ASC->GetGameplayAttributeValueChangeDelegate(Power).Remove(Listener);
    TestTrue(TEXT("power feeds authored action damage"),FMythicAbilityActionSource::IsSupportedStat(Power));
    TestFalse(TEXT("health/armor-only strength is not an action source stat"),FMythicAbilityActionSource::IsSupportedStat(UMythicAttributeSet_Defense::GetStrengthAttribute()));
    TestFalse(TEXT("weapon rolls cannot replace ability action damage"),FMythicAbilityActionSource::IsSupportedStat(UMythicAttributeSet_Offense::GetDamagePerHitAttribute()));
    return true;
}
#endif
