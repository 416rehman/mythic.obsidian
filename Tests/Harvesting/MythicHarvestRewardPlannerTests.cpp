#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/Fragments/Passive/YieldQualityFragment.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "System/MythicAssetManager.h"
#include "World/Harvesting/MythicHarvestRewardOutboxSubsystem.h"
#include "World/Harvesting/MythicHarvestRewardPlanner.h"

#include <limits>

namespace MythicHarvestRewardPlannerTestsPrivate {

FMythicHarvestRewardParticipant MakeParticipant(
    const TCHAR *ContributorKey, const int64 Weight,
    const double QuantityMultiplier = 1.0,
    const int32 ProficiencyLevel = 0) {
    FMythicHarvestRewardParticipant Participant;
    Participant.ContributorKey = ContributorKey;
    Participant.ContributionQuanta = Weight;
    Participant.ItemLevel = 7;
    verify(FMythicHarvestRewardPlanner::TryQuantizeQuantityMultiplier(
        QuantityMultiplier, Participant.QuantityMultiplierQuanta));
    Participant.ProficiencyLevel = ProficiencyLevel;
    return Participant;
}

const FMythicHarvestPlannedRewardGrant *FindGrant(
    const TArray<FMythicHarvestPlannedRewardGrant> &Grants,
    const FString &ContributorKey,
    const EMythicHarvestRewardChannel Channel =
        EMythicHarvestRewardChannel::PrimaryMaterial) {
    return Grants.FindByPredicate(
        [&ContributorKey, Channel](
            const FMythicHarvestPlannedRewardGrant &Grant) {
            return Grant.ContributorKey == ContributorKey
                && Grant.Channel == Channel;
        });
}

} // namespace MythicHarvestRewardPlannerTestsPrivate

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestRewardLargestRemainderTest,
    "Mythic.Harvesting.Rewards.LargestRemainder",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestRewardLargestRemainderTest::RunTest(
    const FString &Parameters) {
    using namespace MythicHarvestRewardPlannerTestsPrivate;

    const TArray<FMythicHarvestRewardParticipant> Unsorted = {
        MakeParticipant(TEXT("char-c"), 1),
        MakeParticipant(TEXT("char-a"), 1),
        MakeParticipant(TEXT("char-b"), 1),
    };
    TArray<int32> Shares;
    TArray<int32> SourceIndices;
    TestTrue(TEXT("equal weights split exactly"),
             FMythicHarvestRewardPlanner::SplitQuantityLargestRemainder(
                 2, Unsorted, Shares, &SourceIndices));
    TestEqual(TEXT("canonical A receives first tie unit"), Shares[0], 1);
    TestEqual(TEXT("canonical B receives second tie unit"), Shares[1], 1);
    TestEqual(TEXT("canonical C receives no unit"), Shares[2], 0);
    TestEqual(TEXT("canonical A maps to original index"), SourceIndices[0], 1);
    TestEqual(TEXT("canonical B maps to original index"), SourceIndices[1], 2);
    TestEqual(TEXT("canonical C maps to original index"), SourceIndices[2], 0);

    const TArray<FMythicHarvestRewardParticipant> HugeWeights = {
        MakeParticipant(TEXT("char-a"), MAX_int64),
        MakeParticipant(TEXT("char-b"), MAX_int64),
    };
    TestTrue(TEXT("95-bit numerator path splits without overflow"),
             FMythicHarvestRewardPlanner::SplitQuantityLargestRemainder(
                 3, HugeWeights, Shares));
    TestEqual(TEXT("stable tie gives A two"), Shares[0], 2);
    TestEqual(TEXT("stable tie gives B one"), Shares[1], 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestRewardQuantityChannelTest,
    "Mythic.Harvesting.Rewards.MaterialQuantityStatIsolation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestRewardQuantityChannelTest::RunTest(
    const FString &Parameters) {
    using namespace MythicHarvestRewardPlannerTestsPrivate;

    UMythicHarvestableDefinition *Definition =
        NewObject<UMythicHarvestableDefinition>();
    UItemDefinition *Material = NewObject<UItemDefinition>(
        GetTransientPackage(), TEXT("HarvestQuantityMaterial"));
    UItemDefinition *Bonus = NewObject<UItemDefinition>(
        GetTransientPackage(), TEXT("HarvestQuantityBonus"));
    FMythicHarvestRewardEntry &MaterialEntry =
        Definition->PrimaryMaterials.AddDefaulted_GetRef();
    MaterialEntry.ItemDefinition = Material;
    MaterialEntry.MinQuantity = 10;
    MaterialEntry.MaxQuantity = 10;
    FMythicHarvestRewardEntry &BonusEntry =
        Definition->BonusLoot.AddDefaulted_GetRef();
    BonusEntry.ItemDefinition = Bonus;
    BonusEntry.MinQuantity = 10;
    BonusEntry.MaxQuantity = 10;

    const TArray<FMythicHarvestRewardParticipant> Participants = {
        MakeParticipant(TEXT("char-b"), 1, 1.0),
        MakeParticipant(TEXT("char-a"), 1, 2.0),
    };
    const FMythicHarvestRewardPlanResult Plan =
        FMythicHarvestRewardPlanner::PlanCompletion(
            *Definition, FGuid(1, 3, 5, 7),
            FMythicHarvestNodeId(FGuid(2, 4, 6, 8)), 1,
            FMythicYieldQualityRules(), Participants);
    TestTrue(TEXT("quantity-channel plan succeeds"), Plan.IsSuccess());

    const FMythicHarvestPlannedRewardGrant *MaterialA = FindGrant(
        Plan.Grants, TEXT("char-a"),
        EMythicHarvestRewardChannel::PrimaryMaterial);
    const FMythicHarvestPlannedRewardGrant *MaterialB = FindGrant(
        Plan.Grants, TEXT("char-b"),
        EMythicHarvestRewardChannel::PrimaryMaterial);
    const FMythicHarvestPlannedRewardGrant *BonusA = FindGrant(
        Plan.Grants, TEXT("char-a"),
        EMythicHarvestRewardChannel::BonusLoot);
    const FMythicHarvestPlannedRewardGrant *BonusB = FindGrant(
        Plan.Grants, TEXT("char-b"),
        EMythicHarvestRewardChannel::BonusLoot);
    if (!TestNotNull(TEXT("material A exists"), MaterialA)
        || !TestNotNull(TEXT("material B exists"), MaterialB)
        || !TestNotNull(TEXT("bonus A exists"), BonusA)
        || !TestNotNull(TEXT("bonus B exists"), BonusB)) {
        return false;
    }
    TestEqual(TEXT("A's 2x ItemQuantityFind applies to A's material entitlement"),
              MaterialA->Quantity, 10);
    TestEqual(TEXT("B's baseline material entitlement remains baseline"),
              MaterialB->Quantity, 5);
    TestEqual(TEXT("ItemQuantityFind never multiplies bonus loot for A"),
              BonusA->Quantity, 5);
    TestEqual(TEXT("ItemQuantityFind never multiplies bonus loot for B"),
              BonusB->Quantity, 5);

    int32 Quanta = 0;
    TestTrue(TEXT("finite multiplier quantizes"),
             FMythicHarvestRewardPlanner::TryQuantizeQuantityMultiplier(
                 1.25, Quanta));
    TestEqual(TEXT("quantity multiplier uses fixed millionths"), Quanta,
              1250000);
    TestFalse(TEXT("negative quantity multiplier fails closed"),
              FMythicHarvestRewardPlanner::TryQuantizeQuantityMultiplier(
                  -0.01, Quanta));
    TestFalse(TEXT("nonfinite quantity multiplier fails closed"),
              FMythicHarvestRewardPlanner::TryQuantizeQuantityMultiplier(
                  std::numeric_limits<double>::quiet_NaN(), Quanta));
    TestFalse(TEXT("arithmetic-ceiling violation fails instead of clamping"),
              FMythicHarvestRewardPlanner::TryQuantizeQuantityMultiplier(
                  FMythicHarvestRewardPlanner::MaximumQuantityMultiplier
                      + 0.01,
                  Quanta));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestRewardResolvedQualityTest,
    "Mythic.Harvesting.Rewards.QualityFrozenAtPlanTime",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestRewardResolvedQualityTest::RunTest(
    const FString &Parameters) {
    using namespace MythicHarvestRewardPlannerTestsPrivate;

    UMythicHarvestableDefinition *Definition =
        NewObject<UMythicHarvestableDefinition>();
    UItemDefinition *FixedItem = NewObject<UItemDefinition>(
        GetTransientPackage(), TEXT("HarvestFixedQualityItem"));
    FixedItem->Fragments.Add(NewObject<UYieldQualityFragment>(FixedItem));
    UItemDefinition *ProficiencyItem = NewObject<UItemDefinition>(
        GetTransientPackage(), TEXT("HarvestProficiencyQualityItem"));
    ProficiencyItem->Fragments.Add(
        NewObject<UYieldQualityFragment>(ProficiencyItem));

    FMythicHarvestRewardEntry &Fixed =
        Definition->PrimaryMaterials.AddDefaulted_GetRef();
    Fixed.ItemDefinition = FixedItem;
    Fixed.QualityPolicy = EMythicHarvestRewardQualityPolicy::Fixed;
    Fixed.FixedQuality = EMythicYieldQuality::Fine;
    FMythicHarvestRewardEntry &FromProficiency =
        Definition->BonusLoot.AddDefaulted_GetRef();
    FromProficiency.ItemDefinition = ProficiencyItem;
    FromProficiency.QualityPolicy =
        EMythicHarvestRewardQualityPolicy::ContributorProficiency;

    FMythicYieldQualityRules Rules;
    Rules.BaseFineChance = 0.0f;
    Rules.FineChancePerMasteryLevel = 0.0f;
    Rules.MaxFineChance = 0.0f;
    Rules.BasePristineChance = 1.0f;
    Rules.PristineChancePerMasteryLevel = 0.0f;
    Rules.MaxPristineChance = 1.0f;

    const TArray<FMythicHarvestRewardParticipant> Participants = {
        MakeParticipant(TEXT("char-quality"), 1, 1.0, 42),
    };
    const FMythicHarvestRewardPlanResult Plan =
        FMythicHarvestRewardPlanner::PlanCompletion(
            *Definition, FGuid(11, 12, 13, 14),
            FMythicHarvestNodeId(FGuid(21, 22, 23, 24)), 3,
            Rules, Participants);
    TestTrue(TEXT("quality plan succeeds"), Plan.IsSuccess());
    const FMythicHarvestPlannedRewardGrant *FixedGrant = FindGrant(
        Plan.Grants, TEXT("char-quality"),
        EMythicHarvestRewardChannel::PrimaryMaterial);
    const FMythicHarvestPlannedRewardGrant *ProficiencyGrant = FindGrant(
        Plan.Grants, TEXT("char-quality"),
        EMythicHarvestRewardChannel::BonusLoot);
    if (!TestNotNull(TEXT("fixed quality grant exists"), FixedGrant)
        || !TestNotNull(TEXT("proficiency quality grant exists"),
                        ProficiencyGrant)) {
        return false;
    }
    TestTrue(TEXT("fixed policy becomes a frozen construction override"),
             FixedGrant->bHasResolvedQuality);
    TestEqual(TEXT("fixed outcome is frozen as Fine"),
              FixedGrant->ResolvedQuality, EMythicYieldQuality::Fine);
    TestTrue(TEXT("proficiency policy becomes a frozen construction override"),
             ProficiencyGrant->bHasResolvedQuality);
    TestEqual(TEXT("forced plan-time roll is frozen as Pristine"),
              ProficiencyGrant->ResolvedQuality,
              EMythicYieldQuality::Pristine);

    UMythicHarvestableDefinition *DefaultDefinition =
        NewObject<UMythicHarvestableDefinition>();
    UItemDefinition *DefaultItem = NewObject<UItemDefinition>(
        GetTransientPackage(), TEXT("HarvestDefaultQualityItem"));
    UYieldQualityFragment *DefaultQuality =
        NewObject<UYieldQualityFragment>(DefaultItem);
    DefaultQuality->QualityTier = EMythicYieldQuality::Fine;
    DefaultItem->Fragments.Add(DefaultQuality);
    FMythicHarvestRewardEntry &DefaultEntry =
        DefaultDefinition->PrimaryMaterials.AddDefaulted_GetRef();
    DefaultEntry.ItemDefinition = DefaultItem;
    const FMythicHarvestRewardPlanResult DefaultPlan =
        FMythicHarvestRewardPlanner::PlanCompletion(
            *DefaultDefinition, FGuid(31, 32, 33, 34),
            FMythicHarvestNodeId(FGuid(41, 42, 43, 44)), 1,
            Rules, Participants);
    TestTrue(TEXT("definition-default quality plan succeeds"),
             DefaultPlan.IsSuccess());
    const FMythicHarvestPlannedRewardGrant *DefaultGrant = FindGrant(
        DefaultPlan.Grants, TEXT("char-quality"));
    if (!TestNotNull(TEXT("definition-default grant exists"), DefaultGrant)) {
        return false;
    }
    TestTrue(TEXT("definition default becomes a frozen construction override"),
             DefaultGrant->bHasResolvedQuality);
    TestEqual(TEXT("definition default is captured as Fine"),
              DefaultGrant->ResolvedQuality, EMythicYieldQuality::Fine);
    DefaultQuality->QualityTier = EMythicYieldQuality::Pristine;
    TestEqual(TEXT("later definition mutation cannot drift an already-planned grant"),
              DefaultGrant->ResolvedQuality, EMythicYieldQuality::Fine);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestRewardPlanDeterminismTest,
    "Mythic.Harvesting.Rewards.PlanDeterminism",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestRewardPlanDeterminismTest::RunTest(
    const FString &Parameters) {
    using namespace MythicHarvestRewardPlannerTestsPrivate;

    UMythicHarvestableDefinition *Definition =
        NewObject<UMythicHarvestableDefinition>();
    UItemDefinition *ItemDefinition = NewObject<UItemDefinition>(
        GetTransientPackage(), TEXT("HarvestRewardPlannerTestItem"));
    FMythicHarvestRewardEntry &Entry =
        Definition->PrimaryMaterials.AddDefaulted_GetRef();
    Entry.ItemDefinition = ItemDefinition;
    Entry.MinQuantity = 5;
    Entry.MaxQuantity = 5;
    Entry.Probability = 1.0f;
    Entry.SelectionWeight = 1.0f;

    const FMythicHarvestNodeId NodeId(
        FGuid(0x10203040, 0x50607080, 0x90a0b0c0, 0xd0e0f001));
    const FGuid WorldEpoch(0xabcddcba, 0x10293847, 0x56473829,
                           0x91827364);
    const TArray<FMythicHarvestRewardParticipant> FirstOrder = {
        MakeParticipant(TEXT("char-b"), 1),
        MakeParticipant(TEXT("char-a"), 2),
    };
    const TArray<FMythicHarvestRewardParticipant> SecondOrder = {
        MakeParticipant(TEXT("char-a"), 2),
        MakeParticipant(TEXT("char-b"), 1),
    };

    const FMythicHarvestRewardPlanResult First =
        FMythicHarvestRewardPlanner::PlanCompletion(
            *Definition, WorldEpoch, NodeId, 11,
            FMythicYieldQualityRules(), FirstOrder);
    const FMythicHarvestRewardPlanResult Second =
        FMythicHarvestRewardPlanner::PlanCompletion(
            *Definition, WorldEpoch, NodeId, 11,
            FMythicYieldQualityRules(), SecondOrder);
    TestTrue(TEXT("first plan succeeds"), First.IsSuccess());
    TestTrue(TEXT("reordered plan succeeds"), Second.IsSuccess());
    TestEqual(TEXT("both contributors receive one immutable grant"),
              First.Grants.Num(), 2);
    TestEqual(TEXT("reorder preserves grant count"), Second.Grants.Num(), 2);

    const FMythicHarvestPlannedRewardGrant *FirstA =
        FindGrant(First.Grants, TEXT("char-a"));
    const FMythicHarvestPlannedRewardGrant *FirstB =
        FindGrant(First.Grants, TEXT("char-b"));
    const FMythicHarvestPlannedRewardGrant *SecondA =
        FindGrant(Second.Grants, TEXT("char-a"));
    const FMythicHarvestPlannedRewardGrant *SecondB =
        FindGrant(Second.Grants, TEXT("char-b"));
    TestNotNull(TEXT("first A grant exists"), FirstA);
    TestNotNull(TEXT("first B grant exists"), FirstB);
    TestNotNull(TEXT("second A grant exists"), SecondA);
    TestNotNull(TEXT("second B grant exists"), SecondB);
    if (!FirstA || !FirstB || !SecondA || !SecondB) {
        return false;
    }

    TestEqual(TEXT("largest remainder gives A three"), FirstA->Quantity, 3);
    TestEqual(TEXT("largest remainder gives B two"), FirstB->Quantity, 2);
    TestEqual(TEXT("reorder preserves A quantity"), FirstA->Quantity,
              SecondA->Quantity);
    TestEqual(TEXT("reorder preserves B quantity"), FirstB->Quantity,
              SecondB->Quantity);
    TestEqual(TEXT("reorder preserves A factory seed"), FirstA->ItemSeed,
              SecondA->ItemSeed);
    TestEqual(TEXT("reorder preserves B factory seed"), FirstB->ItemSeed,
              SecondB->ItemSeed);
    TestNotEqual(TEXT("contributors have distinct factory seeds"),
                 FirstA->ItemSeed, FirstB->ItemSeed);

    const uint64 NextGenerationSeed =
        FMythicHarvestRewardPlanner::DeriveItemSeed(
            WorldEpoch, NodeId, 12,
            EMythicHarvestRewardChannel::PrimaryMaterial, 0,
            TEXT("char-a"));
    TestNotEqual(TEXT("generation participates in factory seed"),
                 FirstA->ItemSeed, NextGenerationSeed);
    const uint64 NextEpochSeed =
        FMythicHarvestRewardPlanner::DeriveItemSeed(
            FGuid(0x11111111, 0x22222222, 0x33333333, 0x44444444),
            NodeId, 11, EMythicHarvestRewardChannel::PrimaryMaterial, 0,
            TEXT("char-a"));
    TestNotEqual(TEXT("world epoch participates in factory seed"),
                 FirstA->ItemSeed, NextEpochSeed);

    TSet<FMythicHarvestRewardCompletionKey> EpochScopedKeys;
    EpochScopedKeys.Add({WorldEpoch, NodeId, 11});
    EpochScopedKeys.Add({FGuid(0x11111111, 0x22222222, 0x33333333,
                               0x44444444), NodeId, 11});
    TestEqual(TEXT("same node generation in two epochs is distinct"),
              EpochScopedKeys.Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestOfflineCompletionDeliveryTest,
    "Mythic.Harvesting.Rewards.OfflineCompletionDeliveryFrozen",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestOfflineCompletionDeliveryTest::RunTest(
    const FString &Parameters) {
    UMythicHarvestableDefinition *Definition =
        NewObject<UMythicHarvestableDefinition>(
            GetTransientPackage(), TEXT("OfflineHarvestDefinition"));
    UProficiencyDefinition *Proficiency =
        NewObject<UProficiencyDefinition>(
            GetTransientPackage(), TEXT("OfflineHarvestProficiency"));
    Definition->ProficiencyDefinition = Proficiency;
    Definition->CompletionProficiencyXP = 100.0f;
    Definition->QuestCredit.bEmitCompletionCredit = true;
    Definition->QuestCredit.CreditCount = 2;

    FMythicHarvestParticipantSnapshot Quarter;
    Quarter.ContributorKey = TEXT("character-a");
    Quarter.ContributionQuanta = 25;
    Quarter.ItemLevel = 7;
    Quarter.QuantityMultiplierQuanta =
        FMythicHarvestRewardPlanner::QuantityMultiplierScale;
    Quarter.ProficiencyLevel = 4;
    FMythicHarvestParticipantSnapshot ThreeQuarters = Quarter;
    ThreeQuarters.ContributorKey = TEXT("character-b");
    ThreeQuarters.ContributionQuanta = 75;
    const TArray<FMythicHarvestParticipantSnapshot> OfflineParticipants = {
        ThreeQuarters, Quarter};

    const FMythicHarvestRewardCompletionKey CompletionKey{
        FGuid(21, 22, 23, 24),
        FMythicHarvestNodeId(FGuid(31, 32, 33, 34)), 7};
    TSet<FMythicHarvestRewardCompletionKey> KnownCompletions;
    TestEqual(TEXT("first exact completion key is admitted once"),
              UMythicHarvestRewardOutboxSubsystem::TryCommitCompletionKey(
                  KnownCompletions, CompletionKey),
              EMythicHarvestCompletionAdmission::Committed);
    TestEqual(TEXT("exact completion replay is recognized"),
              UMythicHarvestRewardOutboxSubsystem::TryCommitCompletionKey(
                  KnownCompletions, CompletionKey),
              EMythicHarvestCompletionAdmission::AlreadyKnown);
    TestEqual(TEXT("replay cannot duplicate the completion ledger"),
              KnownCompletions.Num(), 1);
    TArray<FMythicPendingHarvestCompletionDelivery> Deliveries;
    FName Diagnostic;
    TestTrue(TEXT("disconnected participants freeze without controllers"),
             UMythicHarvestRewardOutboxSubsystem::BuildCompletionDeliveries(
                 *Definition, CompletionKey, OfflineParticipants,
                 Deliveries, Diagnostic));
    TestEqual(TEXT("one delivery row per canonical contributor"),
              Deliveries.Num(), 2);
    if (Deliveries.Num() != 2) {
        return false;
    }
    TestEqual(TEXT("canonical ordering places character A first"),
              Deliveries[0].ContributorKey, FString(TEXT("character-a")));
    TestEqual(TEXT("quarter contributor freezes 25 completion XP"),
              Deliveries[0].CompletionProficiencyXPQuanta,
              int64(25 * FMythicHarvestReceiptQuantity::QuantaPerUnit));
    TestEqual(TEXT("three-quarter contributor freezes 75 completion XP"),
              Deliveries[1].CompletionProficiencyXPQuanta,
              int64(75 * FMythicHarvestReceiptQuantity::QuantaPerUnit));
    TestEqual(TEXT("typed quest count is frozen for A"),
              Deliveries[0].QuestCreditCount, 2);
    TestFalse(TEXT("proficiency remains pending while A is offline"),
              Deliveries[0].bProficiencyDelivered);
    TestFalse(TEXT("quest credit remains pending while A is offline"),
              Deliveries[0].bQuestCreditDelivered);
    TestEqual(TEXT("typed proficiency primary identity is retained"),
              Deliveries[0].ProficiencyDefinitionId.PrimaryAssetType,
              UMythicAssetManager::ProficiencyDefinitionType);
    TestEqual(TEXT("typed harvestable primary identity is retained"),
              Deliveries[0].HarvestableDefinitionId.PrimaryAssetType,
              UMythicAssetManager::HarvestableDefinitionType);

    Definition->CompletionProficiencyXP = 999.0f;
    Definition->QuestCredit.CreditCount = 99;
    TestEqual(TEXT("later balance edits cannot drift frozen XP"),
              Deliveries[0].CompletionProficiencyXPQuanta,
              int64(25 * FMythicHarvestReceiptQuantity::QuantaPerUnit));
    TestEqual(TEXT("later balance edits cannot drift frozen quest credit"),
              Deliveries[0].QuestCreditCount, 2);

    TArray<FMythicHarvestParticipantSnapshot> DuplicateParticipants = {
        Quarter, Quarter};
    TestFalse(TEXT("duplicate canonical contributor fails closed"),
              UMythicHarvestRewardOutboxSubsystem::BuildCompletionDeliveries(
                  *Definition, CompletionKey, DuplicateParticipants,
                  Deliveries, Diagnostic));
    TestEqual(TEXT("duplicate contributor has stable diagnostic"),
              Diagnostic,
              FName(TEXT("InvalidCompletionDeliveryContributor")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestRewardSnapshotValidationTest,
    "Mythic.Harvesting.Rewards.SnapshotValidation",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestRewardSnapshotValidationTest::RunTest(
    const FString &Parameters) {
    FMythicHarvestRewardOutboxSaveV1 Snapshot;
    Snapshot.WorldEpoch = FGuid(5, 6, 7, 8);
    Snapshot.SnapshotSequence = 1;
    Snapshot.RetryQueueCursor = 2;
    FMythicSavedHarvestRewardCompletionV1 &Completion =
        Snapshot.KnownCompletions.AddDefaulted_GetRef();
    Completion.NodeGuid = FGuid(1, 2, 3, 4);
    Completion.WorldEpoch = Snapshot.WorldEpoch;
    Completion.Generation = 9;
    FMythicSavedHarvestGenerationHighWaterV1 &HighWater =
        Snapshot.GenerationHighWatermarks.AddDefaulted_GetRef();
    HighWater.WorldEpoch = Completion.WorldEpoch;
    HighWater.NodeGuid = Completion.NodeGuid;
    HighWater.HighestKnownGeneration = Completion.Generation;

    FMythicSavedHarvestRewardGrantV1 &Grant =
        Snapshot.PendingGrants.AddDefaulted_GetRef();
    Grant.NodeGuid = Completion.NodeGuid;
    Grant.WorldEpoch = Completion.WorldEpoch;
    Grant.Generation = Completion.Generation - 1;
    Grant.Channel = EMythicHarvestRewardChannel::PrimaryMaterial;
    Grant.RewardRowIndex = 2;
    Grant.ContributorKey = TEXT("character-123");
    Grant.ItemDefinitionId = FPrimaryAssetId(
        FPrimaryAssetType(TEXT("ItemDefinition")), FName(TEXT("TestItem")));
    Grant.OriginalQuantity = 7;
    Grant.RemainingQuantity = 3;
    Grant.ItemLevel = 5;
    Grant.ItemSeed = 0x0123456789abcdefull;
    Grant.ReceiptKey = FMythicHarvestReceiptKey::MakeCompletion(
        Grant.WorldEpoch, FMythicHarvestNodeId(Grant.NodeGuid),
        Grant.Generation,
        EMythicHarvestReceiptChannel::PrimaryMaterial,
        static_cast<uint32>(Grant.RewardRowIndex));
    Grant.ReceiptPayloadFingerprint =
        FMythicHarvestReceiptFingerprint::Build(
            Grant.ReceiptKey, Grant.ItemDefinitionId,
            Grant.OriginalQuantity, Grant.ItemSeed,
            static_cast<uint32>(Grant.ItemLevel),
            static_cast<uint32>(
                static_cast<uint8>(Grant.ResolvedQuality)));

    FMythicSavedHarvestCompletionDeliveryV1 &OfflineCompletion =
        Snapshot.PendingCompletionDeliveries.AddDefaulted_GetRef();
    OfflineCompletion.WorldEpoch = Completion.WorldEpoch;
    OfflineCompletion.NodeGuid = Completion.NodeGuid;
    OfflineCompletion.Generation = Completion.Generation;
    OfflineCompletion.ContributorKey = TEXT("character-offline");
    OfflineCompletion.ProficiencyDefinitionId = FPrimaryAssetId(
        UMythicAssetManager::ProficiencyDefinitionType,
        FName(TEXT("Mining_Proficiency")));
    OfflineCompletion.HarvestableDefinitionId = FPrimaryAssetId(
        UMythicAssetManager::HarvestableDefinitionType,
        FName(TEXT("Rock")));
    OfflineCompletion.CompletionProficiencyXPQuanta =
        25 * FMythicHarvestReceiptQuantity::QuantaPerUnit;
    OfflineCompletion.QuestCreditCount = 1;
    OfflineCompletion.ProficiencyReceiptKey =
        FMythicHarvestReceiptKey::MakeCompletion(
            OfflineCompletion.WorldEpoch,
            FMythicHarvestNodeId(OfflineCompletion.NodeGuid),
            OfflineCompletion.Generation,
            EMythicHarvestReceiptChannel::CompletionProficiencyXP);
    OfflineCompletion.ProficiencyReceiptPayloadFingerprint =
        FMythicHarvestReceiptFingerprint::Build(
            OfflineCompletion.ProficiencyReceiptKey,
            OfflineCompletion.ProficiencyDefinitionId,
            OfflineCompletion.CompletionProficiencyXPQuanta,
            0, 0, 0, OfflineCompletion.ProficiencyContextTags);
    OfflineCompletion.QuestReceiptKey =
        FMythicHarvestReceiptKey::MakeCompletion(
            OfflineCompletion.WorldEpoch,
            FMythicHarvestNodeId(OfflineCompletion.NodeGuid),
            OfflineCompletion.Generation,
            EMythicHarvestReceiptChannel::CompletionQuestCredit);
    OfflineCompletion.QuestReceiptPayloadFingerprint =
        FMythicHarvestReceiptFingerprint::Build(
            OfflineCompletion.QuestReceiptKey,
            OfflineCompletion.HarvestableDefinitionId,
            OfflineCompletion.QuestCreditCount, 0, 0, 0);

    FMythicSavedHarvestWorkDeliveryV1 &Work =
        Snapshot.PendingWorkDeliveries.AddDefaulted_GetRef();
    Work.WorldEpoch = Snapshot.WorldEpoch;
    Work.NodeGuid = Completion.NodeGuid;
    Work.Generation = Completion.Generation + 1;
    Work.ContributorKey = TEXT("character-work");
    Work.WorkRewardContract.bInitialized = true;
    Work.WorkRewardContract.ProficiencyDefinitionId = FPrimaryAssetId(
        UMythicAssetManager::ProficiencyDefinitionType,
        FName(TEXT("Mining_Proficiency")));
    Work.WorkRewardContract.ProficiencyXPPerWorkUnitQuanta =
        2 * FMythicHarvestReceiptQuantity::QuantaPerUnit;
    Work.CumulativeAppliedWorkQuanta =
        5 * FMythicHarvestWork::QuantaPerWorkUnit;
    TestTrue(TEXT("valid cumulative work target is representable"),
             FMythicHarvestReceiptQuantity::
                 TryCalculateCumulativeAppliedWorkXP(
                     Work.CumulativeAppliedWorkQuanta,
                     Work.WorkRewardContract.
                         ProficiencyXPPerWorkUnitQuanta,
                     Work.ProficiencyXPQuanta));
    Work.ReceiptKey = FMythicHarvestReceiptKey::MakeAppliedWork(
        Work.WorldEpoch, FMythicHarvestNodeId(Work.NodeGuid),
        Work.Generation, Work.ContributorKey);
    Work.ReceiptPayloadFingerprint =
        FMythicHarvestReceiptFingerprint::BuildAppliedWorkSeries(
            Work.ReceiptKey,
            Work.WorkRewardContract.ProficiencyDefinitionId,
            Work.WorkRewardContract.ProficiencyXPPerWorkUnitQuanta,
            Work.WorkRewardContract.ContextTags);

    FName Diagnostic;
    TestTrue(TEXT("snapshot accepts typed receipt-backed frozen grants"),
             UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
                 Snapshot, Diagnostic));
    TestTrue(TEXT("current witness plus contiguous high-water covers an older pending generation"),
             Grant.Generation < Completion.Generation);
    TestTrue(TEXT("success clears diagnostic"), Diagnostic.IsNone());

    FMythicSavedHarvestRewardCompletionV1 SecondCompletion;
    SecondCompletion.WorldEpoch = Snapshot.WorldEpoch;
    SecondCompletion.NodeGuid = FGuid(9, 10, 11, 12);
    SecondCompletion.Generation = 3;
    Snapshot.KnownCompletions.Add(SecondCompletion);
    FMythicSavedHarvestGenerationHighWaterV1 SecondHighWater;
    SecondHighWater.WorldEpoch = Snapshot.WorldEpoch;
    SecondHighWater.NodeGuid = SecondCompletion.NodeGuid;
    SecondHighWater.HighestKnownGeneration =
        SecondCompletion.Generation;
    Snapshot.GenerationHighWatermarks.Add(SecondHighWater);
    FMythicHarvestRewardOutboxSaveV1 ReorderedSnapshot = Snapshot;
    ReorderedSnapshot.KnownCompletions.Swap(0, 1);
    ReorderedSnapshot.GenerationHighWatermarks.Swap(0, 1);
    FSHA256Signature CanonicalFingerprint{};
    FSHA256Signature ReorderedFingerprint{};
    TestTrue(TEXT("canonical outbox fingerprint builds"),
             UMythicHarvestRewardOutboxSubsystem::
                 BuildSaveSnapshotFingerprint(
                     Snapshot, CanonicalFingerprint));
    TestTrue(TEXT("reordered outbox fingerprint builds"),
             UMythicHarvestRewardOutboxSubsystem::
                 BuildSaveSnapshotFingerprint(
                     ReorderedSnapshot, ReorderedFingerprint));
    TestTrue(TEXT("set-like array order cannot drift the payload binding"),
             FMemory::Memcmp(
                 CanonicalFingerprint.Signature,
                 ReorderedFingerprint.Signature,
                 sizeof(CanonicalFingerprint.Signature)) == 0);

    FMythicHarvestRewardOutboxSaveV1 BindingSnapshot;
    BindingSnapshot.WorldEpoch = FGuid(31, 32, 33, 34);
    BindingSnapshot.SnapshotSequence = 7;
    FMythicSavedHarvestRewardCompletionV1 &BindingCompletion =
        BindingSnapshot.KnownCompletions.AddDefaulted_GetRef();
    BindingCompletion.WorldEpoch = BindingSnapshot.WorldEpoch;
    BindingCompletion.NodeGuid = FGuid(35, 36, 37, 38);
    BindingCompletion.Generation = 4;
    FMythicSavedHarvestGenerationHighWaterV1 &BindingHighWater =
        BindingSnapshot.GenerationHighWatermarks.
            AddDefaulted_GetRef();
    BindingHighWater.WorldEpoch = BindingCompletion.WorldEpoch;
    BindingHighWater.NodeGuid = BindingCompletion.NodeGuid;
    BindingHighWater.HighestKnownGeneration =
        BindingCompletion.Generation;
    UMythicHarvestRewardOutboxSubsystem *BindingOutbox =
        NewObject<UMythicHarvestRewardOutboxSubsystem>();
    FSHA256Signature InstalledFingerprint{};
    TestFalse(TEXT("a fresh outbox has no restored payload binding"),
              BindingOutbox->TryGetRestoredSaveSnapshotFingerprint(
                  InstalledFingerprint));
    TestTrue(TEXT("valid restore installs an exact payload binding"),
             BindingOutbox->RestoreSaveSnapshot(
                 BindingSnapshot, Diagnostic));
    TestTrue(TEXT("installed payload fingerprint is readable"),
             BindingOutbox->TryGetRestoredSaveSnapshotFingerprint(
                 InstalledFingerprint));
    TestTrue(TEXT("installed payload matches its restore input"),
             BindingOutbox->MatchesRestoredSaveSnapshot(
                 BindingSnapshot));
    uint32 RestoredHighWater = 0;
    TestTrue(TEXT("installed generation high-water is readable"),
             BindingOutbox->TryGetHighestKnownGeneration(
                 BindingSnapshot.WorldEpoch,
                 FMythicHarvestNodeId(BindingCompletion.NodeGuid),
                 RestoredHighWater));
    TestEqual(TEXT("installed generation high-water is exact"),
              RestoredHighWater, BindingCompletion.Generation);
    FMythicHarvestRewardOutboxSaveV1 CollidingSequencePayload =
        BindingSnapshot;
    CollidingSequencePayload.RetryQueueCursor = 1;
    TestFalse(TEXT("same sequence with different payload is rejected"),
              BindingOutbox->MatchesRestoredSaveSnapshot(
                  CollidingSequencePayload));

    TArray<uint8> Serialized;
    {
        FMemoryWriter Writer(Serialized, true);
        FMythicHarvestRewardOutboxSaveV1::StaticStruct()->SerializeItem(
            Writer, &Snapshot, nullptr);
    }
    FMythicHarvestRewardOutboxSaveV1 RoundTripped;
    {
        FMemoryReader Reader(Serialized, true);
        FMythicHarvestRewardOutboxSaveV1::StaticStruct()->SerializeItem(
            Reader, &RoundTripped, nullptr);
    }
    TestEqual(TEXT("save round-trip retains offline completion row"),
              RoundTripped.PendingCompletionDeliveries.Num(), 1);
    TestEqual(TEXT("save round-trip retains retry fairness cursor"),
              RoundTripped.RetryQueueCursor, uint8(2));
    if (RoundTripped.PendingCompletionDeliveries.Num() == 1) {
        const FMythicSavedHarvestCompletionDeliveryV1 &Restored =
            RoundTripped.PendingCompletionDeliveries[0];
        TestEqual(TEXT("save round-trip retains canonical contributor"),
                  Restored.ContributorKey,
                  FString(TEXT("character-offline")));
        TestEqual(TEXT("save round-trip retains frozen XP"),
                  Restored.CompletionProficiencyXPQuanta,
                  int64(25 * FMythicHarvestReceiptQuantity::QuantaPerUnit));
        TestEqual(TEXT("save round-trip retains typed quest target"),
                  Restored.HarvestableDefinitionId,
                  OfflineCompletion.HarvestableDefinitionId);
    }

    Snapshot.PendingCompletionDeliveries[0].bProficiencyDelivered = true;
    Snapshot.PendingCompletionDeliveries[0].bQuestCreditDelivered = true;
    TestFalse(TEXT("fully delivered work cannot remain in pending snapshot"),
              UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
                  Snapshot, Diagnostic));
    TestEqual(TEXT("invalid completion delivery has stable diagnostic"),
              Diagnostic,
              FName(TEXT("InvalidSavedCompletionDelivery")));
    Snapshot.PendingCompletionDeliveries[0].bProficiencyDelivered = false;
    Snapshot.PendingCompletionDeliveries[0].bQuestCreditDelivered = false;

    const FMythicSavedHarvestCompletionDeliveryV1 DuplicateDelivery =
        Snapshot.PendingCompletionDeliveries[0];
    Snapshot.PendingCompletionDeliveries.Add(DuplicateDelivery);
    TestFalse(TEXT("duplicate completion plus contributor is rejected"),
              UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
                  Snapshot, Diagnostic));
    TestEqual(TEXT("duplicate completion delivery has stable diagnostic"),
              Diagnostic,
              FName(TEXT("InvalidSavedCompletionProficiencyReceipt")));
    Snapshot.PendingCompletionDeliveries.RemoveAt(1);

    const FMythicHarvestReceiptKey ValidWorkReceipt =
        Snapshot.PendingWorkDeliveries[0].ReceiptKey;
    Snapshot.PendingWorkDeliveries[0].ReceiptKey.SeriesGuid = FGuid();
    TestFalse(TEXT("zero deterministic work-series GUID is rejected"),
              UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
                  Snapshot, Diagnostic));
    TestEqual(TEXT("malformed work receipt key has stable diagnostic"),
              Diagnostic,
              FName(TEXT("InvalidSavedAppliedWorkReceiptKey")));
    Snapshot.PendingWorkDeliveries[0].ReceiptKey = ValidWorkReceipt;
    Snapshot.PendingWorkDeliveries[0].ReceiptKey.EntryOrdinal = 1;
    TestFalse(TEXT("nonzero cumulative work ordinal is rejected"),
              UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
                  Snapshot, Diagnostic));
    Snapshot.PendingWorkDeliveries[0].ReceiptKey = ValidWorkReceipt;

    Snapshot.PendingGrants[0].bHasResolvedQuality = true;
    Snapshot.PendingGrants[0].ResolvedQuality = EMythicYieldQuality::Fine;
    TestFalse(TEXT("quality drift is rejected without a matching immutable fingerprint"),
             UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
                 Snapshot, Diagnostic));
    Snapshot.PendingGrants[0].bHasResolvedQuality = false;
    Snapshot.PendingGrants[0].ResolvedQuality = EMythicYieldQuality::Common;

    Snapshot.PendingGrants[0].RemainingQuantity = 8;
    TestFalse(TEXT("remaining quantity cannot exceed frozen plan"),
              UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
                  Snapshot, Diagnostic));
    TestEqual(TEXT("invalid grant has stable diagnostic"), Diagnostic,
              FName(TEXT("InvalidSavedRewardGrant")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestDurableOverflowRemainderTest,
    "Mythic.Harvesting.Rewards.DurableOverflowRemainder",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestDurableOverflowRemainderTest::RunTest(
    const FString &Parameters) {
    TestEqual(TEXT("a full inventory retains the exact durable remainder"),
              UMythicHarvestRewardOutboxSubsystem::
                  CalculateRemainingQuantityAfterInsertion(7, 0),
              7);
    TestEqual(TEXT("partial insertion retains only the semantic remainder"),
              UMythicHarvestRewardOutboxSubsystem::
                  CalculateRemainingQuantityAfterInsertion(7, 3),
              4);
    TestEqual(TEXT("complete insertion closes the durable row"),
              UMythicHarvestRewardOutboxSubsystem::
                  CalculateRemainingQuantityAfterInsertion(7, 7),
              0);
    TestEqual(TEXT("invalid negative insertion cannot erase a reward"),
              UMythicHarvestRewardOutboxSubsystem::
                  CalculateRemainingQuantityAfterInsertion(7, -5),
              7);
    TestEqual(TEXT("over-reporting cannot underflow the durable row"),
              UMythicHarvestRewardOutboxSubsystem::
                  CalculateRemainingQuantityAfterInsertion(7, 99),
              0);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
