#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "Internationalization/StringTableRegistry.h"
#include "Itemization/Affixes/MythicAffixCompiler.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicAffixProfile.h"
#include "Itemization/Affixes/MythicAffixRollPolicy.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicTags_Affixes.h"
#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"
#include "UObject/StrongObjectPtr.h"
#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

struct FMythicItemizationRegistryRefreshTestAccessor {
#if WITH_EDITOR
    static void NotifyResidentPreEdit(
        UMythicItemizationDataRegistrySubsystem &Registry, UObject *Object) {
        TGuardValue<bool> AcceptCompletionsGuard(
            Registry.bAcceptingAsyncCompletions, true);
        FEditPropertyChain PropertyChain;
        Registry.HandleResidentAssetPrePropertyChange(Object, PropertyChain);
    }

    static bool CapturePreEditSnapshot(
        UMythicItemizationDataRegistrySubsystem &Registry, UObject *Object) {
        return Registry.CaptureResidentAssetPreEditSnapshot(Object);
    }

    static bool RestorePreEditSnapshots(
        UMythicItemizationDataRegistrySubsystem &Registry,
        TArray<FText> &OutErrors) {
        const bool bRestored = Registry.RestoreResidentAssetPreEditSnapshots(OutErrors);
        Registry.ClearResidentAssetPreEditSnapshots();
        return bRestored;
    }

    static bool PublishWithoutOrdinaryBroadcast(
        UMythicItemizationDataRegistrySubsystem &Registry,
        TArray<FText> &OutErrors) {
        return Registry.PublishLoadedAssetsInternal({}, OutErrors, false);
    }

    static void NotifyResidentObjectsReplaced(
        UMythicItemizationDataRegistrySubsystem &Registry,
        const TMap<UObject *, UObject *> &ReplacementMap) {
        TGuardValue<bool> AcceptCompletionsGuard(
            Registry.bAcceptingAsyncCompletions, true);
        Registry.HandleResidentObjectsReplaced(ReplacementMap);
    }

    static void SimulateReflectedResidentReplacement(
        UMythicItemizationDataRegistrySubsystem &Registry,
        UObject *OldObject,
        UObject *ReplacementObject) {
        for (TObjectPtr<UObject> &Resident : Registry.LoadedAssets) {
            if (Resident == OldObject) {
                Resident = ReplacementObject;
            }
        }
    }

    static bool IsEditorRefreshPending(
        const UMythicItemizationDataRegistrySubsystem &Registry) {
        return Registry.bEditorRefreshPending;
    }

    static void ProcessPendingEditorRefresh(
        UMythicItemizationDataRegistrySubsystem &Registry) {
        Registry.ProcessPendingEditorResidentRefresh();
    }
#endif
};

namespace {
const FName RefreshStringTableId(TEXT("MythicAffixRegistryRefreshTests"));

class FScopedRefreshStringTable {
public:
    FScopedRefreshStringTable() {
        FStringTableRegistry::Get().UnregisterStringTable(RefreshStringTableId);
        FStringTableRegistry::Get().Internal_NewLocTable(
            RefreshStringTableId, TEXT("MythicAffixRegistryRefreshTests"));
        FStringTableRegistry::Get().Internal_SetLocTableEntry(
            RefreshStringTableId, TEXT("Defense"), TEXT("Defense"));
        FStringTableRegistry::Get().Internal_SetLocTableEntry(
            RefreshStringTableId, TEXT("IncomingDamage"), TEXT("Incoming Damage"));
        FStringTableRegistry::Get().Internal_SetLocTableEntry(
            RefreshStringTableId, TEXT("Affix"), TEXT("Incoming Damage"));
    }

    ~FScopedRefreshStringTable() {
        FStringTableRegistry::Get().UnregisterStringTable(RefreshStringTableId);
    }

    FText Get(const TCHAR *Key) const {
        return FText::FromStringTable(RefreshStringTableId, Key);
    }
};

FGameplayTag RequireRefreshTag(const TCHAR *Name) {
    return FGameplayTag::RequestGameplayTag(FName(Name), true);
}

struct FRefreshFixture {
    UMythicItemizationDataRegistrySubsystem *CreateRegistry() {
        check(GEngine);
        GameInstance.Reset(NewObject<UGameInstance>(GEngine, NAME_None, RF_Transient));
        Registry.Reset(NewObject<UMythicItemizationDataRegistrySubsystem>(
            GameInstance.Get(), NAME_None, RF_Transient));
        return Registry.Get();
    }

    TStrongObjectPtr<UGameInstance> GameInstance;
    TStrongObjectPtr<UMythicItemizationDataRegistrySubsystem> Registry;
};

struct FRefreshSemanticGraph {
    UMythicStatCategoryDefinition *Category = nullptr;
    UMythicStatDefinition *Stat = nullptr;
    UMythicAffixDefinition *Definition = nullptr;
    UMythicAffixRollPolicy *Policy = nullptr;
    UMythicAffixProfile *Profile = nullptr;
    FMythicAffixGrantSpec StandaloneGrant;
};

FRefreshSemanticGraph BuildRefreshGraph(const FScopedRefreshStringTable &Texts) {
    FRefreshSemanticGraph Graph;
    Graph.Category = NewObject<UMythicStatCategoryDefinition>(GetTransientPackage());
    Graph.Category->DeveloperName = TEXT("Defense");
    Graph.Category->DesignerPurpose = TEXT("Registry refresh fixture.");
    Graph.Category->CategoryTag = RequireRefreshTag(TEXT("Stat.Category.Defense"));
    Graph.Category->DisplayName = Texts.Get(TEXT("Defense"));

    Graph.Stat = NewObject<UMythicStatDefinition>(GetTransientPackage());
    Graph.Stat->DeveloperName = TEXT("IncomingDamageMultiplier");
    Graph.Stat->DesignerPurpose = TEXT("Registry refresh fixture.");
    Graph.Stat->StatTag = RequireRefreshTag(
        TEXT("Stat.Attribute.IncomingDamageMultiplier"));
    Graph.Stat->Attribute = UMythicAttributeSet_Defense::GetIncomingDamageMultiplierAttribute();
    Graph.Stat->DisplayName = Texts.Get(TEXT("IncomingDamage"));
    Graph.Stat->Category.SetAsset(Graph.Category);
    Graph.Stat->NumberPresentation.Format = EMythicStatFormat::Multiplier;
    Graph.Stat->NeutralValue = 1.0f;
    Graph.Stat->bCanBeAffixTarget = true;

    Graph.Definition = NewObject<UMythicAffixDefinition>(GetTransientPackage());
    Graph.Definition->DeveloperName = TEXT("IncomingDamageMultiplier");
    Graph.Definition->DesignerPurpose = TEXT("Registry refresh fixture.");
    Graph.Definition->AffixTag = RequireRefreshTag(
        TEXT("Itemization.Affix.IncomingDamageMultiplier"));
    Graph.Definition->DisplayNameTemplate = Texts.Get(TEXT("Affix"));
    Graph.Definition->TargetStat.SetAsset(Graph.Stat);
    Graph.Definition->ModifierOp = EGameplayModOp::MultiplyCompound;
    Graph.Definition->Quantization.Mode = EMythicAffixQuantizationMode::Step;
    Graph.Definition->Quantization.Step = 0.1f;
    Graph.Definition->StackingGroup = Graph.Definition->AffixTag;
    FMythicAffixTierProgressionDefinition &Progression =
        Graph.Definition->TierProgressions.AddDefaulted_GetRef();
    Progression.DeveloperName = TEXT("Fallback");
    Progression.TuningContext = TEXT("Core");
    FMythicAffixTierDefinition &Tier = Progression.Tiers.AddDefaulted_GetRef();
    Tier.DeveloperName = TEXT("Rank1");
    Tier.MinItemLevel = 1;
    Tier.Magnitude.Min = 0.9f;
    Tier.Magnitude.Max = 0.9f;

    Graph.Policy = NewObject<UMythicAffixRollPolicy>(GetTransientPackage());
    Graph.Policy->DeveloperName = TEXT("RefreshPolicy");
    Graph.Policy->DesignerPurpose = TEXT("Registry refresh fixture.");
    Graph.Policy->PolicyTag = RequireRefreshTag(
        TEXT("Itemization.AffixRollPolicy.Default.S0"));
    for (uint8 Rarity = static_cast<uint8>(EItemRarity::Common);
         Rarity <= static_cast<uint8>(EItemRarity::Mythic); ++Rarity) {
        FMythicAffixRarityBudget &Budget =
            Graph.Policy->BudgetsByRarity.AddDefaulted_GetRef();
        Budget.Rarity = static_cast<EItemRarity>(Rarity);
        Budget.RandomRollCount = 0;
    }

    Graph.Profile = NewObject<UMythicAffixProfile>(GetTransientPackage());
    Graph.Profile->DeveloperName = TEXT("RefreshProfile");
    Graph.Profile->DesignerPurpose = TEXT("Registry refresh fixture.");
    Graph.Profile->ProfileTag = RequireRefreshTag(
        TEXT("Itemization.AffixProfile.Armour.S0"));
    Graph.Profile->RollPolicy.SetAsset(Graph.Policy);
    FMythicAffixGrantSpec &ProfileGrant =
        Graph.Profile->GuaranteedGrants.AddDefaulted_GetRef();
    ProfileGrant.GrantGuid = FGuid(0x101, 0x202, 0x303, 0x404);
    ProfileGrant.DeveloperName = TEXT("ProfileGrant");
    ProfileGrant.AffixDefinition.SetAsset(Graph.Definition);
    ProfileGrant.RollGroup = AFFIX_ROLL_GROUP_SUFFIX;
    ProfileGrant.SourceKind = AFFIX_SOURCE_EXPLICIT;

    Graph.StandaloneGrant = ProfileGrant;
    Graph.StandaloneGrant.GrantGuid = FGuid(0x505, 0x606, 0x707, 0x808);
    Graph.StandaloneGrant.DeveloperName = TEXT("StandaloneGrant");
    Graph.StandaloneGrant.SourceKind = AFFIX_SOURCE_GEM;
    return Graph;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixRegistryTransactionalRefreshTest,
    "Mythic.Itemization.Affixes.Registry.TransactionalSemanticRefresh",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixRegistryTransactionalRefreshTest::RunTest(const FString &Parameters) {
    FScopedRefreshStringTable Texts;
    FRefreshSemanticGraph Graph = BuildRefreshGraph(Texts);
    FRefreshFixture Fixture;
    UMythicItemizationDataRegistrySubsystem *Registry = Fixture.CreateRegistry();
    TArray<UObject *> Assets{
        Graph.Category, Graph.Stat, Graph.Definition, Graph.Policy, Graph.Profile};
    TArray<FText> Errors;
    if (!TestTrue(TEXT("the complete typed graph publishes"),
                  Registry->PublishLoadedAssets(Assets, Errors))
        || !TestTrue(TEXT("the profile compiles"),
                     Registry->CompileProfile(Graph.Profile->GetPrimaryAssetId(), Errors))
        || !TestTrue(TEXT("the standalone grant compiles"),
                     Registry->CompileGrantClosures(
                         MakeArrayView(&Graph.StandaloneGrant, 1), Errors))) {
        return false;
    }
    TestEqual(TEXT("the Blueprint registry surface discovers the resident stat category"),
              Registry->GetAllStatCategoryAssets().Num(), 1);
    TestEqual(TEXT("the Blueprint registry surface discovers the resident stat definition"),
              Registry->GetAllStatDefinitionAssets().Num(), 1);
    TestEqual(TEXT("the Blueprint registry surface discovers the resident affix definition"),
              Registry->GetAllAffixDefinitionAssets().Num(), 1);

    const TSharedPtr<const FCompiledAffixProfile> InitialProfile =
        Registry->FindCompiledProfile(Graph.Profile->GetPrimaryAssetId());
    const TSharedPtr<const FCompiledAffixGrantClosure> InitialGrant =
        Registry->FindCompiledGrant(Graph.StandaloneGrant);
    if (!TestTrue(TEXT("both immutable closures are resident"),
                  InitialProfile.IsValid() && InitialGrant.IsValid())) {
        return false;
    }

    int32 ChangeCount = 0;
    uint64 LastRevision = 0;
    bool bReentrantRefreshRejected = false;
    const FDelegateHandle ChangeHandle = Registry->OnSemanticDataChanged().AddLambda(
        [Registry, &ChangeCount, &LastRevision,
         &bReentrantRefreshRejected](const uint64 Revision) {
            ++ChangeCount;
            LastRevision = Revision;
            if (ChangeCount == 1) {
                TArray<FText> ReentrantErrors;
                bReentrantRefreshRejected =
                    !Registry->RefreshResidentData(ReentrantErrors);
            }
        });
    const uint64 InitialRevision = Registry->GetSemanticDataRevision();

    Graph.Definition->ModifierOp = EGameplayModOp::DivideAdditive;
    ++Graph.Definition->Revision;
    Errors.Reset();
    if (!TestTrue(TEXT("valid resident tuning refreshes atomically"),
                  Registry->RefreshResidentData(Errors))) {
        Registry->OnSemanticDataChanged().Remove(ChangeHandle);
        return false;
    }

    const TSharedPtr<const FCompiledAffixProfile> RefreshedProfile =
        Registry->FindCompiledProfile(Graph.Profile->GetPrimaryAssetId());
    const TSharedPtr<const FCompiledAffixGrantClosure> RefreshedGrant =
        Registry->FindCompiledGrant(Graph.StandaloneGrant);
    TestTrue(TEXT("profile closure is replaced, not retained stale"),
             RefreshedProfile.IsValid() && RefreshedProfile != InitialProfile);
    TestTrue(TEXT("grant closure is replaced, not retained stale"),
             RefreshedGrant.IsValid() && RefreshedGrant != InitialGrant);
    if (!RefreshedProfile.IsValid() || RefreshedProfile->GuaranteedGrants.IsEmpty()
        || !RefreshedGrant.IsValid()) {
        Registry->OnSemanticDataChanged().Remove(ChangeHandle);
        return false;
    }
    TestEqual(TEXT("the profile carries the current modifier operation"),
              RefreshedProfile->GuaranteedGrants[0].Affix.Definition.ModifierOp,
              EGameplayModOp::DivideAdditive);
    TestEqual(TEXT("the grant carries the current modifier operation"),
              RefreshedGrant->Affix.Definition.ModifierOp,
              EGameplayModOp::DivideAdditive);
    TestTrue(TEXT("profile gameplay hash changes with gameplay semantics"),
             RefreshedProfile->GameplayContentHash != InitialProfile->GameplayContentHash);
    TestTrue(TEXT("grant gameplay hash changes with gameplay semantics"),
             RefreshedGrant->GameplayContentHash != InitialGrant->GameplayContentHash);
    TestEqual(TEXT("one successful refresh emits one change event"), ChangeCount, 1);
    TestEqual(TEXT("the change event publishes the committed revision"),
              LastRevision, InitialRevision + 1);
    TestTrue(TEXT("a semantic callback cannot recursively publish another refresh"),
             bReentrantRefreshRejected);

    const TArray<FMythicAffixRarityBudget> ValidBudgets =
        Graph.Policy->BudgetsByRarity;
#if WITH_EDITOR
    TestTrue(TEXT("the editor retains a pre-edit snapshot of the resident semantic asset"),
             FMythicItemizationRegistryRefreshTestAccessor::CapturePreEditSnapshot(
                 *Registry, Graph.Policy));
#endif
    Graph.Policy->BudgetsByRarity.Reset();
    Errors.Reset();
    TestFalse(TEXT("a refresh that cannot recompile every resident profile is rejected"),
              Registry->RefreshResidentData(Errors));
    TestTrue(TEXT("rejected refresh keeps the last valid profile closure"),
             Registry->FindCompiledProfile(Graph.Profile->GetPrimaryAssetId())
                 == RefreshedProfile);
    TestTrue(TEXT("rejected refresh keeps the last valid grant closure"),
             Registry->FindCompiledGrant(Graph.StandaloneGrant) == RefreshedGrant);
    TestEqual(TEXT("rejected refresh does not emit a semantic change"),
              ChangeCount, 1);
    TestEqual(TEXT("rejected refresh does not advance the revision"),
              Registry->GetSemanticDataRevision(), LastRevision);

#if WITH_EDITOR
    TArray<FText> RestoreErrors;
    TestTrue(TEXT("the rejected in-place editor mutation restores the UObject snapshot"),
             FMythicItemizationRegistryRefreshTestAccessor::RestorePreEditSnapshots(
                 *Registry, RestoreErrors));
    TestEqual(TEXT("the restored UObject contains the last published policy semantics"),
              Graph.Policy->BudgetsByRarity.Num(), ValidBudgets.Num());
#else
    Graph.Policy->BudgetsByRarity = ValidBudgets;
#endif
    Errors.Reset();
    TestTrue(TEXT("corrected resident semantics can be published"),
             Registry->RefreshResidentData(Errors));
    TestEqual(TEXT("the corrected publication emits the next change event"),
              ChangeCount, 2);
    Registry->OnSemanticDataChanged().Remove(ChangeHandle);
    return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixRegistryEditorQuarantineRecoveryTest,
    "Mythic.Itemization.Affixes.Registry.EditorPreEditQuarantineRecovery",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixRegistryEditorQuarantineRecoveryTest::RunTest(
    const FString &Parameters) {
    FScopedRefreshStringTable Texts;
    FRefreshSemanticGraph Graph = BuildRefreshGraph(Texts);
    FRefreshFixture Fixture;
    UMythicItemizationDataRegistrySubsystem *Registry = Fixture.CreateRegistry();
    TArray<UObject *> Assets{
        Graph.Category, Graph.Stat, Graph.Definition, Graph.Policy, Graph.Profile};
    TArray<FText> Errors;
    if (!TestTrue(TEXT("the typed fixture publishes"),
                  Registry->PublishLoadedAssets(Assets, Errors))
        || !TestTrue(TEXT("the profile closure compiles"),
                     Registry->CompileProfile(Graph.Profile->GetPrimaryAssetId(), Errors))
        || !TestTrue(TEXT("the standalone grant closure compiles"),
                     Registry->CompileGrantClosures(
                         MakeArrayView(&Graph.StandaloneGrant, 1), Errors))) {
        return false;
    }

    int32 ChangeCount = 0;
    uint64 LastRevision = Registry->GetSemanticDataRevision();
    const uint64 InitialRevision = LastRevision;
    const FDelegateHandle ChangeHandle = Registry->OnSemanticDataChanged().AddLambda(
        [&ChangeCount, &LastRevision](const uint64 Revision) {
            ++ChangeCount;
            LastRevision = Revision;
        });

    FMythicItemizationRegistryRefreshTestAccessor::NotifyResidentPreEdit(
        *Registry, Graph.Definition);
    TestEqual(TEXT("the first resident pre-edit publishes one unavailable transition"),
              ChangeCount, 1);
    TestEqual(TEXT("the unavailable transition advances the semantic revision"),
              LastRevision, InitialRevision + 1);
    TestFalse(TEXT("the stat registry is unavailable before the UObject mutates"),
              Registry->GetStatRegistry().IsBuilt());
    TestNull(TEXT("stat lookup fails closed during the editor batch"),
             Registry->FindStat(Graph.Stat->GetPrimaryAssetId()));
    TestNull(TEXT("attribute lookup fails closed during the editor batch"),
             Registry->FindStat(Graph.Stat->Attribute));
    TestNull(TEXT("affix lookup fails closed during the editor batch"),
             Registry->FindAffix(Graph.Definition->GetPrimaryAssetId()));
    TestNull(TEXT("policy lookup fails closed during the editor batch"),
             Registry->FindPolicy(Graph.Policy->GetPrimaryAssetId()));
    TestNull(TEXT("profile lookup fails closed during the editor batch"),
             Registry->FindProfile(Graph.Profile->GetPrimaryAssetId()));
    TestFalse(TEXT("profile readiness fails closed during the editor batch"),
              Registry->IsProfileReady(Graph.Profile->GetPrimaryAssetId()));
    TestFalse(TEXT("compiled profile lookup fails closed during the editor batch"),
              Registry->FindCompiledProfile(Graph.Profile->GetPrimaryAssetId()).IsValid());
    TestFalse(TEXT("compiled grant lookup fails closed during the editor batch"),
              Registry->FindCompiledGrant(Graph.StandaloneGrant).IsValid());
    TestEqual(TEXT("the Blueprint stat-definition surface is empty while quarantined"),
              Registry->GetAllStatDefinitionAssets().Num(), 0);
    TestEqual(TEXT("the Blueprint category surface is empty while quarantined"),
              Registry->GetAllStatCategoryAssets().Num(), 0);
    TestEqual(TEXT("the Blueprint affix-definition surface is empty while quarantined"),
              Registry->GetAllAffixDefinitionAssets().Num(), 0);

    // A slider/interactive edit can produce many pre-change notifications. The first one owns the availability
    // transition and all later notifications merely extend the same rollback snapshot batch.
    FMythicItemizationRegistryRefreshTestAccessor::NotifyResidentPreEdit(
        *Registry, Graph.Policy);
    TestEqual(TEXT("additional pre-edits do not rebroadcast quarantine"), ChangeCount, 1);

    Errors.Reset();
    TestTrue(TEXT("a valid internal publication recovers quarantined semantics"),
             FMythicItemizationRegistryRefreshTestAccessor::PublishWithoutOrdinaryBroadcast(
                 *Registry, Errors));
    TestEqual(TEXT("silent internal publication emits exactly one recovery transition"),
              ChangeCount, 2);
    TestEqual(TEXT("recovery advances the semantic revision"),
              LastRevision, InitialRevision + 2);
    TestTrue(TEXT("the committed stat registry is visible after recovery"),
             Registry->GetStatRegistry().IsBuilt());
    TestNotNull(TEXT("affix lookup is restored after recovery"),
                Registry->FindAffix(Graph.Definition->GetPrimaryAssetId()));
    TestTrue(TEXT("compiled profile lookup is restored after recovery"),
             Registry->FindCompiledProfile(Graph.Profile->GetPrimaryAssetId()).IsValid());

    const TArray<FMythicAffixRarityBudget> ValidBudgets =
        Graph.Policy->BudgetsByRarity;
    FMythicItemizationRegistryRefreshTestAccessor::NotifyResidentPreEdit(
        *Registry, Graph.Policy);
    TestEqual(TEXT("a new editor batch emits one new unavailable transition"),
              ChangeCount, 3);
    Graph.Policy->BudgetsByRarity.Reset();
    Errors.Reset();
    TestFalse(TEXT("invalid in-place semantics cannot commit"),
              Registry->RefreshResidentData(Errors));
    TestEqual(TEXT("a rejected commit remains in the existing quarantine transition"),
              ChangeCount, 3);
    TestNull(TEXT("lookups remain fail-closed until rollback verification completes"),
             Registry->FindPolicy(Graph.Policy->GetPrimaryAssetId()));

    TArray<FText> RestoreErrors;
    TestTrue(TEXT("the invalid edit restores and verifies the pre-edit UObject graph"),
             FMythicItemizationRegistryRefreshTestAccessor::RestorePreEditSnapshots(
                 *Registry, RestoreErrors));
    TestEqual(TEXT("rollback restores the last committed policy semantics"),
              Graph.Policy->BudgetsByRarity.Num(), ValidBudgets.Num());
    TestEqual(TEXT("verified rollback emits exactly one recovery transition"),
              ChangeCount, 4);
    TestEqual(TEXT("rollback recovery advances the semantic revision"),
              LastRevision, InitialRevision + 4);
    TestNotNull(TEXT("policy lookup is restored only after rollback verification"),
                Registry->FindPolicy(Graph.Policy->GetPrimaryAssetId()));
    TestTrue(TEXT("compiled grant lookup is restored after rollback verification"),
             Registry->FindCompiledGrant(Graph.StandaloneGrant).IsValid());

    Registry->OnSemanticDataChanged().Remove(ChangeHandle);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAffixRegistryResidentReplacementTest,
    "Mythic.Itemization.Affixes.Registry.ResidentObjectReplacement",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAffixRegistryResidentReplacementTest::RunTest(
    const FString &Parameters) {
    FScopedRefreshStringTable Texts;
    FRefreshSemanticGraph Graph = BuildRefreshGraph(Texts);
    FRefreshFixture Fixture;
    UMythicItemizationDataRegistrySubsystem *Registry = Fixture.CreateRegistry();
    TArray<UObject *> Assets{
        Graph.Category, Graph.Stat, Graph.Definition, Graph.Policy, Graph.Profile};
    TArray<FText> Errors;
    if (!TestTrue(TEXT("the replacement fixture publishes"),
                  Registry->PublishLoadedAssets(Assets, Errors))
        || !TestTrue(TEXT("the replacement profile compiles"),
                     Registry->CompileProfile(Graph.Profile->GetPrimaryAssetId(), Errors))
        || !TestTrue(TEXT("the replacement grant compiles"),
                     Registry->CompileGrantClosures(
                         MakeArrayView(&Graph.StandaloneGrant, 1), Errors))) {
        return false;
    }

    const FPrimaryAssetId DefinitionId = Graph.Definition->GetPrimaryAssetId();
    const TSharedPtr<const FCompiledAffixProfile> InitialProfile =
        Registry->FindCompiledProfile(Graph.Profile->GetPrimaryAssetId());
    const TSharedPtr<const FCompiledAffixGrantClosure> InitialGrant =
        Registry->FindCompiledGrant(Graph.StandaloneGrant);
    if (!TestTrue(TEXT("the original immutable closures exist"),
                  InitialProfile.IsValid() && InitialGrant.IsValid())) {
        return false;
    }

    UMythicAffixDefinition *Replacement = DuplicateObject<UMythicAffixDefinition>(
        Graph.Definition, GetTransientPackage(),
        MakeUniqueObjectName(GetTransientPackage(), UMythicAffixDefinition::StaticClass(),
                             TEXT("ReplacementAffixDefinition")));
    if (!TestNotNull(TEXT("a replacement Affix Definition is created"), Replacement)) {
        return false;
    }
    Replacement->ModifierOp = EGameplayModOp::DivideAdditive;
    ++Replacement->Revision;

    int32 ChangeCount = 0;
    uint64 LastRevision = Registry->GetSemanticDataRevision();
    const uint64 InitialRevision = LastRevision;
    const FDelegateHandle ChangeHandle = Registry->OnSemanticDataChanged().AddLambda(
        [&ChangeCount, &LastRevision](const uint64 Revision) {
            ++ChangeCount;
            LastRevision = Revision;
        });

    TMap<UObject *, UObject *> ReplacementMap;
    ReplacementMap.Add(Graph.Definition, Replacement);
    // Some engine replacement paths fix reflected UPROPERTY references before tools receive OnObjectsReplaced.
    // Reproduce that ordering while deliberately leaving the registry's native indexes and immutable closures old.
    FMythicItemizationRegistryRefreshTestAccessor::SimulateReflectedResidentReplacement(
        *Registry, Graph.Definition, Replacement);
    Graph.Profile->GuaranteedGrants[0].AffixDefinition.SetAsset(Replacement);
    Graph.StandaloneGrant.AffixDefinition.SetAsset(Replacement);
    FMythicItemizationRegistryRefreshTestAccessor::NotifyResidentObjectsReplaced(
        *Registry, ReplacementMap);
    TestEqual(TEXT("resident replacement publishes one unavailable transition"),
              ChangeCount, 1);
    TestEqual(TEXT("replacement quarantine advances the semantic revision"),
              LastRevision, InitialRevision + 1);
    TestTrue(TEXT("replacement schedules one coalesced game-thread refresh"),
             FMythicItemizationRegistryRefreshTestAccessor::IsEditorRefreshPending(
                 *Registry));
    TestNull(TEXT("the raw old Affix Definition index is never exposed after replacement"),
             Registry->FindAffix(DefinitionId));
    TestFalse(TEXT("the old compiled profile is never served after replacement"),
              Registry->FindCompiledProfile(Graph.Profile->GetPrimaryAssetId()).IsValid());
    TestFalse(TEXT("the old compiled grant is never served after replacement"),
              Registry->FindCompiledGrant(Graph.StandaloneGrant).IsValid());

    // A second replacement notification in the same engine transaction is folded into the existing unavailable
    // state and pending refresh rather than exposing or publishing an intermediate graph.
    FMythicItemizationRegistryRefreshTestAccessor::NotifyResidentObjectsReplaced(
        *Registry, ReplacementMap);
    TestEqual(TEXT("repeated replacement notification does not rebroadcast quarantine"),
              ChangeCount, 1);
    TestTrue(TEXT("repeated replacement notification retains one pending refresh"),
             FMythicItemizationRegistryRefreshTestAccessor::IsEditorRefreshPending(
                 *Registry));

    FMythicItemizationRegistryRefreshTestAccessor::ProcessPendingEditorRefresh(*Registry);

    TestEqual(TEXT("replacement recovery publishes one committed transition"),
              ChangeCount, 2);
    TestEqual(TEXT("replacement recovery advances the semantic revision"),
              LastRevision, InitialRevision + 2);
    TestFalse(TEXT("replacement refresh is no longer pending after publication"),
              FMythicItemizationRegistryRefreshTestAccessor::IsEditorRefreshPending(
                  *Registry));
    TestTrue(TEXT("the primary index now resolves only the replacement object"),
             Registry->FindAffix(DefinitionId) == Replacement);

    const TSharedPtr<const FCompiledAffixProfile> RefreshedProfile =
        Registry->FindCompiledProfile(Graph.Profile->GetPrimaryAssetId());
    const TSharedPtr<const FCompiledAffixGrantClosure> RefreshedGrant =
        Registry->FindCompiledGrant(Graph.StandaloneGrant);
    TestTrue(TEXT("the profile closure is rebuilt after object replacement"),
             RefreshedProfile.IsValid() && RefreshedProfile != InitialProfile);
    TestTrue(TEXT("the standalone grant closure is rebuilt after object replacement"),
             RefreshedGrant.IsValid() && RefreshedGrant != InitialGrant);
    if (RefreshedProfile.IsValid() && !RefreshedProfile->GuaranteedGrants.IsEmpty()) {
        TestEqual(TEXT("the profile closure contains replacement gameplay semantics"),
                  RefreshedProfile->GuaranteedGrants[0].Affix.Definition.ModifierOp,
                  EGameplayModOp::DivideAdditive);
        TestTrue(TEXT("the profile closure owns the replacement typed reference"),
                 RefreshedProfile->GuaranteedGrants[0]
                         .Affix.Definition.Definition.GetAsset() == Replacement);
    }
    if (RefreshedGrant.IsValid()) {
        TestEqual(TEXT("the grant closure contains replacement gameplay semantics"),
                  RefreshedGrant->Affix.Definition.ModifierOp,
                  EGameplayModOp::DivideAdditive);
        TestTrue(TEXT("the grant cache canonicalizes its non-UObject source spec"),
                 RefreshedGrant->Spec.AffixDefinition.GetAsset() == Replacement);
    }

    Registry->OnSemanticDataChanged().Remove(ChangeHandle);
    return true;
}
#endif

#endif
