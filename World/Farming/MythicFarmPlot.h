
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/IMythicInteractable.h"
#include "Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "MythicCropDefinition.h"
#include "MythicFarmingRules.h"
#include "MythicFarmPlot.generated.h"

class UStaticMeshComponent;
class USceneComponent;
class UCommonGenericInputActionDataTable;
class UMythicItemInstance;
class UMythicCropRegistry;
class UProficiencyDefinition;
class AMythicCorpse;

USTRUCT(BlueprintType)
struct FMythicPlotState {
    GENERATED_BODY()

    // The planted crop, or null when the plot is empty. A DataAsset ref (stably named) — replicates + drives cosmetics.
    UPROPERTY(BlueprintReadOnly, Category = "Farming")
    TObjectPtr<UMythicCropDefinition> Crop = nullptr;

    // Current growth stage index: -1 = empty, 0 = first (seedling) stage ... Crop->GetMatureStageIndex() = mature.
    UPROPERTY(BlueprintReadOnly, Category = "Farming")
    int32 Stage = -1;

    // Quantized moisture (0..255; 255 = full). Resolved lazily at sample points; replicated for client condition UI.
    UPROPERTY(BlueprintReadOnly, Category = "Farming")
    uint8 MoistureQ = 255;

    // The crop died of drought (harvest yields compost feedstock — C6). Only reachable with an authored wither config.
    UPROPERTY(BlueprintReadOnly, Category = "Farming")
    bool bWithered = false;

    // Applied Item.Fertilizer.* leaf (invalid = unfertilized). Consumed by the next harvest.
    UPROPERTY(BlueprintReadOnly, Category = "Farming")
    FGameplayTag FertilizerTag;

    // Gravebloom essence (the buried corpse's AI.Kind.* leaf, or Farming.GraveEssence). Consumed by the next harvest.
    UPROPERTY(BlueprintReadOnly, Category = "Farming")
    FGameplayTag GraveEssenceTag;
};

UCLASS()
class MYTHIC_API AMythicFarmPlot : public AActor, public IMythicInteractable, public IMythicSaveableActor {
    GENERATED_BODY()

public:
    AMythicFarmPlot();

    static constexpr int32 STAGE_EMPTY = -1;

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) override;
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

    UFUNCTION(BlueprintPure, Category = "Farming")
    bool IsEmpty() const { return PlotState.Crop == nullptr; }

    UFUNCTION(BlueprintPure, Category = "Farming")
    bool IsMature() const;

    UFUNCTION(BlueprintPure, Category = "Farming")
    bool IsWithered() const { return PlotState.bWithered; }

    // Coarse lifecycle bucket for BP / details panel (the authoritative state is the fine int32 PlotState.Stage).
    UFUNCTION(BlueprintPure, Category = "Farming")
    EMythicCropStage GetCropLifecycle() const;

    // Moisture resolved to NOW (0..1) — lazy math, no state mutation. Client-side reads use the replicated snapshot
    // (exact enough for UI; the server re-resolves at every gameplay decision).
    UFUNCTION(BlueprintPure, Category = "Farming")
    float GetMoisture01() const;

    // The planted crop's Crop.Type.* identity leaf (invalid when empty/unauthored). The bee hive's diversity scan reads it.
    UFUNCTION(BlueprintPure, Category = "Farming")
    FGameplayTag GetPlantedCropTypeTag() const { return PlotState.Crop ? PlotState.Crop->CropTypeTag : FGameplayTag(); }

    void ServerHandlePrimaryInteract(AActor *Interactor);

    void ServerHandleSecondaryInteract(AActor *Interactor);

    void ServerApplyRaidStageRegression(int32 Stages);

    static AController *ResolveController(AActor *Interactor);
    static int32 ResolveProficiencyLevel(AActor *Interactor, const UProficiencyDefinition *Proficiency);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    UFUNCTION()
    void OnRep_PlotState();

    // Cosmetic stage reaction (mesh swap / grow VFX / harvest glow). Fires on the acting server AND on clients via OnRep.
    // LOCAL/cosmetic — it never gates gameplay (the authoritative state is PlotState). Stage -1 ⇒ show the empty plot.
    UFUNCTION(BlueprintImplementableEvent, Category = "Farming")
    void OnCropVisualChanged(UMythicCropDefinition *Crop, int32 Stage);

    // Cosmetic condition reaction (Wave L): moisture / withered changed — dry-soil tint, wilt pose, essence shimmer.
    // Fires alongside OnCropVisualChanged on server + clients. Signature kept SEPARATE so existing BP graphs bound to
    // OnCropVisualChanged keep working untouched.
    UFUNCTION(BlueprintImplementableEvent, Category = "Farming")
    void OnPlotConditionChanged(float Moisture01, bool bWithered);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
    USceneComponent *SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Farming")
    UStaticMeshComponent *Mesh;

    // Replicated growth state (crop + stage + condition), applied atomically via OnRep_PlotState.
    UPROPERTY(ReplicatedUsing = OnRep_PlotState, BlueprintReadOnly, Category = "Farming")
    FMythicPlotState PlotState;

    // Seed-tag → crop map used to resolve which crop a held seed plants. Authored per plot (or shared).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farming")
    TObjectPtr<UMythicCropRegistry> CropRegistry;

    // Interaction prompt data (matches the toggleable/container pattern).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName PlantInteractionName = FName("Plant");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName HarvestInteractionName = FName("Harvest");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName TendInteractionName = FName("Water");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName BuryInteractionName = FName("Bury");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName ClearInteractionName = FName("Clear");

private:
    FTimerHandle GrowthTimerHandle;

    double MoistureSampleTime = 0.0;
    float WetUptimeSeconds = 0.0f;
    float GrowTimeSeconds = 0.0f;
    float DryStreakSeconds = 0.0f;
    float ArmedGrowthSpeed = 1.0f;
    bool bPressureRegistered = false;

    FMythicFarmingConfig FarmingConfig;

    bool ServerTryPlant(AActor *Interactor);

    bool ServerTryHarvest(AActor *Interactor);

    bool ServerTryClearWithered(AActor *Interactor);

    bool ServerTryTend(AActor *Interactor);

    bool ServerTryBury(AActor *Interactor);

    void AdvanceStage();

    void ArmGrowthTimerForModelSeconds(float ModelSeconds, float GrowthSpeed);

    float GetRemainingModelSeconds() const;

    void SampleMoisture(double Now);

    float CurrentGrowthSpeed() const;

    void ApplyPlotState(UMythicCropDefinition *Crop, int32 Stage);

    void CommitPlotState();

    void UpdatePressureRegistration();

    UMythicItemInstance *FindMatchingSeed(AActor *Interactor, UMythicCropDefinition *&OutCrop) const;

    UMythicItemInstance *FindMatchingFertilizer(AActor *Interactor, FGameplayTag &OutFertilizerTag) const;

    AMythicCorpse *FindNearbyCorpse() const;

    int32 ResolveFarmingLevel(AActor *Interactor, UMythicCropDefinition *Crop) const;
};
