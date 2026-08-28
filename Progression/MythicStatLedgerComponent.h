#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MythicStatCounterTypes.h"
#include "MythicStatLedgerComponent.generated.h"

class UAbilitySystemComponent;
class UMythicInventoryComponent;
struct FGameplayEventData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMythicOnCounterChanged, FGameplayTag, Tag, int64, NewValue);

/** Spawnable authoritative component that records and replicates gameplay-tagged progression counters. */
UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicStatLedgerComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicStatLedgerComponent();

    /**
     * Authority-only additive write to an exact Stat.* counter. Optionally mirrors the delta to the account ledger;
     * invalid tags and save-restore calls are ignored, and successful writes broadcast On Counter Changed.
     */
    UFUNCTION(BlueprintCallable, Category = "Progression|Stats")
    void RecordStat(FGameplayTag Tag, int64 Delta = 1, bool bAccountToo = true);

    /**
     * Authority-only personal-best write for largest, farthest, or fastest records. Returns true only when Value
     * raises the character record; ties and lower values do not mutate or broadcast.
     */
    UFUNCTION(BlueprintCallable, Category = "Progression|Stats")
    bool RecordStatMax(FGameplayTag Tag, int64 Value, bool bAccountToo = true);

    /** Returns the exact character counter for Tag, or zero when that statistic has never been recorded. */
    UFUNCTION(BlueprintPure, Category = "Progression|Stats")
    int64 GetCounter(FGameplayTag Tag) const;

    /** Returns the sum of character counters matching PrefixTag, including all child gameplay tags. */
    UFUNCTION(BlueprintPure, Category = "Progression|Stats")
    int64 GetCounterRollup(FGameplayTag PrefixTag) const;

    const TArray<FMythicStatCounter> &GetCharacterCounters() const { return CharacterCounters.Items; }
    const TArray<FMythicStatCounter> &GetAccountCounters() const { return AccountCounters.Items; }

    /** Authority-side notification carrying the exact changed tag and its new character-ledger value. */
    UPROPERTY(BlueprintAssignable, Category = "Progression|Stats")
    FMythicOnCounterChanged OnCounterChanged;

    void RestoreCharacterCounters(const TArray<FMythicStatCounter> &Saved);

    void SetRestoring(bool bInRestoring) { bIsRestoring = bInRestoring; }

    void ResyncCurrencyBaseline();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(Replicated)
    FMythicStatCounterArray CharacterCounters;

    UPROPERTY(Replicated)
    FMythicStatCounterArray AccountCounters;

    bool bIsRestoring = false;

    /** Enables coarse timer-based distance tracking into Stat.Distance.Traveled; disabled has no runtime sampling cost. */
    UPROPERTY(EditDefaultsOnly, Category = "Progression|Stats")
    bool bDistanceOdometerEnabled = false;

    /** Seconds between distance samples while the optional odometer is enabled. */
    UPROPERTY(EditDefaultsOnly, Category = "Progression|Stats", meta = (ClampMin = "0.5"))
    float DistanceSampleInterval = 5.0f;

private:
    int64 ApplyAndMarkDirty(FMythicStatCounterArray &Array, const FGameplayTag &Tag, int64 Delta);

    UAbilitySystemComponent *ResolveASC() const;

    TArray<UMythicInventoryComponent *> GetOwnerInventories() const;
    int32 ComputeTotalCurrency() const;

    void BindGameplayEvents();
    void HandleKillEvent(const FGameplayEventData *Payload);
    void HandleDeathEvent(const FGameplayEventData *Payload);
    void HandleItemAcquiredEvent(const FGameplayEventData *Payload);
    FDelegateHandle KillEventHandle;
    FDelegateHandle DeathEventHandle;
    FDelegateHandle ItemAcquiredEventHandle;

public:
    /**
     * Whether an acquisition counts as gathered rather than looted. Decided from the item's own type tags, since
     * that is all the acquisition event carries — a resource bought from a vendor therefore reads as gathered,
     * which is the cost of not tracking where an item came from.
     */
    static bool IsGatheredAcquisition(const FGameplayTagContainer &ItemTags, const FGameplayTagContainer &GatheredTypes);

    // Whole items acquired by one event, never fewer than one.
    static int64 QuantityFromEvent(float EventMagnitude);

    UFUNCTION()
    void HandleInventorySlotUpdated(int32 Slot);
    void BindInventoryDelegates();
    void UnbindInventoryDelegates();
    int32 LastKnownCurrency = 0;
    TArray<TWeakObjectPtr<UMythicInventoryComponent>> BoundInventories;
    FTimerHandle InventoryBindRetryTimer;

    void SampleDistance();
    FTimerHandle DistanceTimerHandle;
    FVector LastOdometerLocation = FVector::ZeroVector;
    bool bHasOdometerBaseline = false;
};
