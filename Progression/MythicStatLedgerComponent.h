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

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicStatLedgerComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicStatLedgerComponent();

    // SERVER: add Delta to Tag's character counter and (when bAccountToo) the account counter, then broadcast
    // OnCounterChanged(Tag, newCharValue). No-op off authority, for an invalid tag, or while restoring a save
    // (bIsRestoring) — so the event-driven binds below never double-count a reload. Every other Stat.* producer
    // (loot/craft/discovery/quest/deed systems) calls this directly; it is the single write seam into the ledger.
    UFUNCTION(BlueprintCallable, Category = "Progression|Stats")
    void RecordStat(FGameplayTag Tag, int64 Delta = 1, bool bAccountToo = true);

    // SERVER (Wave P, P6i): RECORD-IF-GREATER write — the personal-best lane (Stat.Fish.Record.<Species> biggest
    // catch, and any future "largest/farthest/fastest" ledger). Pure math: FMythicStatLedger::ApplyMax. Returns TRUE
    // when THIS call raised the character record (the caller's trophy-mint/cue edge); ties and smaller values change
    // nothing and return false. Same authority/restore/validity guards as RecordStat; broadcasts OnCounterChanged
    // only on a genuine new record.
    UFUNCTION(BlueprintCallable, Category = "Progression|Stats")
    bool RecordStatMax(FGameplayTag Tag, int64 Value, bool bAccountToo = true);

    // Pure: exact-tag character counter (0 if never recorded).
    UFUNCTION(BlueprintPure, Category = "Progression|Stats")
    int64 GetCounter(FGameplayTag Tag) const;

    // Pure: hierarchical rollup of the character counters under PrefixTag (e.g. Stat.Kill => Generic + Boss).
    UFUNCTION(BlueprintPure, Category = "Progression|Stats")
    int64 GetCounterRollup(FGameplayTag PrefixTag) const;

    const TArray<FMythicStatCounter> &GetCharacterCounters() const { return CharacterCounters.Items; }
    const TArray<FMythicStatCounter> &GetAccountCounters() const { return AccountCounters.Items; }

    // Broadcast on every RecordStat (server-side). See delegate doc above.
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

    // ── Distance odometer (OPT-IN; DEFAULT OFF) ──
    // A single coarse repeating timer sampling the pawn's position — NEVER per-frame. Off by default (zero cost); a
    // designer enables it per-project. Records Stat.Distance.Traveled in centimetres.
    UPROPERTY(EditDefaultsOnly, Category = "Progression|Stats")
    bool bDistanceOdometerEnabled = false;

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
    FDelegateHandle KillEventHandle;
    FDelegateHandle DeathEventHandle;

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
