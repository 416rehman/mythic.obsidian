#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/Trading/MythicTradeContractTypes.h"
#include "MythicTradeContractComponent.generated.h"

class AMythicVendor;
class AMythicPlayerController;
class AMythicPlayerState;
class UMythicInventoryComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnTradeContractsChanged);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicTradeContractComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicTradeContractComponent();

    // ── Config ──
    // Anti-litter cap on simultaneously ACTIVE contracts (accepts refuse past it). <= 0 = unbounded.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trading", meta = (ClampMin = "0"))
    int32 MaxActiveContracts = 3;


    // Accept an open offer from the trade board. Gates: the offer exists (unexpired), the active cap, no duplicate
    // (same QuestKind + faction) already accepted, and the reader's standing toward the faction — Neutral-or-better,
    // or FRIENDLY when the offer demands it (war-demand arms runs). Co-op-generous: accepting never retires the
    // board offer; every party member may run the same relief.
    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Trading")
    void ServerAcceptContract(int32 OfferId);

    // Abandon an active contract (no penalty in v1 — walking away from a famine is its own consequence; already-
    // delivered units stay delivered and stay paid).
    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Trading")
    void ServerAbandonContract(const FGuid &ContractId);

    // Hand contract goods to a delivery-accepting vendor: validates the vendor (bAcceptsDeliveries + interaction
    // range) and the inventory (must be one of the owning player's), clamps to the contract's remaining units, then
    // routes through AMythicVendor::Server_ExecuteDelivery (scarcity payout + GAS.Event.Deliver + P9 injection) and
    // books the progress/completion here.
    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Trading")
    void ServerDeliverToVendor(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex,
                               int32 Quantity, const FGuid &ContractId);

    // ── Reads (server + owning client) ──
    UFUNCTION(BlueprintPure, Category = "Trading")
    const TArray<FMythicTradeContract> &GetContracts() const { return Contracts; }

    const FMythicTradeContract *FindActiveContractForItemType(const FGameplayTag &ItemType) const;

    const FMythicTradeContract *FindContract(const FGuid &ContractId) const;

    UPROPERTY(BlueprintAssignable, Category = "Trading")
    FMythicOnTradeContractsChanged OnContractsChanged;

    void RestoreContracts(const TArray<FMythicTradeContract> &Saved);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_Contracts, SaveGame)
    TArray<FMythicTradeContract> Contracts;

    UFUNCTION()
    void OnRep_Contracts();

    void PayoutCompletion(const FMythicTradeContract &Contract);

    AMythicPlayerState *ResolveOwningPlayerState() const;
    AMythicPlayerController *ResolveOwningPC() const;

    int32 CountActiveContracts() const;
    bool IsAuthorityComponent() const;
};
