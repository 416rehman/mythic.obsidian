#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Stats/MythicStatRegistry.h"
#include "Itemization/Affixes/MythicAffixTypes.h"
#include "MythicItemizationDataRegistrySubsystem.generated.h"

struct FStreamableHandle;
class FEditPropertyChain;
struct FMythicAsyncLoadTransaction;
struct FMythicItemizationRegistryPublishedState;
class UMythicAffixDefinition;
class UMythicAffixPool;
class UMythicAffixProfile;
class UMythicAffixRollPolicy;
class UMythicItemizationRuleset;
class UMythicStatCategoryDefinition;
class UMythicStatDefinition;
struct FCompiledAffixProfile;
struct FCompiledAffixGrantClosure;
struct FMythicAffixGrantSpec;

/** Global itemization control-plane readiness. Exact profile readiness is queried separately. */
UENUM(BlueprintType)
enum class EMythicItemizationReadiness : uint8 {
    Uninitialized,
    CoreSemanticReady,
    ActiveRulesetReady
};

DECLARE_DELEGATE_OneParam(FOnMythicItemizationDataReady, bool /* bSuccess */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMythicItemizationReadinessChanged,
                                    EMythicItemizationReadiness);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnMythicItemizationSemanticDataChanged,
                                    uint64 /* SemanticRevision */);

/**
 * Server/runtime registry for data-driven stats and item affixes.
 *
 * All authored relationships are typed soft references. PrimaryAssetId values are internal cache keys derived from
 * those references; designers never maintain parallel string joins. The active Itemization Ruleset is the sole
 * release/season control layer and atomically prewarms its concrete profile closures.
 */
UCLASS()
class MYTHIC_API UMythicItemizationDataRegistrySubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    EMythicItemizationReadiness GetReadiness() const { return Readiness; }
    bool IsCoreSemanticReady() const {
        return !bSemanticDataQuarantined
            && Readiness >= EMythicItemizationReadiness::CoreSemanticReady;
    }
    bool IsActiveRulesetReady() const {
        return !bSemanticDataQuarantined
            && Readiness >= EMythicItemizationReadiness::ActiveRulesetReady;
    }
    bool IsProfileReady(FPrimaryAssetId ProfileId) const;

    void RequestCoreSemanticDataAsync(FOnMythicItemizationDataReady Completion);
    void RequestActiveRulesetAsync(FOnMythicItemizationDataReady Completion);
    void RequestProfileClosureAsync(FPrimaryAssetId ProfileId,
                                    FOnMythicItemizationDataReady Completion);
    void RequestGrantClosureAsync(TConstArrayView<FMythicAffixGrantSpec> Grants,
                                  FOnMythicItemizationDataReady Completion);
    FOnMythicItemizationReadinessChanged &OnReadinessChanged() { return ReadinessChanged; }
    /**
     * Fires whenever semantic data becomes unavailable or a complete publication/recovery commits. Consumers must
     * re-check readiness in the callback before reading definitions or compiled closures.
     */
    FOnMythicItemizationSemanticDataChanged &OnSemanticDataChanged() {
        return SemanticDataChanged;
    }
    /** Monotonic publication revision used by native consumers to coalesce semantic refresh work. */
    uint64 GetSemanticDataRevision() const { return SemanticDataRevision; }

    /** Returns the committed stat registry, or an unbuilt empty registry while semantic data is quarantined. */
    const FMythicStatRegistry &GetStatRegistry() const {
        return bSemanticDataQuarantined ? UnavailableStatRegistry : StatRegistry;
    }
    const UMythicStatCategoryDefinition *FindStatCategory(FGameplayTag CategoryTag) const;
    const UMythicStatDefinition *FindStat(FPrimaryAssetId StatId) const;
    const UMythicStatDefinition *FindStat(FGameplayTag StatTag) const;
    const UMythicStatDefinition *FindStat(const FGameplayAttribute &Attribute) const;
    const UMythicAffixDefinition *FindAffix(FGameplayTag AffixTag) const;
    const UMythicAffixDefinition *FindAffix(FPrimaryAssetId DefinitionId) const;
    const UMythicAffixPool *FindPool(FPrimaryAssetId PoolId) const;
    const UMythicAffixRollPolicy *FindPolicy(FPrimaryAssetId PolicyId) const;
    const UMythicAffixProfile *FindProfile(FPrimaryAssetId ProfileId) const;
    const UMythicItemizationRuleset *GetActiveRuleset() const {
        return IsActiveRulesetReady() ? ActiveRuleset.Get() : nullptr;
    }
    TSharedPtr<const FCompiledAffixProfile> FindCompiledProfile(FPrimaryAssetId ProfileId) const;
    TSharedPtr<const FCompiledAffixGrantClosure> FindCompiledGrant(
        const FMythicAffixGrantSpec &Spec) const;

    void GetAllStatDefinitions(TArray<const UMythicStatDefinition *> &Out) const;
    void GetAllStatCategories(TArray<const UMythicStatCategoryDefinition *> &Out) const;
    void GetAllAffixDefinitions(TArray<const UMythicAffixDefinition *> &Out) const;

    /** Returns every committed Stat Definition in stat-sheet order, or an empty array while data is quarantined. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Itemization|Registry")
    TArray<UMythicStatDefinition *> GetAllStatDefinitionAssets() const;

    /** Returns every committed Stat Category in stat-sheet order, or an empty array while data is quarantined. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Itemization|Registry")
    TArray<UMythicStatCategoryDefinition *> GetAllStatCategoryAssets() const;

    /** Returns every committed Affix Definition in Affix Tag order, or an empty array while data is quarantined. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Itemization|Registry")
    TArray<UMythicAffixDefinition *> GetAllAffixDefinitionAssets() const;

    /** Publishes already-loaded assets for deterministic editor validation and automation tests. */
    bool PublishLoadedAssets(TConstArrayView<UObject *> Assets, TArray<FText> &OutErrors);
    /** Revalidates every resident semantic asset and atomically recompiles all published generation closures. */
    bool RefreshResidentData(TArray<FText> &OutErrors);
#if WITH_DEV_AUTOMATION_TESTS
    /** Publishes a complete test semantic closure and advances readiness only after its required indexes exist. */
    bool PublishCoreSemanticAssetsForTests(TConstArrayView<UObject *> Assets,
                                           TArray<FText> &OutErrors);
#endif
    bool CompileProfile(FPrimaryAssetId ProfileId, TArray<FText> &OutErrors);
    bool CompileGrantClosures(TConstArrayView<FMythicAffixGrantSpec> Grants,
                              TArray<FText> &OutErrors);
    bool ValidateAssetRegistryIdentities(TArray<FText> &OutErrors) const;
    bool ValidateActiveRuleset(TArray<FText> &OutErrors) const;

private:
    void SetReadiness(EMythicItemizationReadiness NewReadiness);
    bool RebuildIndexes(TArray<FText> &OutErrors);
    bool PublishLoadedAssetsInternal(TConstArrayView<UObject *> Assets,
                                     TArray<FText> &OutErrors,
                                     bool bBroadcastSemanticChange);
    bool StageResidentCompiledClosures(
        TMap<FPrimaryAssetId, TSharedPtr<const FCompiledAffixProfile>> &OutProfiles,
        TMap<FGuid, TSharedPtr<const FCompiledAffixGrantClosure>> &OutGrants,
        TArray<FText> &OutErrors) const;
    FMythicItemizationRegistryPublishedState CapturePublishedState() const;
    void RestorePublishedState(FMythicItemizationRegistryPublishedState &&State);
    void BroadcastSemanticDataChanged();
    bool GatherAssetRegistryCandidates(TArray<FSoftObjectPath> &OutPaths,
                                       TArray<FText> &OutErrors,
                                       bool bCoreSemanticOnly = false) const;
    void RequestPrimaryAssets(TArray<FPrimaryAssetId> AssetIds,
                              TSharedRef<FMythicAsyncLoadTransaction> Transaction,
                              TFunction<void(bool)> Completion);
    void RequestSoftObjects(TArray<FSoftObjectPath> ObjectPaths,
                            TSharedRef<FMythicAsyncLoadTransaction> Transaction,
                            TFunction<void(bool)> Completion);
    void CommitLoadTransaction(const TSharedRef<FMythicAsyncLoadTransaction> &Transaction);
    void BeginCoreSemanticDiscovery();
    void CompleteCoreSemanticRequest(bool bSuccess);
    void BeginActiveRulesetRequest();
    void CompleteActiveRulesetRequest(bool bSuccess);
    void BeginProfileClosureRequest(FPrimaryAssetId ProfileId);
    void CompleteProfileClosureRequest(FPrimaryAssetId ProfileId, bool bSuccess);
#if WITH_EDITOR
    void HandleResidentAssetPrePropertyChange(UObject *Object,
                                              const FEditPropertyChain &PropertyChain);
    void HandleResidentAssetPropertyChanged(UObject *Object,
                                            struct FPropertyChangedEvent &PropertyChangedEvent);
    void HandleResidentObjectsReplaced(const TMap<UObject *, UObject *> &ReplacementMap);
    bool IsResidentSemanticDependency(const UObject *Object) const;
    void EnterEditorSemanticQuarantine();
    void ScheduleEditorResidentRefresh();
    void ProcessPendingEditorResidentRefresh();
    bool CaptureResidentAssetPreEditSnapshot(UObject *Object);
    bool RestoreResidentAssetPreEditSnapshots(TArray<FText> &OutErrors);
    void ClearResidentAssetPreEditSnapshots();
    void QuarantineSemanticDataAfterEditorRestoreFailure();
#endif

    EMythicItemizationReadiness Readiness = EMythicItemizationReadiness::Uninitialized;
    FOnMythicItemizationReadinessChanged ReadinessChanged;
    FOnMythicItemizationSemanticDataChanged SemanticDataChanged;
    uint64 SemanticDataRevision = 0;
    FMythicStatRegistry StatRegistry;
    FMythicStatRegistry UnavailableStatRegistry;

    UPROPERTY(Transient) TArray<TObjectPtr<UObject>> LoadedAssets;
    UPROPERTY(Transient) TObjectPtr<UMythicItemizationRuleset> ActiveRuleset = nullptr;
    TArray<TSharedPtr<FStreamableHandle>> ActiveLoadHandles;
    bool bAcceptingAsyncCompletions = false;
    bool bCoreSemanticRequestInFlight = false;
    bool bCoreSemanticDiscoveryStarted = false;
    bool bActiveRulesetRequestInFlight = false;
    bool bSemanticRefreshInProgress = false;
    bool bSemanticDataQuarantined = false;
    FDelegateHandle AssetRegistryGatherHandle;
#if WITH_EDITOR
    FDelegateHandle PreObjectPropertyChangedHandle;
    FDelegateHandle ObjectPropertyChangedHandle;
    FDelegateHandle ObjectsReplacedHandle;
    bool bEditorRefreshPending = false;
    bool bRestoringEditorPreEditSnapshots = false;
    TMap<FGuid, TSharedPtr<FMythicAffixGrantSpec>> PendingReplacementGrantSpecs;
#endif
    UPROPERTY(Transient) TArray<TObjectPtr<UObject>> EditorPreEditObjects;
    UPROPERTY(Transient) TArray<TObjectPtr<UObject>> EditorPreEditSnapshots;
    TArray<FOnMythicItemizationDataReady> PendingCoreSemanticRequests;
    TArray<FOnMythicItemizationDataReady> PendingActiveRulesetRequests;
    TSet<FPrimaryAssetId> ProfileClosureRequestsInFlight;
    TMap<FPrimaryAssetId, TArray<FOnMythicItemizationDataReady>> PendingProfileClosureRequests;
    TMap<FPrimaryAssetId, const UMythicAffixDefinition *> AffixesById;
    TMap<FGameplayTag, const UMythicAffixDefinition *> AffixesByTag;
    TMap<FPrimaryAssetId, const UMythicAffixPool *> PoolsById;
    TMap<FPrimaryAssetId, const UMythicAffixRollPolicy *> PoliciesById;
    TMap<FPrimaryAssetId, const UMythicAffixProfile *> ProfilesById;
    TMap<FPrimaryAssetId, const UMythicItemizationRuleset *> RulesetsById;
    TMap<FPrimaryAssetId, TSharedPtr<const FCompiledAffixProfile>> CompiledProfiles;
    TMap<FGuid, TSharedPtr<const FCompiledAffixGrantClosure>> CompiledGrants;

#if WITH_DEV_AUTOMATION_TESTS
    friend struct FMythicItemizationRegistryRefreshTestAccessor;
#endif
};
