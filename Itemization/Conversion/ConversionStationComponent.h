#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ConversionTypes.h"
#include "ConversionStationComponent.generated.h"

class UMythicInventoryComponent;
class UMythicItemInstance;
class UItemDefinition;
class UConversionRecipe;
class UConversionSubsystem;
class AController;
class UAbilitySystemComponent;
class IInventoryProviderInterface;
class UProficiencyDefinition;

USTRUCT(BlueprintType)
struct FConversionJobEntry : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag RecipeId;

    UPROPERTY()
    int32 Quantity = 1;

    UPROPERTY()
    EConversionJobState State = EConversionJobState::Pending;

    UPROPERTY()
    double CycleStartServerTime = 0.0;

    UPROPERTY()
    float CycleDuration = 0.0f;

    UPROPERTY()
    int32 JobId = 0;

    UPROPERTY()
    EConversionStallReason StallReason = EConversionStallReason::None;

    UPROPERTY()
    int32 SnapshotInputLevel = 0;

    UPROPERTY()
    int32 SnapshotCrafterProficiencyLevel = 0;

    UPROPERTY()
    float SnapshotAvgQualityTierValue = -1.0f;

    UPROPERTY()
    float SnapshotMinFreshnessFraction = 1.0f;
};

struct FConversionConsumeSnapshot {
    int32 Level = 0;
    float AvgQualityTierValue = -1.0f;
    float MinFreshnessFraction = 1.0f;
};

USTRUCT()
struct FConversionJobArray : public FFastArraySerializer {
    GENERATED_BODY()

protected:
    UPROPERTY()
    TArray<FConversionJobEntry> Items;

    UPROPERTY(Transient)
    TObjectPtr<UConversionStationComponent> Owner = nullptr;

public:
    bool NetDeltaSerialize(FNetDeltaSerializeInfo &P) {
        return FastArrayDeltaSerialize<FConversionJobEntry, FConversionJobArray>(Items, P, *this);
    }

    FORCEINLINE void SetOwner(UConversionStationComponent *In) { Owner = In; }
    const TArray<FConversionJobEntry> &GetItems() const { return Items; }
    int32 Num() const { return Items.Num(); }

    int32 AddJob(const FGameplayTag &RecipeId, int32 Quantity, int32 JobId, const FConversionConsumeSnapshot &Snapshot,
                 int32 SnapshotCrafterProficiencyLevel);
    void RemoveAt(int32 Index);
    FConversionJobEntry *EditHead();
    FConversionJobEntry *FindById(int32 JobId);
    int32 IndexOfId(int32 JobId) const;
    void MarkDirtyAt(int32 Index);

    void PreReplicatedRemove(const TArrayView<int32> &R, int32 N);
    void PostReplicatedAdd(const TArrayView<int32> &A, int32 N);
    void PostReplicatedChange(const TArrayView<int32> &C, int32 N);
};

template <>
struct TStructOpsTypeTraits<FConversionJobArray> : TStructOpsTypeTraitsBase2<FConversionJobArray> {
    enum { WithNetDeltaSerializer = true };
};

USTRUCT()
struct FConversionFuelState {
    GENERATED_BODY()

    UPROPERTY()
    float BufferedBurnSeconds = 0.f;

    UPROPERTY()
    double LastSampleServerTime = 0.0;

    UPROPERTY()
    bool bBurning = false;

    UPROPERTY()
    float CapacityHintSeconds = 0.f;
};

USTRUCT()
struct FHeldTransform {
    GENERATED_BODY()

    UPROPERTY()
    int32 JobId = 0;

    UPROPERTY()
    TObjectPtr<UMythicItemInstance> Instance = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConversionJobsChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConversionJobCompleted, FConversionJobEntry, Entry);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConversionFuelChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnConversionStalled, int32, JobId, EConversionStallReason, Reason);

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UConversionStationComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UConversionStationComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    UPROPERTY(Replicated)
    FConversionJobArray Jobs;

    UPROPERTY(ReplicatedUsing=OnRep_Fuel)
    FConversionFuelState FuelState;

    // ---- Editor config (identical on all machines) ----
    // Replicated since Wave K: homestead shell tiers GRANT/REVOKE StationTags at runtime (Server_GrantStationTags —
    // the C7 "better gear needs a better home" ladder), so clients must see the effective set for recipe
    // visibility/eligibility. Authored-only stations replicate their (unchanging) authored value — byte-identical UX.
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category="Conversion", meta=(Categories="Itemization.Station"))
    FGameplayTagContainer StationTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversion")
    EConversionTrigger StationMode = EConversionTrigger::ManualSelect;

    // Server-enforced gate to USE the station at all (empty => none).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversion")
    FGameplayTagQuery StationUseRequirement;

    // Auto stations: the bounded set of recipes that may auto-fire here.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversion")
    TArray<TSoftObjectPtr<UConversionRecipe>> AutoRecipes;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversion")
    FGameplayTag InputGroupTag;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversion")
    FGameplayTag FuelGroupTag;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversion")
    FGameplayTag CatalystGroupTag;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversion")
    FGameplayTag OutputGroupTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversion", meta=(ClampMin="1"))
    int32 MaxQueueLength = 16;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversion", meta=(ClampMin="1"))
    int32 MaxJobsPerInstigator = 4;

    // Added to the interaction range to form the server-side use-range gate.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversion")
    float UseRangeTolerance = 50.f;

    FTimerHandle ProcessTimerHandle;
    FTimerHandle AutoEnqueueTimerHandle;
    int32 NextJobId = 1;
    float ServerUseRangeSq = 0.f;
    bool bMutatingStationInv = false;

    struct FConversionSession {
        TWeakObjectPtr<AController> Controller;
        TWeakObjectPtr<AActor> Pawn;
        double LastRequestServerTime = 0.0;
        int32 OwnedJobs = 0;
    };

    TMap<TWeakObjectPtr<AController>, FConversionSession> Sessions;

    struct FJobRefundInfo {
        TWeakObjectPtr<AController> Controller;
        FGameplayTag RecipeId;
    };

    TMap<int32, FJobRefundInfo> JobRefundTargets;

    struct FReservedRefundEntry {
        TWeakObjectPtr<UItemDefinition> Def;
        int32 TotalQty = 0;
    };

    struct FJobReservation {
        int32 ReservedCycles = 0;
        int32 SnapshotLevel = 0;
        TArray<FReservedRefundEntry> Items;
    };

    TMap<int32, FJobReservation> JobReservations;

    UPROPERTY()
    TArray<FHeldTransform> HeldTransforms;

    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicInventoryComponent> StationInv;
    UPROPERTY(Transient)
    TWeakObjectPtr<UConversionSubsystem> Subsystem;

    UFUNCTION()
    void OnRep_Fuel();
    void HandleRecipesReady();

    void AdvanceProcessing();
    bool BeginCycle(FConversionJobEntry &Head, UConversionRecipe *Recipe, bool bReConsume);
    void CompleteCurrentCycleAndContinue();
    void OnCycleComplete();
    void ScheduleAutoEnqueue();
    void TryAutoEnqueue();
    UFUNCTION()
    void HandleStationInventoryChanged(int32 Slot);

    void GetSourceInventories(AController *JobController, TArray<UMythicInventoryComponent *> &OutInvs,
                              FGameplayTag &OutInputGroup, FGameplayTag &OutCatalystGroup) const;
    void GatherInstances(const TArray<UMythicInventoryComponent *> &Invs, const FGameplayTag &GroupFilter,
                         TArray<UMythicItemInstance *> &Out) const;

    bool VerifyInputs(UConversionRecipe *R, AController *JobController, int32 Cycles, bool bCheckCatalysts) const;
    bool ConsumeInputs(UConversionRecipe *R, AController *JobController, int32 Cycles, int32 JobId, FConversionConsumeSnapshot &OutSnapshot);
    bool VerifyCatalystsPresent(UConversionRecipe *R, AController *JobController) const;
    bool EnsureFuel(UConversionRecipe *R);
    void ProduceAndRoute(UConversionRecipe *R, const FConversionJobEntry &Job);
    void RouteInstance(UMythicItemInstance *Inst, EConversionOutputRouting Routing, const FConversionJobEntry &Job);
    void RefundJob(const FConversionJobEntry &Job);
    void MintTo(UItemDefinition *Def, int32 Qty, AController *C, int32 Level) const;
    void ClearJobBookkeeping(int32 JobId);
    AController *ResolveInstigatorController(int32 JobId) const;
    bool IsActorInRange(const AActor *A) const;
    bool HasAuthority() const;
    double ServerNow() const;
    int32 ComputeProductLevel(const struct FConversionProduct &P, const FConversionJobEntry &Job) const;

public:
    /** Designer-set level of this crafting station. Drives `EProductLevelMode::InheritStationLevel` product output
     *  levels (previously hard-stubbed to 1 — "no station-level concept yet" — so that mode silently always yielded
     *  level 1). Per-station config (default 1 → backward-compatible); a future station-progression system can raise it. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversion", meta=(ClampMin="1"))
    int32 StationLevel = 1;

    static int32 ResolveProductLevel(EProductLevelMode LevelMode, int32 InputLevel, int32 InStationLevel, int32 FixedLevel);

    static int32 ComputeProficiencyScaledLevel(int32 CrafterProfLevel, int32 BaseLevel, int32 PerLevelBonus, int32 MaxLevel);

    static float ComputeCraftingXpReward(float BaseXpPerCraft, int32 Cycles, int32 CrafterLevel, int32 NoGainAtOrAboveLevel);

    static int32 ResolveCrafterProficiencyLevel(AController *Crafter, UProficiencyDefinition *ProfDef);

    UPROPERTY(BlueprintAssignable, Category="Conversion")
    FOnConversionJobsChanged OnJobsChanged;
    UPROPERTY(BlueprintAssignable, Category="Conversion")
    FOnConversionJobCompleted OnJobCompleted;
    UPROPERTY(BlueprintAssignable, Category="Conversion")
    FOnConversionFuelChanged OnFuelChanged;
    UPROPERTY(BlueprintAssignable, Category="Conversion")
    FOnConversionStalled OnJobStalled;

    UMythicInventoryComponent *GetStationInventory() const;
    const FGameplayTagContainer &GetStationTags() const { return StationTags; }

    void Server_GrantStationTags(const FGameplayTagContainer &NewTags);

    void Server_RevokeStationTags(const FGameplayTagContainer &TagsToRemove);
    EConversionTrigger GetStationMode() const { return StationMode; }
    const FConversionJobArray &GetJobs() const { return Jobs; }
    const FConversionFuelState &GetFuelState() const { return FuelState; }
    const FGameplayTagQuery &GetStationUseRequirement() const { return StationUseRequirement; }
    float GetServerUseRangeSq() const { return ServerUseRangeSq; }

    void NotifyJobsReplicated();

    void FlushOwnerNetDormancy();

    // Advisory eligibility evaluator — the SAME logic the VM shows and the server trusts.
    UFUNCTION(BlueprintCallable, Category="Conversion")
    bool EvaluateEligibility(const UConversionRecipe *Recipe, const AActor *Interactor, FText &OutReason) const;

    void Server_RegisterInstigator(AController *Controller, AActor *Pawn);
    void Server_RequestStart(AController *Controller, FGameplayTag RecipeId, int32 Quantity);
    void Server_CancelJob(AController *Controller, int32 JobId);
    void Server_SetAutoRepeat(AController *Controller, bool bRepeat);
};
