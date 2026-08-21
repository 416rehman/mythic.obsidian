
#include "World/Trading/MythicTradeLedgerSubsystem.h"

#include "World/Trading/MythicTags_Trading.h"
#include "World/Trading/MythicCargoRisk.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/MythicPlayerEconomyDelta.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/POI/MythicPOIDiscoverySubsystem.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicFactionStandingComponent.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Settings/MythicDeveloperSettings.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Mythic.h"

bool UMythicTradeLedgerSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) {
        return false;
    }
    if (World->GetNetMode() == NM_Client) {
        return false;
    }
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    return Dev && Dev->bEnableTrading;
}

void UMythicTradeLedgerSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);
    if (!IsAuthority()) {
        return;
    }
    if (UGameInstance *GI = InWorld.GetGameInstance()) {
        if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            LivingWorld = LWS;
            CommitHandle = LWS->OnWorldSimCommitted.AddUObject(this, &UMythicTradeLedgerSubsystem::HandleWorldSimCommitted);
        }
    }
    UE_LOG(Myth, Log, TEXT("TradeLedger: subsystem live (trading enabled)"));
}

void UMythicTradeLedgerSubsystem::Deinitialize() {
    if (UMythicLivingWorldSubsystem *LWS = LivingWorld.Get()) {
        if (CommitHandle.IsValid()) {
            LWS->OnWorldSimCommitted.Remove(CommitHandle);
        }
    }
    CommitHandle.Reset();
    LivingWorld = nullptr;
    LiveLedger.Reset();
    RumorSnapshot.Reset();
    OpenOffers.Reset();
    Super::Deinitialize();
}

bool UMythicTradeLedgerSubsystem::IsAuthority() const {
    const UWorld *World = GetWorld();
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}


void UMythicTradeLedgerSubsystem::HandleWorldSimCommitted() {
    if (!IsAuthority()) {
        return;
    }
    UMythicLivingWorldSubsystem *LWS = LivingWorld.Get();
    UWorld *World = GetWorld();
    if (!LWS || !LWS->IsSystemActive() || !World) {
        return;
    }
    const double NowSeconds = World->GetTimeSeconds();

    SampleLedger(NowSeconds);
    EmitDeficitBeats();
    EmitRumorBeat(NowSeconds);

    for (int32 i = OpenOffers.Num() - 1; i >= 0; --i) {
        if (NowSeconds >= OpenOffers[i].ExpireTimeSeconds) {
            OpenOffers.RemoveAt(i);
        }
    }
}

void UMythicTradeLedgerSubsystem::SampleLedger(double NowSeconds) {
    UMythicLivingWorldSubsystem *LWS = LivingWorld.Get();
    UMythicFactionDatabase *FactionDB = LWS ? LWS->GetFactionDatabase() : nullptr;
    if (!FactionDB) {
        return;
    }

    TArray<int32> SettlementIds;
    LWS->CopyAllSettlementIds(SettlementIds);

    LiveLedger.Reset();
    for (const int32 Id : SettlementIds) {
        FMythicSettlementData Data;
        if (!LWS->CopySettlementById(Id, Data) || !Data.GoverningFaction.IsValid()) {
            continue;
        }
        FMythicFactionData Faction;
        if (!FactionDB->GetFaction(Data.GoverningFaction, Faction) || !Faction.bHasEconomy) {
            continue;
        }
        FMythicTradeLedgerEntry Entry;
        Entry.SettlementId = Id;
        Entry.GoverningFactionIndex = Data.GoverningFaction.Index;
        Entry.Prices = Faction.Prices;
        Entry.Reserves = Faction.Reserves;
        Entry.SampledAtSeconds = NowSeconds;
        LiveLedger.Add(Id, Entry);
    }

    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    const float RefreshSeconds = Dev ? FMath::Max(Dev->Trading.RumorSnapshotRefreshSeconds, 1.0f) : 300.0f;
    if (NowSeconds - LastRumorSnapshotSeconds >= RefreshSeconds) {
        RumorSnapshot = LiveLedger;
        LastRumorSnapshotSeconds = NowSeconds;
    }
}

void UMythicTradeLedgerSubsystem::EmitDeficitBeats() {
    UMythicLivingWorldSubsystem *LWS = LivingWorld.Get();
    UMythicFactionDatabase *FactionDB = LWS ? LWS->GetFactionDatabase() : nullptr;
    if (!FactionDB) {
        return;
    }
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    const float DeficitThreshold = Dev ? Dev->Trading.DeficitReserveThreshold : -25.0f;
    const float RearmThreshold = Dev ? Dev->Trading.DeficitRearmThreshold : 0.0f;

    TMap<uint8, int32> FactionToSettlement;
    for (const TPair<int32, FMythicTradeLedgerEntry> &Pair : LiveLedger) {
        if (!FactionToSettlement.Contains(Pair.Value.GoverningFactionIndex)) {
            FactionToSettlement.Add(Pair.Value.GoverningFactionIndex, Pair.Key);
        }
    }

    const EMythicResourceType Axes[] = {EMythicResourceType::Materials, EMythicResourceType::Arms};

    for (const TPair<uint8, int32> &FS : FactionToSettlement) {
        FMythicFactionId FactionId;
        FactionId.Index = FS.Key;
        FMythicFactionData Faction;
        if (!FactionDB->GetFaction(FactionId, Faction)) {
            continue;
        }
        for (const EMythicResourceType Axis : Axes) {
            const uint32 Key = MythicPlayerEconomyDelta::MakeKey(FactionId, Axis);
            bool &bLatched = DeficitLatches.FindOrAdd(Key);
            bool bNewLatch = bLatched;
            const bool bFire = MythicTradeContracts::ShouldFireDeficitBeat(Faction.Reserves.GetResource(Axis),
                                                                           DeficitThreshold, RearmThreshold, bLatched, bNewLatch);
            bLatched = bNewLatch;
            if (!bFire) {
                continue;
            }
            FVector Anchor = FVector::ZeroVector;
            ResolveSettlementAnchor(FS.Value, Anchor);
            const FGameplayTag Tag = (Axis == EMythicResourceType::Arms) ? TAG_TRADING_EVENT_DEFICIT_ARMS
                                                                         : TAG_TRADING_EVENT_DEFICIT_MATERIALS;
            SubmitTradeBeat(Tag, FactionId, Anchor, 0.5f);
            UE_LOG(Myth, Log, TEXT("TradeLedger: deficit beat %s for faction %d (reserve %.1f <= %.1f)"),
                   *Tag.ToString(), static_cast<int32>(FactionId.Index), Faction.Reserves.GetResource(Axis), DeficitThreshold);
        }
    }
}

void UMythicTradeLedgerSubsystem::EmitRumorBeat(double NowSeconds) {
    if (LiveLedger.Num() < 2) {
        return;
    }
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    const float MinDifferential = Dev ? Dev->Trading.RumorMinDifferential : 0.4f;
    const float Cooldown = Dev ? Dev->Trading.RumorCooldownSeconds : 600.0f;

    TArray<FMythicTradeLedgerEntry> Entries;
    LiveLedger.GenerateValueArray(Entries);

    for (int32 AxisOrd = 0; AxisOrd < ResourceTypeCount; ++AxisOrd) {
        if (NowSeconds - LastRumorBeatSeconds[AxisOrd] < Cooldown) {
            continue;
        }
        const EMythicResourceType Axis = static_cast<EMythicResourceType>(AxisOrd);
        int32 FromIdx = INDEX_NONE, ToIdx = INDEX_NONE;
        float Differential = 0.0f;
        if (!MythicTradeLedger::FindBestArbitrage(Entries, Axis, FromIdx, ToIdx, Differential) ||
            Differential < MinDifferential) {
            continue;
        }
        FVector Anchor = FVector::ZeroVector;
        ResolveSettlementAnchor(Entries[ToIdx].SettlementId, Anchor);
        FMythicFactionId Faction;
        Faction.Index = Entries[ToIdx].GoverningFactionIndex;
        SubmitTradeBeat(TAG_TRADING_EVENT_RUMOR, Faction, Anchor, 0.55f);
        LastRumorBeatSeconds[AxisOrd] = NowSeconds;
        UE_LOG(Myth, Log, TEXT("TradeLedger: rumor beat axis %d — settlement %d buys dear (+%.2f over settlement %d)"),
               AxisOrd, Entries[ToIdx].SettlementId, Differential, Entries[FromIdx].SettlementId);
    }
}


bool UMythicTradeLedgerSubsystem::GetLedgerViewForPlayer(int32 SettlementId, AMythicPlayerController *Reader,
                                                         FMythicTradeLedgerView &OutView) const {
    const FMythicTradeLedgerEntry *Live = LiveLedger.Find(SettlementId);
    if (!Live) {
        return false;
    }
    FMythicFactionId Faction;
    Faction.Index = Live->GoverningFactionIndex;

    bool bPOIUnlocked = false;
    FVector Anchor;
    if (ResolveSettlementAnchor(SettlementId, Anchor)) {
        if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
            if (const UMythicPOIDiscoverySubsystem *POI = GI->GetSubsystem<UMythicPOIDiscoverySubsystem>()) {
                const int32 POIId = POI->ResolveCurrentPOI(Anchor);
                bPOIUnlocked = POIId != INDEX_NONE && POI->IsPOIUnlocked(POIId);
            }
        }
    }

    bool bStandingOk = false;
    if (Reader && Faction.IsValid()) {
        const AMythicPlayerState *PS = Reader->GetPlayerState<AMythicPlayerState>();
        if (const UMythicFactionStandingComponent *Standing = PS ? PS->GetFactionStanding() : nullptr) {
            bStandingOk = Standing->TierForStanding(Standing->GetStanding(Faction)) != EMythicStandingTier::Hostile;
        }
    }

    OutView = FMythicTradeLedgerView();
    OutView.SettlementId = SettlementId;
    OutView.GoverningFaction = Faction;

    if (MythicTradeLedger::IsLedgerLiveForReader(bPOIUnlocked, bStandingOk)) {
        OutView.bLive = true;
        OutView.Staleness = 0.0f;
        OutView.Prices = Live->Prices;
        OutView.Reserves = Live->Reserves;
        return true;
    }

    const FMythicTradeLedgerEntry *Rumor = RumorSnapshot.Find(SettlementId);
    if (!Rumor) {
        return false;
    }
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    const float HalfLife = Dev ? Dev->Trading.LedgerStalenessHalfLifeSeconds : 600.0f;
    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : Rumor->SampledAtSeconds;
    const float Staleness = MythicTradeLedger::ComputeStaleness(Now - Rumor->SampledAtSeconds, HalfLife);
    OutView.bLive = false;
    OutView.Staleness = Staleness;
    for (int32 AxisOrd = 0; AxisOrd < ResourceTypeCount; ++AxisOrd) {
        const EMythicResourceType Axis = static_cast<EMythicResourceType>(AxisOrd);
        OutView.Prices.GetResourceMutable(Axis) =
            MythicTradeLedger::QuantizeStalePrice(Rumor->Prices.GetResource(Axis), Staleness);
    }
    return true;
}

TArray<int32> UMythicTradeLedgerSubsystem::GetLedgerSettlementIds() const {
    TArray<int32> Ids;
    LiveLedger.GenerateKeyArray(Ids);
    return Ids;
}


int32 UMythicTradeLedgerSubsystem::RegisterContractOffer(FMythicTradeContractOffer Offer) {
    if (!IsAuthority() || !Offer.FactionId.IsValid() || Offer.Units <= 0 || !Offer.DeliveryItemTag.IsValid()) {
        return INDEX_NONE;
    }
    for (const FMythicTradeContractOffer &Open : OpenOffers) {
        if (Open.QuestKind == Offer.QuestKind && Open.FactionId == Offer.FactionId) {
            return INDEX_NONE;
        }
    }
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    const float Lifetime = Dev ? Dev->Trading.ContractOfferLifetimeSeconds : 900.0f;
    const UWorld *World = GetWorld();
    Offer.OfferId = NextOfferId++;
    Offer.ExpireTimeSeconds = (World ? World->GetTimeSeconds() : 0.0) + Lifetime;
    const int32 OfferId = Offer.OfferId;
    const int32 SettlementId = Offer.SettlementId;
    const FMythicFactionId Faction = Offer.FactionId;
    OpenOffers.Add(MoveTemp(Offer));

    FVector Anchor = FVector::ZeroVector;
    ResolveSettlementAnchor(SettlementId, Anchor);
    SubmitTradeBeat(TAG_TRADING_EVENT_CONTRACT_POSTED, Faction, Anchor, 0.55f);
    UE_LOG(Myth, Log, TEXT("TradeLedger: contract offer %d posted (faction %d, settlement %d)"), OfferId,
           static_cast<int32>(Faction.Index), SettlementId);
    return OfferId;
}

const FMythicTradeContractOffer *UMythicTradeLedgerSubsystem::FindOffer(int32 OfferId) const {
    for (const FMythicTradeContractOffer &Open : OpenOffers) {
        if (Open.OfferId == OfferId) {
            return &Open;
        }
    }
    return nullptr;
}

void UMythicTradeLedgerSubsystem::GetOpenOfferKinds(TArray<FGameplayTag> &OutKinds) const {
    for (const FMythicTradeContractOffer &Open : OpenOffers) {
        OutKinds.Add(Open.QuestKind);
    }
}

void UMythicTradeLedgerSubsystem::SubmitTradeBeat(const FGameplayTag &EventTag, FMythicFactionId PrimaryFaction,
                                                  const FVector &Location, float Significance) {
    UMythicLivingWorldSubsystem *LWS = LivingWorld.Get();
    UWorld *World = GetWorld();
    if (!LWS || !World || !EventTag.IsValid()) {
        return;
    }
    FMythicWorldEvent Event;
    Event.EventTag = EventTag;
    Event.PrimaryFaction = PrimaryFaction;
    Event.WorldTime = World->GetTimeSeconds();
    Event.Significance = Significance;
    Event.CategoryFlags = EMythicEventCategory::Trade;
    if (UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
        Event.Cell = Grid->WorldToCell(Location);
    }
    LWS->SubmitWorldEvent(Event);
}


float UMythicTradeLedgerSubsystem::ComputeCargoValueForPlayer(AMythicPlayerController *PC) const {
    if (!PC) {
        return 0.0f;
    }
    float Total = 0.0f;
    for (UMythicInventoryComponent *Inv : PC->GetAllInventoryComponents()) {
        if (!Inv) {
            continue;
        }
        for (int32 Slot = 0; Slot < Inv->GetNumSlots(); ++Slot) {
            if (UMythicItemInstance *Item = Inv->GetItem(Slot)) {
                if (const UItemDefinition *Def = Item->GetItemDefinition()) {
                    Total += static_cast<float>(FMath::Max(Def->Value, 0)) * static_cast<float>(FMath::Max(Item->GetStacks(), 0));
                }
            }
        }
    }
    return Total;
}

float UMythicTradeLedgerSubsystem::GetCargoHeatForPlayer(AMythicPlayerController *PC) const {
    UMythicLivingWorldSubsystem *LWS = LivingWorld.Get();
    const APawn *Pawn = PC ? PC->GetPawn() : nullptr;
    UMythicTerritoryGrid *Grid = LWS ? LWS->GetTerritoryGrid() : nullptr;
    if (!Pawn || !Grid) {
        return 0.0f;
    }
    const int32 DangerTier = static_cast<int32>(Grid->GetCellDangerTier(Grid->WorldToCell(Pawn->GetActorLocation())));
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    const float ValueRef = Dev ? Dev->Trading.CargoHeatValueReference : 2000.0f;
    const int32 MinTier = Dev ? Dev->Trading.CargoHeatMinDangerTier : 2;
    if (DangerTier < MinTier) {
        return 0.0f;
    }
    return MythicCargoRisk::ComputeCargoHeat(ComputeCargoValueForPlayer(PC), DangerTier, ValueRef, MinTier);
}

float UMythicTradeLedgerSubsystem::GetMaxCargoHeatAt(const FVector &Location, float Radius) const {
    UWorld *World = GetWorld();
    if (!World) {
        return 0.0f;
    }
    const float RadiusSq = FMath::Square(FMath::Max(Radius, 0.0f));
    float MaxHeat = 0.0f;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        AMythicPlayerController *PC = Cast<AMythicPlayerController>(It->Get());
        const APawn *Pawn = PC ? PC->GetPawn() : nullptr;
        if (!Pawn || FVector::DistSquared(Pawn->GetActorLocation(), Location) > RadiusSq) {
            continue;
        }
        MaxHeat = FMath::Max(MaxHeat, GetCargoHeatForPlayer(PC));
    }
    return MaxHeat;
}


bool UMythicTradeLedgerSubsystem::ResolveSettlementAnchor(int32 SettlementId, FVector &OutAnchor) const {
    UMythicLivingWorldSubsystem *LWS = LivingWorld.Get();
    if (!LWS || SettlementId == INDEX_NONE) {
        return false;
    }
    FMythicSettlementData Data;
    if (!LWS->CopySettlementById(SettlementId, Data)) {
        return false;
    }
    if (UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
        OutAnchor = Grid->CellToWorld(Data.CenterCell);
        return true;
    }
    return false;
}
