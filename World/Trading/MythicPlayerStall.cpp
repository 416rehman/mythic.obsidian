
#include "World/Trading/MythicPlayerStall.h"

#include "World/Trading/MythicStallSales.h"
#include "World/Trading/MythicTags_Trading.h"
#include "World/Trading/MythicTradeLedgerSubsystem.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicCurrency.h"
#include "Itemization/Inventory/MythicTrade.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "Settings/MythicDeveloperSettings.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Mythic.h"

namespace {
    int32 StallSumPlayerCurrency(AMythicPlayerController *Player) {
        int32 Total = 0;
        if (!Player) {
            return 0;
        }
        for (UMythicInventoryComponent *Inv : Player->GetAllInventoryComponents()) {
            if (Inv) {
                Total += Inv->GetTotalCurrency();
            }
        }
        return Total;
    }

    void StallChargePlayerCurrency(AMythicPlayerController *Player, int32 Amount) {
        if (!Player || Amount <= 0) {
            return;
        }
        int32 Remaining = Amount;
        for (UMythicInventoryComponent *Inv : Player->GetAllInventoryComponents()) {
            if (Remaining <= 0) {
                break;
            }
            if (Inv) {
                Remaining -= Inv->SpendCurrency(Remaining);
            }
        }
    }

    void StallMintCurrency(UMythicInventoryComponent *Inv, AController *Recipient, int32 Amount, UItemDefinition *CurrencyDef,
                           UMythicLootManagerSubsystem *Loot) {
        if (!Inv || !CurrencyDef || !Loot || Amount <= 0) {
            return;
        }
        const int32 Cap = FMath::Max(1, CurrencyDef->StackSizeMax);
        const int64 MaxChunks = (static_cast<int64>(Amount) + Cap - 1) / Cap + 4;
        int32 Remaining = Amount;
        int64 Guard = 0;
        while (Remaining > 0 && Guard++ < MaxChunks) {
            const int32 Chunk = FMath::Min(Remaining, Cap);
            UMythicItemInstance *Coins = Loot->Create(CurrencyDef, Chunk, Recipient, 0);
            if (!Coins) {
                break;
            }
            Inv->AddItem(Coins, Recipient);
            Remaining -= Chunk;
        }
        if (Remaining > 0) {
            UE_LOG(Myth, Warning, TEXT("MythicPlayerStall: could not mint %d of %d till coins for the collect"), Remaining, Amount);
        }
    }
}

AMythicPlayerStall::AMythicPlayerStall() {
}

void AMythicPlayerStall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMythicPlayerStall, ListedPriceMultiplier);
    DOREPLIFETIME(AMythicPlayerStall, TillCoins);
    DOREPLIFETIME(AMythicPlayerStall, OwnerPlayerKey);
}

void AMythicPlayerStall::BeginPlay() {
    Super::BeginPlay();
    if (!HasAuthority()) {
        return;
    }
    ResolveOwnerFromInstigator();
    if (LastDrainUnixTime == 0) {
        LastDrainUnixTime = FDateTime::UtcNow().ToUnixTimestamp();
    }
    if (UMythicInventoryComponent *Inv = GetContainerInventory()) {
        Inv->OnSlotUpdated.AddDynamic(this, &AMythicPlayerStall::HandleSlotUpdated);
    }
    if (HasAnyStock()) {
        ArmDrainTimer();
    }
}

void AMythicPlayerStall::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(DrainTimer);
    }
    Super::EndPlay(EndPlayReason);
}


void AMythicPlayerStall::SerializeCustomData(TArray<uint8> &OutCustomData) {
    TArray<uint8> BasePayload;
    Super::SerializeCustomData(BasePayload);
    MythicStallSales::SerializeStallState(OutCustomData, TillCoins, LastDrainUnixTime, ListedPriceMultiplier,
                                          OwnerPlayerKey, BasePayload);
}

void AMythicPlayerStall::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    int32 SavedTill = 0;
    int64 SavedAnchor = 0;
    float SavedListedMult = 1.0f;
    FString SavedOwnerKey;
    TArray<uint8> BasePayload;
    if (!MythicStallSales::DeserializeStallState(InCustomData, SavedTill, SavedAnchor, SavedListedMult, SavedOwnerKey, BasePayload)) {
        Super::DeserializeCustomData(InCustomData);
        return;
    }
    TillCoins = SavedTill;
    ListedPriceMultiplier = SavedListedMult;
    if (!SavedOwnerKey.IsEmpty()) {
        OwnerPlayerKey = SavedOwnerKey;
    }
    Super::DeserializeCustomData(BasePayload);

    if (!HasAuthority()) {
        return;
    }

    const int64 NowUnix = FDateTime::UtcNow().ToUnixTimestamp();
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    if (Dev && Dev->bEnableTrading && SavedAnchor > 0) {
        const int32 Passes = MythicStallSales::ComputeAccruedDrains(static_cast<double>(NowUnix - SavedAnchor),
                                                                    Dev->Trading.StallDrainIntervalSeconds,
                                                                    Dev->Trading.StallMaxAccruedDrains);
        if (Passes > 0) {
            FRandomStream Rng(FMath::Rand());
            int32 TotalSold = 0;
            for (int32 Pass = 0; Pass < Passes && HasAnyStock(); ++Pass) {
                TotalSold += RunDrainPass(Rng);
            }
            if (TotalSold > 0) {
                if (UMythicTradeLedgerSubsystem *Ledger = GetWorld() ? GetWorld()->GetSubsystem<UMythicTradeLedgerSubsystem>() : nullptr) {
                    Ledger->SubmitTradeBeat(TAG_TRADING_EVENT_STALL_SALE, ResolveLocalFaction(), GetActorLocation(), 0.5f);
                }
            }
        }
    }
    LastDrainUnixTime = NowUnix;
    if (HasAnyStock()) {
        ArmDrainTimer();
    }
}


void AMythicPlayerStall::ResolveOwnerFromInstigator() {
    if (!OwnerPlayerKey.IsEmpty()) {
        return;
    }
    const APawn *InstigatorPawn = GetInstigator();
    const AMythicPlayerState *PS = InstigatorPawn ? InstigatorPawn->GetPlayerState<AMythicPlayerState>() : nullptr;
    if (PS) {
        OwnerPlayerKey = PS->GetCanonicalPlayerKey();
    }
}

bool AMythicPlayerStall::IsStallOwner(const AController *Controller) const {
    if (OwnerPlayerKey.IsEmpty() || !Controller) {
        return false;
    }
    const AMythicPlayerState *PS = Controller->GetPlayerState<AMythicPlayerState>();
    return PS && PS->GetCanonicalPlayerKey() == OwnerPlayerKey;
}


FMythicFactionId AMythicPlayerStall::ResolveLocalFaction() const {
    const UGameInstance *GI = GetGameInstance();
    UMythicLivingWorldSubsystem *LWS = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    UMythicTerritoryGrid *Grid = LWS ? LWS->GetTerritoryGrid() : nullptr;
    if (!LWS || !Grid) {
        return FMythicFactionId();
    }
    const FMythicCellCoord Cell = Grid->WorldToCell(GetActorLocation());
    FMythicSettlementData Settlement;
    if (LWS->CopySettlementAtCell(Cell, Settlement) && Settlement.GoverningFaction.IsValid()) {
        return Settlement.GoverningFaction;
    }
    return Grid->GetDominantFaction(Cell);
}

float AMythicPlayerStall::ComputeFairUnitPrice(const UItemDefinition *Def) const {
    if (!Def || Def->Value <= 0) {
        return 0.0f;
    }
    float Scarcity = 1.0f;
    if (EconomyParams.Elasticity > 0.0f && ItemAxisMap.Num() > 0) {
        FGameplayTagContainer TypeTags;
        TypeTags.AddTag(Def->ItemType);
        const EMythicEconomyAxis Axis = FMythicEconomyPricing::AxisForItem(TypeTags, ItemAxisMap);
        const FMythicFactionId Faction = ResolveLocalFaction();
        if (Axis != EMythicEconomyAxis::None && Faction.IsValid()) {
            const UGameInstance *GI = GetGameInstance();
            const UMythicLivingWorldSubsystem *LWS = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
            const UMythicFactionDatabase *FDB = LWS ? LWS->GetFactionDatabase() : nullptr;
            FMythicFactionData Data;
            if (FDB && FDB->GetFaction(Faction, Data) && Data.bHasEconomy) {
                const EMythicResourceType Res = static_cast<EMythicResourceType>(static_cast<uint8>(Axis) - 1);
                Scarcity = FMythicEconomyPricing::ComputeScarcityMultiplier(Data.Reserves.GetResource(Res),
                                                                            Data.Demand.GetResource(Res),
                                                                            Data.Prices.GetResource(Res), Axis, EconomyParams);
            }
        }
    }
    return static_cast<float>(Def->Value) * Scarcity;
}


bool AMythicPlayerStall::HasAnyStock() const {
    UMythicInventoryComponent *Inv = GetContainerInventory();
    if (!Inv) {
        return false;
    }
    for (int32 Slot = 0; Slot < Inv->GetNumSlots(); ++Slot) {
        if (Inv->GetItem(Slot)) {
            return true;
        }
    }
    return false;
}

void AMythicPlayerStall::HandleSlotUpdated(int32) {
    if (HasAuthority() && HasAnyStock()) {
        ArmDrainTimer();
    }
}

void AMythicPlayerStall::ArmDrainTimer() {
    UWorld *World = GetWorld();
    if (!World || !HasAuthority()) {
        return;
    }
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    if (!Dev || !Dev->bEnableTrading) {
        return;
    }
    if (World->GetTimerManager().IsTimerActive(DrainTimer)) {
        return;
    }
    const float Interval = FMath::Max(Dev->Trading.StallDrainIntervalSeconds, 5.0f);
    World->GetTimerManager().SetTimer(DrainTimer, this, &AMythicPlayerStall::HandleDrainTimer, Interval, false);
}

void AMythicPlayerStall::HandleDrainTimer() {
    if (!HasAuthority()) {
        return;
    }
    FRandomStream Rng(FMath::Rand());
    const int32 Sold = RunDrainPass(Rng);
    LastDrainUnixTime = FDateTime::UtcNow().ToUnixTimestamp();
    if (Sold > 0) {
        if (UMythicTradeLedgerSubsystem *Ledger = GetWorld() ? GetWorld()->GetSubsystem<UMythicTradeLedgerSubsystem>() : nullptr) {
            Ledger->SubmitTradeBeat(TAG_TRADING_EVENT_STALL_SALE, ResolveLocalFaction(), GetActorLocation(), 0.5f);
        }
    }
    if (HasAnyStock()) {
        ArmDrainTimer();
    }
}

int32 AMythicPlayerStall::RunDrainPass(FRandomStream &Rng) {
    UMythicInventoryComponent *Inv = GetContainerInventory();
    if (!Inv || !HasAuthority()) {
        return 0;
    }
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    if (!Dev || !Dev->bEnableTrading) {
        return 0;
    }
    const float BaseChance = Dev->Trading.StallBaseSaleChancePerDrain;
    const float CeilingRatio = Dev->Trading.StallPriceCeilingRatio;
    const float UnitsToReserve = Dev->Trading.StallUnitsToReservePerUnit;

    int32 TotalSold = 0;
    int32 Proceeds = 0;
    float AxisUnits[ResourceTypeCount] = {0.0f, 0.0f, 0.0f, 0.0f};

    for (int32 Slot = 0; Slot < Inv->GetNumSlots(); ++Slot) {
        UMythicItemInstance *Item = Inv->GetItem(Slot);
        if (!Item) {
            continue;
        }
        const UItemDefinition *Def = Item->GetItemDefinition();
        if (!Def || Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
            continue;
        }
        const float Fair = ComputeFairUnitPrice(Def);
        const float Listed = Fair * FMath::Max(ListedPriceMultiplier, 0.0f);
        const float Chance = MythicStallSales::ComputeSaleChance(Listed, Fair, BaseChance, CeilingRatio);
        const int32 SoldUnits = MythicStallSales::RollUnitsSold(Item->GetStacks(), Chance, Rng.FRand());
        if (SoldUnits <= 0) {
            continue;
        }
        Inv->ServerRemoveItem(Item, SoldUnits);
        TotalSold += SoldUnits;
        Proceeds += FMath::Max(FMath::RoundToInt(Listed), 0) * SoldUnits;
        FGameplayTagContainer TypeTags;
        TypeTags.AddTag(Def->ItemType);
        const EMythicEconomyAxis Axis = FMythicEconomyPricing::AxisForItem(TypeTags, ItemAxisMap);
        if (Axis != EMythicEconomyAxis::None) {
            AxisUnits[static_cast<uint8>(Axis) - 1] += static_cast<float>(SoldUnits);
        }
    }

    if (Proceeds > 0) {
        TillCoins += Proceeds;
    }

    if (TotalSold > 0 && UnitsToReserve > 0.0f) {
        const FMythicFactionId Faction = ResolveLocalFaction();
        const UGameInstance *GI = GetGameInstance();
        UMythicLivingWorldSubsystem *LWS = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
        if (LWS && Faction.IsValid()) {
            for (int32 AxisOrd = 0; AxisOrd < ResourceTypeCount; ++AxisOrd) {
                if (AxisUnits[AxisOrd] > 0.0f) {
                    LWS->EnqueuePlayerResourceDelta(Faction, static_cast<EMythicResourceType>(AxisOrd),
                                                    AxisUnits[AxisOrd] * UnitsToReserve);
                }
            }
        }
    }
    return TotalSold;
}


bool AMythicPlayerStall::ServerSetListedPriceMultiplier_Validate(float NewMultiplier) {
    return true;
}

void AMythicPlayerStall::ServerSetListedPriceMultiplier_Implementation(float NewMultiplier) {
    if (!HasAuthority()) {
        return;
    }
    const AController *Repricer = GetOwner() ? Cast<AController>(GetOwner()) : nullptr;
    if (Repricer && !IsStallOwner(Repricer)) {
        return;
    }
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    const float Ceiling = Dev ? Dev->Trading.StallPriceCeilingRatio : 2.0f;
    ListedPriceMultiplier = FMath::Clamp(NewMultiplier, 0.5f, Ceiling);
}

void AMythicPlayerStall::OnSecondaryInteract_Implementation(AActor *Interactor) {
    AController *Controller = ResolveController(Interactor);
    if (!HasAuthority() || !Controller || !IsStallOwner(Controller)) {
        Super::OnSecondaryInteract_Implementation(Interactor);
        return;
    }
    if (TillCoins <= 0 || !CurrencyItemDefinition) {
        return;
    }
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(Controller);
    UGameInstance *GI = GetGameInstance();
    UMythicLootManagerSubsystem *Loot = GI ? GI->GetSubsystem<UMythicLootManagerSubsystem>() : nullptr;
    if (!PC || !Loot) {
        return;
    }
    UMythicInventoryComponent *Target = nullptr;
    for (UMythicInventoryComponent *Inv : PC->GetAllInventoryComponents()) {
        if (Inv && Inv->CanAcceptItemType(CurrencyItemDefinition->ItemType)) {
            Target = Inv;
            break;
        }
    }
    if (!Target) {
        return;
    }
    const int32 Collected = TillCoins;
    TillCoins = 0;
    StallMintCurrency(Target, PC, Collected, CurrencyItemDefinition, Loot);
    if (AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>()) {
        if (UMythicStatLedgerComponent *StatLedger = PS->GetStatLedgerComponent()) {
            StatLedger->RecordStat(STAT_TRADE_PROFIT, Collected);
        }
    }
    UE_LOG(Myth, Log, TEXT("MythicPlayerStall: %s collected %d coins from the till"), *GetNameSafe(PC), Collected);
}


FMythicTradePlan AMythicPlayerStall::Server_ExecuteStallPurchase(AMythicPlayerController *Buyer, int32 StallSlotIndex, int32 Quantity) {
    FMythicTradePlan Reject;
    UMythicInventoryComponent *Stock = GetContainerInventory();
    if (!HasAuthority() || !Buyer || !Stock || Quantity <= 0) {
        return Reject;
    }
    if (IsStallOwner(Buyer)) {
        return Reject;
    }
    UMythicItemInstance *StockItem = Stock->GetItem(StallSlotIndex);
    if (!StockItem) {
        Reject.Result = EMythicTradeResult::OutOfStock;
        return Reject;
    }
    UItemDefinition *Def = StockItem->GetItemDefinition();
    if (!Def || Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
        return Reject;
    }
    UGameInstance *GI = GetGameInstance();
    UMythicLootManagerSubsystem *Loot = GI ? GI->GetSubsystem<UMythicLootManagerSubsystem>() : nullptr;
    if (!Loot) {
        return Reject;
    }
    UMythicInventoryComponent *Target = nullptr;
    for (UMythicInventoryComponent *Inv : Buyer->GetAllInventoryComponents()) {
        if (Inv && Inv->CanAcceptItemType(Def->ItemType)) {
            Target = Inv;
            break;
        }
    }

    const float Fair = ComputeFairUnitPrice(Def);
    const float ListedMultiplier = (Def->Value > 0) ? (Fair / static_cast<float>(Def->Value)) * FMath::Max(ListedPriceMultiplier, 0.0f) : 0.0f;
    const FMythicTradePlan Plan = MythicTrade::PlanBuy(Quantity, Def->Value, ListedMultiplier, StockItem->GetStacks(),
                                                       StallSumPlayerCurrency(Buyer), Target != nullptr);
    if (Plan.Quantity <= 0) {
        return Plan;
    }

    if (Plan.Quantity > 0 && Plan.Quantity >= StockItem->GetStacks()) {
        UMythicItemInstance *Carried = Stock->ReleaseFromSlot(StallSlotIndex);
        if (!Carried) {
            return Reject;
        }
        StallChargePlayerCurrency(Buyer, Plan.TotalPrice);
        Target->AddItem(Carried, Buyer);
    }
    else {
        const int32 StackCap = FMath::Max(1, Def->StackSizeMax);
        TArray<UMythicItemInstance *, TInlineAllocator<8>> Goods;
        for (int32 ToMake = Plan.Quantity; ToMake > 0;) {
            const int32 Chunk = FMath::Min(ToMake, StackCap);
            UMythicItemInstance *G = Loot->Create(Def, Chunk, Buyer, StockItem->GetItemLevel());
            if (!G) {
                for (UMythicItemInstance *Made : Goods) {
                    if (Made) {
                        Made->Destroy();
                    }
                }
                return Reject;
            }
            Goods.Add(G);
            ToMake -= Chunk;
        }
        StallChargePlayerCurrency(Buyer, Plan.TotalPrice);
        Stock->ServerRemoveItem(StockItem, Plan.Quantity);
        for (UMythicItemInstance *G : Goods) {
            Target->AddItem(G, Buyer);
        }
    }

    TillCoins += Plan.TotalPrice;
    return Plan;
}
