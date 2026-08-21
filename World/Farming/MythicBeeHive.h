
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interaction/IMythicInteractable.h"
#include "Rewards/RewardBase.h"
#include "Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "MythicBeeHive.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMythicPollinationAuraComponent;
class UCommonGenericInputActionDataTable;
class UProficiencyDefinition;

USTRUCT(BlueprintType)
struct FMythicHoneyVarietyRow {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Honey", meta = (ClampMin = "0"))
    int32 MinDistinctCropTypes = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Honey")
    FRewardsToGive Rewards;
};

UCLASS()
class MYTHIC_API AMythicBeeHive : public AActor, public IMythicInteractable, public IMythicSaveableActor {
    GENERATED_BODY()

public:
    AMythicBeeHive();

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) override;
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

    UFUNCTION(BlueprintPure, Category = "Apiary")
    int32 GetStoredUnits() const { return StoredUnits; }

    void ServerHandleCollect(AActor *Interactor);

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    UFUNCTION()
    void OnRep_StoredUnits();

    // Cosmetic fill reaction (comb bulge / bee swarm density). Fires on the acting server AND clients via OnRep.
    UFUNCTION(BlueprintImplementableEvent, Category = "Apiary")
    void OnHiveVisualChanged(int32 NewStoredUnits, int32 MaxUnits);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apiary")
    USceneComponent *SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apiary")
    UStaticMeshComponent *Mesh;

    /** The P4 pollination preset (query-only Influence.Pollination) — plots inside pull quality/yield bonuses.
     *  Content tunes its InfluenceRadius/Magnitude; the same radius bounds the diversity scan at collection. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Apiary")
    UMythicPollinationAuraComponent *PollinationAura;

    // ── Production tuning (per-actor/BP content) ──
    /** Seconds of productive time per honey unit. <= 0 disables production entirely (an inert decoration). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apiary", meta = (ClampMin = "0.0"))
    float SecondsPerHoneyUnit = 1800.0f;

    /** Storage cap — a full hive STALLS (drops further accrual) until collected; it never dies or overflows. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apiary", meta = (ClampMin = "1"))
    int32 MaxStoredUnits = 5;

    /** Bees don't fly in snow: while the weather is Environment.Weather.Snow at a sample point, the window produces
     *  nothing (lazy check — accepted coarseness, documented). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apiary")
    bool bPauseProductionInSnow = true;

    /** Variety ladder (diversity-resonance honey). The row with the highest qualifying MinDistinctCropTypes routes;
     *  author row 0 with MinDistinctCropTypes = 0 as the plain honey. EMPTY = collection yields nothing (warned once)
     *  — the hive is then aura-only content. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apiary")
    TArray<FMythicHoneyVarietyRow> VarietyRows;

    /** OPTIONAL wax co-product granted once per collection (candle/wax-seal recipes are conversion CONTENT). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apiary")
    FRewardsToGive WaxRewards;

    // ── Proficiency/meta (mirrors the crop XP block) ──
    /** Beekeeping proficiency track granted XP on collection (a DATA asset — content). Unset = no XP. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apiary|Progression")
    TObjectPtr<UProficiencyDefinition> BeekeepingProficiency;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apiary|Progression", meta = (ClampMin = "0.0"))
    float BeekeepingXPPerUnit = 5.0f;

    /** Anti-grind: no XP once the collector's level reaches/passes this (0 = no cap). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Apiary|Progression", meta = (ClampMin = "0"))
    int32 XpNoGainAtOrAboveLevel = 0;

    // Interaction prompt data (matches the plot/container pattern).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName CollectInteractionName = FName("Collect");

    // Replicated fill level (cosmetics + prompt gating only — the server re-samples at every decision).
    UPROPERTY(ReplicatedUsing = OnRep_StoredUnits, BlueprintReadOnly, Category = "Apiary")
    int32 StoredUnits = 0;

private:
    double LastSampleTime = 0.0;
    float CarryoverSeconds = 0.0f;
    FTimerHandle ProductionTimerHandle;
    bool bWarnedMissingVarietyContent = false;

    void SampleProduction(double Now);

    void RearmProductionTimer();

    float ProductionMultiplierNow() const;

    int32 CountDistinctCropsInRadius() const;

    void SetStoredUnits(int32 NewUnits);
};
