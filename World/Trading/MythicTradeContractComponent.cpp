
#include "World/Trading/MythicTradeContractComponent.h"

#include "World/Trading/MythicTradeLedgerSubsystem.h"
#include "World/Trading/MythicTags_Trading.h"
#include "Itemization/Vendor/MythicVendor.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicTrade.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicFactionStandingComponent.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Mythic.h"

UMythicTradeContractComponent::UMythicTradeContractComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicTradeContractComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicTradeContractComponent, Contracts, COND_OwnerOnly);
}

void UMythicTradeContractComponent::OnRep_Contracts() {
    OnContractsChanged.Broadcast();
}

bool UMythicTradeContractComponent::IsAuthorityComponent() const {
    const AActor *Owner = GetOwner();
    return Owner && Owner->HasAuthority();
}

AMythicPlayerState *UMythicTradeContractComponent::ResolveOwningPlayerState() const {
    return Cast<AMythicPlayerState>(GetOwner());
}

AMythicPlayerController *UMythicTradeContractComponent::ResolveOwningPC() const {
    const AMythicPlayerState *PS = ResolveOwningPlayerState();
    return PS ? Cast<AMythicPlayerController>(PS->GetPlayerController()) : nullptr;
}

int32 UMythicTradeContractComponent::CountActiveContracts() const {
    int32 Count = 0;
    for (const FMythicTradeContract &C : Contracts) {
        if (C.IsActive()) {
            ++Count;
        }
    }
    return Count;
}

const FMythicTradeContract *UMythicTradeContractComponent::FindContract(const FGuid &ContractId) const {
    for (const FMythicTradeContract &C : Contracts) {
        if (C.ContractId == ContractId) {
            return &C;
        }
    }
    return nullptr;
}

const FMythicTradeContract *UMythicTradeContractComponent::FindActiveContractForItemType(const FGameplayTag &ItemType) const {
    for (const FMythicTradeContract &C : Contracts) {
        if (C.IsActive() && ItemType.MatchesTag(C.DeliveryItemTag)) {
            return &C;
        }
    }
    return nullptr;
}


bool UMythicTradeContractComponent::ServerAcceptContract_Validate(int32 OfferId) {
    return true;
}

void UMythicTradeContractComponent::ServerAcceptContract_Implementation(int32 OfferId) {
    if (!IsAuthorityComponent()) {
        return;
    }
    UWorld *World = GetWorld();
    UMythicTradeLedgerSubsystem *Board = World ? World->GetSubsystem<UMythicTradeLedgerSubsystem>() : nullptr;
    const FMythicTradeContractOffer *Offer = Board ? Board->FindOffer(OfferId) : nullptr;
    if (!Offer) {
        return;
    }
    if (MaxActiveContracts > 0 && CountActiveContracts() >= MaxActiveContracts) {
        return;
    }
    for (const FMythicTradeContract &C : Contracts) {
        if (C.IsActive() && C.QuestKind == Offer->QuestKind && C.FactionId == Offer->FactionId) {
            return;
        }
    }
    const AMythicPlayerState *PS = ResolveOwningPlayerState();
    const UMythicFactionStandingComponent *Standing = PS ? PS->GetFactionStanding() : nullptr;
    if (!Standing) {
        return;
    }
    const EMythicStandingTier Tier = Standing->TierForStanding(Standing->GetStanding(Offer->FactionId));
    if (Tier == EMythicStandingTier::Hostile || (Offer->bRequiresFriendlyStanding && Tier != EMythicStandingTier::Friendly)) {
        return;
    }

    FMythicTradeContract Contract;
    Contract.ContractId = FGuid::NewGuid();
    Contract.QuestKind = Offer->QuestKind;
    Contract.FactionId = Offer->FactionId;
    Contract.SettlementId = Offer->SettlementId;
    Contract.DeliveryItemTag = Offer->DeliveryItemTag;
    Contract.UnitsRequired = Offer->Units;
    Contract.UnitsDelivered = 0;
    Contract.State = EMythicTradeContractState::Active;
    Contract.ReserveAxis = Offer->ReserveAxis;
    Contract.StandingReward = Offer->StandingReward;
    Contract.Headline = Offer->Headline;
    Contracts.Add(MoveTemp(Contract));
    OnContractsChanged.Broadcast();
    UE_LOG(Myth, Log, TEXT("TradeContract: %s accepted offer %d (%s)"), *GetNameSafe(GetOwner()), OfferId,
           *Offer->QuestKind.ToString());
}

bool UMythicTradeContractComponent::ServerAbandonContract_Validate(const FGuid &ContractId) {
    return true;
}

void UMythicTradeContractComponent::ServerAbandonContract_Implementation(const FGuid &ContractId) {
    if (!IsAuthorityComponent()) {
        return;
    }
    for (int32 i = 0; i < Contracts.Num(); ++i) {
        if (Contracts[i].ContractId == ContractId && Contracts[i].IsActive()) {
            Contracts.RemoveAt(i);
            OnContractsChanged.Broadcast();
            return;
        }
    }
}


bool UMythicTradeContractComponent::ServerDeliverToVendor_Validate(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory,
                                                                   int32 PlayerSlotIndex, int32 Quantity, const FGuid &ContractId) {
    return true;
}

void UMythicTradeContractComponent::ServerDeliverToVendor_Implementation(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory,
                                                                         int32 PlayerSlotIndex, int32 Quantity, const FGuid &ContractId) {
    if (!IsAuthorityComponent() || !Vendor || !PlayerInventory || Quantity <= 0) {
        return;
    }
    AMythicPlayerController *PC = ResolveOwningPC();
    AMythicPlayerState *PS = ResolveOwningPlayerState();
    if (!PC || !PS) {
        return;
    }
    const APawn *Pawn = PC->GetPawn();
    if (!Vendor->AcceptsDeliveries() || !Pawn || !Vendor->IsActorInRange(Pawn)) {
        return;
    }
    if (!PC->GetAllInventoryComponents().Contains(PlayerInventory)) {
        return;
    }
    FMythicTradeContract *Contract = nullptr;
    for (FMythicTradeContract &C : Contracts) {
        if (C.ContractId == ContractId) {
            Contract = &C;
            break;
        }
    }
    if (!Contract || !Contract->IsActive() || Contract->UnitsRemaining() <= 0) {
        return;
    }
    const int32 RequestUnits = FMath::Min(Quantity, Contract->UnitsRemaining());

    const FMythicTradePlan Plan = Vendor->Server_ExecuteDelivery(PC, PlayerInventory, PlayerSlotIndex, RequestUnits,
                                                                 Contract->DeliveryItemTag, Contract->ReserveAxis);
    if (Plan.Quantity <= 0) {
        return;
    }

    const MythicTradeContracts::FDeliveryApplication Applied =
        MythicTradeContracts::ApplyDelivery(Contract->UnitsRequired, Contract->UnitsDelivered, Plan.Quantity);
    Contract->UnitsDelivered += Applied.AcceptedUnits;

    if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
        if (Applied.AcceptedUnits > 0) {
            Ledger->RecordStat(STAT_TRADE_UNITS_DELIVERED, Applied.AcceptedUnits);
        }
        if (Plan.TotalPrice > 0) {
            Ledger->RecordStat(STAT_TRADE_PROFIT, Plan.TotalPrice);
        }
    }

    if (Applied.bCompleted) {
        Contract->State = EMythicTradeContractState::Completed;
        PayoutCompletion(*Contract);
        for (int32 i = 0; i < Contracts.Num(); ++i) {
            if (Contracts[i].ContractId == ContractId) {
                Contracts.RemoveAt(i);
                break;
            }
        }
    }
    OnContractsChanged.Broadcast();
}

void UMythicTradeContractComponent::PayoutCompletion(const FMythicTradeContract &Contract) {
    AMythicPlayerState *PS = ResolveOwningPlayerState();
    if (!PS) {
        return;
    }
    if (Contract.StandingReward != 0.0f && Contract.FactionId.IsValid()) {
        if (UMythicFactionStandingComponent *Standing = PS->GetFactionStanding()) {
            Standing->ServerAdjustStanding(Contract.FactionId, Contract.StandingReward);
        }
    }
    if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
        Ledger->RecordStat(STAT_TRADE_CONTRACTS_COMPLETED, 1);
    }
    UWorld *World = GetWorld();
    if (UMythicTradeLedgerSubsystem *Board = World ? World->GetSubsystem<UMythicTradeLedgerSubsystem>() : nullptr) {
        const AMythicPlayerController *PC = ResolveOwningPC();
        const APawn *Pawn = PC ? PC->GetPawn() : nullptr;
        Board->SubmitTradeBeat(TAG_TRADING_EVENT_CONTRACT_COMPLETED, Contract.FactionId,
                               Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector, 0.6f);
    }
    UE_LOG(Myth, Log, TEXT("TradeContract: %s completed %s (+%.0f standing with faction %d)"), *GetNameSafe(GetOwner()),
           *Contract.QuestKind.ToString(), Contract.StandingReward, static_cast<int32>(Contract.FactionId.Index));
}


void UMythicTradeContractComponent::RestoreContracts(const TArray<FMythicTradeContract> &Saved) {
    if (!IsAuthorityComponent() || Saved.Num() == 0) {
        return;
    }
    Contracts = Saved;
    for (int32 i = Contracts.Num() - 1; i >= 0; --i) {
        if (!Contracts[i].IsActive()) {
            Contracts.RemoveAt(i);
        }
    }
    OnContractsChanged.Broadcast();
}
