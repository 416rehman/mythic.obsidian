#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Interaction/IMythicInteractable.h"
#include "Subsystem/SaveSystem/World/MythicSaveableActor.h"
#include "MythicFishingSpot.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UMythicFishCatchTable;
class UCommonGenericInputActionDataTable;

UCLASS()
class MYTHIC_API AMythicFishingSpot : public AActor, public IMythicInteractable, public IMythicSaveableActor {
    GENERATED_BODY()

public:
    AMythicFishingSpot();

    virtual void OnPrimaryInteract_Implementation(AActor *Interactor) override;
    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override {}
    virtual USceneComponent *GetWidgetAttachmentComponent_Implementation() const override;
    virtual bool GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const override;
    virtual void OnFocused_Implementation(AActor *Interactor) override {}
    virtual void OnUnfocused_Implementation(AActor *Interactor) override {}

    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) override;
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

    UMythicFishCatchTable *GetCatchTable() const { return CatchTable; }

    const FGameplayTagContainer &GetLocationTags() const { return LocationTags; }

    // ── P3i stock reads/writes (SERVER; every call lazily resolves regen first) ──
    // Current stock units. Returns INT32_MAX while stocks are disabled (bottomless — the inert default).
    UFUNCTION(BlueprintCallable, Category = "Fishing|Stocks")
    int32 GetCurrentStock();

    // True when the spot is fished out (0 units) — the ability degrades the table to trash-only entries off this.
    UFUNCTION(BlueprintCallable, Category = "Fishing|Stocks")
    bool IsCurrentlyExhausted();

    // SERVER: a successful catch landed here — consume one stock unit, push the per-catch Pressure.Fish accrual, and
    // on the exhaustion EDGE push the one-shot fished-out spike. No-op while stocks are disabled / off authority.
    UFUNCTION(BlueprintCallable, Category = "Fishing|Stocks")
    void ServerNotifyCatch();

protected:
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fishing")
    USceneComponent *SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Fishing")
    UStaticMeshComponent *Mesh;

    // Designer-authored catch table for this spot.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fishing")
    TObjectPtr<UMythicFishCatchTable> CatchTable;

    // Location tag(s) describing this spot (river / lake / coast) — matched against each catch entry's Location gate.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fishing")
    FGameplayTagContainer LocationTags;

    // Per-spot max stock override. 0 (default) = use FMythicFishStockConfig::DefaultMaxStock.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fishing|Stocks", meta = (ClampMin = "0"))
    int32 MaxStockOverride = 0;

    // Interaction prompt data (matches the toggleable/container pattern).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    TObjectPtr<const UCommonGenericInputActionDataTable> InputActionDataTable;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Interaction")
    FName PrimaryInteractionName = FName("Fish");

private:
    int32 EffectiveMaxStock() const;

    double NowSeconds() const;

    int32 StockUnits = 0;
    double RegenAnchorTime = 0.0;
    bool bStockInitialized = false;

    int32 ResolveStockNow();
};
