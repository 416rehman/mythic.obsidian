
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "MassEntitySubsystem.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicFactionStandingComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Dialogue/DialogueSelector.h"
#include "World/LivingWorld/Dialogue/MythicDialogueTypes.h"
#include "World/LivingWorld/Chronicle/MythicWorldChronicleSubsystem.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY(LogMythCognition);


UMythicCognitiveBrainComponent::UMythicCognitiveBrainComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    bWantsInitializeComponent = false;
    CachedSchedulePhase = EMythicSchedulePhase::Idle;
}


void UMythicCognitiveBrainComponent::BeginPlay() {
    Super::BeginPlay();

    if (UGameInstance *GI = GetWorld()->GetGameInstance()) {
        if (UMythicLivingWorldSubsystem *LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            CausalFabric = LW->GetCausalFabric();
            FactionDB = LW->GetFactionDatabase();
            SocialGraph = LW->GetSocialGraph();
            Settings = LW->GetSettings();
        }
    }

    if (!CausalFabric || !FactionDB || !Settings) {
        UE_LOG(LogMythCognition, Warning,
               TEXT("CognitiveBrain: Could not cache living world references. Brain disabled."));
        return;
    }

    Beliefs.Reserve(MaxBeliefsPerNPC);
    LastDesires.Reserve(DesireTypeCount);

    StartThinking();
}

void UMythicCognitiveBrainComponent::StartThinking() {
    if (!CausalFabric || !FactionDB || !Settings) {
        return;
    }

    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    const float MinInterval = Settings->CognitiveThinkIntervalMin;
    const float MaxInterval = Settings->CognitiveThinkIntervalMax;
    ThinkInterval = FMath::RandRange(MinInterval, MaxInterval);

    const float InitialDelay = FMath::RandRange(0.0f, ThinkInterval);

    World->GetTimerManager().SetTimer(
        ThinkTimerHandle,
        this,
        &UMythicCognitiveBrainComponent::Think,
        ThinkInterval,
true,
        InitialDelay);
}

void UMythicCognitiveBrainComponent::ResetForReuse() {
    {
        FScopeLock Lock(&BeliefsLock);
        Beliefs.Reset();
        LastDesires.Reset();
    }
    CurrentIntention = FMythicIntention();
    SourceEntity = FMassEntityHandle();
    bInitialized = false;
    CachedSchedulePhase = EMythicSchedulePhase::Idle;
}

void UMythicCognitiveBrainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(ThinkTimerHandle);

        if (SourceEntity.IsSet()) {
            if (UGameInstance *GI = World->GetGameInstance()) {
                if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                    LWS->UnregisterEmbodiedActor(SourceEntity);
                }
            }
        }
    }

    if (AsyncThinkTask.IsValid() && !AsyncThinkTask.IsCompleted()) {
        AsyncThinkTask.Wait();
    }

    Super::EndPlay(EndPlayReason);
}

void UMythicCognitiveBrainComponent::StopThinking() {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(ThinkTimerHandle);
    }
    bInitialized = false;

    if (AsyncThinkTask.IsValid() && !AsyncThinkTask.IsCompleted()) {
        AsyncThinkTask.Wait();
    }
}


void UMythicCognitiveBrainComponent::InitializeBrain(
    FMythicFactionId InFaction,
    FMythicCellCoord InHomeCell,
    const FMythicPersonalityFragment &InPersonality,
    FMassEntityHandle InSourceEntity,
    FMythicFactionId InTrueFaction,
    FGameplayTag InRole) {
    Faction = InFaction;
    HomeCell = InHomeCell;
    Personality = InPersonality;
    SourceEntity = InSourceEntity;
    TrueFaction = InTrueFaction.IsValid() ? InTrueFaction : InFaction;
    Role = InRole;
    bInitialized = true;

    UE_LOG(LogMythCognition, Verbose,
           TEXT("BDI Brain initialized: Faction=%d, Home=%s, Role=%s"),
           Faction.Index, *HomeCell.ToString(), *Role.ToString());
}


FText UMythicCognitiveBrainComponent::GetDisplayName() const {
    if (SourceEntity.IsSet()) {
        if (UMassEntitySubsystem *Ess = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld())) {
            if (Ess->GetEntityManager().IsEntityValid(SourceEntity)) {
                if (const FMythicIdentityFragment *Id = Ess->GetEntityManager().GetFragmentDataPtr<FMythicIdentityFragment>(SourceEntity)) {
                    return FText::FromName(FMythicNPCGenerator::ReconstructNameFromHash(Id->NameHash, Faction.Index));
                }
            }
        }
    }
    return FText::GetEmpty();
}

FText UMythicCognitiveBrainComponent::SelectDialogue(AActor *InteractingPlayer, bool bCompanionCommentary, float PlayerActionMoralScore) const {
    if (!bInitialized) {
        return FText::FromString(TEXT("..."));
    }
    if (!Settings || Settings->DialogueDatabase.IsNull()) {
        return FText::FromString(TEXT("..."));
    }

    UMythicDialogueDatabase *Database = Settings->DialogueDatabase.LoadSynchronous();
    if (!Database) {
        return FText::FromString(TEXT("..."));
    }

    FMythicDialogueContext Context;
    Context.RoleTag = Role;

    Context.bIsCompanionCommentary = bCompanionCommentary;
    Context.PlayerActionMoralScore = PlayerActionMoralScore;

    FMythicFactionData FactionData;
    const bool bHasFactionData = (FactionDB && FactionDB->GetFaction(Faction, FactionData));
    if (bHasFactionData) {
        Context.FactionTag = FactionData.FactionTag;
    }

    float MaxPressure = 0.0f;
    for (int32 i = 0; i < PressureChannelCount; ++i) {
        if (PressureChannels[i] > MaxPressure) {
            MaxPressure = PressureChannels[i];
            Context.DominantPressureChannel = i;
        }
    }
    Context.PressureChannels = PressureChannels;
    Context.PressureChannelCount = PressureChannelCount;

    FMythicDialogueResult Result = FMythicDialogueSelector::SelectTemplate(Database, Context);
    if (!Result.IsValid()) {
        return bCompanionCommentary ? FText::GetEmpty() : FText::FromString(TEXT("Hello."));
    }

    FMythicDialogueVariables Vars;
    if (bHasFactionData) {
        Vars.FactionName = FactionData.DisplayName.ToString();
    }

    if (Context.DominantPressureChannel >= 0 && Context.DominantPressureChannel < PressureChannelCount) {
        switch (static_cast<EMythicPressureChannel>(Context.DominantPressureChannel)) {
        case EMythicPressureChannel::Threat:
            Vars.SpeakerMood = TEXT("afraid");
            break;
        case EMythicPressureChannel::Injustice:
            Vars.SpeakerMood = TEXT("indignant");
            break;
        case EMythicPressureChannel::Grief:
            Vars.SpeakerMood = TEXT("grieving");
            break;
        case EMythicPressureChannel::Shame:
            Vars.SpeakerMood = TEXT("ashamed");
            break;
        case EMythicPressureChannel::Desire:
            Vars.SpeakerMood = TEXT("yearning");
            break;
        case EMythicPressureChannel::Wrath:
            Vars.SpeakerMood = TEXT("furious");
            break;
        default:
            break;
        }
    }

    if (const APlayerController *PC = Cast<APlayerController>(InteractingPlayer)) {
        if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(PC->PlayerState)) {
            Vars.TargetName = PS->GetPlayerName();
            if (const UMythicFactionStandingComponent *Standing = PS->GetFactionStanding()) {
                const float Rep = Standing->GetStanding(Faction);
                if (Rep <= Standing->GetHostileThreshold()) {
                    Vars.PlayerReputationDescriptor = TEXT("hostile");
                }
                else if (Rep >= Standing->GetFriendlyThreshold()) {
                    Vars.PlayerReputationDescriptor = TEXT("friendly");
                }
                else {
                    Vars.PlayerReputationDescriptor = TEXT("neutral");
                }
            }
        }
    }

    Vars.NPCName = GetDisplayName().ToString();

    {
        const TArray<FMythicBelief> BeliefSnapshot = GetBeliefsCopy();
        float BestConfidence = 0.0f;
        for (const FMythicBelief &B : BeliefSnapshot) {
            if (B.EventTag.IsValid() && B.Confidence > BestConfidence) {
                BestConfidence = B.Confidence;
                Vars.RecentEvent = UMythicWorldChronicleSubsystem::EventTagToReadable(B.EventTag);
            }
        }
    }

    if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
        if (UMythicLivingWorldSubsystem *LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            FMythicSettlementData HomeSettlement;
            if (LW->CopySettlementAtCell(HomeCell, HomeSettlement)) {
                Vars.SettlementName = HomeSettlement.DisplayName.ToString();
            }
        }
    }

    return FMythicDialogueSelector::ResolveVariables(Result.Template->DialogueText, Vars);
}

void UMythicCognitiveBrainComponent::InjectBelief(const FMythicBelief &Belief) {
    FScopeLock Lock(&BeliefsLock);
    InjectBeliefInternal(Belief);
}

TArray<FMythicBelief> UMythicCognitiveBrainComponent::GetBeliefsCopy() const {
    FScopeLock Lock(&BeliefsLock);
    return Beliefs;
}

TArray<FMythicDesire> UMythicCognitiveBrainComponent::GetLastDesiresCopy() const {
    FScopeLock Lock(&BeliefsLock);
    return LastDesires;
}

void UMythicCognitiveBrainComponent::InjectBeliefInternal(const FMythicBelief &Belief) {
    for (FMythicBelief &Existing : Beliefs) {
        if (Existing.SourceEventId == Belief.SourceEventId && Existing.SourceEventId != 0) {
            Existing.Confidence = FMath::Max(Existing.Confidence, Belief.Confidence);
            return;
        }
    }

    if (Beliefs.Num() >= MaxBeliefsPerNPC) {
        int32 WeakestIndex = 0;
        float WeakestConfidence = Beliefs[0].Confidence;
        for (int32 i = 1; i < Beliefs.Num(); ++i) {
            if (Beliefs[i].Confidence < WeakestConfidence) {
                WeakestConfidence = Beliefs[i].Confidence;
                WeakestIndex = i;
            }
        }
        Beliefs.RemoveAtSwap(WeakestIndex);
    }

    Beliefs.Add(Belief);
}

void UMythicCognitiveBrainComponent::OnSignificantEvent(const FGameplayTag &EventTag, FMythicCellCoord EventCell) {
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(ThinkTimerHandle);
        Think();

        GetWorld()->GetTimerManager().SetTimer(
            ThinkTimerHandle,
            this,
            &UMythicCognitiveBrainComponent::Think,
            ThinkInterval,
true);
    }
}


void UMythicCognitiveBrainComponent::Think() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicCognitiveBrain_Think);

    if (!bInitialized || !CausalFabric || !FactionDB) {
        return;
    }

    if (bIsThinkingAsync.exchange(true)) {
        return;
    }

    const double WorldTime = GetWorld()->GetTimeSeconds();

    if (SourceEntity.IsSet()) {
        if (UMassEntitySubsystem *Ess = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld())) {
            if (Ess->GetEntityManager().IsEntityValid(SourceEntity)) {
                if (const FMythicPsychodynamicFragment *Psyche =
                    Ess->GetEntityManager().GetFragmentDataPtr<FMythicPsychodynamicFragment>(SourceEntity)) {
                    for (int32 i = 0; i < PressureChannelCount; ++i) {
                        PressureChannels[i] = Psyche->Pressure[i];
                    }
                }
                if (const FMythicScheduleFragment *Schedule =
                    Ess->GetEntityManager().GetFragmentDataPtr<FMythicScheduleFragment>(SourceEntity)) {
                    CachedSchedulePhase = Schedule->Phase;
                    CachedWorkCell = Schedule->WorkCell;
                }
            }
        }
    }

    TWeakObjectPtr<UMythicCognitiveBrainComponent> WeakThis(this);
    AsyncThinkTask = UE::Tasks::Launch(
        TEXT("BDI_Think"),
        [this, WorldTime, WeakThis]() {
            {
                FScopeLock BeliefsScope(&BeliefsLock);

                UpdateBeliefs(WorldTime);

                ScoreDesires(WorldTime);
            }

            AsyncTask(ENamedThreads::GameThread, [WeakThis, WorldTime]() {
                if (UMythicCognitiveBrainComponent *StrongThis = WeakThis.Get()) {
                    StrongThis->OnAsyncThinkCompleted(WorldTime);
                }
            });
        },
        UE::Tasks::ETaskPriority::BackgroundNormal
        );
}

void UMythicCognitiveBrainComponent::OnAsyncThinkCompleted(double WorldTime) {
    ValidateIntention(WorldTime);
    CommitIntention(WorldTime);

    bIsThinkingAsync.store(false);

    if (Settings) {
        AActor *Owner = GetOwner();
        IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(Owner);
        if (ASI && ASI->GetAbilitySystemComponent()) {
            UAbilitySystemComponent *ASC = ASI->GetAbilitySystemComponent();

            const float TotalPressure = PressureChannels[0] + PressureChannels[1] +
                PressureChannels[2] + PressureChannels[3] +
                PressureChannels[4] + PressureChannels[5];

            if (TotalPressure >= Settings->DespairThreshold) {
                ASC->AddLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_DESPAIR);
            }
            else {
                ASC->RemoveLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_DESPAIR);
            }

            if (PressureChannels[static_cast<int32>(EMythicPressureChannel::Threat)] > 3.0f) {
                ASC->AddLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_FEAR);
            }
            else {
                ASC->RemoveLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_FEAR);
            }

            if (PressureChannels[static_cast<int32>(EMythicPressureChannel::Grief)] > 3.0f) {
                ASC->AddLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_GRIEF);
            }
            else {
                ASC->RemoveLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_GRIEF);
            }

            if (TotalPressure < 1.0f && CurrentIntention.bValid &&
                (CurrentIntention.Desire.Type == EMythicDesireType::Socialize ||
                    CurrentIntention.Desire.Type == EMythicDesireType::Rest)) {
                ASC->AddLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_JOY);
            }
            else {
                ASC->RemoveLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_JOY);
            }
        }
    }
}

void UMythicCognitiveBrainComponent::UpdateBeliefs(double WorldTime) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicCognitiveBrain_UpdateBeliefs);

    if (!CausalFabric) {
        return;
    }

    TArray<FMythicWorldEvent> NearbyEvents;
    CausalFabric->QueryEventsByCell(
        HomeCell,
        WorldTime - 120.0,
        WorldTime,
        8,
        NearbyEvents);

    for (const FMythicWorldEvent &EventRef : NearbyEvents) {
        const FMythicWorldEvent *Event = &EventRef;

        float RelevanceWeight = 0.3f;

        if (Event->CategoryFlags & EMythicEventCategory::Combat) {
            RelevanceWeight += Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Fight)] * 0.5f;
        }
        if (Event->CategoryFlags & EMythicEventCategory::Crime) {
            RelevanceWeight += Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Enforce)] * 0.5f;
            RelevanceWeight += Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Report)] * 0.3f;
        }
        if (Event->CategoryFlags & EMythicEventCategory::Social) {
            RelevanceWeight += Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Rally)] * 0.3f;
            RelevanceWeight += Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Tend)] * 0.3f;
        }

        if (RelevanceWeight < 0.4f) {
            continue;
        }

        FMythicBelief NewBelief;
        NewBelief.EventTag = Event->EventTag;
        NewBelief.Cell = Event->Cell;
        NewBelief.InvolvedFaction = Event->PrimaryFaction;
        NewBelief.Confidence = FMath::Clamp(Event->Significance * RelevanceWeight, 0.1f, 1.0f);
        NewBelief.FormationTime = WorldTime;
        NewBelief.LastDecayTime = WorldTime;
        NewBelief.PropagationHops = 0;
        NewBelief.SourceEventId = Event->EventId;

        if (TrueFaction.IsValid() && TrueFaction != Faction) {
            NewBelief.InvolvedFaction = TrueFaction;
        }

        InjectBeliefInternal(NewBelief);
    }

    const float DecayRate = Settings ? Settings->BeliefConfidenceDecayRate : 0.005f;
    const float PruneThreshold = Settings ? Settings->BeliefPruneThreshold : 0.05f;
    for (int32 i = Beliefs.Num() - 1; i >= 0; --i) {
        double &Last = Beliefs[i].LastDecayTime;
        if (Last <= 0.0) {
            Last = Beliefs[i].FormationTime > 0.0 ? Beliefs[i].FormationTime : WorldTime;
        }
        const double Delta = WorldTime - Last;
        Beliefs[i].Confidence = DecayBeliefConfidence(Beliefs[i].Confidence, DecayRate, Delta);
        Last = WorldTime;

        if (Beliefs[i].Confidence < PruneThreshold) {
            Beliefs.RemoveAtSwap(i);
        }
    }
}

float UMythicCognitiveBrainComponent::DecayBeliefConfidence(float Confidence, float DecayRate, double DeltaSeconds) {
    const double Delta = FMath::Max(0.0, DeltaSeconds);
    return Confidence * FMath::Exp(-DecayRate * static_cast<float>(Delta));
}

void UMythicCognitiveBrainComponent::ScoreDesires(double WorldTime) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicCognitiveBrain_ScoreDesires);

    LastDesires.Reset();
    LastDesires.SetNum(DesireTypeCount);

    LastDesires[static_cast<int32>(EMythicDesireType::Survive)].Type = EMythicDesireType::Survive;
    LastDesires[static_cast<int32>(EMythicDesireType::Survive)].Utility = ScoreSurvive(WorldTime);

    LastDesires[static_cast<int32>(EMythicDesireType::Defend)].Type = EMythicDesireType::Defend;
    LastDesires[static_cast<int32>(EMythicDesireType::Defend)].Utility = ScoreDefend(WorldTime);

    LastDesires[static_cast<int32>(EMythicDesireType::Avenge)].Type = EMythicDesireType::Avenge;
    LastDesires[static_cast<int32>(EMythicDesireType::Avenge)].Utility = ScoreAvenge(WorldTime);

    LastDesires[static_cast<int32>(EMythicDesireType::Patrol)].Type = EMythicDesireType::Patrol;
    LastDesires[static_cast<int32>(EMythicDesireType::Patrol)].Utility = ScorePatrol(WorldTime);

    LastDesires[static_cast<int32>(EMythicDesireType::Trade)].Type = EMythicDesireType::Trade;
    LastDesires[static_cast<int32>(EMythicDesireType::Trade)].Utility = ScoreTrade(WorldTime);

    LastDesires[static_cast<int32>(EMythicDesireType::Socialize)].Type = EMythicDesireType::Socialize;
    LastDesires[static_cast<int32>(EMythicDesireType::Socialize)].Utility = ScoreSocialize(WorldTime);

    LastDesires[static_cast<int32>(EMythicDesireType::JoinPlayer)].Type = EMythicDesireType::JoinPlayer;
    LastDesires[static_cast<int32>(EMythicDesireType::JoinPlayer)].Utility = ScoreJoinPlayer(WorldTime);

    LastDesires[static_cast<int32>(EMythicDesireType::Flee)].Type = EMythicDesireType::Flee;
    LastDesires[static_cast<int32>(EMythicDesireType::Flee)].Utility = ScoreFlee(WorldTime);

    LastDesires[static_cast<int32>(EMythicDesireType::Rest)].Type = EMythicDesireType::Rest;
    LastDesires[static_cast<int32>(EMythicDesireType::Rest)].Utility = ScoreRest(WorldTime);

    LastDesires[static_cast<int32>(EMythicDesireType::Exploit)].Type = EMythicDesireType::Exploit;
    LastDesires[static_cast<int32>(EMythicDesireType::Exploit)].Utility = ScoreExploit(WorldTime);

    LastDesires[static_cast<int32>(EMythicDesireType::Rally)].Type = EMythicDesireType::Rally;
    LastDesires[static_cast<int32>(EMythicDesireType::Rally)].Utility = ScoreRally(WorldTime);

    LastDesires[static_cast<int32>(EMythicDesireType::Report)].Type = EMythicDesireType::Report;
    LastDesires[static_cast<int32>(EMythicDesireType::Report)].Utility = ScoreReport(WorldTime);

    LastDesires[static_cast<int32>(EMythicDesireType::FollowSchedule)].Type = EMythicDesireType::FollowSchedule;
    LastDesires[static_cast<int32>(EMythicDesireType::FollowSchedule)].Utility = ScoreFollowSchedule(WorldTime);

    FMythicCellCoord DefendCell = HomeCell;
    float BestThreatConfidence = 0.0f;
    for (const FMythicBelief &B : Beliefs) {
        if (FMath::Abs(B.Cell.X - HomeCell.X) <= 2 && FMath::Abs(B.Cell.Y - HomeCell.Y) <= 2 && B.Confidence > BestThreatConfidence) {
            BestThreatConfidence = B.Confidence;
            DefendCell = B.Cell;
        }
    }
    LastDesires[static_cast<int32>(EMythicDesireType::Defend)].TargetCell = DefendCell;
    LastDesires[static_cast<int32>(EMythicDesireType::Rest)].TargetCell = HomeCell;
    LastDesires[static_cast<int32>(EMythicDesireType::Patrol)].TargetCell = HomeCell;
    LastDesires[static_cast<int32>(EMythicDesireType::FollowSchedule)].TargetCell = CachedWorkCell;

    FMythicCellCoord AvengeCell = HomeCell;
    float BestGrievanceConfidence = 0.0f;
    for (const FMythicBelief &B : Beliefs) {
        if (B.Confidence > BestGrievanceConfidence) {
            BestGrievanceConfidence = B.Confidence;
            AvengeCell = B.Cell;
        }
    }
    LastDesires[static_cast<int32>(EMythicDesireType::Avenge)].TargetCell = AvengeCell;

    float TotalPressure = 0.0f;
    for (int32 i = 0; i < PressureChannelCount; ++i) {
        TotalPressure += PressureChannels[i];
    }
    if (Settings && TotalPressure > Settings->DespairThreshold) {
        for (FMythicDesire &D : LastDesires) {
            if (D.Type != EMythicDesireType::Flee && D.Type != EMythicDesireType::Rest) {
                D.Utility *= 0.1f;
            }
        }
    }
}

void UMythicCognitiveBrainComponent::ValidateIntention(double WorldTime) {
    if (!CurrentIntention.bValid) {
        return;
    }

    const double Elapsed = WorldTime - CurrentIntention.CommitTime;
    if (Elapsed > static_cast<double>(CurrentIntention.TimeoutSeconds)) {
        UE_LOG(LogMythCognition, Verbose, TEXT("Intention timed out: %d"), static_cast<int32>(CurrentIntention.Desire.Type));
        CurrentIntention.Reset();
    }
}

bool UMythicCognitiveBrainComponent::ShouldOverrideIntention(float BestUtility, float CurrentUtility, float Hysteresis) {
    return BestUtility >= CurrentUtility + Hysteresis;
}

void UMythicCognitiveBrainComponent::CommitIntention(double WorldTime) {
    int32 BestIndex = -1;
    float BestUtility = -1.0f;

    for (int32 i = 0; i < LastDesires.Num(); ++i) {
        if (LastDesires[i].Utility > BestUtility) {
            BestUtility = LastDesires[i].Utility;
            BestIndex = i;
        }
    }

    if (BestIndex < 0 || BestUtility <= 0.0f) {
        return;
    }

    const float Hysteresis = Settings ? Settings->DesireHysteresis : 0.2f;

    if (CurrentIntention.bValid) {
        const int32 CurrentDesireIndex = static_cast<int32>(CurrentIntention.Desire.Type);
        if (CurrentDesireIndex < LastDesires.Num()) {
            const float CurrentUtility = LastDesires[CurrentDesireIndex].Utility;
            if (!ShouldOverrideIntention(BestUtility, CurrentUtility, Hysteresis)) {
                return;
            }
        }
    }

    CurrentIntention.Desire = LastDesires[BestIndex];
    CurrentIntention.CommitTime = WorldTime;
    CurrentIntention.TimeoutSeconds = 30.0f;
    CurrentIntention.bStarted = false;
    CurrentIntention.bValid = true;

    UE_LOG(LogMythCognition, Verbose,
           TEXT("New intention: Type=%d, Utility=%.2f"),
           static_cast<int32>(CurrentIntention.Desire.Type),
           CurrentIntention.Desire.Utility);
}


float UMythicCognitiveBrainComponent::ScoreSurvive(double WorldTime) const {
    const float Threat = PressureChannels[static_cast<int32>(EMythicPressureChannel::Threat)];
    const float FleeWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Flee)];

    return Threat * Threat * (0.5f + FleeWeight) * 2.0f;
}

float UMythicCognitiveBrainComponent::ScoreDefend(double WorldTime) const {
    float CombatNearHome = 0.0f;
    for (const FMythicBelief &B : Beliefs) {
        if (FMath::Abs(B.Cell.X - HomeCell.X) <= 2 && FMath::Abs(B.Cell.Y - HomeCell.Y) <= 2) {
            CombatNearHome += B.Confidence * 0.5f;
        }
    }

    const float FightWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Fight)];
    const float LoyaltyBoost = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Rally)] * 0.3f;

    return CombatNearHome * (FightWeight + LoyaltyBoost) * 1.5f;
}

float UMythicCognitiveBrainComponent::ScoreAvenge(double WorldTime) const {
    const float Wrath = PressureChannels[static_cast<int32>(EMythicPressureChannel::Wrath)];
    const float Grief = PressureChannels[static_cast<int32>(EMythicPressureChannel::Grief)];
    const float FightWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Fight)];

    return (Wrath * 0.7f + Grief * 0.3f) * FightWeight * 1.8f;
}

float UMythicCognitiveBrainComponent::ScorePatrol(double WorldTime) const {
    const float EnforceWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Enforce)];

    return EnforceWeight * 0.6f;
}

float UMythicCognitiveBrainComponent::ScoreTrade(double WorldTime) const {
    const float ExploitWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Exploit)];

    const float Threat = PressureChannels[static_cast<int32>(EMythicPressureChannel::Threat)];
    const float SafetyMod = FMath::Max(0.0f, 1.0f - Threat);

    return ExploitWeight * SafetyMod * 0.5f;
}

float UMythicCognitiveBrainComponent::ScoreSocialize(double WorldTime) const {
    const float TendWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Tend)];
    const float RallyWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Rally)];

    return (TendWeight + RallyWeight) * 0.3f;
}

float UMythicCognitiveBrainComponent::ScoreJoinPlayer(double WorldTime) const {
    if (!SocialGraph || !FactionDB) {
        return 0.0f;
    }

    float Utility = 0.0f;

    TArray<FMythicSocialEdge> AllEdges;
    SocialGraph->GetEdges(SourceEntity, WorldTime, AllEdges);

    float BestPlayerRelationStrength = 0.0f;
    bool bHasLifeDebt = false;
    for (const FMythicSocialEdge &Edge : AllEdges) {
        if (Edge.Relation == EMythicSocialRelation::Debt) {
            bHasLifeDebt = true;
            BestPlayerRelationStrength = FMath::Max(BestPlayerRelationStrength, Edge.Strength);
        }
        else if (Edge.Relation == EMythicSocialRelation::Friend && Edge.Strength > 0.6f) {
            BestPlayerRelationStrength = FMath::Max(BestPlayerRelationStrength, Edge.Strength);
        }
    }

    if (BestPlayerRelationStrength < 0.1f) {
        return 0.0f;
    }

    Utility += bHasLifeDebt ? 0.5f : BestPlayerRelationStrength * 0.3f;

    FMythicFactionData FactionData;
    if (FactionDB->GetFaction(Faction, FactionData)) {
        if (FactionData.ControlledCellCount <= 1) {
            Utility += 0.3f;
        }
        else if (FactionData.Population < 10) {
            Utility += 0.2f;
        }
    }

    const float Threat = PressureChannels[static_cast<int32>(EMythicPressureChannel::Threat)];
    const float Grief = PressureChannels[static_cast<int32>(EMythicPressureChannel::Grief)];
    Utility += FMath::Min((Threat + Grief) * 0.15f, 0.3f);

    const float TendWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Tend)];
    const float RallyWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Rally)];
    Utility *= (0.5f + TendWeight * 0.3f + RallyWeight * 0.2f);

    return FMath::Max(0.0f, Utility);
}

float UMythicCognitiveBrainComponent::ScoreFlee(double WorldTime) const {
    const float Threat = PressureChannels[static_cast<int32>(EMythicPressureChannel::Threat)];
    const float FleeWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Flee)];
    const float FightWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Fight)];

    float FleeScore = Threat * FleeWeight * (1.0f - FightWeight * 0.5f) * 1.5f;

    if (Settings && SocialGraph) {
        TArray<FMythicSocialEdge> SocialEdges;
        if (SocialGraph->GetEdges(SourceEntity, WorldTime, SocialEdges)) {
            for (const FMythicSocialEdge &Edge : SocialEdges) {
                if (Edge.Relation == EMythicSocialRelation::Debt || Edge.Relation == EMythicSocialRelation::Family ||
                    Edge.Relation == EMythicSocialRelation::Friend) {
                    if (Edge.Strength > Settings->SacrificeThreshold) {
                        FleeScore *= 0.1f;
                        break;
                    }
                }
            }
        }
    }

    return FleeScore;
}

float UMythicCognitiveBrainComponent::ScoreRest(double WorldTime) const {
    if (CachedSchedulePhase == EMythicSchedulePhase::Rest) {
        return 0.8f;
    }
    return 0.1f;
}

float UMythicCognitiveBrainComponent::ScoreExploit(double WorldTime) const {
    const float ExploitWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Exploit)];
    const float Desire = PressureChannels[static_cast<int32>(EMythicPressureChannel::Desire)];

    float Opportunity = 0.0f;
    for (const FMythicBelief &B : Beliefs) {
        if (B.Confidence > 0.5f) {
            Opportunity += 0.1f;
        }
    }

    return FMath::Min((ExploitWeight * 0.5f + Desire * 0.3f) * (1.0f + Opportunity), RoutineDesireCeiling);
}

float UMythicCognitiveBrainComponent::ScoreRoutineDesire(float Weight, float Pressure, float Multiplier) {
    return FMath::Min(FMath::Max(0.0f, Weight * Pressure * Multiplier), RoutineDesireCeiling);
}

float UMythicCognitiveBrainComponent::ScoreRally(double WorldTime) const {
    const float RallyWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Rally)];
    const float Injustice = PressureChannels[static_cast<int32>(EMythicPressureChannel::Injustice)];

    return ScoreRoutineDesire(RallyWeight, Injustice, 1.5f);
}

float UMythicCognitiveBrainComponent::ScoreReport(double WorldTime) const {
    const float ReportWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Report)];
    const float Injustice = PressureChannels[static_cast<int32>(EMythicPressureChannel::Injustice)];

    return ScoreRoutineDesire(ReportWeight, Injustice, 1.2f);
}

float UMythicCognitiveBrainComponent::ScoreFollowSchedule(double WorldTime) const {
    return (CachedSchedulePhase == EMythicSchedulePhase::Work) ? 0.7f : 0.05f;
}
