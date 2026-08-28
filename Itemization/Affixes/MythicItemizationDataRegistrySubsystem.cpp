#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"

#include "Algo/Unique.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StreamableManager.h"
#include "Itemization/Affixes/MythicAffixCompiler.h"
#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicAffixPool.h"
#include "Itemization/Affixes/MythicAffixProfile.h"
#include "Itemization/Affixes/MythicAffixRollPolicy.h"
#include "Itemization/Affixes/MythicItemizationRuleset.h"
#include "Mythic/Mythic.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"
#include "System/MythicAssetManager.h"
#if WITH_EDITOR
#include "Async/Async.h"
#include "Engine/Engine.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"
#endif

struct FMythicAsyncLoadTransaction {
    ~FMythicAsyncLoadTransaction() {
        for (const TSharedPtr<FStreamableHandle> &Handle : Handles) {
            if (Handle.IsValid()) Handle->ReleaseHandle();
        }
    }
    TArray<TSharedPtr<FStreamableHandle>> Handles;
};

struct FMythicItemizationRegistryPublishedState {
    TArray<TObjectPtr<UObject>> LoadedAssets;
    TObjectPtr<UMythicItemizationRuleset> ActiveRuleset = nullptr;
    FMythicStatRegistry StatRegistry;
    TMap<FPrimaryAssetId, const UMythicAffixDefinition *> AffixesById;
    TMap<FGameplayTag, const UMythicAffixDefinition *> AffixesByTag;
    TMap<FPrimaryAssetId, const UMythicAffixPool *> PoolsById;
    TMap<FPrimaryAssetId, const UMythicAffixRollPolicy *> PoliciesById;
    TMap<FPrimaryAssetId, const UMythicAffixProfile *> ProfilesById;
    TMap<FPrimaryAssetId, const UMythicItemizationRuleset *> RulesetsById;
    TMap<FPrimaryAssetId, TSharedPtr<const FCompiledAffixProfile>> CompiledProfiles;
    TMap<FGuid, TSharedPtr<const FCompiledAffixGrantClosure>> CompiledGrants;
    bool bSemanticDataQuarantined = false;
};

namespace {
struct FMythicOneShotAsyncCompletion {
    explicit FMythicOneShotAsyncCompletion(TFunction<void(bool)> InCompletion)
        : Completion(MoveTemp(InCompletion)) {}
    void Complete(const bool bSuccess) {
        if (bCompleted.Exchange(true)) return;
        if (Completion) Completion(bSuccess);
        Completion = nullptr;
    }
    TAtomic<bool> bCompleted{false};
    TFunction<void(bool)> Completion;
};

bool GrantSpecsMatch(const FMythicAffixGrantSpec &A, const FMythicAffixGrantSpec &B) {
    return A.GrantGuid == B.GrantGuid && A.AffixDefinition == B.AffixDefinition
        && A.TierMode == B.TierMode && A.ExactTierRank == B.ExactTierRank
        && A.RollGroup == B.RollGroup && A.SourceKind == B.SourceKind
        && A.bLocked == B.bLocked;
}

FPrimaryAssetId GetSemanticAssetId(const UObject *Object) {
    if (const UMythicStatCategoryDefinition *Typed =
            Cast<UMythicStatCategoryDefinition>(Object)) return Typed->GetPrimaryAssetId();
    if (const UMythicStatDefinition *Typed =
            Cast<UMythicStatDefinition>(Object)) return Typed->GetPrimaryAssetId();
    if (const UMythicAffixDefinition *Typed =
            Cast<UMythicAffixDefinition>(Object)) return Typed->GetPrimaryAssetId();
    if (const UMythicAffixPool *Typed =
            Cast<UMythicAffixPool>(Object)) return Typed->GetPrimaryAssetId();
    if (const UMythicAffixRollPolicy *Typed =
            Cast<UMythicAffixRollPolicy>(Object)) return Typed->GetPrimaryAssetId();
    if (const UMythicAffixProfile *Typed =
            Cast<UMythicAffixProfile>(Object)) return Typed->GetPrimaryAssetId();
    if (const UMythicItemizationRuleset *Typed =
            Cast<UMythicItemizationRuleset>(Object)) return Typed->GetPrimaryAssetId();
    return {};
}

bool IsSemanticAsset(const UObject *Object) {
    return Object && (Object->IsA<UMythicStatCategoryDefinition>()
        || Object->IsA<UMythicStatDefinition>()
        || Object->IsA<UMythicAffixDefinition>()
        || Object->IsA<UMythicAffixPool>()
        || Object->IsA<UMythicAffixRollPolicy>()
        || Object->IsA<UMythicAffixProfile>()
        || Object->IsA<UMythicItemizationRuleset>());
}

void ResolveObjects(const TArray<FSoftObjectPath> &Paths, TArray<UObject *> &Out) {
    Out.Reset(Paths.Num());
    for (const FSoftObjectPath &Path : Paths) {
        if (UObject *Object = Path.ResolveObject()) Out.AddUnique(Object);
    }
}
}

void UMythicItemizationDataRegistrySubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);
    bAcceptingAsyncCompletions = true;
#if WITH_EDITOR
    PreObjectPropertyChangedHandle =
        FCoreUObjectDelegates::OnPreObjectPropertyChanged.AddUObject(
            this, &ThisClass::HandleResidentAssetPrePropertyChange);
    ObjectPropertyChangedHandle = FCoreUObjectDelegates::OnObjectPropertyChanged.AddUObject(
        this, &ThisClass::HandleResidentAssetPropertyChanged);
    ObjectsReplacedHandle = FCoreUObjectDelegates::OnObjectsReplaced.AddUObject(
        this, &ThisClass::HandleResidentObjectsReplaced);
#endif
    TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> WeakThis(this);
    RequestCoreSemanticDataAsync(FOnMythicItemizationDataReady::CreateLambda(
        [WeakThis](const bool bReady) {
            if (bReady && WeakThis.IsValid()) {
                WeakThis->RequestActiveRulesetAsync(FOnMythicItemizationDataReady());
            }
        }));
}

void UMythicItemizationDataRegistrySubsystem::Deinitialize() {
    bAcceptingAsyncCompletions = false;
#if WITH_EDITOR
    if (PreObjectPropertyChangedHandle.IsValid()) {
        FCoreUObjectDelegates::OnPreObjectPropertyChanged.Remove(
            PreObjectPropertyChangedHandle);
    }
    if (ObjectPropertyChangedHandle.IsValid()) {
        FCoreUObjectDelegates::OnObjectPropertyChanged.Remove(ObjectPropertyChangedHandle);
    }
    if (ObjectsReplacedHandle.IsValid()) {
        FCoreUObjectDelegates::OnObjectsReplaced.Remove(ObjectsReplacedHandle);
    }
    PreObjectPropertyChangedHandle.Reset();
    ObjectPropertyChangedHandle.Reset();
    ObjectsReplacedHandle.Reset();
    bEditorRefreshPending = false;
    bRestoringEditorPreEditSnapshots = false;
    PendingReplacementGrantSpecs.Reset();
    ClearResidentAssetPreEditSnapshots();
#endif
    if (AssetRegistryGatherHandle.IsValid()
        && FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry"))) {
        FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"))
            .Get().OnKnownGathersComplete().Remove(AssetRegistryGatherHandle);
    }
    AssetRegistryGatherHandle.Reset();
    ReadinessChanged.Clear();
    SemanticDataChanged.Clear();
    for (const TSharedPtr<FStreamableHandle> &Handle : ActiveLoadHandles) {
        if (Handle.IsValid()) Handle->CancelHandle();
    }
    ActiveLoadHandles.Reset();
    TArray<FOnMythicItemizationDataReady> Cancelled = MoveTemp(PendingCoreSemanticRequests);
    Cancelled.Append(MoveTemp(PendingActiveRulesetRequests));
    for (TPair<FPrimaryAssetId, TArray<FOnMythicItemizationDataReady>> &Pair
         : PendingProfileClosureRequests) Cancelled.Append(MoveTemp(Pair.Value));
    PendingCoreSemanticRequests.Reset();
    PendingActiveRulesetRequests.Reset();
    PendingProfileClosureRequests.Reset();
    ProfileClosureRequestsInFlight.Reset();
    bCoreSemanticRequestInFlight = false;
    bCoreSemanticDiscoveryStarted = false;
    bActiveRulesetRequestInFlight = false;
    LoadedAssets.Reset();
    ActiveRuleset = nullptr;
    StatRegistry.Reset();
    AffixesById.Reset();
    AffixesByTag.Reset();
    PoolsById.Reset();
    PoliciesById.Reset();
    ProfilesById.Reset();
    RulesetsById.Reset();
    CompiledProfiles.Reset();
    CompiledGrants.Reset();
    Readiness = EMythicItemizationReadiness::Uninitialized;
    SemanticDataRevision = 0;
    bSemanticRefreshInProgress = false;
    bSemanticDataQuarantined = false;
    for (FOnMythicItemizationDataReady &Completion : Cancelled) Completion.ExecuteIfBound(false);
    Super::Deinitialize();
}

void UMythicItemizationDataRegistrySubsystem::SetReadiness(
    const EMythicItemizationReadiness NewReadiness) {
    if (NewReadiness > Readiness) {
        Readiness = NewReadiness;
        ReadinessChanged.Broadcast(Readiness);
    }
}

bool UMythicItemizationDataRegistrySubsystem::IsProfileReady(
    const FPrimaryAssetId ProfileId) const {
    return !bSemanticDataQuarantined && ProfileId.IsValid()
        && CompiledProfiles.Contains(ProfileId);
}

void UMythicItemizationDataRegistrySubsystem::RequestPrimaryAssets(
    TArray<FPrimaryAssetId> AssetIds,
    TSharedRef<FMythicAsyncLoadTransaction> Transaction,
    TFunction<void(bool)> Completion) {
    if (!bAcceptingAsyncCompletions
        || AssetIds.ContainsByPredicate([](const FPrimaryAssetId &Id) { return !Id.IsValid(); })) {
        if (Completion) Completion(false);
        return;
    }
    AssetIds.Sort([](const FPrimaryAssetId &A, const FPrimaryAssetId &B) {
        return A.ToString() < B.ToString();
    });
    AssetIds.SetNum(Algo::Unique(AssetIds));
    TArray<FSoftObjectPath> Paths;
    for (const FPrimaryAssetId &Id : AssetIds) {
        const FSoftObjectPath Path = UMythicAssetManager::Get().GetPrimaryAssetPath(Id);
        if (!Path.IsValid()) {
            if (Completion) Completion(false);
            return;
        }
        Paths.Add(Path);
    }
    RequestSoftObjects(MoveTemp(Paths), Transaction, MoveTemp(Completion));
}

void UMythicItemizationDataRegistrySubsystem::RequestSoftObjects(
    TArray<FSoftObjectPath> ObjectPaths,
    TSharedRef<FMythicAsyncLoadTransaction> Transaction,
    TFunction<void(bool)> Completion) {
    if (!bAcceptingAsyncCompletions
        || ObjectPaths.ContainsByPredicate([](const FSoftObjectPath &Path) { return !Path.IsValid(); })) {
        if (Completion) Completion(false);
        return;
    }
    ObjectPaths.Sort([](const FSoftObjectPath &A, const FSoftObjectPath &B) {
        return A.ToString() < B.ToString();
    });
    ObjectPaths.SetNum(Algo::Unique(ObjectPaths));
    if (ObjectPaths.IsEmpty()) {
        if (Completion) Completion(true);
        return;
    }
    TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> WeakThis(this);
    const TSharedRef<FMythicOneShotAsyncCompletion> Gate =
        MakeShared<FMythicOneShotAsyncCompletion>(MoveTemp(Completion));
    TSharedPtr<FStreamableHandle> Handle =
        UMythicAssetManager::Get().GetStreamableManager().RequestAsyncLoad(
            ObjectPaths, FStreamableDelegate::CreateLambda(
                [WeakThis, Transaction, ObjectPaths, Gate]() {
                    if (!WeakThis.IsValid() || !WeakThis->bAcceptingAsyncCompletions) {
                        Gate->Complete(false);
                        return;
                    }
                    const bool bLoaded = !ObjectPaths.ContainsByPredicate(
                        [](const FSoftObjectPath &Path) { return Path.ResolveObject() == nullptr; });
                    Gate->Complete(bLoaded);
                }));
    if (Handle.IsValid()) Transaction->Handles.AddUnique(Handle);
    else {
        const bool bResolved = !ObjectPaths.ContainsByPredicate(
            [](const FSoftObjectPath &Path) { return Path.ResolveObject() == nullptr; });
        Gate->Complete(bResolved);
    }
}

void UMythicItemizationDataRegistrySubsystem::CommitLoadTransaction(
    const TSharedRef<FMythicAsyncLoadTransaction> &Transaction) {
    for (const TSharedPtr<FStreamableHandle> &Handle : Transaction->Handles) {
        if (Handle.IsValid()) ActiveLoadHandles.AddUnique(Handle);
    }
    Transaction->Handles.Reset();
}

bool UMythicItemizationDataRegistrySubsystem::GatherAssetRegistryCandidates(
    TArray<FSoftObjectPath> &OutPaths, TArray<FText> &OutErrors,
    const bool bCoreSemanticOnly) const {
    OutPaths.Reset();
    FAssetRegistryModule &Module =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    struct FScan { UClass *Class; FPrimaryAssetType Type; bool bCore; };
    const TArray<FScan> Scans = {
        {UMythicStatCategoryDefinition::StaticClass(), UMythicAssetManager::StatCategoryDefinitionType, true},
        {UMythicStatDefinition::StaticClass(), UMythicAssetManager::StatDefinitionType, true},
        {UMythicAffixDefinition::StaticClass(), UMythicAssetManager::AffixDefinitionType, true},
        {UMythicAffixPool::StaticClass(), UMythicAssetManager::AffixPoolType, false},
        {UMythicAffixRollPolicy::StaticClass(), UMythicAssetManager::AffixRollPolicyType, false},
        {UMythicAffixProfile::StaticClass(), UMythicAssetManager::AffixProfileType, false},
        {UMythicItemizationRuleset::StaticClass(), UMythicAssetManager::ItemizationRulesetType, false},
    };
    TMap<FPrimaryAssetId, FString> Claims;
    bool bValid = true;
    for (const FScan &Scan : Scans) {
        if (bCoreSemanticOnly && !Scan.bCore) continue;
        TArray<FAssetData> Assets;
        Module.Get().GetAssetsByClass(Scan.Class->GetClassPathName(), Assets, true);
        if (Assets.IsEmpty()) {
            OutErrors.Add(FText::FromString(FString::Printf(
                TEXT("No assets are registered for required itemization type %s."),
                *Scan.Type.ToString())));
            bValid = false;
            continue;
        }
        for (const FAssetData &Data : Assets) {
            const FPrimaryAssetId Id = Data.GetPrimaryAssetId();
            if (!Id.IsValid() || Id.PrimaryAssetType != Scan.Type || Claims.Contains(Id)) {
                OutErrors.Add(FText::FromString(FString::Printf(
                    TEXT("Itemization asset %s has duplicate or invalid primary identity %s."),
                    *Data.GetObjectPathString(), *Id.ToString())));
                bValid = false;
                continue;
            }
            Claims.Add(Id, Data.GetObjectPathString());
            OutPaths.Add(Data.GetSoftObjectPath());
        }
    }
    OutPaths.Sort([](const FSoftObjectPath &A, const FSoftObjectPath &B) {
        return A.ToString() < B.ToString();
    });
    return bValid;
}

void UMythicItemizationDataRegistrySubsystem::RequestCoreSemanticDataAsync(
    FOnMythicItemizationDataReady Completion) {
    if (!bAcceptingAsyncCompletions) {
        Completion.ExecuteIfBound(false);
        return;
    }
    if (IsCoreSemanticReady()) {
        Completion.ExecuteIfBound(true);
        return;
    }
    PendingCoreSemanticRequests.Add(Completion);
    if (bCoreSemanticRequestInFlight) return;
    bCoreSemanticRequestInFlight = true;
    FAssetRegistryModule &Module =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    if (Module.Get().IsGathering()) {
        TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> WeakThis(this);
        AssetRegistryGatherHandle = Module.Get().OnKnownGathersComplete().AddLambda([WeakThis]() {
            if (WeakThis.IsValid()) WeakThis->BeginCoreSemanticDiscovery();
        });
        if (Module.Get().IsGathering()) return;
    }
    BeginCoreSemanticDiscovery();
}

void UMythicItemizationDataRegistrySubsystem::BeginCoreSemanticDiscovery() {
    if (!bAcceptingAsyncCompletions || !bCoreSemanticRequestInFlight
        || bCoreSemanticDiscoveryStarted) return;
    bCoreSemanticDiscoveryStarted = true;
    if (AssetRegistryGatherHandle.IsValid()) {
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"))
            .Get().OnKnownGathersComplete().Remove(AssetRegistryGatherHandle);
        AssetRegistryGatherHandle.Reset();
    }
    TArray<FSoftObjectPath> Paths;
    TArray<FText> Errors;
    if (!GatherAssetRegistryCandidates(Paths, Errors, true)) {
        CompleteCoreSemanticRequest(false);
        return;
    }
    const TSharedRef<FMythicAsyncLoadTransaction> Transaction =
        MakeShared<FMythicAsyncLoadTransaction>();
    TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> WeakThis(this);
    RequestSoftObjects(Paths, Transaction,
        [WeakThis, Transaction, Paths](const bool bLoaded) {
            if (!WeakThis.IsValid() || !WeakThis->bAcceptingAsyncCompletions) return;
            FMythicItemizationRegistryPublishedState Previous =
                WeakThis->CapturePublishedState();
            TArray<UObject *> Objects;
            ResolveObjects(Paths, Objects);
            TArray<FText> BuildErrors;
            const bool bSuccess = bLoaded && Objects.Num() == Paths.Num()
                && WeakThis->PublishLoadedAssetsInternal(Objects, BuildErrors, false)
                && WeakThis->StatRegistry.IsBuilt() && !WeakThis->AffixesById.IsEmpty();
            if (bSuccess) {
                WeakThis->CommitLoadTransaction(Transaction);
                WeakThis->SetReadiness(EMythicItemizationReadiness::CoreSemanticReady);
            }
            else {
                WeakThis->RestorePublishedState(MoveTemp(Previous));
            }
            WeakThis->CompleteCoreSemanticRequest(bSuccess);
        });
}

void UMythicItemizationDataRegistrySubsystem::CompleteCoreSemanticRequest(
    const bool bSuccess) {
    bCoreSemanticRequestInFlight = false;
    bCoreSemanticDiscoveryStarted = false;
    TArray<FOnMythicItemizationDataReady> Pending = MoveTemp(PendingCoreSemanticRequests);
    PendingCoreSemanticRequests.Reset();
    for (FOnMythicItemizationDataReady &Completion : Pending) Completion.ExecuteIfBound(bSuccess);
}

void UMythicItemizationDataRegistrySubsystem::RequestActiveRulesetAsync(
    FOnMythicItemizationDataReady Completion) {
    if (!bAcceptingAsyncCompletions) {
        Completion.ExecuteIfBound(false);
        return;
    }
    if (IsActiveRulesetReady()) {
        Completion.ExecuteIfBound(true);
        return;
    }
    PendingActiveRulesetRequests.Add(Completion);
    if (bActiveRulesetRequestInFlight) return;
    bActiveRulesetRequestInFlight = true;
    if (!IsCoreSemanticReady()) {
        TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> WeakThis(this);
        RequestCoreSemanticDataAsync(FOnMythicItemizationDataReady::CreateLambda(
            [WeakThis](const bool bReady) {
                if (!WeakThis.IsValid()) return;
                if (bReady) WeakThis->BeginActiveRulesetRequest();
                else WeakThis->CompleteActiveRulesetRequest(false);
            }));
        return;
    }
    BeginActiveRulesetRequest();
}

void UMythicItemizationDataRegistrySubsystem::BeginActiveRulesetRequest() {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const FSoftObjectPath RulesetPath = Settings
        ? Settings->ActiveItemizationRuleset.ToSoftObjectPath() : FSoftObjectPath();
    if (!RulesetPath.IsValid()) {
        CompleteActiveRulesetRequest(false);
        return;
    }
    const TSharedRef<FMythicAsyncLoadTransaction> Transaction =
        MakeShared<FMythicAsyncLoadTransaction>();
    TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> WeakThis(this);
    RequestSoftObjects({RulesetPath}, Transaction,
        [WeakThis, Transaction, RulesetPath](const bool bRulesetLoaded) {
            if (!WeakThis.IsValid() || !WeakThis->bAcceptingAsyncCompletions) return;
            UMythicItemizationRuleset *Ruleset =
                Cast<UMythicItemizationRuleset>(RulesetPath.ResolveObject());
            if (!bRulesetLoaded || !Ruleset || Ruleset->Profiles.IsEmpty()) {
                WeakThis->CompleteActiveRulesetRequest(false);
                return;
            }
            TArray<FSoftObjectPath> ProfilePaths;
            for (const TSoftObjectPtr<UMythicAffixProfile> &Profile : Ruleset->Profiles) {
                if (Profile.IsNull()) {
                    WeakThis->CompleteActiveRulesetRequest(false);
                    return;
                }
                ProfilePaths.Add(Profile.ToSoftObjectPath());
            }
            WeakThis->RequestSoftObjects(ProfilePaths, Transaction,
                [WeakThis, Transaction, RulesetPath, ProfilePaths](const bool bProfilesLoaded) {
                    if (!WeakThis.IsValid() || !WeakThis->bAcceptingAsyncCompletions) return;
                    if (!bProfilesLoaded) {
                        WeakThis->CompleteActiveRulesetRequest(false);
                        return;
                    }
                    TArray<FSoftObjectPath> DependencyPaths;
                    for (const FSoftObjectPath &Path : ProfilePaths) {
                        const UMythicAffixProfile *Profile = Cast<UMythicAffixProfile>(Path.ResolveObject());
                        if (!Profile || Profile->RollPolicy.Asset.IsNull()) {
                            WeakThis->CompleteActiveRulesetRequest(false);
                            return;
                        }
                        DependencyPaths.Add(Profile->RollPolicy.Asset.ToSoftObjectPath());
                        for (const FMythicAffixPoolSlice &Slice : Profile->RandomPoolSlices) {
                            if (Slice.Pool.Asset.IsNull()) {
                                WeakThis->CompleteActiveRulesetRequest(false);
                                return;
                            }
                            DependencyPaths.Add(Slice.Pool.Asset.ToSoftObjectPath());
                        }
                    }
                    WeakThis->RequestSoftObjects(DependencyPaths, Transaction,
                        [WeakThis, Transaction, RulesetPath, ProfilePaths,
                         DependencyPaths](const bool bDependenciesLoaded) {
                            if (!WeakThis.IsValid() || !WeakThis->bAcceptingAsyncCompletions) return;
                            TArray<UObject *> Graph;
                            TArray<FSoftObjectPath> RulesetPaths;
                            RulesetPaths.Add(RulesetPath);
                            ResolveObjects(RulesetPaths, Graph);
                            TArray<UObject *> More;
                            ResolveObjects(ProfilePaths, More);
                            for (UObject *Object : More) Graph.AddUnique(Object);
                            ResolveObjects(DependencyPaths, More);
                            for (UObject *Object : More) Graph.AddUnique(Object);
                            FMythicItemizationRegistryPublishedState Previous =
                                WeakThis->CapturePublishedState();
                            TArray<FText> Errors;
                            bool bSuccess = bDependenciesLoaded
                                && WeakThis->PublishLoadedAssetsInternal(Graph, Errors, false);
                            UMythicItemizationRuleset *Ruleset =
                                Cast<UMythicItemizationRuleset>(RulesetPath.ResolveObject());
                            if (bSuccess && Ruleset) {
                                for (const TSoftObjectPtr<UMythicAffixProfile> &ProfileRef : Ruleset->Profiles) {
                                    const UMythicAffixProfile *Profile = ProfileRef.Get();
                                    bSuccess &= Profile
                                        && WeakThis->CompileProfile(Profile->GetPrimaryAssetId(), Errors);
                                }
                            }
                            WeakThis->ActiveRuleset = bSuccess ? Ruleset : nullptr;
                            bSuccess &= WeakThis->ValidateActiveRuleset(Errors);
                            if (bSuccess) {
                                WeakThis->CommitLoadTransaction(Transaction);
                                WeakThis->SetReadiness(EMythicItemizationReadiness::ActiveRulesetReady);
                            }
                            else {
                                WeakThis->RestorePublishedState(MoveTemp(Previous));
                            }
                            WeakThis->CompleteActiveRulesetRequest(bSuccess);
                        });
                });
        });
}

void UMythicItemizationDataRegistrySubsystem::CompleteActiveRulesetRequest(
    const bool bSuccess) {
    bActiveRulesetRequestInFlight = false;
    TArray<FOnMythicItemizationDataReady> Pending = MoveTemp(PendingActiveRulesetRequests);
    PendingActiveRulesetRequests.Reset();
    for (FOnMythicItemizationDataReady &Completion : Pending) Completion.ExecuteIfBound(bSuccess);
}

void UMythicItemizationDataRegistrySubsystem::RequestProfileClosureAsync(
    const FPrimaryAssetId ProfileId, FOnMythicItemizationDataReady Completion) {
    if (!bAcceptingAsyncCompletions || !ProfileId.IsValid()
        || ProfileId.PrimaryAssetType != UMythicAssetManager::AffixProfileType) {
        Completion.ExecuteIfBound(false);
        return;
    }
    if (IsProfileReady(ProfileId)) {
        Completion.ExecuteIfBound(true);
        return;
    }
    PendingProfileClosureRequests.FindOrAdd(ProfileId).Add(Completion);
    if (ProfileClosureRequestsInFlight.Contains(ProfileId)) return;
    ProfileClosureRequestsInFlight.Add(ProfileId);
    if (!IsCoreSemanticReady()) {
        TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> WeakThis(this);
        RequestCoreSemanticDataAsync(FOnMythicItemizationDataReady::CreateLambda(
            [WeakThis, ProfileId](const bool bReady) {
                if (!WeakThis.IsValid()) return;
                if (bReady) WeakThis->BeginProfileClosureRequest(ProfileId);
                else WeakThis->CompleteProfileClosureRequest(ProfileId, false);
            }));
        return;
    }
    BeginProfileClosureRequest(ProfileId);
}

void UMythicItemizationDataRegistrySubsystem::BeginProfileClosureRequest(
    const FPrimaryAssetId ProfileId) {
    const TSharedRef<FMythicAsyncLoadTransaction> Transaction =
        MakeShared<FMythicAsyncLoadTransaction>();
    TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> WeakThis(this);
    RequestPrimaryAssets({ProfileId}, Transaction,
        [WeakThis, Transaction, ProfileId](const bool bProfileLoaded) {
            if (!WeakThis.IsValid() || !WeakThis->bAcceptingAsyncCompletions) return;
            const UMythicAffixProfile *Profile = Cast<UMythicAffixProfile>(
                UMythicAssetManager::Get().GetPrimaryAssetObject(ProfileId));
            if (!bProfileLoaded || !Profile || Profile->RollPolicy.Asset.IsNull()) {
                WeakThis->CompleteProfileClosureRequest(ProfileId, false);
                return;
            }
            TArray<FSoftObjectPath> Dependencies{Profile->RollPolicy.Asset.ToSoftObjectPath()};
            for (const FMythicAffixPoolSlice &Slice : Profile->RandomPoolSlices) {
                if (Slice.Pool.Asset.IsNull()) {
                    WeakThis->CompleteProfileClosureRequest(ProfileId, false);
                    return;
                }
                Dependencies.Add(Slice.Pool.Asset.ToSoftObjectPath());
            }
            WeakThis->RequestSoftObjects(Dependencies, Transaction,
                [WeakThis, Transaction, ProfileId, Dependencies](const bool bDependenciesLoaded) {
                    if (!WeakThis.IsValid() || !WeakThis->bAcceptingAsyncCompletions) return;
                    TArray<UObject *> Graph;
                    if (UObject *ProfileObject = UMythicAssetManager::Get()
                            .GetPrimaryAssetObject(ProfileId)) Graph.Add(ProfileObject);
                    TArray<UObject *> More;
                    ResolveObjects(Dependencies, More);
                    for (UObject *Object : More) Graph.AddUnique(Object);
                    FMythicItemizationRegistryPublishedState Previous =
                        WeakThis->CapturePublishedState();
                    TArray<FText> Errors;
                    const bool bSuccess = bDependenciesLoaded
                        && WeakThis->PublishLoadedAssetsInternal(Graph, Errors, false)
                        && WeakThis->CompileProfile(ProfileId, Errors);
                    if (bSuccess) WeakThis->CommitLoadTransaction(Transaction);
                    else {
                        WeakThis->RestorePublishedState(MoveTemp(Previous));
                    }
                    WeakThis->CompleteProfileClosureRequest(ProfileId, bSuccess);
                });
        });
}

void UMythicItemizationDataRegistrySubsystem::CompleteProfileClosureRequest(
    const FPrimaryAssetId ProfileId, const bool bSuccess) {
    ProfileClosureRequestsInFlight.Remove(ProfileId);
    TArray<FOnMythicItemizationDataReady> Pending;
    PendingProfileClosureRequests.RemoveAndCopyValue(ProfileId, Pending);
    for (FOnMythicItemizationDataReady &Completion : Pending) Completion.ExecuteIfBound(bSuccess);
}

void UMythicItemizationDataRegistrySubsystem::RequestGrantClosureAsync(
    TConstArrayView<FMythicAffixGrantSpec> Grants,
    FOnMythicItemizationDataReady Completion) {
    if (!bAcceptingAsyncCompletions) {
        Completion.ExecuteIfBound(false);
        return;
    }
    TArray<FMythicAffixGrantSpec> Copy;
    Copy.Append(Grants.GetData(), Grants.Num());
    auto Compile = [WeakThis = TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem>(this),
                    Copy = MoveTemp(Copy), Completion]() mutable {
        if (!WeakThis.IsValid()) {
            Completion.ExecuteIfBound(false);
            return;
        }
        TArray<FText> Errors;
        Completion.ExecuteIfBound(WeakThis->CompileGrantClosures(Copy, Errors));
    };
    if (IsCoreSemanticReady()) Compile();
    else RequestCoreSemanticDataAsync(FOnMythicItemizationDataReady::CreateLambda(
        [Compile = MoveTemp(Compile), Completion](const bool bReady) mutable {
            if (bReady) Compile();
            else Completion.ExecuteIfBound(false);
        }));
}

bool UMythicItemizationDataRegistrySubsystem::RebuildIndexes(TArray<FText> &OutErrors) {
    TArray<UMythicStatCategoryDefinition *> Categories;
    TArray<UMythicStatDefinition *> Stats;
    TMap<FPrimaryAssetId, const UMythicAffixDefinition *> NewAffixes;
    TMap<FGameplayTag, const UMythicAffixDefinition *> NewAffixesByTag;
    TMap<FPrimaryAssetId, const UMythicAffixPool *> NewPools;
    TMap<FPrimaryAssetId, const UMythicAffixRollPolicy *> NewPolicies;
    TMap<FPrimaryAssetId, const UMythicAffixProfile *> NewProfiles;
    TMap<FPrimaryAssetId, const UMythicItemizationRuleset *> NewRulesets;
    TMap<FPrimaryAssetId, FString> PrimaryClaims;
    TMap<FGameplayTag, FString> TagClaims;
    TMap<FGuid, FString> GuidClaims;
    bool bValid = true;
    auto Error = [&OutErrors, &bValid](const FString &Message) {
        OutErrors.Add(FText::FromString(Message));
        bValid = false;
    };
    auto ClaimPrimary = [&PrimaryClaims, &Error](const FPrimaryAssetId &Id,
                                                 const FPrimaryAssetType &Type,
                                                 const FString &Owner) {
        if (!Id.IsValid() || Id.PrimaryAssetType != Type || PrimaryClaims.Contains(Id)) {
            Error(FString::Printf(TEXT("%s has duplicate/invalid primary identity %s."),
                                  *Owner, *Id.ToString()));
            return false;
        }
        PrimaryClaims.Add(Id, Owner);
        return true;
    };
    auto ClaimTag = [&TagClaims, &Error](const FGameplayTag &Tag, const FString &Owner) {
        if (!Tag.IsValid() || TagClaims.Contains(Tag)) {
            Error(FString::Printf(TEXT("%s has duplicate/invalid semantic tag %s."),
                                  *Owner, *Tag.ToString()));
            return false;
        }
        TagClaims.Add(Tag, Owner);
        return true;
    };
    auto ClaimGuid = [&GuidClaims, &Error](const FGuid &Guid, const FString &Owner) {
        if (!Guid.IsValid() || GuidClaims.Contains(Guid)) {
            Error(FString::Printf(TEXT("%s has duplicate/invalid deterministic GUID %s."),
                                  *Owner, *Guid.ToString()));
            return false;
        }
        GuidClaims.Add(Guid, Owner);
        return true;
    };

    for (UObject *Object : LoadedAssets) {
        if (!Object) {
            Error(TEXT("The resident itemization semantic closure contains a null object."));
        }
        else if (UMythicStatCategoryDefinition *Category = Cast<UMythicStatCategoryDefinition>(Object)) {
            ClaimPrimary(Category->GetPrimaryAssetId(), UMythicAssetManager::StatCategoryDefinitionType,
                         Category->GetPathName());
            ClaimTag(Category->CategoryTag, Category->GetPathName());
            Categories.Add(Category);
        }
        else if (UMythicStatDefinition *Stat = Cast<UMythicStatDefinition>(Object)) {
            ClaimPrimary(Stat->GetPrimaryAssetId(), UMythicAssetManager::StatDefinitionType,
                         Stat->GetPathName());
            ClaimTag(Stat->StatTag, Stat->GetPathName());
            Stats.Add(Stat);
        }
        else if (const UMythicAffixDefinition *Affix = Cast<UMythicAffixDefinition>(Object)) {
            if (ClaimPrimary(Affix->GetPrimaryAssetId(), UMythicAssetManager::AffixDefinitionType,
                             Affix->GetPathName())
                && ClaimTag(Affix->AffixTag, Affix->GetPathName())) {
                NewAffixes.Add(Affix->GetPrimaryAssetId(), Affix);
                NewAffixesByTag.Add(Affix->AffixTag, Affix);
            }
        }
        else if (const UMythicAffixPool *Pool = Cast<UMythicAffixPool>(Object)) {
            if (ClaimPrimary(Pool->GetPrimaryAssetId(), UMythicAssetManager::AffixPoolType,
                             Pool->GetPathName()) && ClaimTag(Pool->PoolTag, Pool->GetPathName()))
                NewPools.Add(Pool->GetPrimaryAssetId(), Pool);
        }
        else if (const UMythicAffixRollPolicy *Policy = Cast<UMythicAffixRollPolicy>(Object)) {
            if (ClaimPrimary(Policy->GetPrimaryAssetId(), UMythicAssetManager::AffixRollPolicyType,
                             Policy->GetPathName()) && ClaimTag(Policy->PolicyTag, Policy->GetPathName()))
                NewPolicies.Add(Policy->GetPrimaryAssetId(), Policy);
        }
        else if (const UMythicAffixProfile *Profile = Cast<UMythicAffixProfile>(Object)) {
            if (ClaimPrimary(Profile->GetPrimaryAssetId(), UMythicAssetManager::AffixProfileType,
                             Profile->GetPathName()) && ClaimTag(Profile->ProfileTag, Profile->GetPathName()))
                NewProfiles.Add(Profile->GetPrimaryAssetId(), Profile);
        }
        else if (const UMythicItemizationRuleset *Ruleset = Cast<UMythicItemizationRuleset>(Object)) {
            if (ClaimPrimary(Ruleset->GetPrimaryAssetId(), UMythicAssetManager::ItemizationRulesetType,
                             Ruleset->GetPathName()) && ClaimTag(Ruleset->RulesetTag, Ruleset->GetPathName()))
                NewRulesets.Add(Ruleset->GetPrimaryAssetId(), Ruleset);
        }
        else {
            Error(FString::Printf(
                TEXT("Resident object %s is not a supported itemization semantic asset."),
                *Object->GetPathName()));
        }
    }

    FMythicStatRegistry NewStatRegistry;
    if ((!Categories.IsEmpty() || !Stats.IsEmpty())
        && !NewStatRegistry.Build(Categories, Stats, OutErrors)) bValid = false;
    for (const TPair<FPrimaryAssetId, const UMythicAffixDefinition *> &Pair : NewAffixes) {
        if (!FMythicAffixCompiler::ValidateDefinitionAuthoring(*Pair.Value, NewStatRegistry,
                                                               OutErrors)) bValid = false;
    }
    for (const TPair<FPrimaryAssetId, const UMythicAffixPool *> &Pair : NewPools) {
        if (!FMythicAffixCompiler::ValidatePoolAuthoring(*Pair.Value, OutErrors)) bValid = false;
        for (const FMythicAffixPoolEntry &Row : Pair.Value->Entries) {
            ClaimGuid(Row.PoolRowGuid, Pair.Value->GetPathName());
            if (!NewAffixes.Contains(Row.AffixDefinition.GetPrimaryAssetId())) {
                Error(FString::Printf(TEXT("Pool %s references an unloaded Affix Definition asset."),
                                      *Pair.Value->GetPathName()));
            }
        }
    }
    for (const TPair<FPrimaryAssetId, const UMythicAffixRollPolicy *> &Pair : NewPolicies) {
        if (Pair.Value->Revision < 1 || Pair.Value->AlgorithmVersion != 1) {
            Error(FString::Printf(TEXT("Roll policy %s has unsupported metadata/algorithm."),
                                  *Pair.Value->GetPathName()));
        }
    }
    for (const TPair<FPrimaryAssetId, const UMythicAffixProfile *> &Pair : NewProfiles) {
        const UMythicAffixProfile &Profile = *Pair.Value;
        if (!NewPolicies.Contains(Profile.RollPolicy.GetPrimaryAssetId())) {
            Error(FString::Printf(TEXT("Profile %s has an unloaded direct Roll Policy reference."),
                                  *Profile.GetPathName()));
        }
        for (const FMythicAffixGrantSpec &Grant : Profile.GuaranteedGrants) {
            ClaimGuid(Grant.GrantGuid, Profile.GetPathName());
            if (!NewAffixes.Contains(Grant.AffixDefinition.GetPrimaryAssetId())
                || !MythicAffixGrant::IsTierSelectionValid(Grant.TierMode,
                                                           Grant.ExactTierRank)) {
                Error(FString::Printf(TEXT("Profile %s has an invalid direct guaranteed grant."),
                                      *Profile.GetPathName()));
            }
        }
        TSet<FPrimaryAssetId> UsedPools;
        for (const FMythicAffixPoolSlice &Slice : Profile.RandomPoolSlices) {
            ClaimGuid(Slice.SliceGuid, Profile.GetPathName());
            const FPrimaryAssetId PoolId = Slice.Pool.GetPrimaryAssetId();
            if (!NewPools.Contains(PoolId) || UsedPools.Contains(PoolId)) {
                Error(FString::Printf(TEXT("Profile %s has an invalid/duplicate direct pool slice."),
                                      *Profile.GetPathName()));
            }
            UsedPools.Add(PoolId);
        }
    }
    for (const TPair<FPrimaryAssetId, const UMythicItemizationRuleset *> &Pair : NewRulesets) {
        TSet<FPrimaryAssetId> UsedProfiles;
        for (const TSoftObjectPtr<UMythicAffixProfile> &Profile : Pair.Value->Profiles) {
            const UMythicAffixProfile *Loaded = Profile.Get();
            const FPrimaryAssetId ProfileId = Loaded ? Loaded->GetPrimaryAssetId()
                : UMythicAssetManager::Get().GetPrimaryAssetIdForPath(Profile.ToSoftObjectPath());
            if (!Loaded || !NewProfiles.Contains(ProfileId) || UsedProfiles.Contains(ProfileId)) {
                Error(FString::Printf(TEXT("Ruleset %s has an invalid/duplicate direct profile reference."),
                                      *Pair.Value->GetPathName()));
            }
            UsedProfiles.Add(ProfileId);
        }
    }
    if (!bValid) return false;
    StatRegistry = MoveTemp(NewStatRegistry);
    AffixesById = MoveTemp(NewAffixes);
    AffixesByTag = MoveTemp(NewAffixesByTag);
    PoolsById = MoveTemp(NewPools);
    PoliciesById = MoveTemp(NewPolicies);
    ProfilesById = MoveTemp(NewProfiles);
    RulesetsById = MoveTemp(NewRulesets);
    return true;
}

FMythicItemizationRegistryPublishedState
UMythicItemizationDataRegistrySubsystem::CapturePublishedState() const {
    FMythicItemizationRegistryPublishedState State;
    State.LoadedAssets = LoadedAssets;
    State.ActiveRuleset = ActiveRuleset;
    State.StatRegistry = StatRegistry;
    State.AffixesById = AffixesById;
    State.AffixesByTag = AffixesByTag;
    State.PoolsById = PoolsById;
    State.PoliciesById = PoliciesById;
    State.ProfilesById = ProfilesById;
    State.RulesetsById = RulesetsById;
    State.CompiledProfiles = CompiledProfiles;
    State.CompiledGrants = CompiledGrants;
    State.bSemanticDataQuarantined = bSemanticDataQuarantined;
    return State;
}

void UMythicItemizationDataRegistrySubsystem::RestorePublishedState(
    FMythicItemizationRegistryPublishedState &&State) {
    LoadedAssets = MoveTemp(State.LoadedAssets);
    ActiveRuleset = State.ActiveRuleset;
    StatRegistry = MoveTemp(State.StatRegistry);
    AffixesById = MoveTemp(State.AffixesById);
    AffixesByTag = MoveTemp(State.AffixesByTag);
    PoolsById = MoveTemp(State.PoolsById);
    PoliciesById = MoveTemp(State.PoliciesById);
    ProfilesById = MoveTemp(State.ProfilesById);
    RulesetsById = MoveTemp(State.RulesetsById);
    CompiledProfiles = MoveTemp(State.CompiledProfiles);
    CompiledGrants = MoveTemp(State.CompiledGrants);
    bSemanticDataQuarantined = State.bSemanticDataQuarantined;
}

bool UMythicItemizationDataRegistrySubsystem::StageResidentCompiledClosures(
    TMap<FPrimaryAssetId, TSharedPtr<const FCompiledAffixProfile>> &OutProfiles,
    TMap<FGuid, TSharedPtr<const FCompiledAffixGrantClosure>> &OutGrants,
    TArray<FText> &OutErrors) const {
    OutProfiles.Reset();
    OutGrants.Reset();

    TArray<FPrimaryAssetId> ProfileIds;
    CompiledProfiles.GetKeys(ProfileIds);
    if (ActiveRuleset) {
        for (const TSoftObjectPtr<UMythicAffixProfile> &ProfileRef : ActiveRuleset->Profiles) {
            const UMythicAffixProfile *Profile = ProfileRef.Get();
            if (!Profile) {
                OutErrors.Add(FText::FromString(
                    TEXT("The active Itemization Ruleset contains a non-resident profile during refresh.")));
                return false;
            }
            ProfileIds.AddUnique(Profile->GetPrimaryAssetId());
        }
    }
    ProfileIds.Sort([](const FPrimaryAssetId &A, const FPrimaryAssetId &B) {
        return A.ToString() < B.ToString();
    });
    for (const FPrimaryAssetId &ProfileId : ProfileIds) {
        const UMythicAffixProfile *Profile = FindProfile(ProfileId);
        TSharedPtr<const FCompiledAffixProfile> Compiled;
        if (!Profile || !FMythicAffixCompiler::Compile(
                *Profile, *this, Compiled, OutErrors) || !Compiled.IsValid()) {
            OutErrors.Add(FText::FromString(FString::Printf(
                TEXT("Resident Affix Profile %s could not be recompiled."),
                *ProfileId.ToString())));
            return false;
        }
        OutProfiles.Add(ProfileId, MoveTemp(Compiled));
    }

    TArray<FGuid> GrantGuids;
    CompiledGrants.GetKeys(GrantGuids);
    GrantGuids.Sort();
    for (const FGuid &GrantGuid : GrantGuids) {
        const TSharedPtr<const FCompiledAffixGrantClosure> *Existing =
            CompiledGrants.Find(GrantGuid);
        if (!Existing || !Existing->IsValid()
            || (*Existing)->Spec.GrantGuid != GrantGuid) {
            OutErrors.Add(FText::FromString(FString::Printf(
                TEXT("Resident affix grant cache entry %s is invalid."),
                *GrantGuid.ToString())));
            return false;
        }
        FMythicAffixGrantSpec CanonicalSpec = (*Existing)->Spec;
#if WITH_EDITOR
        if (const TSharedPtr<FMythicAffixGrantSpec> *ReplacementSpec =
                PendingReplacementGrantSpecs.Find(GrantGuid);
            ReplacementSpec && ReplacementSpec->IsValid()) {
            CanonicalSpec = **ReplacementSpec;
        }
#endif
        if (const UMythicAffixDefinition *CanonicalDefinition =
                FindAffix(CanonicalSpec.AffixDefinition.GetPrimaryAssetId())) {
            CanonicalSpec.AffixDefinition.SetAsset(
                const_cast<UMythicAffixDefinition *>(CanonicalDefinition));
        }
        TSharedPtr<const FCompiledAffixGrantClosure> Compiled;
        if (!FMythicAffixCompiler::CompileGrant(
                CanonicalSpec, *this, Compiled, OutErrors) || !Compiled.IsValid()) {
            OutErrors.Add(FText::FromString(FString::Printf(
                TEXT("Resident affix grant %s could not be recompiled."),
                *GrantGuid.ToString())));
            return false;
        }
        OutGrants.Add(GrantGuid, MoveTemp(Compiled));
    }
    return true;
}

bool UMythicItemizationDataRegistrySubsystem::PublishLoadedAssetsInternal(
    TConstArrayView<UObject *> Assets,
    TArray<FText> &OutErrors,
    const bool bBroadcastSemanticChange) {
    if (!IsInGameThread()) {
        OutErrors.Add(FText::FromString(
            TEXT("Itemization semantic data can only be published on the game thread.")));
        return false;
    }
    if (bSemanticRefreshInProgress) {
        OutErrors.Add(FText::FromString(
            TEXT("An itemization semantic-data publication is already in progress.")));
        return false;
    }

    bool bCommitted = false;
    bool bRecoveredFromQuarantine = false;
    {
        TGuardValue<bool> RefreshGuard(bSemanticRefreshInProgress, true);
        FMythicItemizationRegistryPublishedState Previous = CapturePublishedState();
        const bool bWasQuarantined = Previous.bSemanticDataQuarantined;
        const FPrimaryAssetId ActiveRulesetId = ActiveRuleset
            ? ActiveRuleset->GetPrimaryAssetId() : FPrimaryAssetId();

        TMap<FPrimaryAssetId, UObject *> BatchClaims;
        for (UObject *Asset : Assets) {
            if (!Asset) {
                continue;
            }
            if (!IsSemanticAsset(Asset)) {
                OutErrors.Add(FText::FromString(FString::Printf(
                    TEXT("Unsupported object %s was supplied to the itemization semantic registry."),
                    *Asset->GetPathName())));
                RestorePublishedState(MoveTemp(Previous));
                return false;
            }
            const FPrimaryAssetId AssetId = GetSemanticAssetId(Asset);

            if (AssetId.IsValid()) {
                if (UObject **Claim = BatchClaims.Find(AssetId)) {
                    if (*Claim != Asset) {
                        OutErrors.Add(FText::FromString(FString::Printf(
                            TEXT("A semantic publication contains two assets claiming %s."),
                            *AssetId.ToString())));
                        RestorePublishedState(MoveTemp(Previous));
                        return false;
                    }
                }
                else {
                    BatchClaims.Add(AssetId, Asset);
                }
            }

            int32 ReplacementIndex = INDEX_NONE;
            if (AssetId.IsValid()) {
                for (int32 Index = 0; Index < LoadedAssets.Num(); ++Index) {
                    UObject *Existing = LoadedAssets[Index];
                    if (!Existing || Existing == Asset) {
                        continue;
                    }
                    if (GetSemanticAssetId(Existing) == AssetId) {
                        ReplacementIndex = Index;
                        break;
                    }
                }
            }
            if (ReplacementIndex != INDEX_NONE) {
                LoadedAssets[ReplacementIndex] = Asset;
            }
            else {
                LoadedAssets.AddUnique(Asset);
            }
        }

        if (!RebuildIndexes(OutErrors)) {
            RestorePublishedState(MoveTemp(Previous));
            return false;
        }

        // Staging compilers resolve through the same public lookup surface as runtime consumers. Publication owns
        // the game thread and has already rebuilt every index, so temporarily open that surface for staging; any
        // failure below restores the quarantined state before control returns to external code.
        bSemanticDataQuarantined = false;
        if (ActiveRulesetId.IsValid()) {
            const UMythicItemizationRuleset *const *ResidentRuleset =
                RulesetsById.Find(ActiveRulesetId);
            if (!ResidentRuleset || !*ResidentRuleset) {
                OutErrors.Add(FText::FromString(
                    TEXT("The active Itemization Ruleset is missing from the refreshed resident closure.")));
                RestorePublishedState(MoveTemp(Previous));
                return false;
            }
            ActiveRuleset = const_cast<UMythicItemizationRuleset *>(*ResidentRuleset);
        }

        TMap<FPrimaryAssetId, TSharedPtr<const FCompiledAffixProfile>> StagedProfiles;
        TMap<FGuid, TSharedPtr<const FCompiledAffixGrantClosure>> StagedGrants;
        if (!StageResidentCompiledClosures(StagedProfiles, StagedGrants, OutErrors)) {
            RestorePublishedState(MoveTemp(Previous));
            return false;
        }
        CompiledProfiles = MoveTemp(StagedProfiles);
        CompiledGrants = MoveTemp(StagedGrants);
        if (Readiness >= EMythicItemizationReadiness::ActiveRulesetReady
            && !ValidateActiveRuleset(OutErrors)) {
            RestorePublishedState(MoveTemp(Previous));
            return false;
        }
        bSemanticDataQuarantined = false;
        bRecoveredFromQuarantine = bWasQuarantined;
#if WITH_EDITOR
        PendingReplacementGrantSpecs.Reset();
        // Any publication rebuilds from the current resident graph, so it also satisfies a queued editor refresh.
        // The queued task re-checks this flag before doing work and cannot emit a duplicate recovery revision.
        bEditorRefreshPending = false;
#endif
        bCommitted = true;
    }

    if (bCommitted && (bBroadcastSemanticChange || bRecoveredFromQuarantine)) {
#if WITH_EDITOR
        ClearResidentAssetPreEditSnapshots();
#endif
        TGuardValue<bool> BroadcastGuard(bSemanticRefreshInProgress, true);
        BroadcastSemanticDataChanged();
    }
    return bCommitted;
}

void UMythicItemizationDataRegistrySubsystem::BroadcastSemanticDataChanged() {
    if (SemanticDataRevision < MAX_uint64) {
        ++SemanticDataRevision;
    }
    SemanticDataChanged.Broadcast(SemanticDataRevision);
}

#if WITH_EDITOR
void UMythicItemizationDataRegistrySubsystem::HandleResidentAssetPrePropertyChange(
    UObject *Object,
    const FEditPropertyChain &PropertyChain) {
    (void)PropertyChain;
    if (!bAcceptingAsyncCompletions || bRestoringEditorPreEditSnapshots
        || !Object || !LoadedAssets.Contains(Object)) {
        return;
    }
    if (!CaptureResidentAssetPreEditSnapshot(Object)) {
        UE_LOG(Myth, Error,
               TEXT("Could not snapshot resident semantic asset %s before an editor property edit."),
               *Object->GetPathName());
    }

    // Registry maps intentionally retain the last committed graph so a rejected edit can be restored. They point at
    // mutable editor UObjects, however, so no consumer may read through them after this callback returns and before
    // the complete graph is revalidated. One availability transition covers the whole coalesced/interactive batch.
    EnterEditorSemanticQuarantine();
}

bool UMythicItemizationDataRegistrySubsystem::IsResidentSemanticDependency(
    const UObject *Object) const {
    if (!Object) {
        return false;
    }

    for (const UObject *Resident : LoadedAssets) {
        if (const UMythicStatDefinition *Stat = Cast<UMythicStatDefinition>(Resident)) {
            if (Stat->Category.GetAsset() == Object || Stat->PairedStat.GetAsset() == Object) {
                return true;
            }
        }
        else if (const UMythicAffixDefinition *Affix = Cast<UMythicAffixDefinition>(Resident)) {
            if (Affix->TargetStat.GetAsset() == Object) {
                return true;
            }
        }
        else if (const UMythicAffixPool *Pool = Cast<UMythicAffixPool>(Resident)) {
            for (const FMythicAffixPoolEntry &Entry : Pool->Entries) {
                if (Entry.AffixDefinition.GetAsset() == Object) {
                    return true;
                }
            }
        }
        else if (const UMythicAffixProfile *Profile = Cast<UMythicAffixProfile>(Resident)) {
            if (Profile->RollPolicy.GetAsset() == Object) {
                return true;
            }
            for (const FMythicAffixGrantSpec &Grant : Profile->GuaranteedGrants) {
                if (Grant.AffixDefinition.GetAsset() == Object) {
                    return true;
                }
            }
            for (const FMythicAffixPoolSlice &Slice : Profile->RandomPoolSlices) {
                if (Slice.Pool.GetAsset() == Object) {
                    return true;
                }
            }
        }
        else if (const UMythicItemizationRuleset *Ruleset =
                     Cast<UMythicItemizationRuleset>(Resident)) {
            for (const TSoftObjectPtr<UMythicAffixProfile> &ProfileRef : Ruleset->Profiles) {
                if (ProfileRef.Get() == Object) {
                    return true;
                }
            }
        }
    }

    for (const TPair<FGuid, TSharedPtr<const FCompiledAffixGrantClosure>> &Pair
         : CompiledGrants) {
        if (Pair.Value.IsValid()
            && Pair.Value->Spec.AffixDefinition.GetAsset() == Object) {
            return true;
        }
    }
    for (const TPair<FGuid, TSharedPtr<FMythicAffixGrantSpec>> &Pair
         : PendingReplacementGrantSpecs) {
        if (Pair.Value.IsValid()
            && Pair.Value->AffixDefinition.GetAsset() == Object) {
            return true;
        }
    }
    return false;
}

void UMythicItemizationDataRegistrySubsystem::EnterEditorSemanticQuarantine() {
    if (bSemanticDataQuarantined) {
        return;
    }
    bSemanticDataQuarantined = true;
    TGuardValue<bool> BroadcastGuard(bSemanticRefreshInProgress, true);
    BroadcastSemanticDataChanged();
}

void UMythicItemizationDataRegistrySubsystem::HandleResidentObjectsReplaced(
    const TMap<UObject *, UObject *> &ReplacementMap) {
    if (!bAcceptingAsyncCompletions || bRestoringEditorPreEditSnapshots
        || ReplacementMap.IsEmpty()) {
        return;
    }
    if (!ensureMsgf(IsInGameThread(),
                    TEXT("Resident itemization UObject replacement must be handled on the game thread."))) {
        return;
    }

    TSet<UObject *> AffectedObjects;
    for (const TPair<UObject *, UObject *> &Pair : ReplacementMap) {
        // Depending on the replacement path, reflected UPROPERTY references may be fixed either before tools are
        // notified or after this delegate returns. Inspect both sides so the raw native indexes are quarantined in
        // either ordering.
        if (Pair.Key
            && (LoadedAssets.Contains(Pair.Key)
                || IsResidentSemanticDependency(Pair.Key)
                || (Pair.Value && (LoadedAssets.Contains(Pair.Value)
                                   || IsResidentSemanticDependency(Pair.Value))))) {
            AffectedObjects.Add(Pair.Key);
        }
    }
    if (AffectedObjects.IsEmpty()) {
        return;
    }

    // A reinstancing/reload transaction supersedes property-level rollback snapshots. Quarantine before any raw
    // index can be observed, then retain the replacement objects strongly until the coalesced publication commits.
    ClearResidentAssetPreEditSnapshots();
    EnterEditorSemanticQuarantine();

    for (const TPair<FGuid, TSharedPtr<const FCompiledAffixGrantClosure>> &Pair
         : CompiledGrants) {
        if (!Pair.Value.IsValid()) {
            continue;
        }
        FMythicAffixGrantSpec CanonicalSpec = Pair.Value->Spec;
        if (const TSharedPtr<FMythicAffixGrantSpec> *PendingSpec =
                PendingReplacementGrantSpecs.Find(Pair.Key);
            PendingSpec && PendingSpec->IsValid()) {
            CanonicalSpec = **PendingSpec;
        }
        if (UMythicAffixDefinition *OldDefinition =
                CanonicalSpec.AffixDefinition.GetAsset()) {
            if (UObject *const *Replacement = ReplacementMap.Find(OldDefinition)) {
                CanonicalSpec.AffixDefinition.SetAsset(
                    Cast<UMythicAffixDefinition>(*Replacement));
            }
        }
        PendingReplacementGrantSpecs.Add(
            Pair.Key, MakeShared<FMythicAffixGrantSpec>(MoveTemp(CanonicalSpec)));
    }

    TSet<UObject *> ReplacedResidents;
    for (TObjectPtr<UObject> &Resident : LoadedAssets) {
        UObject *OldObject = Resident.Get();
        if (UObject *const *Replacement = ReplacementMap.Find(OldObject)) {
            Resident = *Replacement;
            ReplacedResidents.Add(OldObject);
        }
    }
    // A dependency should already be resident in every valid closure. Retaining a replacement discovered only
    // through a direct typed edge is conservative and lets the full rebuild diagnose any incomplete prior graph.
    for (UObject *OldObject : AffectedObjects) {
        if (ReplacedResidents.Contains(OldObject)) {
            continue;
        }
        UObject *const *Replacement = ReplacementMap.Find(OldObject);
        LoadedAssets.AddUnique(Replacement ? *Replacement : nullptr);
    }

    if (ActiveRuleset) {
        if (UObject *const *Replacement = ReplacementMap.Find(ActiveRuleset.Get())) {
            ActiveRuleset = Cast<UMythicItemizationRuleset>(*Replacement);
        }
    }
    ScheduleEditorResidentRefresh();
}

void UMythicItemizationDataRegistrySubsystem::ScheduleEditorResidentRefresh() {
    if (bEditorRefreshPending) {
        return;
    }
    bEditorRefreshPending = true;
    TWeakObjectPtr<UMythicItemizationDataRegistrySubsystem> WeakThis(this);
    AsyncTask(ENamedThreads::GameThread, [WeakThis]() {
        if (!WeakThis.IsValid() || !WeakThis->bAcceptingAsyncCompletions
            || !WeakThis->bEditorRefreshPending) {
            return;
        }
        WeakThis->ProcessPendingEditorResidentRefresh();
    });
}

void UMythicItemizationDataRegistrySubsystem::ProcessPendingEditorResidentRefresh() {
    bEditorRefreshPending = false;
    TArray<FText> Errors;
    if (RefreshResidentData(Errors)) {
        return;
    }

    TArray<FText> RestoreErrors;
    const bool bHasRollbackSnapshot = !EditorPreEditObjects.IsEmpty();
    const bool bRestored = bHasRollbackSnapshot
        && RestoreResidentAssetPreEditSnapshots(RestoreErrors);
    if (!bHasRollbackSnapshot) {
        RestoreErrors.Add(FText::FromString(
            TEXT("The rejected semantic replacement has no property-edit snapshot to restore.")));
    }
    for (const FText &Error : Errors) {
        UE_LOG(Myth, Error, TEXT("Itemization semantic refresh rejected: %s"),
               *Error.ToString());
    }
    for (const FText &Error : RestoreErrors) {
        UE_LOG(Myth, Error, TEXT("Itemization semantic editor rollback failed: %s"),
               *Error.ToString());
    }
    if (!bRestored) {
        QuarantineSemanticDataAfterEditorRestoreFailure();
    }
    else {
        ClearResidentAssetPreEditSnapshots();
    }
}

bool UMythicItemizationDataRegistrySubsystem::CaptureResidentAssetPreEditSnapshot(
    UObject *Object) {
    if (!Object || EditorPreEditObjects.Contains(Object)) {
        return Object != nullptr;
    }

    const FName SnapshotName = MakeUniqueObjectName(
        GetTransientPackage(), Object->GetClass(),
        FName(*(Object->GetName() + TEXT("_MythicSemanticPreEdit"))));
    UObject *Snapshot = DuplicateObject<UObject>(Object, GetTransientPackage(), SnapshotName);
    if (!Snapshot || Snapshot->GetClass() != Object->GetClass()) {
        return false;
    }
    Snapshot->SetFlags(RF_Transient);
    Snapshot->ClearFlags(RF_Public | RF_Standalone | RF_Transactional);
    EditorPreEditObjects.Add(Object);
    EditorPreEditSnapshots.Add(Snapshot);
    return true;
}

void UMythicItemizationDataRegistrySubsystem::ClearResidentAssetPreEditSnapshots() {
    EditorPreEditObjects.Reset();
    EditorPreEditSnapshots.Reset();
}

bool UMythicItemizationDataRegistrySubsystem::RestoreResidentAssetPreEditSnapshots(
    TArray<FText> &OutErrors) {
    if (EditorPreEditObjects.Num() != EditorPreEditSnapshots.Num()) {
        OutErrors.Add(FText::FromString(
            TEXT("Resident semantic pre-edit snapshot storage is inconsistent.")));
        return false;
    }

    TGuardValue<bool> RestoreGuard(bRestoringEditorPreEditSnapshots, true);
    for (int32 Index = 0; Index < EditorPreEditObjects.Num(); ++Index) {
        UObject *Object = EditorPreEditObjects[Index];
        UObject *Snapshot = EditorPreEditSnapshots[Index];
        if (!IsValid(Object) || !IsValid(Snapshot)
            || Object->GetClass() != Snapshot->GetClass()
            || !LoadedAssets.Contains(Object)) {
            OutErrors.Add(FText::FromString(
                TEXT("A resident semantic pre-edit snapshot can no longer be restored safely.")));
            return false;
        }
    }

    for (int32 Index = 0; Index < EditorPreEditObjects.Num(); ++Index) {
        UObject *Object = EditorPreEditObjects[Index];
        UObject *Snapshot = EditorPreEditSnapshots[Index];
        UEngine::FCopyPropertiesForUnrelatedObjectsParams CopyParams;
        CopyParams.bDoDelta = false;
        CopyParams.bReplaceObjectClassReferences = false;
        UEngine::CopyPropertiesForUnrelatedObjects(Snapshot, Object, CopyParams);
    }
    // Notify only after the complete batch has been restored, so other editor listeners cannot observe a partially
    // rolled-back semantic graph. This registry's post-change callback is suppressed by RestoreGuard.
    for (UObject *Object : EditorPreEditObjects) {
        Object->PostEditChange();
    }

    // The rejected transaction already restored the prior maps and immutable closures. Recompiling the restored
    // object graph proves that those maps and closures once again describe the UObject state; because the pre-edit
    // hook quarantined consumers, a successful internal verification also publishes the recovery transition.
    TArray<FText> VerificationErrors;
    if (!PublishLoadedAssetsInternal({}, VerificationErrors, false)) {
        OutErrors.Append(MoveTemp(VerificationErrors));
        OutErrors.Add(FText::FromString(
            TEXT("Restored semantic assets could not reproduce the last published closure.")));
        return false;
    }
    return true;
}

void UMythicItemizationDataRegistrySubsystem::QuarantineSemanticDataAfterEditorRestoreFailure() {
    const bool bWasQuarantined = bSemanticDataQuarantined;
    StatRegistry.Reset();
    AffixesById.Reset();
    AffixesByTag.Reset();
    PoolsById.Reset();
    PoliciesById.Reset();
    ProfilesById.Reset();
    RulesetsById.Reset();
    CompiledProfiles.Reset();
    CompiledGrants.Reset();
    PendingReplacementGrantSpecs.Reset();
    bSemanticDataQuarantined = true;
    ClearResidentAssetPreEditSnapshots();

    UE_LOG(Myth, Error,
           TEXT("Itemization semantic registry quarantined after an editor rollback could not be verified; no semantic lookup or compiled closure will be served until a valid refresh commits."));
    if (!bWasQuarantined) {
        TGuardValue<bool> BroadcastGuard(bSemanticRefreshInProgress, true);
        BroadcastSemanticDataChanged();
    }
}

void UMythicItemizationDataRegistrySubsystem::HandleResidentAssetPropertyChanged(
    UObject *Object,
    FPropertyChangedEvent &PropertyChangedEvent) {
    if (!bAcceptingAsyncCompletions || bRestoringEditorPreEditSnapshots
        || !Object || !LoadedAssets.Contains(Object)
        || (PropertyChangedEvent.ChangeType & EPropertyChangeType::Interactive) != 0
        || bEditorRefreshPending) {
        return;
    }

    ScheduleEditorResidentRefresh();
}
#endif

bool UMythicItemizationDataRegistrySubsystem::PublishLoadedAssets(
    TConstArrayView<UObject *> Assets, TArray<FText> &OutErrors) {
    return PublishLoadedAssetsInternal(Assets, OutErrors, true);
}

bool UMythicItemizationDataRegistrySubsystem::RefreshResidentData(
    TArray<FText> &OutErrors) {
    if (LoadedAssets.IsEmpty()) {
        OutErrors.Add(FText::FromString(
            TEXT("No resident itemization semantic data is available to refresh.")));
        return false;
    }
    return PublishLoadedAssetsInternal({}, OutErrors, true);
}

#if WITH_DEV_AUTOMATION_TESTS
bool UMythicItemizationDataRegistrySubsystem::PublishCoreSemanticAssetsForTests(
    TConstArrayView<UObject *> Assets, TArray<FText> &OutErrors) {
    FMythicItemizationRegistryPublishedState Previous = CapturePublishedState();
    if (!PublishLoadedAssetsInternal(Assets, OutErrors, false)
        || !StatRegistry.IsBuilt() || AffixesById.IsEmpty()) {
        RestorePublishedState(MoveTemp(Previous));
        OutErrors.Add(FText::FromString(
            TEXT("A test core semantic closure requires built stat indexes and at least one Affix Definition.")));
        return false;
    }
    SetReadiness(EMythicItemizationReadiness::CoreSemanticReady);
    return true;
}
#endif

bool UMythicItemizationDataRegistrySubsystem::CompileProfile(
    const FPrimaryAssetId ProfileId, TArray<FText> &OutErrors) {
    if (!IsInGameThread()) {
        OutErrors.Add(FText::FromString(
            TEXT("Affix Profiles can only be compiled on the game thread.")));
        return false;
    }
    const UMythicAffixProfile *Profile = FindProfile(ProfileId);
    if (!Profile) {
        OutErrors.Add(FText::FromString(TEXT("Profile is not loaded.")));
        return false;
    }
    TSharedPtr<const FCompiledAffixProfile> Compiled;
    if (!FMythicAffixCompiler::Compile(*Profile, *this, Compiled, OutErrors)) return false;
    CompiledProfiles.Add(ProfileId, MoveTemp(Compiled));
    return true;
}

bool UMythicItemizationDataRegistrySubsystem::CompileGrantClosures(
    TConstArrayView<FMythicAffixGrantSpec> Grants, TArray<FText> &OutErrors) {
    if (!IsInGameThread()) {
        OutErrors.Add(FText::FromString(
            TEXT("Affix grant closures can only be compiled on the game thread.")));
        return false;
    }
    TMap<FGuid, TSharedPtr<const FCompiledAffixGrantClosure>> Staged = CompiledGrants;
    TSet<FGuid> Batch;
    for (const FMythicAffixGrantSpec &Grant : Grants) {
        if (!Grant.GrantGuid.IsValid() || Batch.Contains(Grant.GrantGuid)) return false;
        Batch.Add(Grant.GrantGuid);
        if (const TSharedPtr<const FCompiledAffixGrantClosure> *Existing =
                Staged.Find(Grant.GrantGuid)) {
            if (!Existing->IsValid() || !GrantSpecsMatch((*Existing)->Spec, Grant)) return false;
        }
        TSharedPtr<const FCompiledAffixGrantClosure> Compiled;
        if (!FMythicAffixCompiler::CompileGrant(Grant, *this, Compiled, OutErrors)) return false;
        Staged.Add(Grant.GrantGuid, MoveTemp(Compiled));
    }
    CompiledGrants = MoveTemp(Staged);
    return true;
}

const UMythicStatCategoryDefinition *UMythicItemizationDataRegistrySubsystem::FindStatCategory(
    const FGameplayTag Tag) const {
    return bSemanticDataQuarantined ? nullptr : StatRegistry.FindCategory(Tag);
}
const UMythicStatDefinition *UMythicItemizationDataRegistrySubsystem::FindStat(
    const FPrimaryAssetId Id) const {
    return bSemanticDataQuarantined ? nullptr : StatRegistry.FindStat(Id);
}
const UMythicStatDefinition *UMythicItemizationDataRegistrySubsystem::FindStat(
    const FGameplayTag Tag) const {
    return bSemanticDataQuarantined ? nullptr : StatRegistry.FindStat(Tag);
}
const UMythicStatDefinition *UMythicItemizationDataRegistrySubsystem::FindStat(
    const FGameplayAttribute &Attribute) const {
    return bSemanticDataQuarantined ? nullptr : StatRegistry.FindStat(Attribute);
}
const UMythicAffixDefinition *UMythicItemizationDataRegistrySubsystem::FindAffix(
    const FGameplayTag Tag) const {
    if (bSemanticDataQuarantined) return nullptr;
    const auto *Found = AffixesByTag.Find(Tag);
    return Found ? *Found : nullptr;
}
const UMythicAffixDefinition *UMythicItemizationDataRegistrySubsystem::FindAffix(
    const FPrimaryAssetId Id) const {
    if (bSemanticDataQuarantined) return nullptr;
    const auto *Found = AffixesById.Find(Id);
    return Found ? *Found : nullptr;
}
const UMythicAffixPool *UMythicItemizationDataRegistrySubsystem::FindPool(
    const FPrimaryAssetId Id) const {
    if (bSemanticDataQuarantined) return nullptr;
    const auto *Found = PoolsById.Find(Id);
    return Found ? *Found : nullptr;
}
const UMythicAffixRollPolicy *UMythicItemizationDataRegistrySubsystem::FindPolicy(
    const FPrimaryAssetId Id) const {
    if (bSemanticDataQuarantined) return nullptr;
    const auto *Found = PoliciesById.Find(Id);
    return Found ? *Found : nullptr;
}
const UMythicAffixProfile *UMythicItemizationDataRegistrySubsystem::FindProfile(
    const FPrimaryAssetId Id) const {
    if (bSemanticDataQuarantined) return nullptr;
    const auto *Found = ProfilesById.Find(Id);
    return Found ? *Found : nullptr;
}
TSharedPtr<const FCompiledAffixProfile>
UMythicItemizationDataRegistrySubsystem::FindCompiledProfile(const FPrimaryAssetId Id) const {
    if (bSemanticDataQuarantined) return nullptr;
    const auto *Found = CompiledProfiles.Find(Id);
    return Found ? *Found : nullptr;
}
TSharedPtr<const FCompiledAffixGrantClosure>
UMythicItemizationDataRegistrySubsystem::FindCompiledGrant(
    const FMythicAffixGrantSpec &Spec) const {
    if (bSemanticDataQuarantined) return nullptr;
    const auto *Found = CompiledGrants.Find(Spec.GrantGuid);
    return Found && Found->IsValid() && GrantSpecsMatch((*Found)->Spec, Spec) ? *Found : nullptr;
}

void UMythicItemizationDataRegistrySubsystem::GetAllStatDefinitions(
    TArray<const UMythicStatDefinition *> &Out) const {
    if (bSemanticDataQuarantined) {
        Out.Reset();
        return;
    }
    StatRegistry.GetAllStatDefinitions(Out);
}
void UMythicItemizationDataRegistrySubsystem::GetAllStatCategories(
    TArray<const UMythicStatCategoryDefinition *> &Out) const {
    if (bSemanticDataQuarantined) {
        Out.Reset();
        return;
    }
    StatRegistry.GetAllCategories(Out);
}
void UMythicItemizationDataRegistrySubsystem::GetAllAffixDefinitions(
    TArray<const UMythicAffixDefinition *> &Out) const {
    if (bSemanticDataQuarantined) {
        Out.Reset();
        return;
    }
    AffixesById.GenerateValueArray(Out);
    Out.Sort([](const UMythicAffixDefinition &A, const UMythicAffixDefinition &B) {
        return A.AffixTag.ToString() < B.AffixTag.ToString();
    });
}

TArray<UMythicStatDefinition *>
UMythicItemizationDataRegistrySubsystem::GetAllStatDefinitionAssets() const {
    TArray<const UMythicStatDefinition *> Definitions;
    GetAllStatDefinitions(Definitions);
    TArray<UMythicStatDefinition *> Result;
    Result.Reserve(Definitions.Num());
    for (const UMythicStatDefinition *Definition : Definitions) {
        Result.Add(const_cast<UMythicStatDefinition *>(Definition));
    }
    return Result;
}

TArray<UMythicStatCategoryDefinition *>
UMythicItemizationDataRegistrySubsystem::GetAllStatCategoryAssets() const {
    TArray<const UMythicStatCategoryDefinition *> Categories;
    GetAllStatCategories(Categories);
    TArray<UMythicStatCategoryDefinition *> Result;
    Result.Reserve(Categories.Num());
    for (const UMythicStatCategoryDefinition *Category : Categories) {
        Result.Add(const_cast<UMythicStatCategoryDefinition *>(Category));
    }
    return Result;
}

TArray<UMythicAffixDefinition *>
UMythicItemizationDataRegistrySubsystem::GetAllAffixDefinitionAssets() const {
    TArray<const UMythicAffixDefinition *> Definitions;
    GetAllAffixDefinitions(Definitions);
    TArray<UMythicAffixDefinition *> Result;
    Result.Reserve(Definitions.Num());
    for (const UMythicAffixDefinition *Definition : Definitions) {
        Result.Add(const_cast<UMythicAffixDefinition *>(Definition));
    }
    return Result;
}

bool UMythicItemizationDataRegistrySubsystem::ValidateAssetRegistryIdentities(
    TArray<FText> &OutErrors) const {
    TArray<FSoftObjectPath> Ignored;
    return GatherAssetRegistryCandidates(Ignored, OutErrors, false);
}

bool UMythicItemizationDataRegistrySubsystem::ValidateActiveRuleset(
    TArray<FText> &OutErrors) const {
    if (!ActiveRuleset || ActiveRuleset->Profiles.IsEmpty()) {
        OutErrors.Add(FText::FromString(TEXT("No active typed Itemization Ruleset is loaded.")));
        return false;
    }
    bool bValid = true;
    TSet<FPrimaryAssetId> Profiles;
    for (const TSoftObjectPtr<UMythicAffixProfile> &ProfileRef : ActiveRuleset->Profiles) {
        const UMythicAffixProfile *Profile = ProfileRef.Get();
        const FPrimaryAssetId Id = Profile ? Profile->GetPrimaryAssetId() : FPrimaryAssetId();
        const TSharedPtr<const FCompiledAffixProfile> Compiled = FindCompiledProfile(Id);
        if (!Profile || Profiles.Contains(Id) || !Compiled.IsValid()
            || Compiled->GameplayContentHash.IsZero()
            || Compiled->PresentationContentHash.IsZero()
            || Compiled->DependencyManifest.ProfileId != Id
            || Compiled->DependencyManifest.RuntimePrimaryAssets.IsEmpty()) {
            OutErrors.Add(FText::FromString(FString::Printf(
                TEXT("Active ruleset profile %s is duplicate, unloaded, uncompiled, or unhashed."),
                *ProfileRef.ToSoftObjectPath().ToString())));
            bValid = false;
        }
        Profiles.Add(Id);
    }
    return bValid;
}
