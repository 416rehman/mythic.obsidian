
#include "World/LivingWorld/EmergentQuests/MythicEmergentQuestSubsystem.h"

#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Territory/MythicDanger.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "Rewards/LootReward.h"
#include "GAS/MythicTags_GAS.h"
#include "World/Trading/MythicTradeLedgerSubsystem.h"
#include "World/Trading/MythicTradeContractTypes.h"
#include "World/Trading/MythicTags_Trading.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicFactionStandingComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "NativeGameplayTags.h"
#include "Mythic.h"

#include <initializer_list>

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_QUEST_EMERGENT_REPEL, "Quest.Emergent.Repel");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_QUEST_EMERGENT_RELIEF, "Quest.Emergent.Relief");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_QUEST_EMERGENT_BOUNTY_ASSASSIN, "Quest.Emergent.Bounty.Assassin");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_QUEST_EMERGENT_WAREFFORT, "Quest.Emergent.WarEffort");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_QUEST_EMERGENT_BOUNTY_CRIME, "Quest.Emergent.Bounty.Crime");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_QUEST_EMERGENT_DELIVERY_FOOD, "Quest.Emergent.Delivery.Food");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_QUEST_EMERGENT_DELIVERY_MATERIALS, "Quest.Emergent.Delivery.Materials");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_QUEST_EMERGENT_DELIVERY_ARMS, "Quest.Emergent.Delivery.Arms");


bool UMythicEmergentQuestSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) {
        return false;
    }
    return World->GetNetMode() != NM_Client;
}

void UMythicEmergentQuestSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);
}

void UMythicEmergentQuestSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);
    if (!IsAuthority()) {
        return;
    }

    BuildDefaultRulePool();

    if (UGameInstance *GI = InWorld.GetGameInstance()) {
        if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            LivingWorld = LWS;
            CommitHandle = LWS->OnWorldSimCommitted.AddUObject(this, &UMythicEmergentQuestSubsystem::HandleWorldSimCommitted);
        }
    }

    if (CleanupIntervalSeconds > 0.0f) {
        InWorld.GetTimerManager().SetTimer(CleanupTimerHandle, this, &UMythicEmergentQuestSubsystem::HandleCleanupTimer,
                                           CleanupIntervalSeconds, true);
    }

    UE_LOG(Myth, Log, TEXT("EmergentQuest: subsystem live (%d code-default rules)"), RulePool.Num());
}

void UMythicEmergentQuestSubsystem::Deinitialize() {
    if (UMythicLivingWorldSubsystem *LWS = LivingWorld.Get()) {
        if (CommitHandle.IsValid()) {
            LWS->OnWorldSimCommitted.Remove(CommitHandle);
        }
    }
    CommitHandle.Reset();
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(CleanupTimerHandle);
    }
    LivingWorld = nullptr;
    ActiveQuests.Reset();
    ActiveObjectiveRoots.Reset();
    Super::Deinitialize();
}

bool UMythicEmergentQuestSubsystem::IsAuthority() const {
    const UWorld *World = GetWorld();
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}


void UMythicEmergentQuestSubsystem::BuildDefaultRulePool() {
    RulePool.Reset();
    auto AnyOf = [](std::initializer_list<FGameplayTag> Tags) {
        FGameplayTagContainer C;
        for (const FGameplayTag &T : Tags) {
            C.AddTag(T);
        }
        return FGameplayTagQuery::MakeQuery_MatchAnyTags(C);
    };

    {
        FMythicEmergentQuestRule R;
        R.EventTagQuery = AnyOf({TAG_LIVINGWORLD_EVENT_ENCOUNTER_SPAWNED});
        R.MinSignificance = 0.35f;
        R.QuestKind = TAG_QUEST_EMERGENT_REPEL;
        R.ObjectiveTriggerTag = GAS_EVENT_KILL;
        R.BaseCount = 6;
        R.RewardTierMultiplier = 1.0f;
        R.FactionStandingReward = 0.0f;
        R.Headline = FText::FromString(TEXT("Repel the raid ({Count} attackers)"));
        RulePool.Add(MoveTemp(R));
    }

    {
        FMythicEmergentQuestRule R;
        R.EventTagQuery = AnyOf({TAG_LIVINGWORLD_EVENT_FACTION_FAMINE});
        R.MinSignificance = 0.0f;
        R.QuestKind = TAG_QUEST_EMERGENT_RELIEF;
        R.ObjectiveTriggerTag = GAS_EVENT_ITEM_ACQUIRED;
        R.bCountByMagnitude = true;
        R.BaseCount = 10;
        R.RewardTierMultiplier = 1.0f;
        R.FactionStandingReward = 15.0f;
        R.Headline = FText::FromString(TEXT("Gather {Count} supplies for {Faction}"));
        RulePool.Add(MoveTemp(R));
    }

    {
        FMythicEmergentQuestRule R;
        R.EventTagQuery = AnyOf({TAG_WORLD_EVENT_DEATH_PERMANENT});
        R.MinSignificance = 0.5f;
        R.QuestKind = TAG_QUEST_EMERGENT_BOUNTY_ASSASSIN;
        R.ObjectiveTriggerTag = GAS_EVENT_KILL;
        R.BaseCount = 1;
        R.RewardTierMultiplier = 1.5f;
        R.FactionStandingReward = 20.0f;
        R.Headline = FText::FromString(TEXT("Hunt the killer ({Faction})"));
        RulePool.Add(MoveTemp(R));
    }

    {
        FMythicEmergentQuestRule R;
        R.EventTagQuery = AnyOf({TAG_LIVINGWORLD_EVENT_DIPLOMACY_SHIFT});
        R.MinSignificance = 0.4f;
        R.ReqPlayerRelationToPrimary = EMythicFactionRelation::Friendly;
        R.QuestKind = TAG_QUEST_EMERGENT_WAREFFORT;
        R.ObjectiveTriggerTag = GAS_EVENT_KILL;
        R.BaseCount = 5;
        R.RewardTierMultiplier = 1.25f;
        R.FactionStandingReward = 12.0f;
        R.Headline = FText::FromString(TEXT("War effort for {Faction}: slay {Count} enemies"));
        RulePool.Add(MoveTemp(R));
    }

    {
        FMythicEmergentQuestRule R;
        R.EventTagQuery = AnyOf({TAG_LIVINGWORLD_ACTION_THEFT_STEAL, TAG_LIVINGWORLD_ACTION_VIOLENCE_KILL});
        R.MinSignificance = 0.2f;
        R.ReqPlayerRelationToPrimary = EMythicFactionRelation::Neutral;
        R.QuestKind = TAG_QUEST_EMERGENT_BOUNTY_CRIME;
        R.ObjectiveTriggerTag = GAS_EVENT_KILL;
        R.BaseCount = 2;
        R.RewardTierMultiplier = 1.0f;
        R.FactionStandingReward = 10.0f;
        R.Headline = FText::FromString(TEXT("Bounty for {Faction}: bring {Count} to justice"));
        RulePool.Add(MoveTemp(R));
    }

    if (GetDefault<UMythicDeveloperSettings>()->bEnableTrading) {
        {
            FMythicEmergentQuestRule R;
            R.EventTagQuery = AnyOf({TAG_LIVINGWORLD_EVENT_FACTION_FAMINE});
            R.MinSignificance = 0.0f;
            R.QuestKind = TAG_QUEST_EMERGENT_DELIVERY_FOOD;
            R.DeliveryItemTag = ITEMIZATION_TYPE_CONSUMABLE_FOOD;
            R.DeliveryUnits = 15;
            R.DeliveryReserveAxis = EMythicResourceType::Food;
            R.RewardTierMultiplier = 1.0f;
            R.FactionStandingReward = 15.0f;
            R.Headline = FText::FromString(TEXT("Relief convoy: deliver {Count} food to {Faction}"));
            RulePool.Add(MoveTemp(R));
        }
        {
            FMythicEmergentQuestRule R;
            R.EventTagQuery = AnyOf({TAG_TRADING_EVENT_DEFICIT_MATERIALS});
            R.MinSignificance = 0.0f;
            R.QuestKind = TAG_QUEST_EMERGENT_DELIVERY_MATERIALS;
            R.DeliveryItemTag = ITEMIZATION_TYPE_MINING;
            R.DeliveryUnits = 12;
            R.DeliveryReserveAxis = EMythicResourceType::Materials;
            R.RewardTierMultiplier = 1.0f;
            R.FactionStandingReward = 12.0f;
            R.Headline = FText::FromString(TEXT("Supply run: deliver {Count} materials to {Faction}"));
            RulePool.Add(MoveTemp(R));
        }
        {
            FMythicEmergentQuestRule R;
            R.EventTagQuery = AnyOf({TAG_TRADING_EVENT_DEFICIT_ARMS, TAG_LIVINGWORLD_EVENT_FACTION_WEAKNESS});
            R.MinSignificance = 0.0f;
            R.ReqPlayerRelationToPrimary = EMythicFactionRelation::Friendly;
            R.QuestKind = TAG_QUEST_EMERGENT_DELIVERY_ARMS;
            R.DeliveryItemTag = ITEMIZATION_TYPE_EQUIPMENT_WEAPON;
            R.DeliveryUnits = 5;
            R.DeliveryReserveAxis = EMythicResourceType::Arms;
            R.RewardTierMultiplier = 1.25f;
            R.FactionStandingReward = 18.0f;
            R.Headline = FText::FromString(TEXT("War demand: deliver {Count} arms to {Faction}"));
            RulePool.Add(MoveTemp(R));
        }
    }
}


void UMythicEmergentQuestSubsystem::HandleWorldSimCommitted() {
    if (!IsAuthority()) {
        return;
    }
    UMythicLivingWorldSubsystem *LWS = LivingWorld.Get();
    if (!LWS || !LWS->IsSystemActive()) {
        return;
    }
    UMythicCausalFabric *Fabric = LWS->GetCausalFabric();
    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();
    UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    if (!Fabric || !Grid) {
        return;
    }
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    const uint32 NewestId = Fabric->GetTotalEventCount();
    if (!bSeeded) {
        LastSeenEventId = (NewestId > 0) ? NewestId - 1 : 0;
        bSeeded = true;
        return;
    }
    if (NewestId == 0 || (NewestId - 1) <= LastSeenEventId) {
        ReconcileCompletions();
        return;
    }

    const int32 Unseen = static_cast<int32>((NewestId - 1) - LastSeenEventId);
    const int32 Want = FMath::Min(Unseen, Fabric->GetCapacity());
    const TArray<FMythicWorldEvent> Recent = Fabric->GetRecentEvents(Want);

    uint32 MaxId = LastSeenEventId;
    TArray<const FMythicWorldEvent *> Fresh;
    for (const FMythicWorldEvent &E : Recent) {
        if (E.EventId <= LastSeenEventId) {
            continue;
        }
        MaxId = FMath::Max(MaxId, E.EventId);
        if (E.Significance < MinConsideredSignificance) {
            continue;
        }
        Fresh.Add(&E);
    }
    LastSeenEventId = MaxId;

    if (Fresh.Num() > 0) {
        Fresh.Sort([](const FMythicWorldEvent &A, const FMythicWorldEvent &B) { return A.EventId < B.EventId; });

        TArray<FGameplayTag> ActiveKinds;
        ActiveKinds.Reserve(ActiveQuests.Num() + Fresh.Num());
        for (const FMythicActiveEmergentQuest &Q : ActiveQuests) {
            ActiveKinds.Add(Q.QuestKind);
        }
        if (UMythicTradeLedgerSubsystem *Board = World->GetSubsystem<UMythicTradeLedgerSubsystem>()) {
            Board->GetOpenOfferKinds(ActiveKinds);
        }

        const double NowSeconds = World->GetTimeSeconds();

        for (const FMythicWorldEvent *EPtr : Fresh) {
            const FMythicWorldEvent &E = *EPtr;
            const FMythicCellCoord Cell = E.Cell;
            const FVector CellCenter = Grid->CellToWorld(Cell);
            const int32 DangerTier = static_cast<int32>(Grid->GetCellDangerTier(Cell));

            FMythicWorldEventSnapshot Snap;
            Snap.EventTag = E.EventTag;
            Snap.Significance = E.Significance;
            Snap.PrimaryFactionId = E.PrimaryFaction.IsValid() ? static_cast<int32>(E.PrimaryFaction.Index) : -1;
            Snap.PlayerRelation = EMythicFactionRelation::Allied;
            Snap.Cell = FIntPoint(Cell.X, Cell.Y);
            Snap.DangerTier = DangerTier;

            const int32 RuleIdx = MythicEmergentQuestRules::SelectQuestRuleForEvent(Snap, RulePool, ActiveKinds, E.EventId);
            if (RuleIdx == INDEX_NONE) {
                continue;
            }
            const FMythicEmergentQuestRule &Rule = RulePool[RuleIdx];

            if (Rule.IsDeliveryRow()) {
                PostDeliveryContractOffer(Rule, E, DangerTier);
                ActiveKinds.Add(Rule.QuestKind);
                continue;
            }

            TArray<AMythicPlayerController *> Recipients;
            for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
                AMythicPlayerController *PC = Cast<AMythicPlayerController>(It->Get());
                if (!PC || !PC->HasAuthority()) {
                    continue;
                }
                const APawn *Pawn = PC->GetPawn();
                if (!Pawn) {
                    continue;
                }
                const bool bInRange = FVector::Dist(Pawn->GetActorLocation(), CellCenter) <= OfferRadius ||
                                      Grid->WorldToCell(Pawn->GetActorLocation()) == Cell;
                if (!bInRange) {
                    continue;
                }
                const EMythicFactionRelation Rel = ResolvePlayerRelationToFaction(PC, E.PrimaryFaction);
                if (!MythicEmergentQuestRules::PassesRelationGate(Rel, Rule.ReqPlayerRelationToPrimary)) {
                    continue;
                }
                Recipients.Add(PC);
            }
            if (Recipients.Num() == 0) {
                continue;
            }

            float FactionStrength = 0.0f;
            FText FactionName = FText::GetEmpty();
            if (FactionDB && E.PrimaryFaction.IsValid()) {
                FMythicFactionData Data;
                if (FactionDB->GetFaction(E.PrimaryFaction, Data)) {
                    FactionStrength = Data.MilitaryStrength;
                    FactionName = Data.DisplayName;
                }
            }

            const FMythicEmergentReward Reward = MythicEmergentQuestRules::ComputeEmergentReward(Rule, DangerTier, FactionStrength);
            UObjectiveDefinition *Obj = BuildEmergentObjective(Rule, Reward, CellCenter, FactionName);
            if (!Obj) {
                continue;
            }

            FMythicActiveEmergentQuest Active;
            Active.Objective = Obj;
            Active.QuestKind = Rule.QuestKind;
            Active.RewardFaction = E.PrimaryFaction;
            Active.StandingReward = Rule.FactionStandingReward * static_cast<float>(FMath::Max(1, Reward.RewardTier));
            Active.ExpireTimeSeconds = NowSeconds + OfferLifetimeSeconds;
            ActiveObjectiveRoots.Add(Obj);
            ActiveQuests.Add(MoveTemp(Active));
            ActiveKinds.Add(Rule.QuestKind);

            for (AMythicPlayerController *PC : Recipients) {
                if (UObjectiveTracker *Tracker = PC->GetObjectiveTracker()) {
                    Tracker->ServerAddObjective(Obj);
                }
            }

            UE_LOG(Myth, Log, TEXT("EmergentQuest: spawned '%s' (kind=%s, count=%d, tier=%d) for %d player(s) from event %u (%s)"),
                   *Obj->DisplayText.ToString(), *Rule.QuestKind.ToString(), Reward.QuestCount, Reward.RewardTier,
                   Recipients.Num(), E.EventId, *E.EventTag.ToString());
        }
    }

    ReconcileCompletions();
}


UObjectiveDefinition *UMythicEmergentQuestSubsystem::BuildEmergentObjective(const FMythicEmergentQuestRule &Rule,
                                                                            const FMythicEmergentReward &Reward,
                                                                            const FVector &MarkerLocation,
                                                                            const FText &FactionName) {
    UObjectiveDefinition *Obj = NewObject<UObjectiveDefinition>(this, NAME_None, RF_Transient);
    if (!Obj) {
        return nullptr;
    }

    Obj->TriggerEventTag = Rule.ObjectiveTriggerTag.IsValid() ? Rule.ObjectiveTriggerTag : GAS_EVENT_KILL;
    Obj->RequiredCount = FMath::Max(1, Reward.QuestCount);
    Obj->bCountByEventMagnitude = Rule.bCountByMagnitude;
    if (Rule.ObjectivePayloadTag.IsValid()) {
        Obj->RequiredPayloadTag = Rule.ObjectivePayloadTag;
    }

    FFormatNamedArguments Args;
    Args.Add(TEXT("Faction"), FactionName.IsEmpty() ? FText::FromString(TEXT("the realm")) : FactionName);
    Args.Add(TEXT("Count"), FText::AsNumber(Obj->RequiredCount));
    Obj->DisplayText = FText::Format(FTextFormat(Rule.Headline), Args);
    Obj->CompletedText = FText::Format(NSLOCTEXT("Mythic", "EmergentQuestDone", "{0} — done!"), Obj->DisplayText);
    Obj->QuestName = NSLOCTEXT("Mythic", "EmergentQuestGroup", "World Event");

    Obj->bShowOnMap = true;
    Obj->WorldMarkerLocation = MarkerLocation;

    Obj->GrantStoryTagsOnComplete = Rule.GrantStoryTagsOnComplete;

    Obj->Rewards.LootReward = NewObject<ULootReward>(Obj);

    Obj->bRepeatable = false;
    return Obj;
}


void UMythicEmergentQuestSubsystem::PostDeliveryContractOffer(const FMythicEmergentQuestRule &Rule,
                                                              const FMythicWorldEvent &Event, int32 DangerTier) {
    UWorld *World = GetWorld();
    UMythicTradeLedgerSubsystem *Board = World ? World->GetSubsystem<UMythicTradeLedgerSubsystem>() : nullptr;
    UMythicLivingWorldSubsystem *LWS = LivingWorld.Get();
    if (!Board || !LWS || !Event.PrimaryFaction.IsValid()) {
        return;
    }

    float FactionStrength = 0.0f;
    FText FactionName = FText::GetEmpty();
    if (UMythicFactionDatabase *DB = LWS->GetFactionDatabase()) {
        FMythicFactionData Data;
        if (DB->GetFaction(Event.PrimaryFaction, Data)) {
            FactionStrength = Data.MilitaryStrength;
            FactionName = Data.DisplayName;
        }
    }

    int32 SettlementId = INDEX_NONE;
    FMythicSettlementData AtCell;
    if (LWS->CopySettlementAtCell(Event.Cell, AtCell) && AtCell.GoverningFaction == Event.PrimaryFaction) {
        SettlementId = AtCell.SettlementId;
    }
    else {
        TArray<int32> Ids;
        LWS->CopyAllSettlementIds(Ids);
        for (const int32 Id : Ids) {
            FMythicSettlementData Data;
            if (LWS->CopySettlementById(Id, Data) && Data.GoverningFaction == Event.PrimaryFaction) {
                SettlementId = Id;
                break;
            }
        }
    }

    const FMythicEmergentReward Reward = MythicEmergentQuestRules::ComputeEmergentReward(Rule, DangerTier, FactionStrength);

    FMythicTradeContractOffer Offer;
    Offer.QuestKind = Rule.QuestKind;
    Offer.FactionId = Event.PrimaryFaction;
    Offer.SettlementId = SettlementId;
    Offer.DeliveryItemTag = Rule.DeliveryItemTag;
    Offer.Units = FMath::Max(1, Rule.DeliveryUnits + DangerTier);
    Offer.ReserveAxis = Rule.DeliveryReserveAxis;
    Offer.StandingReward = Rule.FactionStandingReward * static_cast<float>(FMath::Max(1, Reward.RewardTier));
    Offer.bRequiresFriendlyStanding =
        static_cast<uint8>(Rule.ReqPlayerRelationToPrimary) <= static_cast<uint8>(EMythicFactionRelation::Friendly);

    FFormatNamedArguments Args;
    Args.Add(TEXT("Faction"), FactionName.IsEmpty() ? FText::FromString(TEXT("the realm")) : FactionName);
    Args.Add(TEXT("Count"), FText::AsNumber(Offer.Units));
    Offer.Headline = FText::Format(FTextFormat(Rule.Headline), Args);

    const int32 OfferId = Board->RegisterContractOffer(MoveTemp(Offer));
    if (OfferId != INDEX_NONE) {
        UE_LOG(Myth, Log, TEXT("EmergentQuest: delivery contract %d posted (kind=%s) from event %u (%s)"), OfferId,
               *Rule.QuestKind.ToString(), Event.EventId, *Event.EventTag.ToString());
    }
}


EMythicFactionRelation UMythicEmergentQuestSubsystem::ResolvePlayerRelationToFaction(AMythicPlayerController *PC,
                                                                                     FMythicFactionId Faction) const {
    if (!PC || !Faction.IsValid()) {
        return EMythicFactionRelation::Neutral;
    }
    const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>();
    const UMythicFactionStandingComponent *Standing = PS ? PS->GetFactionStanding() : nullptr;
    if (!Standing) {
        return EMythicFactionRelation::Neutral;
    }
    switch (Standing->TierForStanding(Standing->GetStanding(Faction))) {
    case EMythicStandingTier::Hostile:
        return EMythicFactionRelation::Hostile;
    case EMythicStandingTier::Friendly:
        return EMythicFactionRelation::Friendly;
    case EMythicStandingTier::Neutral:
    default:
        return EMythicFactionRelation::Neutral;
    }
}


void UMythicEmergentQuestSubsystem::ReconcileCompletions() {
    if (!IsAuthority() || ActiveQuests.Num() == 0) {
        return;
    }
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    for (FMythicActiveEmergentQuest &Q : ActiveQuests) {
        if (!Q.Objective || Q.StandingReward == 0.0f || !Q.RewardFaction.IsValid()) {
            continue;
        }
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
            AMythicPlayerController *PC = Cast<AMythicPlayerController>(It->Get());
            if (!PC || !PC->HasAuthority()) {
                continue;
            }
            UObjectiveTracker *Tracker = PC->GetObjectiveTracker();
            if (!Tracker || Q.RewardedTrackers.Contains(Tracker)) {
                continue;
            }
            FObjectiveProgress Prog;
            if (!Tracker->FindObjectiveProgress(Q.Objective, Prog) || !Prog.bCompleted) {
                continue;
            }
            if (AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>()) {
                if (UMythicFactionStandingComponent *Standing = PS->GetFactionStanding()) {
                    Standing->ServerAdjustStanding(Q.RewardFaction, Q.StandingReward);
                    Q.RewardedTrackers.Add(Tracker);
                    UE_LOG(Myth, Log, TEXT("EmergentQuest: '%s' completed by %s → +%.0f standing with faction %d"),
                           *Q.Objective->DisplayText.ToString(), *GetNameSafe(PC), Q.StandingReward,
                           static_cast<int32>(Q.RewardFaction.Index));
                }
            }
        }
    }
}

void UMythicEmergentQuestSubsystem::HandleCleanupTimer() {
    if (!IsAuthority()) {
        return;
    }
    UWorld *World = GetWorld();
    const double NowSeconds = World ? World->GetTimeSeconds() : 0.0;

    for (int32 i = ActiveQuests.Num() - 1; i >= 0; --i) {
        if (NowSeconds >= ActiveQuests[i].ExpireTimeSeconds) {
            ActiveObjectiveRoots.Remove(ActiveQuests[i].Objective);
            ActiveQuests.RemoveAt(i);
        }
    }

    ReconcileCompletions();
}
