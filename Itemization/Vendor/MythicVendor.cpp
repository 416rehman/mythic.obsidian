
#include "MythicVendor.h"

#include "Mythic.h"
#include "Engine/GameInstance.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicCurrency.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/MythicTrade.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicFactionStandingComponent.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/Events/ActionEventTypes.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "World/Trading/MythicTags_Trading.h"
#include "World/Trading/MythicTradeContractTypes.h"
#include "Settings/MythicDeveloperSettings.h"

namespace {
    EMythicResourceType ResourceTypeForAxis(EMythicEconomyAxis Axis) {
        switch (Axis) {
        case EMythicEconomyAxis::Materials:
            return EMythicResourceType::Materials;
        case EMythicEconomyAxis::Arms:
            return EMythicResourceType::Arms;
        case EMythicEconomyAxis::Wealth:
            return EMythicResourceType::Wealth;
        case EMythicEconomyAxis::Food:
        default:
            return EMythicResourceType::Food;
        }
    }

    int32 SumPlayerCurrency(AMythicPlayerController *Player) {
        return Player ? Player->GetCarriedCurrency() : 0;
    }
}

AMythicVendor::AMythicVendor() {
}

UItemDefinition *AMythicVendor::ResolveCurrencyItemDefinition() const {
    if (CurrencyItemDefinition) {
        return CurrencyItemDefinition;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    return Settings ? Settings->GetCurrencyItemDefinition() : nullptr;
}

void AMythicVendor::BeginPlay() {
    if (VendorFactionTag.IsValid()) {
        const UGameInstance *GI = GetGameInstance();
        const UMythicLivingWorldSubsystem *LW = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
        const UMythicFactionDatabase *DB = LW ? LW->GetFactionDatabase() : nullptr;
        if (DB) {
            const FMythicFactionId Resolved = DB->FindFactionId(VendorFactionTag);
            if (Resolved.IsValid()) {
                VendorFaction = Resolved;
            }
            else {
                UE_LOG(Myth, Warning,
                       TEXT("%s: VendorFactionTag '%s' is not a registered faction — reputation pricing stays inert."),
                       *GetName(), *VendorFactionTag.ToString());
            }
        }
    }

    Super::BeginPlay();
}

float AMythicVendor::ComputeReputationAdjustedMultiplier(float BaseMultiplier, EMythicStandingTier Tier, float HostileMult, float NeutralMult, float FriendlyMult) {
    float TierMult;
    switch (Tier) {
    case EMythicStandingTier::Hostile:
        TierMult = HostileMult;
        break;
    case EMythicStandingTier::Friendly:
        TierMult = FriendlyMult;
        break;
    case EMythicStandingTier::Neutral:
    default:
        TierMult = NeutralMult;
        break;
    }
    return BaseMultiplier * FMath::Max(0.0f, TierMult);
}

float AMythicVendor::EnforceBuyAboveSellRate(float EffBuyMultiplier, float EffSellRate) {
    return FMath::Max(EffBuyMultiplier, FMath::Clamp(EffSellRate, 0.0f, 1.0f));
}

bool AMythicVendor::TradeConsumesWholeStack(int32 PlanQuantity, int32 AvailableStacks) {
    return PlanQuantity > 0 && AvailableStacks > 0 && PlanQuantity >= AvailableStacks;
}

bool AMythicVendor::IsValidBuybackIndex(int32 Index, int32 Num) {
    return Index >= 0 && Index < Num;
}

float AMythicVendor::ComputeTradingXpReward(float XpPerCoin, int32 Coins, int32 TraderLevel, int32 NoGainAtOrAboveLevel) {
    if (XpPerCoin <= 0.0f || Coins <= 0) {
        return 0.0f;
    }
    if (NoGainAtOrAboveLevel > 0 && TraderLevel >= NoGainAtOrAboveLevel) {
        return 0.0f;
    }
    return XpPerCoin * (float)Coins;
}

int32 AMythicVendor::ResolveTraderLevel(AMythicPlayerController *Trader) const {
    if (!TradingProficiency || !Trader) {
        return 0;
    }
    const UProficiencyComponent *Prof = Trader->GetProficiencyComponent();
    if (!Prof) {
        return 0;
    }
    for (const FProficiency &P : Prof->Proficiencies) {
        if (P.Definition == TradingProficiency) {
            if (const UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Trader)) {
                const float CurrentXP = ASC->GetNumericAttribute(TradingProficiency->GetProgressAttribute());
                return UProficiencyDefinition::CalcLevelAtXP(CurrentXP, TradingProficiency);
            }
            break;
        }
    }
    return 0;
}

void AMythicVendor::GrantTradingXp(AMythicPlayerController *Trader, int32 Coins) {
    if (!TradingProficiency || !Trader || Coins <= 0) {
        return;
    }
    UProficiencyComponent *Prof = Trader->GetProficiencyComponent();
    if (!Prof) {
        return;
    }
    const int32 TraderLevel = ResolveTraderLevel(Trader);
    const float Xp = ComputeTradingXpReward(TradingXpPerCoin, Coins, TraderLevel, TradingXpNoGainAtOrAboveLevel);
    if (Xp > 0.0f) {
        Prof->GrantProficiencyXP(TradingProficiency, Xp);
    }
}


float AMythicVendor::ComputeHagglingBuyMultiplier(int32 TraderLevel, float DiscountPerLevel, float MinMultiplier) {
    if (TraderLevel <= 0 || DiscountPerLevel <= 0.0f) {
        return 1.0f;
    }
    const float Floor = FMath::Clamp(MinMultiplier, 0.0f, 1.0f);
    return FMath::Clamp(1.0f - static_cast<float>(TraderLevel) * DiscountPerLevel, Floor, 1.0f);
}

float AMythicVendor::ComputeHagglingSellMultiplier(int32 TraderLevel, float BonusPerLevel, float MaxMultiplier) {
    if (TraderLevel <= 0 || BonusPerLevel <= 0.0f) {
        return 1.0f;
    }
    const float Ceiling = FMath::Max(MaxMultiplier, 1.0f);
    return FMath::Clamp(1.0f + static_cast<float>(TraderLevel) * BonusPerLevel, 1.0f, Ceiling);
}


bool AMythicVendor::IsContrabandItem(const UItemDefinition *Def) const {
    if (!Def || BannedItemTags.IsEmpty()) {
        return false;
    }
    return Def->ItemType.MatchesAny(BannedItemTags);
}

int32 AMythicVendor::ApplyContrabandPremium(int32 Payout, float Premium) {
    if (Payout <= 0) {
        return FMath::Max(Payout, 0);
    }
    return FMath::Max(FMath::RoundToInt(static_cast<float>(Payout) * FMath::Max(Premium, 1.0f)), 0);
}

void AMythicVendor::RecordContrabandSale(AMythicPlayerController *Seller, int32 Units) {
    if (!Seller || Units <= 0) {
        return;
    }
    AMythicPlayerState *PS = Seller->GetPlayerState<AMythicPlayerState>();

    if (PS) {
        if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
            Ledger->RecordStat(STAT_TRADE_CONTRABAND_SOLD, Units);
        }
    }

    UWorld *World = GetWorld();
    UMythicActionEventSubsystem *ActionSub = World ? World->GetSubsystem<UMythicActionEventSubsystem>() : nullptr;
    if (!ActionSub) {
        return;
    }
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    FMythicActionEvent Sale;
    Sale.Perpetrator = Seller->GetPawn();
    Sale.ActionTag = TAG_TRADING_ACTION_CONTRABAND;
    Sale.CategoryFlags = EMythicEventCategory::Crime | EMythicEventCategory::Trade;
    Sale.Significance = Dev ? Dev->Trading.ContrabandCrimeSignificance : 0.4f;
    Sale.MoralVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Authority)] = -0.5f;
    Sale.MoralVector.AxisValues[static_cast<int32>(EMythicMoralAxis::Loyalty)] = -0.2f;
    if (PS) {
        Sale.PerpPlayerKey = PS->GetCanonicalPlayerKey();
    }
    ActionSub->SubmitAction(Sale);
}

int32 AMythicVendor::BuybackRingWriteSlot(int32 TotalPushes, int32 Capacity) {
    if (Capacity <= 0) {
        return 0;
    }
    return (TotalPushes % Capacity + Capacity) % Capacity;
}

bool AMythicVendor::HasReputationPricing() const {
    if (!VendorFaction.IsValid()) {
        return false;
    }
    const FVendorReputationPricing &P = ReputationPricing;
    return P.HostileBuyMultiplier != 1.0f || P.FriendlyBuyMultiplier != 1.0f
        || P.HostileSellMultiplier != 1.0f || P.FriendlySellMultiplier != 1.0f;
}

EMythicStandingTier AMythicVendor::ResolvePatronTier(AMythicPlayerController *Patron) const {
    if (!Patron || !VendorFaction.IsValid()) {
        return EMythicStandingTier::Neutral;
    }
    const AMythicPlayerState *PS = Patron->GetPlayerState<AMythicPlayerState>();
    const UMythicFactionStandingComponent *Standing = PS ? PS->GetFactionStanding() : nullptr;
    if (!Standing) {
        return EMythicStandingTier::Neutral;
    }
    return Standing->TierForStanding(Standing->GetStanding(VendorFaction));
}

float AMythicVendor::ResolveScarcityMultiplier(const UItemDefinition *Def) const {
    if (!Def || EconomyParams.Elasticity <= 0.0f || ItemAxisMap.Num() == 0 || !VendorFaction.IsValid()) {
        return 1.0f;
    }
    FGameplayTagContainer TypeTags;
    TypeTags.AddTag(Def->ItemType);
    const EMythicEconomyAxis Axis = FMythicEconomyPricing::AxisForItem(TypeTags, ItemAxisMap);
    if (Axis == EMythicEconomyAxis::None) {
        return 1.0f;
    }

    const UGameInstance *GI = GetGameInstance();
    const UMythicLivingWorldSubsystem *LWS = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    const UMythicFactionDatabase *FDB = LWS ? LWS->GetFactionDatabase() : nullptr;
    if (!FDB) {
        return 1.0f;
    }
    FMythicFactionData FData;
    if (!FDB->GetFaction(VendorFaction, FData) || !FData.bHasEconomy) {
        return 1.0f;
    }
    const EMythicResourceType Res = ResourceTypeForAxis(Axis);
    return FMythicEconomyPricing::ComputeScarcityMultiplier(
        FData.Reserves.GetResource(Res), FData.Demand.GetResource(Res), FData.Prices.GetResource(Res), Axis, EconomyParams);
}

float AMythicVendor::GetDecayedSellPressureUnits(const FGameplayTag &TypeTag) const {
    if (SellPressurePerUnit <= 0.0f) {
        return 0.0f;
    }
    const FSellPressureState *State = SellPressureByType.Find(TypeTag);
    if (!State || State->Units <= 0.0f) {
        return 0.0f;
    }
    const UWorld *World = GetWorld();
    const double Now = World ? World->GetTimeSeconds() : State->LastUpdateSeconds;
    const float HalfLife = FMath::Max(SellPressureHalfLifeSeconds, 1.0f);
    const double Elapsed = FMath::Max(Now - State->LastUpdateSeconds, 0.0);
    const float Decayed = State->Units * FMath::Pow(0.5f, static_cast<float>(Elapsed) / HalfLife);
    return FMath::Max(Decayed, 0.0f);
}

void AMythicVendor::RecordSellPressure(const FGameplayTag &TypeTag, int32 Units) {
    if (SellPressurePerUnit <= 0.0f || Units <= 0 || !TypeTag.IsValid()) {
        return;
    }
    const UWorld *World = GetWorld();
    const double Now = World ? World->GetTimeSeconds() : 0.0;
    FSellPressureState &State = SellPressureByType.FindOrAdd(TypeTag);
    if (State.Units > 0.0f) {
        const float HalfLife = FMath::Max(SellPressureHalfLifeSeconds, 1.0f);
        const double Elapsed = FMath::Max(Now - State.LastUpdateSeconds, 0.0);
        State.Units *= FMath::Pow(0.5f, static_cast<float>(Elapsed) / HalfLife);
    }
    State.Units += static_cast<float>(Units);
    State.LastUpdateSeconds = Now;
}

float AMythicVendor::ResolveEffectiveBuyMultiplier(AMythicPlayerController *Buyer, const UItemDefinition *Def) const {
    const EMythicStandingTier Tier = ResolvePatronTier(Buyer);
    const float Scarcity = ResolveScarcityMultiplier(Def);
    const int32 TraderLevel = (HagglingBuyDiscountPerLevel > 0.0f || HagglingSellBonusPerLevel > 0.0f) ? ResolveTraderLevel(Buyer) : 0;
    const float HaggleBuy = ComputeHagglingBuyMultiplier(TraderLevel, HagglingBuyDiscountPerLevel);
    const float HaggleSell = ComputeHagglingSellMultiplier(TraderLevel, HagglingSellBonusPerLevel);
    const float EffBuy = ComputeReputationAdjustedMultiplier(BuyPriceMultiplier, Tier,
        ReputationPricing.HostileBuyMultiplier, ReputationPricing.NeutralBuyMultiplier, ReputationPricing.FriendlyBuyMultiplier) * Scarcity * HaggleBuy;
    const float EffSell = ComputeReputationAdjustedMultiplier(SellRate, Tier,
        ReputationPricing.HostileSellMultiplier, ReputationPricing.NeutralSellMultiplier, ReputationPricing.FriendlySellMultiplier) * Scarcity * HaggleSell;
    return EnforceBuyAboveSellRate(EffBuy, EffSell);
}

float AMythicVendor::ResolveEffectiveSellRate(AMythicPlayerController *Seller, const UItemDefinition *Def) const {
    float EffSell = ComputeReputationAdjustedMultiplier(SellRate, ResolvePatronTier(Seller),
        ReputationPricing.HostileSellMultiplier, ReputationPricing.NeutralSellMultiplier, ReputationPricing.FriendlySellMultiplier);
    EffSell *= ResolveScarcityMultiplier(Def);
    if (HagglingSellBonusPerLevel > 0.0f) {
        EffSell *= ComputeHagglingSellMultiplier(ResolveTraderLevel(Seller), HagglingSellBonusPerLevel);
    }
    if (Def) {
        const float DumpedUnits = GetDecayedSellPressureUnits(Def->ItemType);
        EffSell = FMythicEconomyPricing::ApplyLocalSellPressure(EffSell, DumpedUnits, SellPressurePerUnit, SellPressureFloorMult);
    }
    return EffSell;
}

int32 AMythicVendor::GetBuyPriceForSlot(int32 StockSlotIndex, int32 Quantity, AMythicPlayerController *Buyer) const {
    if (UMythicInventoryComponent *Stock = GetContainerInventory()) {
        if (UMythicItemInstance *Item = Stock->GetItem(StockSlotIndex)) {
            if (const UItemDefinition *Def = Item->GetItemDefinition()) {
                return MythicCurrency::ComputeBuyPrice(Def->Value, Quantity, ResolveEffectiveBuyMultiplier(Buyer, Def));
            }
        }
    }
    return 0;
}

int32 AMythicVendor::GetSalePriceForItem(const UMythicItemInstance *Item, int32 Quantity, AMythicPlayerController *Seller) const {
    if (!ResolveCurrencyItemDefinition() || !Item) {
        return 0;
    }
    if (const UItemDefinition *Def = Item->GetItemDefinition()) {
        return MythicCurrency::ComputeSalePrice(Def->Value, Quantity, ResolveEffectiveSellRate(Seller, Def));
    }
    return 0;
}

FMythicTradePlan AMythicVendor::Server_ExecuteBuy(AMythicPlayerController *Buyer, int32 StockSlotIndex, int32 Quantity) {
    FMythicTradePlan Reject;
    UMythicInventoryComponent *Stock = GetContainerInventory();
    if (!HasAuthority() || !Buyer || !Stock || Quantity <= 0) {
        return Reject;
    }

    UMythicItemInstance *StockItem = Stock->GetItem(StockSlotIndex);
    if (!StockItem) {
        Reject.Result = EMythicTradeResult::OutOfStock;
        return Reject;
    }
    UItemDefinition *Def = StockItem->GetItemDefinition();
    if (!Def) {
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

    const float EffBuyMult = ResolveEffectiveBuyMultiplier(Buyer, Def);

    const FMythicTradePlan Plan = MythicTrade::PlanBuy(Quantity, Def->Value, EffBuyMult, StockItem->GetStacks(),
                                                       SumPlayerCurrency(Buyer), Target != nullptr);
    if (Plan.Quantity <= 0) {
        return Plan;
    }

    if (TradeConsumesWholeStack(Plan.Quantity, StockItem->GetStacks())) {
        UMythicItemInstance *Carried = Stock->ReleaseFromSlot(StockSlotIndex);
        if (!Carried) {
            return Reject;
        }
        if (!Buyer->TryChargeCurrency(Plan.TotalPrice)) {
            Stock->SetItemInSlot(StockSlotIndex, Carried);
            Reject.Result = EMythicTradeResult::InsufficientFunds;
            return Reject;
        }
        RemoveFromBuyback(Carried);
        Target->AddItem(Carried, Buyer);
        GrantTradingXp(Buyer, Plan.TotalPrice);
        return Plan;
    }

    const int32 GoodsLevel = StockItem->GetItemLevel();
    const int32 StackCap = FMath::Max(1, Def->StackSizeMax);
    TArray<UMythicItemInstance *, TInlineAllocator<8>> Goods;
    auto DestroyGoods = [&Goods]() {
        for (UMythicItemInstance *Made : Goods) {
            if (Made) {
                Made->Destroy();
            }
        }
    };
    for (int32 ToMake = Plan.Quantity; ToMake > 0;) {
        const int32 Chunk = FMath::Min(ToMake, StackCap);
        UMythicItemInstance *G = Loot->Create(Def, Chunk, Buyer, GoodsLevel);
        if (!G) {
            DestroyGoods();
            return Reject;
        }
        Goods.Add(G);
        ToMake -= Chunk;
    }

    if (!Buyer->TryChargeCurrency(Plan.TotalPrice)) {
        DestroyGoods();
        Reject.Result = EMythicTradeResult::InsufficientFunds;
        return Reject;
    }

    Stock->ServerRemoveItem(StockItem, Plan.Quantity);
    for (UMythicItemInstance *G : Goods) {
        Target->AddItem(G, Buyer);
    }
    GrantTradingXp(Buyer, Plan.TotalPrice);
    return Plan;
}

FMythicTradePlan AMythicVendor::Server_ExecuteSell(AMythicPlayerController *Seller, UMythicInventoryComponent *PlayerInventory,
                                                   int32 PlayerSlotIndex, int32 Quantity) {
    FMythicTradePlan Reject;
    if (!HasAuthority() || !Seller || !PlayerInventory || Quantity <= 0) {
        return Reject;
    }

    UMythicItemInstance *Item = PlayerInventory->GetItem(PlayerSlotIndex);
    if (!Item) {
        Reject.Result = EMythicTradeResult::OutOfStock;
        return Reject;
    }
    UItemDefinition *Def = Item->GetItemDefinition();
    if (!Def) {
        return Reject;
    }

    const bool bContraband = IsContrabandItem(Def);
    if (bContraband && !bBlackMarket) {
        Reject.Result = EMythicTradeResult::NotSellable;
        return Reject;
    }

    const float EffSellRate = ResolveEffectiveSellRate(Seller, Def);

    UItemDefinition *CurrencyDef = ResolveCurrencyItemDefinition();
    const bool bIsCurrency = Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY);
    FMythicTradePlan Plan = MythicTrade::PlanSell(Quantity, Item->GetStacks(), Def->Value, EffSellRate,
                                                  CurrencyDef != nullptr,
                                                  PlayerInventory->CanPlayerTakeFromSlot(PlayerSlotIndex), bIsCurrency);
    if (Plan.Quantity <= 0) {
        return Plan;
    }

    if (bContraband) {
        Plan.TotalPrice = ApplyContrabandPremium(Plan.TotalPrice, ContrabandPremium);
    }

    UGameInstance *GI = GetGameInstance();
    UMythicLootManagerSubsystem *Loot = GI ? GI->GetSubsystem<UMythicLootManagerSubsystem>() : nullptr;
    if (!Loot) {
        return Reject;
    }

    UMythicInventoryComponent *Stock = GetContainerInventory();
    const bool bWholeStack = TradeConsumesWholeStack(Plan.Quantity, Item->GetStacks());
    const bool bAbsorbToStock = bAbsorbSoldItems && Stock && Stock->CanAcceptItemType(Def->ItemType);

    if (bWholeStack && bAbsorbToStock) {
        if (UMythicItemInstance *Sold = PlayerInventory->ReleaseFromSlot(PlayerSlotIndex)) {
            const int32 Added = Stock->AddToAnySlot(Sold);
            if (Added > 0 && IsValid(Sold) && Sold->GetInventoryComponent() == Stock) {
                RecordBuyback(Sold, Plan.TotalPrice);
            }
            else if (Added <= 0 && IsValid(Sold)) {
                Sold->Destroy();
            }
        }
    }
    else {
        PlayerInventory->ServerRemoveItem(Item, Plan.Quantity);
        if (!bWholeStack && bAbsorbToStock) {
            if (UMythicItemInstance *StockGoods = Loot->Create(Def, Plan.Quantity, nullptr, 0)) {
                Stock->AddToAnySlot(StockGoods);
            }
        }
    }

    Seller->GrantCurrencyOfDefinition(CurrencyDef, Plan.TotalPrice);
    GrantTradingXp(Seller, Plan.TotalPrice);
    RecordSellPressure(Def->ItemType, Plan.Quantity);
    if (bContraband) {
        RecordContrabandSale(Seller, Plan.Quantity);
    }
    return Plan;
}


FMythicTradePlan AMythicVendor::Server_ExecuteDelivery(AMythicPlayerController *Deliverer, UMythicInventoryComponent *PlayerInventory,
                                                       int32 PlayerSlotIndex, int32 Quantity, const FGameplayTag &ContractItemTag,
                                                       EMythicResourceType ReserveAxis) {
    FMythicTradePlan Reject;
    if (!HasAuthority() || !bAcceptsDeliveries || !Deliverer || !PlayerInventory || Quantity <= 0 || !ContractItemTag.IsValid()) {
        return Reject;
    }

    UMythicItemInstance *Item = PlayerInventory->GetItem(PlayerSlotIndex);
    if (!Item) {
        Reject.Result = EMythicTradeResult::OutOfStock;
        return Reject;
    }
    UItemDefinition *Def = Item->GetItemDefinition();
    if (!Def) {
        return Reject;
    }
    if (!Def->ItemType.MatchesTag(ContractItemTag) || Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY) ||
        !PlayerInventory->CanPlayerTakeFromSlot(PlayerSlotIndex)) {
        Reject.Result = EMythicTradeResult::NotSellable;
        return Reject;
    }
    UItemDefinition *CurrencyDef = ResolveCurrencyItemDefinition();
    if (!CurrencyDef) {
        Reject.Result = EMythicTradeResult::VendorCannotPay;
        return Reject;
    }
    UGameInstance *GI = GetGameInstance();
    if (!GI) {
        return Reject;
    }

    const int32 Units = FMath::Min(Quantity, Item->GetStacks());
    if (Units <= 0) {
        Reject.Result = EMythicTradeResult::OutOfStock;
        return Reject;
    }

    const float Scarcity = ResolveScarcityMultiplier(Def);
    const int32 RawPayout = MythicTradeContracts::ComputeDeliveryPayout(Def->Value, Units, Scarcity);

    const int32 EffectiveBuyCost = MythicCurrency::ComputeBuyPrice(Def->Value, Units, ResolveEffectiveBuyMultiplier(Deliverer, Def));
    const int32 Payout = FMath::Min(RawPayout, EffectiveBuyCost);

    PlayerInventory->ServerRemoveItem(Item, Units);
    Deliverer->GrantCurrencyOfDefinition(CurrencyDef, Payout);
    GrantTradingXp(Deliverer, Payout);

    if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
        const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
        const float PerUnit = Dev ? Dev->Trading.DeliveryUnitsToReservePerUnit : 1.0f;
        LWS->EnqueuePlayerResourceDelta(VendorFaction, ReserveAxis, PerUnit * static_cast<float>(Units));
    }

    if (UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Deliverer)) {
        FGameplayEventData Payload;
        Payload.EventTag = GAS_EVENT_DELIVER;
        Payload.Instigator = Deliverer->GetPawn();
        Payload.Target = ASC->GetAvatarActor();
        Payload.TargetTags.AddTag(Def->ItemType);
        Payload.EventMagnitude = static_cast<float>(Units);
        ASC->HandleGameplayEvent(GAS_EVENT_DELIVER, &Payload);
    }

    FMythicTradePlan Plan;
    Plan.Result = EMythicTradeResult::Success;
    Plan.Quantity = Units;
    Plan.TotalPrice = Payout;
    return Plan;
}

int32 AMythicVendor::GetRepairCostForItem(UMythicItemInstance *Item) const {
    if (!bCanRepair || !Item) {
        return 0;
    }
    const UItemDefinition *Def = Item->GetItemDefinition();
    if (!Def) {
        return 0;
    }
    if (const UDurabilityFragment *Dura = Item->GetFragment<UDurabilityFragment>()) {
        return MythicCurrency::ComputeRepairCost(Dura->GetCurrentDurability(), Dura->GetMaxDurability(), Def->Value, RepairCostFraction);
    }
    return 0;
}

FMythicTradePlan AMythicVendor::Server_ExecuteRepair(AMythicPlayerController *Payer, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex) {
    FMythicTradePlan Reject;
    if (!HasAuthority() || !Payer || !PlayerInventory || !bCanRepair) {
        return Reject;
    }

    UMythicItemInstance *Item = PlayerInventory->GetItem(PlayerSlotIndex);
    if (!Item) {
        Reject.Result = EMythicTradeResult::NothingToRepair;
        return Reject;
    }
    const UItemDefinition *Def = Item->GetItemDefinition();
    const UDurabilityFragment *Dura = Item->GetFragment<UDurabilityFragment>();
    if (!Def || !Dura) {
        Reject.Result = EMythicTradeResult::NothingToRepair;
        return Reject;
    }
    if (Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
        Reject.Result = EMythicTradeResult::NothingToRepair;
        return Reject;
    }

    const FMythicTradePlan Plan = MythicTrade::PlanRepair(Dura->GetCurrentDurability(), Dura->GetMaxDurability(),
                                                          Def->Value, RepairCostFraction, SumPlayerCurrency(Payer));
    if (Plan.Result != EMythicTradeResult::Success) {
        return Plan;
    }

    if (!Payer->TryChargeCurrency(Plan.TotalPrice)) {
        Reject.Result = EMythicTradeResult::InsufficientFunds;
        return Reject;
    }
    const_cast<UDurabilityFragment *>(Dura)->ServerRepair(Plan.Quantity);
    return Plan;
}

FMythicTradePlan AMythicVendor::Server_ExecuteRepairAll(AMythicPlayerController *Payer, UMythicInventoryComponent *PlayerInventory) {
    FMythicTradePlan Reject;
    if (!HasAuthority() || !Payer || !PlayerInventory || !bCanRepair) {
        return Reject;
    }

    struct FRepairCandidate {
        const UDurabilityFragment *Dura = nullptr;
        int32 RestoreAmount = 0;
        int32 Cost = 0;
    };
    TArray<FRepairCandidate> Candidates;
    const int32 NumSlots = PlayerInventory->GetNumSlots();
    for (int32 i = 0; i < NumSlots; ++i) {
        UMythicItemInstance *Item = PlayerInventory->GetItem(i);
        if (!Item) {
            continue;
        }
        const UItemDefinition *Def = Item->GetItemDefinition();
        const UDurabilityFragment *Dura = Item->GetFragment<UDurabilityFragment>();
        if (!Def || !Dura) {
            continue;
        }
        if (Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
            continue;
        }
        const int32 Cur = Dura->GetCurrentDurability();
        const int32 Max = Dura->GetMaxDurability();
        if (Max <= 0 || Cur >= Max) {
            continue;
        }
        const int32 Cost = MythicCurrency::ComputeRepairCost(Cur, Max, Def->Value, RepairCostFraction);
        if (Cost <= 0) {
            continue;
        }
        Candidates.Add({Dura, Max - Cur, Cost});
    }
    if (Candidates.Num() == 0) {
        Reject.Result = EMythicTradeResult::NothingToRepair;
        return Reject;
    }

    Candidates.Sort([](const FRepairCandidate &A, const FRepairCandidate &B) { return A.Cost < B.Cost; });
    TArray<int32> CostsAscending;
    CostsAscending.Reserve(Candidates.Num());
    for (const FRepairCandidate &C : Candidates) {
        CostsAscending.Add(C.Cost);
    }
    const FMythicTradePlan Plan = MythicTrade::ComputeRepairAllPlan(CostsAscending, SumPlayerCurrency(Payer));
    if (Plan.Result != EMythicTradeResult::Success) {
        return Plan;
    }

    if (!Payer->TryChargeCurrency(Plan.TotalPrice)) {
        Reject.Result = EMythicTradeResult::InsufficientFunds;
        return Reject;
    }
    for (int32 i = 0; i < Plan.Quantity && i < Candidates.Num(); ++i) {
        const_cast<UDurabilityFragment *>(Candidates[i].Dura)->ServerRepair(Candidates[i].RestoreAmount);
    }
    return Plan;
}


int32 AMythicVendor::ComputeBuybackPrice(int32 PricePaid) const {
    if (PricePaid <= 0) {
        return 0;
    }
    return FMath::Max(0, FMath::CeilToInt(static_cast<float>(PricePaid) * FMath::Max(1.0f, BuybackPriceMultiplier)));
}

bool AMythicVendor::IsBuybackEntryLive(const FVendorBuybackEntry &Entry) const {
    UMythicItemInstance *Inst = Entry.Instance.Get();
    return IsValid(Inst) && Inst->GetInventoryComponent() == GetContainerInventory();
}

void AMythicVendor::RecordBuyback(UMythicItemInstance *Instance, int32 PricePaid) {
    if (!IsValid(Instance)) {
        return;
    }
    const int32 Capacity = FMath::Max(1, BuybackCapacity);
    const int32 Slot = BuybackRingWriteSlot(BuybackPushCount, Capacity);
    if (BuybackEntries.Num() <= Slot) {
        BuybackEntries.SetNum(Slot + 1);
    }
    FVendorBuybackEntry NewEntry;
    NewEntry.Instance = Instance;
    NewEntry.PricePaid = PricePaid;
    BuybackEntries[Slot] = NewEntry;
    ++BuybackPushCount;
}

void AMythicVendor::RemoveFromBuyback(const UMythicItemInstance *Instance) {
    if (!Instance) {
        return;
    }
    for (FVendorBuybackEntry &Entry : BuybackEntries) {
        if (Entry.Instance.Get() == Instance) {
            Entry.Instance.Reset();
            Entry.PricePaid = 0;
        }
    }
}

UMythicItemInstance *AMythicVendor::GetBuybackItem(int32 Index) const {
    if (!IsValidBuybackIndex(Index, BuybackEntries.Num()) || !IsBuybackEntryLive(BuybackEntries[Index])) {
        return nullptr;
    }
    return BuybackEntries[Index].Instance.Get();
}

int32 AMythicVendor::GetBuybackPrice(int32 Index) const {
    if (!IsValidBuybackIndex(Index, BuybackEntries.Num()) || !IsBuybackEntryLive(BuybackEntries[Index])) {
        return 0;
    }
    return ComputeBuybackPrice(BuybackEntries[Index].PricePaid);
}

FMythicTradePlan AMythicVendor::Server_ExecuteBuyback(AMythicPlayerController *Buyer, int32 BuybackIndex) {
    FMythicTradePlan Reject;
    UMythicInventoryComponent *Stock = GetContainerInventory();
    if (!HasAuthority() || !Buyer || !Stock) {
        return Reject;
    }
    if (!IsValidBuybackIndex(BuybackIndex, BuybackEntries.Num())) {
        return Reject;
    }

    FVendorBuybackEntry &Entry = BuybackEntries[BuybackIndex];
    if (!IsBuybackEntryLive(Entry)) {
        Entry.Instance.Reset();
        Reject.Result = EMythicTradeResult::OutOfStock;
        return Reject;
    }
    UMythicItemInstance *Inst = Entry.Instance.Get();
    UItemDefinition *Def = Inst->GetItemDefinition();
    if (!Def) {
        return Reject;
    }

    const int32 Price = ComputeBuybackPrice(Entry.PricePaid);
    if (!MythicCurrency::CanAfford(SumPlayerCurrency(Buyer), Price)) {
        Reject.Result = EMythicTradeResult::InsufficientFunds;
        return Reject;
    }

    UMythicInventoryComponent *Target = nullptr;
    for (UMythicInventoryComponent *Inv : Buyer->GetAllInventoryComponents()) {
        if (Inv && Inv->CanAcceptItemType(Def->ItemType)) {
            Target = Inv;
            break;
        }
    }
    if (!Target) {
        Reject.Result = EMythicTradeResult::NoRoom;
        return Reject;
    }

    const int32 StockSlot = Inst->GetSlot();
    UMythicItemInstance *Carried = Stock->ReleaseFromSlot(StockSlot);
    if (!Carried) {
        Reject.Result = EMythicTradeResult::OutOfStock;
        return Reject;
    }
    if (!Buyer->TryChargeCurrency(Price)) {
        Stock->SetItemInSlot(StockSlot, Carried);
        Reject.Result = EMythicTradeResult::InsufficientFunds;
        return Reject;
    }
    Entry.Instance.Reset();
    Entry.PricePaid = 0;

    const int32 Qty = Carried->GetStacks();
    Target->AddItem(Carried, Buyer);

    FMythicTradePlan Plan;
    Plan.Result = EMythicTradeResult::Success;
    Plan.Quantity = Qty;
    Plan.TotalPrice = Price;
    return Plan;
}
