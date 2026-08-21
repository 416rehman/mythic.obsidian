#pragma once

#include "CoreMinimal.h"
#include "Itemization/Storage/MythicStorageContainer.h"
#include "Itemization/Vendor/MythicEconomyPricing.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicPlayerStall.generated.h"

class UItemDefinition;
class AMythicPlayerController;
struct FMythicTradePlan;

UCLASS()
class MYTHIC_API AMythicPlayerStall : public AMythicStorageContainer {
    GENERATED_BODY()

public:
    AMythicPlayerStall();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) override;
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) override;

    virtual void OnSecondaryInteract_Implementation(AActor *Interactor) override;


    // Is this controller the stall's owner (canonical-player-key match)? Owner-only verbs gate on this.
    UFUNCTION(BlueprintPure, Category = "Stall")
    bool IsStallOwner(const AController *Controller) const;

    UFUNCTION(BlueprintPure, Category = "Stall")
    int32 GetTillCoins() const { return TillCoins; }

    UFUNCTION(BlueprintPure, Category = "Stall")
    float GetListedPriceMultiplier() const { return ListedPriceMultiplier; }

    // OWNER RPC: set the stall-wide markup over the fair scarcity price (0.5..PriceCeiling; 1.0 = at fair price).
    // Client-callable by the DEPLOYING player this session (the deploy path spawns the stall Owner = their PC);
    // otherwise call server-side. Validated: only the stall owner may reprice.
    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Stall")
    void ServerSetListedPriceMultiplier(float NewMultiplier);

    FMythicTradePlan Server_ExecuteStallPurchase(AMythicPlayerController *Buyer, int32 StallSlotIndex, int32 Quantity);

    float ComputeFairUnitPrice(const UItemDefinition *Def) const;

protected:

    // Scarcity curve for the FAIR price (Elasticity 0 default ⇒ fair price == item Value exactly).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stall|Economy")
    FMythicEconomyPricingParams EconomyParams;

    // Item-type → economy axis map (empty default ⇒ every item is axis None ⇒ no scarcity, no P9 injection axis).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stall|Economy")
    TMap<FGameplayTag, EMythicEconomyAxis> ItemAxisMap;

    // The currency def the COLLECT verb mints the till into (the project gold). Unset ⇒ collect refuses (honest).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stall")
    TObjectPtr<UItemDefinition> CurrencyItemDefinition = nullptr;


    // Stall-wide markup over the fair price, owner-set. Replicated so the container UI can show listed prices.
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Stall")
    float ListedPriceMultiplier = 1.0f;

    // Uncollected proceeds. Replicated so the owner's UI can show the till without an RPC round-trip.
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Stall")
    int32 TillCoins = 0;

    // The owner's canonical player key (AMythicPlayerState::GetCanonicalPlayerKey), resolved from the deploy
    // Instigator on first BeginPlay and PERSISTED — ownership survives reloads and sessions.
    UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Stall")
    FString OwnerPlayerKey;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;


    void ArmDrainTimer();

    void HandleDrainTimer();

    int32 RunDrainPass(FRandomStream &Rng);

    UFUNCTION()
    void HandleSlotUpdated(int32 Slot);

    FMythicFactionId ResolveLocalFaction() const;

    void ResolveOwnerFromInstigator();

    bool HasAnyStock() const;

    FTimerHandle DrainTimer;

    int64 LastDrainUnixTime = 0;
};
