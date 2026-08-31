#include "Misc/AutomationTest.h"

#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Inventory/Fragments/Passive/HarvestToolFragment.h"
#include "Itemization/Inventory/Fragments/Passive/YieldQualityFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Misc/ConfigCacheIni.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "System/MythicAssetManager.h"
#include "World/Harvesting/MythicHarvestSettings.h"
#include "World/Harvesting/MythicHarvestParticipantSnapshot.h"
#include "World/Harvesting/MythicHarvestToolTypeDefinition.h"
#include "World/Harvesting/MythicHarvestTypes.h"
#include "World/Harvesting/MythicHarvestableDefinition.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestPersistentParticipantIdentityGateTest,
    "Mythic.Harvesting.Authority.PersistentParticipantIdentityGate",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestPersistentParticipantIdentityGateTest::RunTest(
    const FString &Parameters) {
    FString ResolvedKey = TEXT("must-be-cleared");
    TestFalse(
        TEXT("pre-load session identity cannot commit harvest work"),
        FMythicHarvestParticipantIdentityPolicy::TryResolveReadyContributorKey(
            FString(), TEXT("session:37"), TEXT("session:37"),
            ResolvedKey));
    TestTrue(TEXT("failed pre-load resolution clears its output"),
             ResolvedKey.IsEmpty());

    TestFalse(
        TEXT("loaded identity cannot commit before registry rekey completes"),
        FMythicHarvestParticipantIdentityPolicy::TryResolveReadyContributorKey(
            TEXT("character-37"), TEXT("character-37"),
            TEXT("session:37"), ResolvedKey));
    TestFalse(
        TEXT("stale canonical state cannot commit under a persistent registry key"),
        FMythicHarvestParticipantIdentityPolicy::TryResolveReadyContributorKey(
            TEXT("character-37"), TEXT("session:37"),
            TEXT("character-37"), ResolvedKey));

    TestTrue(
        TEXT("fully loaded and rekeyed persistent identity may commit"),
        FMythicHarvestParticipantIdentityPolicy::TryResolveReadyContributorKey(
            TEXT("character-37"), TEXT("character-37"),
            TEXT("character-37"), ResolvedKey));
    TestEqual(TEXT("authority resolves only the persistent character key"),
              ResolvedKey, FString(TEXT("character-37")));

    TestFalse(
        TEXT("a later mismatch fails closed instead of retaining a prior key"),
        FMythicHarvestParticipantIdentityPolicy::TryResolveReadyContributorKey(
            TEXT("character-37"), TEXT("character-37"),
            TEXT("character-other"), ResolvedKey));
    TestTrue(TEXT("failed revalidation clears the previously resolved key"),
             ResolvedKey.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestParticipantLedgerTest,
    "Mythic.Harvesting.Authority.CanonicalParticipantLedger",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestParticipantLedgerTest::RunTest(
    const FString &Parameters) {
    TMap<FString, FMythicHarvestParticipantSnapshot> Ledger;
    FMythicHarvestParticipantSnapshot FirstAcceptedWork;
    FirstAcceptedWork.ContributorKey = TEXT("canonical:player-a");
    FirstAcceptedWork.ContributionQuanta = 2500;
    FirstAcceptedWork.ItemLevel = 7;
    FirstAcceptedWork.QuantityMultiplierQuanta = 1000000;
    FirstAcceptedWork.ProficiencyLevel = 3;

    TestTrue(TEXT("first authoritative snapshot enters canonical ledger"),
             FMythicHarvestParticipantLedger::TryAccumulate(
                 Ledger, FirstAcceptedWork));

    FMythicHarvestParticipantSnapshot LatestAcceptedWork =
        FirstAcceptedWork;
    LatestAcceptedWork.ContributionQuanta = 7500;
    LatestAcceptedWork.ItemLevel = 8;
    LatestAcceptedWork.QuantityMultiplierQuanta = 1250000;
    LatestAcceptedWork.ProficiencyLevel = 4;
    TestTrue(TEXT("same canonical player accumulates without a duplicate row"),
             FMythicHarvestParticipantLedger::TryAccumulate(
                 Ledger, LatestAcceptedWork));
    TestEqual(TEXT("canonical identity owns exactly one participant row"),
              Ledger.Num(), 1);
    const FMythicHarvestParticipantSnapshot *Combined =
        Ledger.Find(TEXT("canonical:player-a"));
    TestNotNull(TEXT("canonical participant remains addressable"), Combined);
    if (Combined) {
        TestEqual(TEXT("accepted work accumulates with checked integer math"),
                  Combined->ContributionQuanta,
                  static_cast<int64>(10000));
        TestEqual(TEXT("latest accepted hit refreshes frozen reward inputs"),
                  Combined->QuantityMultiplierQuanta, 1250000);
    }

    // A null weak controller models disconnect after accepted work. Eligibility
    // must be driven by the canonical snapshot, never UObject lifetime.
    TArray<FMythicHarvestParticipantSnapshot> Eligible;
    int64 TotalEligibleQuanta = 0;
    TestTrue(TEXT("disconnect does not invalidate frozen entitlement"),
             FMythicHarvestParticipantLedger::BuildEligibleSnapshots(
                 Ledger, 5000, Eligible, &TotalEligibleQuanta));
    TestEqual(TEXT("disconnected contributor remains eligible"),
              Eligible.Num(), 1);
    if (Eligible.Num() == 1) {
        TestFalse(TEXT("test contributor has no live-controller dependency"),
                  Eligible[0].CurrentController.IsValid());
    }
    TestEqual(TEXT("eligible contribution remains exact"),
              TotalEligibleQuanta, static_cast<int64>(10000));

    TMap<FString, FMythicHarvestParticipantSnapshot> TamperedLedger;
    TamperedLedger.Add(TEXT("canonical:wrong-map-key"),
                       FirstAcceptedWork);
    TestFalse(TEXT("map key/payload identity mismatch fails closed"),
              FMythicHarvestParticipantLedger::BuildEligibleSnapshots(
                  TamperedLedger, 1, Eligible));

    TMap<FString, FMythicHarvestParticipantSnapshot> OverflowLedger;
    FMythicHarvestParticipantSnapshot Maximum = FirstAcceptedWork;
    Maximum.ContributionQuanta = MAX_int64;
    TestTrue(TEXT("maximum representable contribution can be staged"),
             FMythicHarvestParticipantLedger::TryAccumulate(
                 OverflowLedger, Maximum));
    FMythicHarvestParticipantSnapshot OneMore = FirstAcceptedWork;
    OneMore.ContributionQuanta = 1;
    TestFalse(TEXT("contribution overflow fails without mutating ledger"),
              FMythicHarvestParticipantLedger::TryAccumulate(
                  OverflowLedger, OneMore));
    const FMythicHarvestParticipantSnapshot *Preserved =
        OverflowLedger.Find(TEXT("canonical:player-a"));
    TestNotNull(TEXT("overflow cannot remove the prior snapshot"), Preserved);
    if (Preserved) {
        TestEqual(TEXT("failed accumulation preserves prior exact value"),
                  Preserved->ContributionQuanta, MAX_int64);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicTypedHarvestObjectiveMatchTest,
    "Mythic.Harvesting.Foundation.TypedObjectiveReference",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicTypedHarvestObjectiveMatchTest::RunTest(
    const FString &Parameters) {
    UMythicHarvestableDefinition *Tree =
        NewObject<UMythicHarvestableDefinition>();
    UMythicHarvestableDefinition *Rock =
        NewObject<UMythicHarvestableDefinition>();
    UObjectiveDefinition *Objective = NewObject<UObjectiveDefinition>();
    Objective->RequiredHarvestableDefinition = Tree;

    TestTrue(TEXT("exact direct harvestable reference matches"),
             UObjectiveTracker::MatchesHarvestableDefinition(Objective,
                                                              Tree));
    TestFalse(TEXT("different direct harvestable does not match"),
              UObjectiveTracker::MatchesHarvestableDefinition(Objective,
                                                               Rock));
    TestFalse(TEXT("null objective fails closed"),
              UObjectiveTracker::MatchesHarvestableDefinition(nullptr,
                                                               Tree));
    TestFalse(TEXT("null harvestable fails closed"),
              UObjectiveTracker::MatchesHarvestableDefinition(Objective,
                                                               nullptr));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicHarvestFixedWorkTest, "Mythic.Harvesting.Foundation.FixedWork",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestFixedWorkTest::RunTest(const FString &Parameters) {
    FMythicHarvestWork Work;
    TestTrue(TEXT("finite fractional work quantizes"), FMythicHarvestWork::TryFromWorkUnits(1.2345, Work));
    TestEqual(TEXT("one work unit owns exactly 10,000 quanta"), Work.GetQuanta(), static_cast<int64>(12345));
    TestEqual(TEXT("quantized work round-trips"), Work.ToWorkUnits(), 1.2345);

    FMythicHarvestWork One;
    TestTrue(TEXT("one unit quantizes"), FMythicHarvestWork::TryFromWorkUnits(1.0, One));
    TestEqual(TEXT("clamped subtraction cannot become negative"), One.SubtractClamped(Work).GetQuanta(), static_cast<int64>(0));
    TestEqual(TEXT("minimum uses exact quanta"), FMythicHarvestWork::Min(One, Work).GetQuanta(), One.GetQuanta());

    TestFalse(TEXT("negative work fails closed"), FMythicHarvestWork::TryFromWorkUnits(-1.0, Work));
    TestFalse(TEXT("nonfinite work fails closed"), FMythicHarvestWork::TryFromWorkUnits(std::numeric_limits<double>::quiet_NaN(), Work));
    TestFalse(TEXT("overflowing work fails closed"), FMythicHarvestWork::TryFromWorkUnits(std::numeric_limits<double>::max(), Work));

    const double QuarterCompletionXP =
        FMythicHarvestContributionMath::CalculateProportionalShare(
            2500, 10000, 120.0);
    const double ThreeQuarterCompletionXP =
        FMythicHarvestContributionMath::CalculateProportionalShare(
            7500, 10000, 120.0);
    TestEqual(TEXT("completion XP follows exact applied-work proportion"),
              QuarterCompletionXP, 30.0);
    TestEqual(TEXT("proportional completion XP conserves the authored pool"),
              QuarterCompletionXP + ThreeQuarterCompletionXP, 120.0);
    TestEqual(TEXT("invalid proportional contribution fails closed"),
              FMythicHarvestContributionMath::CalculateProportionalShare(
                  10001, 10000, 120.0),
              0.0);
    TestFalse(TEXT("untouched available nodes are implicit across streaming"),
              FMythicHarvestStreamingPolicy::ShouldRetainDetachedNode(
                  EMythicHarvestNodeState::Available, 1, false));
    TestTrue(TEXT("partial generation-one work survives provider streaming"),
             FMythicHarvestStreamingPolicy::ShouldRetainDetachedNode(
                 EMythicHarvestNodeState::Available, 1, true));
    TestFalse(TEXT("untouched advanced generations reconstruct from durable high-water"),
              FMythicHarvestStreamingPolicy::ShouldRetainDetachedNode(
                  EMythicHarvestNodeState::Available, 2, false));
    TestTrue(TEXT("unavailable generation-one lifecycle remains durable"),
             FMythicHarvestStreamingPolicy::ShouldRetainDetachedNode(
                 EMythicHarvestNodeState::Depleted, 1, false));

    double AttackCycleExpiry = 0.0;
    TestTrue(TEXT("attack cadence derives one finite server expiry"),
             FMythicHarvestCadencePolicy::TryCalculateExpiry(
                 10.0, 2.0, 2.0, 0.075, AttackCycleExpiry));
    TestTrue(TEXT("cadence tolerance participates in expiry"),
             FMath::IsNearlyEqual(AttackCycleExpiry, 11.075));
    TestFalse(TEXT("attack cycle remains valid at its exact boundary"),
              FMythicHarvestCadencePolicy::IsExpired(
                  AttackCycleExpiry, AttackCycleExpiry));
    TestTrue(TEXT("attack cycle rejects replay after its boundary"),
             FMythicHarvestCadencePolicy::IsExpired(
                 AttackCycleExpiry + 0.001, AttackCycleExpiry));
    TestFalse(TEXT("nonfinite cadence cannot mint provenance"),
              FMythicHarvestCadencePolicy::TryCalculateExpiry(
                  10.0, 2.0,
                  std::numeric_limits<double>::quiet_NaN(), 0.075,
                  AttackCycleExpiry));

    const FMythicHarvestRequest EmptyRequest;
    TestEqual(TEXT("request generation is explicit and fail-closed"), EmptyRequest.ExpectedGeneration, static_cast<uint32>(0));
    TestFalse(TEXT("default attack-cycle provenance is invalid"), EmptyRequest.AttackCycleToken.IsValid());

    const FMythicHarvestResult EmptyResult;
    TestEqual(TEXT("default transaction outcome rejects"), EmptyResult.Outcome, EMythicHarvestOutcome::Rejected);
    TestEqual(TEXT("default transaction reason reports world readiness"), EmptyResult.RejectReason, EMythicHarvestRejectReason::WorldNotReady);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicHarvestDefinitionValidationTest, "Mythic.Harvesting.Foundation.DefinitionValidation",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestDefinitionValidationTest::RunTest(const FString &Parameters) {
    UMythicHarvestToolTypeDefinition *ToolType = NewObject<UMythicHarvestToolTypeDefinition>(GetTransientPackage(), TEXT("TestAxeType"));
    TestNotNull(TEXT("tool type can be constructed"), ToolType);
    TestEqual(TEXT("tool type uses exact primary asset type"), ToolType->GetPrimaryAssetId().PrimaryAssetType,
              UMythicAssetManager::HarvestToolTypeDefinitionType);

    TArray<FText> Errors;
    TestFalse(TEXT("empty tool type is invalid"), ToolType->AppendValidationErrors(Errors));
    ToolType->DisplayName = FText::FromString(TEXT("Axe"));
    Errors.Reset();
    TestTrue(TEXT("presentable tool type is valid"), ToolType->AppendValidationErrors(Errors));

    UMythicHarvestableDefinition *Harvestable = NewObject<UMythicHarvestableDefinition>(GetTransientPackage(), TEXT("TestTree"));
    TestNotNull(TEXT("harvestable can be constructed"), Harvestable);
    TestEqual(TEXT("harvestable uses exact primary asset type"), Harvestable->GetPrimaryAssetId().PrimaryAssetType,
              UMythicAssetManager::HarvestableDefinitionType);

    Harvestable->DisplayName = FText::FromString(TEXT("Tree"));
    Harvestable->HarvestVerb = FText::FromString(TEXT("Chop"));
    Harvestable->RequiredToolType = ToolType;
    Harvestable->MinimumToolTier = 1;
    Harvestable->MaxWork = 3.75f;
    Harvestable->ProficiencyDefinition = NewObject<UProficiencyDefinition>(Harvestable);

    FMythicHarvestRewardEntry Material;
    Material.ItemDefinition = NewObject<UItemDefinition>(Harvestable);
    Material.MinQuantity = 1;
    Material.MaxQuantity = 3;
    Harvestable->PrimaryMaterials.Add(Material);

    Errors.Reset();
    TestTrue(TEXT("valid direct-reference harvestable passes"), Harvestable->AppendValidationErrors(Errors));

    Harvestable->RequiredToolType = nullptr;
    Errors.Reset();
    TestFalse(TEXT("missing launch tool family fails closed"), Harvestable->AppendValidationErrors(Errors));
    Harvestable->MinimumToolTier = 0;
    Errors.Reset();
    TestFalse(TEXT("tier zero still requires exact tool provenance"), Harvestable->AppendValidationErrors(Errors));

    Harvestable->RequiredToolType = ToolType;
    Harvestable->PrimaryMaterials[0].QualityPolicy =
        EMythicHarvestRewardQualityPolicy::Fixed;
    Harvestable->PrimaryMaterials[0].FixedQuality =
        EMythicYieldQuality::Fine;
    Errors.Reset();
    TestFalse(TEXT("quality override requires an exact quality fragment"),
              Harvestable->AppendValidationErrors(Errors));
    Material.ItemDefinition->Fragments.Add(
        NewObject<UYieldQualityFragment>(Material.ItemDefinition));
    Errors.Reset();
    TestTrue(TEXT("fixed quality becomes valid with one direct fragment"),
             Harvestable->AppendValidationErrors(Errors));
    Harvestable->PrimaryMaterials[0].FixedQuality =
        EMythicYieldQuality::Ragged;
    Errors.Reset();
    TestFalse(TEXT("harvesting cannot mint hunting-only Ragged quality"),
              Harvestable->AppendValidationErrors(Errors));
    Harvestable->PrimaryMaterials[0].FixedQuality =
        EMythicYieldQuality::Fine;

    Harvestable->BonusLoot.Add(Material);
    Errors.Reset();
    TestFalse(TEXT("duplicate direct reward item is rejected across channels"), Harvestable->AppendValidationErrors(Errors));
    return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicHarvestToolFragmentValidationTest, "Mythic.Harvesting.Foundation.ToolFragmentComposition",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestToolFragmentValidationTest::RunTest(const FString &Parameters) {
    UItemDefinition *Definition = NewObject<UItemDefinition>(GetTransientPackage());
    UMythicHarvestToolTypeDefinition *ToolType = NewObject<UMythicHarvestToolTypeDefinition>(Definition);
    ToolType->DisplayName = FText::FromString(TEXT("Pickaxe"));

    UHarvestToolFragment *Harvest = NewObject<UHarvestToolFragment>(Definition);
    Harvest->ToolType = ToolType;
    Harvest->BaseWork = 1.25f;
    UDurabilityFragment *Durability = NewObject<UDurabilityFragment>(Definition);
    UAttackFragment *Attack = NewObject<UAttackFragment>(Definition);
    Definition->Fragments = {Harvest, Attack, Durability};

    FText Error;
    TestTrue(TEXT("same-item harvest/attack/durability composition passes"), Harvest->IsValidFragment(Error));

    // The tool grants the harvesting swing itself, so a weaponless player can still work a node.
    Definition->Fragments = {Harvest, Durability};
    TestFalse(TEXT("a tool with no Attack Fragment fails closed"), Harvest->IsValidFragment(Error));
    TestTrue(TEXT("the rejection names the missing Attack Fragment"),
             Error.ToString().Contains(TEXT("Attack Fragment")));
    Definition->Fragments = {Harvest, Attack, Durability};
    TestTrue(TEXT("native live-item detector is null-safe"), UHarvestToolFragment::FindOnItem(nullptr) == nullptr);
    TestTrue(TEXT("native definition detector returns exact fragment"), UHarvestToolFragment::FindOnDefinition(Definition) == Harvest);

    Durability->DurabilityConfig.MaxDurability = 0;
    TestFalse(TEXT("harvest tools require positive definition-owned durability"), Harvest->IsValidFragment(Error));
    Durability->DurabilityConfig.MaxDurability = 100;
    TestTrue(TEXT("restoring valid durability composition passes"), Harvest->IsValidFragment(Error));

    Definition->Fragments.Remove(Durability);
    TestFalse(TEXT("missing exact durability sibling fails closed"), Harvest->IsValidFragment(Error));
    return true;
}
#endif

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicHarvestSettingsValidationTest, "Mythic.Harvesting.Foundation.SettingsValidation",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestSettingsValidationTest::RunTest(const FString &Parameters) {
    UMythicHarvestSettings *Settings = NewObject<UMythicHarvestSettings>(GetTransientPackage());
    TArray<FText> Errors;
    TestTrue(TEXT("default cross-cutting settings are valid"), Settings->AppendValidationErrors(Errors));
    double ClampedWorkMultiplier = 0.0;
    TestTrue(TEXT("settings own the canonical work multiplier clamp"),
             Settings->TryClampHarvestWorkMultiplier(
                 Settings->MaximumWorkMultiplier + 50.0,
                 ClampedWorkMultiplier));
    TestEqual(TEXT("canonical work clamp applies the authored maximum"),
              ClampedWorkMultiplier,
              static_cast<double>(Settings->MaximumWorkMultiplier));
    TestFalse(TEXT("canonical work clamp rejects nonfinite GAS values"),
              Settings->TryClampHarvestWorkMultiplier(
                  std::numeric_limits<double>::quiet_NaN(),
                  ClampedWorkMultiplier));
    TestEqual(TEXT("failed work clamp clears its output"),
              ClampedWorkMultiplier, 0.0);

    Settings->FocusRangeCentimeters = Settings->AuthoritativeRangeCentimeters + 1.0f;
    Errors.Reset();
    TestFalse(TEXT("predictive focus cannot exceed authority range"), Settings->AppendValidationErrors(Errors));

    Settings->FocusRangeCentimeters = Settings->AuthoritativeRangeCentimeters;
    Settings->RewardOutboxRetryIntervalSeconds = 0.0f;
    Errors.Reset();
    TestFalse(TEXT("reward retry cadence must be positive"), Settings->AppendValidationErrors(Errors));

    Settings->RewardOutboxRetryIntervalSeconds = 2.0f;
    Settings->RewardOutboxGrantBudget = 257;
    Errors.Reset();
    TestFalse(TEXT("reward retry work remains bounded"), Settings->AppendValidationErrors(Errors));

    Settings->RewardOutboxGrantBudget = 8;
    const float ValidReplicationCull =
        Settings->ReplicationCullDistanceCentimeters;
    Settings->ReplicationCullDistanceCentimeters =
        Settings->ReplicationGridSizeCentimeters * 0.5f;
    Errors.Reset();
    TestFalse(TEXT("spatial replication cull covers every cell corner plus its margin"),
              Settings->AppendValidationErrors(Errors));
    Settings->ReplicationCullDistanceCentimeters = ValidReplicationCull;

    Settings->RestoreMaximumReplicationCells = 0;
    Errors.Reset();
    TestFalse(TEXT("deployment restore proxy budget must be positive"),
              Settings->AppendValidationErrors(Errors));
    Settings->RestoreMaximumReplicationCells = 4096;

    const float ValidMaximumWorkMultiplier =
        Settings->MaximumWorkMultiplier;
    Settings->MaximumWorkMultiplier =
        Settings->MinimumWorkMultiplier * 0.5f;
    TestFalse(TEXT("canonical work clamp rejects invalid designer bounds"),
              Settings->TryClampHarvestWorkMultiplier(
                  1.0, ClampedWorkMultiplier));
    Settings->MaximumWorkMultiplier = ValidMaximumWorkMultiplier;

    const float ValidMinimumWorkMultiplier =
        Settings->MinimumWorkMultiplier;
    Settings->MinimumWorkMultiplier = 0.0f;
    Errors.Reset();
    TestFalse(TEXT("zero minimum work multiplier cannot author no-op hits"),
              Settings->AppendValidationErrors(Errors));
    TestFalse(TEXT("canonical work clamp fails closed for a zero minimum"),
              Settings->TryClampHarvestWorkMultiplier(
                  1.0, ClampedWorkMultiplier));
    Settings->MinimumWorkMultiplier = ValidMinimumWorkMultiplier;
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicHarvestPrimaryAssetScanTest, "Mythic.Harvesting.Foundation.PrimaryAssetScans",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestPrimaryAssetScanTest::RunTest(const FString &Parameters) {
    TArray<FString> ScanRules;
    GConfig->GetArray(TEXT("/Script/Engine.AssetManagerSettings"), TEXT("PrimaryAssetTypesToScan"), ScanRules, GGameIni);

    const auto HasAlwaysCookRule = [&ScanRules](const TCHAR *TypeName, const TCHAR *Directory) {
        return ScanRules.ContainsByPredicate([TypeName, Directory](const FString &Rule) {
            return Rule.Contains(FString::Printf(TEXT("PrimaryAssetType=\"%s\""), TypeName), ESearchCase::CaseSensitive) &&
                Rule.Contains(Directory, ESearchCase::CaseSensitive) && Rule.Contains(TEXT("bApplyRecursively=True"), ESearchCase::CaseSensitive) &&
                Rule.Contains(TEXT("CookRule=AlwaysCook"), ESearchCase::CaseSensitive);
        });
    };

    TestTrue(TEXT("HarvestToolTypeDefinition recursively AlwaysCooks"),
             HasAlwaysCookRule(TEXT("HarvestToolTypeDefinition"), TEXT("/Game/Mythic/World/Harvesting/ToolTypes")));
    TestTrue(TEXT("HarvestableDefinition recursively AlwaysCooks"),
             HasAlwaysCookRule(TEXT("HarvestableDefinition"), TEXT("/Game/Mythic/World/Harvesting/Harvestables")));
    TestTrue(TEXT("ProficiencyDefinition recursively AlwaysCooks"),
             HasAlwaysCookRule(TEXT("ProficiencyDefinition"),
                               TEXT("/Game/Mythic/Proficiencies")));

    const UProficiencyDefinition *Proficiency =
        NewObject<UProficiencyDefinition>(
            GetTransientPackage(), TEXT("HarvestPrimaryAssetTest"));
    TestEqual(TEXT("proficiency reports the exact registered primary type"),
              Proficiency->GetPrimaryAssetId().PrimaryAssetType,
              UMythicAssetManager::ProficiencyDefinitionType);
    return true;
}
