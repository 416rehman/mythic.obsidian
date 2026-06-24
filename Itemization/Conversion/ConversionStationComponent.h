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

/** One queued conversion job. Only RecipeId (a tag) crosses the wire to identify the recipe. */
USTRUCT(BlueprintType)
struct FConversionJobEntry : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY()
    FGameplayTag RecipeId;

    // Remaining cycles. 0 = unbounded (Continuous + repeat).
    UPROPERTY()
    int32 Quantity = 1;

    UPROPERTY()
    EConversionJobState State = EConversionJobState::Pending;

    // SERVER WORLD time the current cycle began (anchor for client-side progress).
    UPROPERTY()
    double CycleStartServerTime = 0.0;

    UPROPERTY()
    float CycleDuration = 0.0f;

    // Monotonic job id; addresses this job in RPCs and refund/source bookkeeping.
    UPROPERTY()
    int32 JobId = 0;

    UPROPERTY()
    EConversionStallReason StallReason = EConversionStallReason::None;

    // Captured at enqueue from the consumed input(s); reproducible InheritInputLevel after the source is gone.
    UPROPERTY()
    int32 SnapshotInputLevel = 0;
};

/** Replicated FastArray of jobs (mirrors FCraftingQueue's style). */
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

    int32 AddJob(const FGameplayTag &RecipeId, int32 Quantity, int32 JobId, int32 SnapshotInputLevel);
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

/** Replicated fuel buffer state (furnace/fireplace). */
USTRUCT()
struct FConversionFuelState {
    GENERATED_BODY()

    // Seconds of burn currently buffered (enough fuel has been consumed to cover this much processing).
    UPROPERTY()
    float BufferedBurnSeconds = 0.f;

    UPROPERTY()
    double LastSampleServerTime = 0.0;

    UPROPERTY()
    bool bBurning = false;

    // Largest buffer seen (for UI gauge scaling).
    UPROPERTY()
    float CapacityHintSeconds = 0.f;
};

/** A transform product releases (not destroys) its input; the instance is GC-rooted here until routed. */
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

/**
 * The generic, server-authoritative conversion engine. Configured purely by data (station tags, mode,
 * gate, recipes, inventory profile groups) to become a crafting bench / furnace / fireplace / alchemy
 * table / smelter / etc. Mutating entry points are plain server methods invoked by the owning
 * PlayerController's RPCs (which carry trusted sender identity).
 */
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UConversionStationComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UConversionStationComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    // FastArray: the PostReplicated* callbacks drive the client refresh (no ReplicatedUsing, to avoid a
    // duplicate refresh from both OnRep and the callbacks).
    UPROPERTY(Replicated)
    FConversionJobArray Jobs;

    UPROPERTY(ReplicatedUsing=OnRep_Fuel)
    FConversionFuelState FuelState;

    // ---- Editor config (identical on all machines -> NOT replicated) ----
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Conversion", meta=(Categories="Itemization.Station"))
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

    // ---- Server-only runtime (NOT replicated) ----
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
        TWeakObjectPtr<AController> Controller; // null => auto job (refund to station)
        FGameplayTag RecipeId;
    };

    TMap<int32, FJobRefundInfo> JobRefundTargets;

    // Exact materials reserved per job, for dupe-safe refund of consumed (non-transform) inputs on cancel/teardown.
    struct FReservedRefundEntry {
        TWeakObjectPtr<UItemDefinition> Def;
        int32 TotalQty = 0;
    };

    struct FJobReservation {
        int32 ReservedCycles = 0;
        int32 SnapshotLevel = 0; // level of the consumed inputs, so refunds restore the same level
        TArray<FReservedRefundEntry> Items;
    };

    TMap<int32, FJobReservation> JobReservations;

    // Transform products release (not destroy) their input; the instances are GC-rooted here until routed.
    UPROPERTY()
    TArray<FHeldTransform> HeldTransforms;

    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicInventoryComponent> StationInv;
    UPROPERTY(Transient)
    TWeakObjectPtr<UConversionSubsystem> Subsystem;

    UFUNCTION()
    void OnRep_Fuel();
    void HandleRecipesReady();

    // Server core
    void AdvanceProcessing();
    // Runs the per-cycle gates (catalyst -> fuel -> continuous re-consume) and, on success, marks the head
    // Processing and anchors its timing. bReConsume drives per-cycle input consumption (Continuous only).
    bool BeginCycle(FConversionJobEntry &Head, UConversionRecipe *Recipe, bool bReConsume);
    void CompleteCurrentCycleAndContinue();
    void OnCycleComplete();
    void ScheduleAutoEnqueue();
    void TryAutoEnqueue();
    UFUNCTION()
    void HandleStationInventoryChanged(int32 Slot);

    // Source helpers (manual jobs draw from the instigator; auto jobs from the station).
    void GetSourceInventories(AController *JobController, TArray<UMythicInventoryComponent *> &OutInvs,
                              FGameplayTag &OutInputGroup, FGameplayTag &OutCatalystGroup) const;
    void GatherInstances(const TArray<UMythicInventoryComponent *> &Invs, const FGameplayTag &GroupFilter,
                         TArray<UMythicItemInstance *> &Out) const;

    bool VerifyInputs(UConversionRecipe *R, AController *JobController, int32 Cycles, bool bCheckCatalysts) const;
    bool ConsumeInputs(UConversionRecipe *R, AController *JobController, int32 Cycles, int32 JobId, int32 &OutSnapshotLevel);
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

    /** Pure product-level rule (extracted from the member ComputeProductLevel for unit-testing): FixedLevel→FixedLevel,
     *  InheritInputLevel→InputLevel, InheritStationLevel→StationLevel. Static + pure (no actor/world). */
    static int32 ResolveProductLevel(EProductLevelMode LevelMode, int32 InputLevel, int32 InStationLevel, int32 FixedLevel);

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
    EConversionTrigger GetStationMode() const { return StationMode; }
    const FConversionJobArray &GetJobs() const { return Jobs; }
    const FConversionFuelState &GetFuelState() const { return FuelState; }
    const FGameplayTagQuery &GetStationUseRequirement() const { return StationUseRequirement; }
    float GetServerUseRangeSq() const { return ServerUseRangeSq; }

    // Used by the FastArray callbacks to refresh the VM / fire delegates on clients.
    void NotifyJobsReplicated();
    void NotifyFuelReplicated();

    // Advisory eligibility evaluator — the SAME logic the VM shows and the server trusts.
    UFUNCTION(BlueprintCallable, Category="Conversion")
    bool EvaluateEligibility(const UConversionRecipe *Recipe, const AActor *Interactor, FText &OutReason) const;

    // ---- Plain server-side entry points invoked by the owning PC's RPCs. NOT RPCs themselves. ----
    void Server_RegisterInstigator(AController *Controller, AActor *Pawn);
    void Server_RequestStart(AController *Controller, FGameplayTag RecipeId, int32 Quantity);
    void Server_CancelJob(AController *Controller, int32 JobId);
    void Server_SetAutoRepeat(AController *Controller, bool bRepeat);
};
