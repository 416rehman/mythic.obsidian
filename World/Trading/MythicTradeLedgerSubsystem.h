#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/Trading/MythicTradeLedger.h"
#include "World/Trading/MythicTradeContractTypes.h"
#include "MythicTradeLedgerSubsystem.generated.h"

class UMythicLivingWorldSubsystem;
class AMythicPlayerController;

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicTradeLedgerView {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    int32 SettlementId = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    FMythicFactionId GoverningFaction;

    /** True = current-commit prices (POI unlocked or standing ≥ Neutral); false = the aging rumor snapshot. */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    bool bLive = false;

    /** 0 for live reads; 1 − 0.5^(age/halflife) for rumors (1 = worthless hearsay). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    float Staleness = 0.0f;

    /** Per-axis prices — exact when live, band-quantized when stale (vaguer the older the rumor). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    FMythicResourceStock Prices;

    /** Per-axis reserves — populated ONLY on live reads (rumors carry prices, not the faction's books). */
    UPROPERTY(BlueprintReadOnly, Category = "Trading")
    FMythicResourceStock Reserves;
};

UCLASS()
class MYTHIC_API UMythicTradeLedgerSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;
    virtual void Deinitialize() override;


    /** Fog-of-war-resolved ledger read for Reader: LIVE where the settlement's POI is unlocked or Reader's standing
     *  toward the governing faction is Neutral-or-better; else the aging rumor snapshot (quantized prices, no
     *  reserves). False when the settlement is unknown / never sampled. Server-side (the sim data lives here). */
    UFUNCTION(BlueprintCallable, Category = "Trading")
    bool GetLedgerViewForPlayer(int32 SettlementId, AMythicPlayerController *Reader, FMythicTradeLedgerView &OutView) const;

    /** All currently-sampled settlement ids (the ledger's row keys), for enumerating reads. */
    UFUNCTION(BlueprintCallable, Category = "Trading")
    TArray<int32> GetLedgerSettlementIds() const;


    int32 RegisterContractOffer(FMythicTradeContractOffer Offer);

    /** The open (unexpired) offers, newest last. */
    UFUNCTION(BlueprintCallable, Category = "Trading")
    const TArray<FMythicTradeContractOffer> &GetOpenOffers() const { return OpenOffers; }

    const FMythicTradeContractOffer *FindOffer(int32 OfferId) const;

    void GetOpenOfferKinds(TArray<FGameplayTag> &OutKinds) const;

    void SubmitTradeBeat(const FGameplayTag &EventTag, FMythicFactionId PrimaryFaction, const FVector &Location,
                         float Significance);


    float ComputeCargoValueForPlayer(AMythicPlayerController *PC) const;

    /** Cargo heat [0,1] for a player at their pawn's location: pure MythicCargoRisk::ComputeCargoHeat over their
     *  cargo value and the live danger tier (0 below danger tier Trading.CargoHeatMinDangerTier). */
    UFUNCTION(BlueprintCallable, Category = "Trading")
    float GetCargoHeatForPlayer(AMythicPlayerController *PC) const;

    /** The HIGHEST cargo heat among players within Radius of Location — the single call the EncounterDirector
     *  ambush-weight one-liner consumes (0 with no players near / no valuable cargo / low danger). */
    UFUNCTION(BlueprintCallable, Category = "Trading")
    float GetMaxCargoHeatAt(const FVector &Location, float Radius = 30000.0f) const;

private:
    void HandleWorldSimCommitted();

    void SampleLedger(double NowSeconds);
    void EmitDeficitBeats();
    void EmitRumorBeat(double NowSeconds);

    bool IsAuthority() const;

    bool ResolveSettlementAnchor(int32 SettlementId, FVector &OutAnchor) const;

    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorld = nullptr;

    FDelegateHandle CommitHandle;

    TMap<int32, FMythicTradeLedgerEntry> LiveLedger;
    TMap<int32, FMythicTradeLedgerEntry> RumorSnapshot;
    double LastRumorSnapshotSeconds = -1.0e18;

    double LastRumorBeatSeconds[ResourceTypeCount] = {-1.0e18, -1.0e18, -1.0e18, -1.0e18};

    TMap<uint32, bool> DeficitLatches;

    TArray<FMythicTradeContractOffer> OpenOffers;
    int32 NextOfferId = 1;
};
