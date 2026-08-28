#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "Itemization/Affixes/MythicPermanentStatLedger.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "Stats/MythicStatTypes.h"
#include "MythicAffixApplicationComponent.generated.h"

class UMythicAbilitySystemComponent;
class UMythicItemInstance;
class UMythicInventoryComponent;
class UMythicItemizationDataRegistrySubsystem;
enum class EMythicItemizationReadiness : uint8;

/** Player-facing lifecycle result for applying one canonical affix definition to a live item. */
UENUM(BlueprintType)
enum class EMythicAffixApplyResult : uint8 {
    Active,
    Suppressed,
    Failed
};

/** Owner-visible split between non-equipment permanent stats and the composed equipment layer. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicReplicatedPermanentStatLayer {
    GENERATED_BODY()

    /** Gameplay Ability System attribute whose permanent layers are described by this entry. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FGameplayAttribute Attribute;

    /** Base after source-addressed progression/reward modifiers, before equipment affixes. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float NonEquipmentBaseValue = 0.0f;

    /** GAS base after all active permanent equipment affixes have been composed. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    float EquipmentBaseValue = 0.0f;

    bool operator==(const FMythicReplicatedPermanentStatLayer &Other) const {
        return Attribute == Other.Attribute
            && NonEquipmentBaseValue == Other.NonEquipmentBaseValue
            && EquipmentBaseValue == Other.EquipmentBaseValue;
    }
};

DECLARE_MULTICAST_DELEGATE(FOnMythicPermanentStatLayerChanged);

struct FMythicAffixApplicationCandidate {
    TWeakObjectPtr<UMythicItemInstance> SourceItem;
    FRolledAffix Snapshot;
};

/** A not-yet-published equipment slot value used to reconcile GAS before Fast Array state is committed. */
struct FMythicAffixEquipmentSlotOverride {
    UMythicInventoryComponent *Inventory = nullptr;
    int32 SlotIndex = INDEX_NONE;
    UMythicItemInstance *ProposedItem = nullptr;
};

struct FMythicAppliedAffixState {
    FGuid RollGuid;
    FGuid SourceItemGuid;
    TWeakObjectPtr<UMythicItemInstance> SourceItem;
    FRolledAffix Snapshot;
    EMythicAffixApplyResult State = EMythicAffixApplyResult::Suppressed;
    FGameplayTag TargetStatTag;
    FGameplayTag StackingGroup;
    EMythicAffixStackingRule StackingRule = EMythicAffixStackingRule::UniquePerItem;
    FGameplayTagContainer ConflictGroups;
    TEnumAsByte<EGameplayModOp::Type> ModifierOp = EGameplayModOp::AddBase;
    EMythicStatComparisonDirection ComparisonDirection = EMythicStatComparisonDirection::HigherIsBetter;
    float NeutralValue = 0.0f;
    float Magnitude = 0.0f;
};

/** Typed, source-addressed permanent progression/reward modifier staged for one atomic ledger transaction. */
struct FMythicPermanentStatSourceSpec {
    FGuid SourceGuid;
    FMythicStatDefinitionHandle StatDefinition;
    TEnumAsByte<EGameplayModOp::Type> ModifierOp = EGameplayModOp::AddBase;
    float Magnitude = 0.0f;
};

/**
 * Authoritative owner of the permanent equipment-affix stat-source layer.
 *
 * The component retains every equipped candidate, including suppressed Highest candidates, and is the only runtime
 * authority for cross-item stacking. Active affixes are resolved from current Affix/Stat Definitions and composed
 * transactionally into GAS base attributes. Temporary buffs/debuffs remain ordinary GameplayEffects layered by GAS
 * over that base. The component never removes an affix by applying an inverse value.
 */
UCLASS(ClassGroup=(Mythic), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicAffixApplicationComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicAffixApplicationComponent();

    /** Adds an entire source batch in one ledger transition; failure restores every previously written GAS base. */
    bool ApplySnapshotsTransactional(UMythicItemInstance *SourceItem,
                                     TConstArrayView<FRolledAffix> Snapshots);
    /** Removes an exact source batch in one ledger transition, or leaves the prior ledger intact. */
    bool RemoveSnapshotsTransactional(UMythicItemInstance *SourceItem,
                                      TConstArrayView<FRolledAffix> Snapshots);
    void GetActiveSourcesForStat(FGameplayTag StatTag, TArray<FGuid> &OutRollGuids) const;

    // Re-enumerates every authoritative equipment slot (base, equipped-gem and socket snapshots), removes orphans,
    // and restores active/suppressed winners. This is idempotent when the authoritative set is unchanged.
    void ReconcileFromAuthoritativeSnapshots();

    /**
     * Production notification seam used by equipment mutation, restore, socket/craft refresh and ASC avatar changes.
     * Requests made during a transaction are coalesced and replayed after the transaction leaves its guard.
     */
    void RequestAuthoritativeReconciliation();

    /** Called by UMythicAbilitySystemComponent after InitAbilityActorInfo on authority. */
    void NotifyAbilitySystemActorInfoChanged(UMythicAbilitySystemComponent *InAbilitySystemComponent);

    // Replaces the complete registered set in one transaction. Equipment/load code can use this after enumerating
    // authoritative item snapshots, without exposing or mutating the internal ledger incrementally.
    bool ReplaceAuthoritativeSetTransactional(TConstArrayView<FMythicAffixApplicationCandidate> Candidates);

    /**
     * Reconciles the complete authoritative equipment set as though the supplied slot values were already present.
     * The inventory must publish those values only after this returns true. Deferred/invalid closure leaves the
     * complete previous ledger and every previously composed base value untouched.
     */
    bool ReconcileEquipmentMutationTransactional(
        TConstArrayView<FMythicAffixEquipmentSlotOverride> SlotOverrides);

    /**
     * Reconciles an equipped item's staged base-affix snapshots without publishing them first. Socket and gem
     * snapshots on the host, plus every other equipped item, remain part of the same winner calculation.
     */
    bool ReconcileItemSnapshotMutationTransactional(
        UMythicItemInstance *SourceItem,
        TConstArrayView<FRolledAffix> ProposedBaseSnapshots);

    bool IsRegistered(FGuid RollGuid) const;
    bool IsActive(FGuid RollGuid) const;
    bool IsApplicationQuarantined() const { return bApplicationQuarantined; }
    void GetQuarantinedSourceItems(TArray<FGuid> &OutSourceItemGuids) const;
    int32 GetRegisteredCandidateCount() const { return Ledger.Num(); }
    int32 GetSuppressedCandidateCount() const { return SuppressedRollGuids.Num(); }

    /**
     * Returns the authoritative/replicated permanent layers for an attribute. False means equipment does not
     * currently modify that attribute, so the caller may use the live GAS base for both outputs.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Stats")
    bool GetPermanentStatLayerValues(FGameplayAttribute Attribute,
                                     float &OutNonEquipmentBaseValue,
                                     float &OutEquipmentBaseValue) const;

    /**
     * Upserts one idempotent permanent progression/reward source and recomposes active equipment atomically. Source
     * Guid is an operational identity, while the typed Stat Definition remains the sole stat/attribute reference.
     */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Stats")
    bool SetPermanentStatSourceTransactional(
        FGuid SourceGuid,
        FMythicStatDefinitionHandle StatDefinition,
        TEnumAsByte<EGameplayModOp::Type> ModifierOp,
        float Magnitude);

    /** Removes one exact permanent progression/reward source and recomposes active equipment atomically. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Mythic|Stats")
    bool RemovePermanentStatSourceTransactional(FGuid SourceGuid);

    /** Atomically replaces a caller-owned source set; used for idempotent save restore and respec workflows. */
    bool ReplacePermanentStatSourceSetTransactional(
        TConstArrayView<FGuid> OwnedSourceGuids,
        TConstArrayView<FMythicPermanentStatSourceSpec> DesiredSources);

    FOnMythicPermanentStatLayerChanged &OnPermanentStatLayerChanged() {
        return PermanentStatLayerChanged;
    }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

private:
    enum class EAuthoritativeCollectionResult : uint8 {
        Ready,
        Deferred,
        InvalidSourcesExcluded
    };

    bool HasServerAuthority() const;
    bool ResolveAbilitySystem();
    void BindRegistryReadiness();
    void UnbindRegistryReadiness();
    void HandleRegistryReadinessChanged(EMythicItemizationReadiness NewReadiness);
    void HandleRegistrySemanticDataChanged(uint64 SemanticRevision);
    EAuthoritativeCollectionResult BuildAuthoritativeCandidateSet(
        TArray<FMythicAffixApplicationCandidate> &OutCandidates,
        TSet<FGuid> &OutInvalidSourceItems,
        TConstArrayView<FMythicAffixEquipmentSlotOverride> SlotOverrides = {},
        UMythicItemInstance *SnapshotOverrideSource = nullptr,
        TConstArrayView<FRolledAffix> ProposedBaseSnapshots = {});
    bool AppendEquippedItemCandidates(UMythicItemInstance &SourceItem,
                                      UMythicItemizationDataRegistrySubsystem &Registry,
                                      TArray<FMythicAffixApplicationCandidate> &OutCandidates,
                                      bool &bOutDeferred,
                                      TConstArrayView<FRolledAffix> ProposedBaseSnapshots = {},
                                      bool bHasBaseSnapshotOverride = false) const;
    bool RequestItemDataClosure(UMythicItemInstance &SourceItem,
                                UMythicItemizationDataRegistrySubsystem &Registry);
    bool ReconcileCollectedSetTransactional(
        TConstArrayView<FMythicAffixEquipmentSlotOverride> SlotOverrides,
        UMythicItemInstance *SnapshotOverrideSource,
        TConstArrayView<FRolledAffix> ProposedBaseSnapshots);
    bool CommitCollectedCandidatesTransactional(
        EAuthoritativeCollectionResult Result,
        TConstArrayView<FMythicAffixApplicationCandidate> Candidates,
        const TSet<FGuid> &InvalidSourceItems,
        bool bRepairOrphans);
    static bool CollectionPermitsTransition(EAuthoritativeCollectionResult Result) {
        return Result != EAuthoritativeCollectionResult::Deferred;
    }
    bool ValidateCandidate(FMythicAppliedAffixState &Candidate) const;
    bool ResolveCandidateContributions(
        const FMythicAppliedAffixState &Candidate,
        TArray<FMythicPermanentStatContribution> &OutContributions,
        FString *OutFailureReason = nullptr) const;
    bool BuildDesiredPermanentContributions(
        const TMap<FGuid, FMythicAppliedAffixState> &Candidates,
        const TSet<FGuid> &ActiveRolls,
        const TMap<FGuid, FMythicPermanentStatContribution> &PermanentSources,
        TArray<FMythicPermanentStatContribution> &OutContributions,
        FString *OutFailureReason = nullptr) const;
    bool ResolvePermanentStatSource(
        const FMythicPermanentStatSourceSpec &Spec,
        FMythicPermanentStatContribution &OutContribution) const;
    bool ComputeDesiredActiveRolls(TMap<FGuid, FMythicAppliedAffixState> &Candidates,
                                   TSet<FGuid> &OutActiveRolls) const;
    static bool ComputeStackingWinners(const TMap<FGuid, FMythicAppliedAffixState> &Candidates,
                                       TSet<FGuid> &OutActiveRolls);
    bool TransitionLedgerTransactional(TMap<FGuid, FMythicAppliedAffixState> &&DesiredLedger,
                                       bool bForceFullRecompose = false);
    void QuarantineLedgerAfterRestoreFailure(TMap<FGuid, FMythicAppliedAffixState> &&FailedLedger);
    void QuarantineApplicationAfterSemanticReconciliationFailure(uint64 SemanticRevision);
    void ClearApplicationQuarantine();
    void RebuildIndexes();
    void PublishPermanentStatLayer();

    UFUNCTION()
    void OnRep_PermanentStatLayer();

    static bool SnapshotsEquivalent(const FRolledAffix &A, const FRolledAffix &B);
    static bool GuidLexicalLess(const FGuid &A, const FGuid &B);
    static double GetNormalizedContributionDelta(const FMythicAppliedAffixState &Candidate);
    static bool IsBetterHighestCandidate(const FMythicAppliedAffixState &Candidate,
                                         const FMythicAppliedAffixState &Incumbent);

    UPROPERTY(Transient)
    TObjectPtr<UMythicAbilitySystemComponent> AbilitySystemComponent;

    /** Non-replicated authoritative composition state used to publish GAS bases and the owner-only layer split. */
    FMythicPermanentStatLedger PermanentStatLedger;

    UPROPERTY(Transient, ReplicatedUsing = OnRep_PermanentStatLayer)
    TArray<FMythicReplicatedPermanentStatLayer> PermanentStatLayer;

    FOnMythicPermanentStatLayerChanged PermanentStatLayerChanged;
    TMap<FGuid, FMythicPermanentStatContribution> PermanentStatSources;
    TMap<FGuid, FMythicAppliedAffixState> Ledger;
    TMultiMap<FGuid, FGuid> RollGuidsBySourceItem;
    TMultiMap<FGameplayTag, FGuid> RollGuidsByStackingGroup;
    TMultiMap<FGameplayTag, FGuid> RollGuidsByConflictGroup;
    TMultiMap<FGameplayTag, FGuid> RollGuidsByStat;
    TSet<FGuid> ActiveRollGuids;
    TSet<FGuid> SuppressedRollGuids;
    TSet<FGuid> AuthoritativeDataQuarantinedSourceItemGuids;
    TSet<FGuid> FatalQuarantinedSourceItemGuids;
    TSet<FPrimaryAssetId> PendingProfileClosures;
    TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> BoundRegistry;
    FDelegateHandle RegistryReadinessHandle;
    FDelegateHandle RegistrySemanticDataChangedHandle;
    bool bApplicationQuarantined = false;
    bool bReconciliationInProgress = false;
    bool bAuthoritativeEnumerationInProgress = false;
    bool bReconciliationRequested = false;
    uint64 PendingSemanticDataRevision = 0;

#if WITH_DEV_AUTOMATION_TESTS
    // Deterministic one-shot transaction failure seams. A value of zero fails the next matching layer transition.
    mutable int32 TestApplyFailureCountdown = INDEX_NONE;
    mutable int32 TestRemoveFailureCountdown = INDEX_NONE;
    static bool ConsumeTestFailure(int32 &Countdown);
#endif

    friend struct FMythicAffixApplicationTestAccessor;
};
