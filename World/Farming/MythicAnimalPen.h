
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interaction/IMythicInteractable.h"
#include "Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "World/Gathering/MythicYieldQuality.h"
#include "MythicLivestockGenome.h"
#include "MythicAnimalPen.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UCommonGenericInputActionDataTable;
class UMythicLivestockDefinition;
class UMythicLivestockRegistry;
class UMythicItemInstance;

USTRUCT()
struct FMythicLivestockRecord {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UMythicLivestockDefinition> Def = nullptr;

    EMythicYieldQuality FeedTier = EMythicYieldQuality::Common;

    double FedUntilTime = 0.0;

    double LastSampleTime = 0.0;

    float CarryoverSeconds = 0.0f;

    int32 StoredUnits = 0;

    FMythicLivestockGenome Genome;
};

USTRUCT(BlueprintType)
struct FMythicPenState {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Husbandry")
    int32 AnimalCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Husbandry")
    int32 ReadyUnits = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Husbandry")
    bool bAnyFed = false;
};

UCLASS()
class MYTHIC_API AMythicAnimalPen : public AActor, public IMythicInteractable, public IMythicSaveableActor {
    GENERATED_BODY()

public:
    AMythicAnimalPen();

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override;
    virtual void OnUnfocused_Implementation(AActor *Interactor) override;

    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) override;
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

    UFUNCTION(BlueprintPure, Category = "Husbandry")
    const FMythicPenState &GetPenState() const { return PenState; }

    void ServerHandlePrimaryInteract(AActor *Interactor);
    bool ServerTryCollect(AActor *Interactor);
    bool ServerTryAddLivestock(AActor *Interactor);
    bool ServerTryFeed(AActor *Interactor);

    /**
     * SERVER BREEDING: pair two DISTINCT same-species FED parents in this pen and mint an offspring animal whose genome
     * is inherited (blended + mutated) from theirs — the emergent selective-breeding loop. Authority-guarded; no-op when
     * the master switch is off, the pen is full, or no eligible pair is fed. Reuses the EXISTING feed clock as the gate
     * (both parents must be currently fed) and its COST (breeding exhausts the parents' fed window — re-feed to breed
     * again); it invents no competing cooldown. BlueprintCallable so pen UI / debug can drive it. Returns true on a mint.
     */
    UFUNCTION(BlueprintCallable, Category = "Husbandry")
    bool ServerTryBreed(AActor *Interactor);

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    UFUNCTION()
    void OnRep_PenState();

    // Cosmetic reaction (animal meshes, produce sparkle, hungry idle). Fires on server + clients via OnRep.
    UFUNCTION(BlueprintImplementableEvent, Category = "Husbandry")
    void OnPenVisualChanged(const FMythicPenState &NewState);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Husbandry")
    USceneComponent *SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Husbandry")
    UStaticMeshComponent *Mesh;

    /** Livestock-item → species map (mirrors the plot's crop registry). Authored per pen (or shared). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Husbandry")
    TObjectPtr<UMythicLivestockRegistry> LivestockRegistry;

    /** Animal capacity (anti-litter — a pen is a pen, not a feedlot). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Husbandry", meta = (ClampMin = "1"))
    int32 MaxAnimals = 4;

    // Interaction prompt data (matches the plot/container pattern).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName CollectInteractionName = FName("Collect");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName AddAnimalInteractionName = FName("PenAnimal");

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName FeedInteractionName = FName("Feed");

    // Replicated snapshot (cosmetics + prompts; the server re-samples at every decision).
    UPROPERTY(ReplicatedUsing = OnRep_PenState, BlueprintReadOnly, Category = "Husbandry")
    FMythicPenState PenState;

private:
    UPROPERTY()
    TArray<FMythicLivestockRecord> Records;

    FTimerHandle ProductionTimerHandle;

    void SampleProduction(double Now);

    void RearmProductionTimer();

    void RefreshPenState();

    UMythicItemInstance *FindLivestockItem(AActor *Interactor, UMythicLivestockDefinition *&OutDef) const;
    UMythicItemInstance *FindFeedItem(AActor *Interactor, EMythicYieldQuality &OutFeedTier) const;
};
