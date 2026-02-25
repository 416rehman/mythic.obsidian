// Mythic Living World — Cognitive Brain Component Implementation
// BDI brain for Tier 2-3 NPCs. Staggered think tick, personality-biased beliefs,
// utility-scored desires, hysteretic intentions.

#include "AI/Cognition/CognitiveBrainComponent.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Engine/World.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Dialogue/DialogueSelector.h"
#include "World/LivingWorld/Dialogue/MythicDialogueTypes.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogMythCognition);

// ─────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────

UMythicCognitiveBrainComponent::UMythicCognitiveBrainComponent() {
    PrimaryComponentTick.bCanEverTick = false; // Uses timer, not tick
    bWantsInitializeComponent = false;
}

// ─────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────

void UMythicCognitiveBrainComponent::BeginPlay() {
    Super::BeginPlay();

    // Cache shared data references
    if (UGameInstance* GI = GetWorld()->GetGameInstance()) {
        if (UMythicLivingWorldSubsystem* LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
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

    // Stagger think timer — random offset prevents all NPCs from thinking at the same time
    const float MinInterval = Settings->CognitiveThinkIntervalMin;
    const float MaxInterval = Settings->CognitiveThinkIntervalMax;
    ThinkInterval = FMath::RandRange(MinInterval, MaxInterval);

    // Random initial delay for staggering
    const float InitialDelay = FMath::RandRange(0.0f, ThinkInterval);

    GetWorld()->GetTimerManager().SetTimer(
        ThinkTimerHandle,
        this,
        &UMythicCognitiveBrainComponent::Think,
        ThinkInterval,
        /*bLoop=*/true,
        InitialDelay);

    Beliefs.Reserve(MaxBeliefsPerNPC);
    LastDesires.Reserve(DesireTypeCount);
}

void UMythicCognitiveBrainComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(ThinkTimerHandle);
    }

    Super::EndPlay(EndPlayReason);
}

// ─────────────────────────────────────────────────────────────
// Initialization
// ─────────────────────────────────────────────────────────────

void UMythicCognitiveBrainComponent::InitializeBrain(
    FMythicFactionId InFaction,
    FMythicCellCoord InHomeCell,
    const FMythicPersonalityFragment& InPersonality,
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

// ─────────────────────────────────────────────────────────────
// External Events
// ─────────────────────────────────────────────────────────────

FText UMythicCognitiveBrainComponent::SelectDialogue(AActor* InteractingPlayer) const {
    if (!Settings || Settings->DialogueDatabase.IsNull()) {
        return FText::FromString(TEXT("..."));
    }

    UMythicDialogueDatabase* Database = Settings->DialogueDatabase.LoadSynchronous();
    if (!Database) return FText::FromString(TEXT("..."));

    FMythicDialogueContext Context;
    Context.RoleTag = Role;
    // Context.FactionTag = (Would extract FactionTag from FactionDB if available)
    
    // Determine dominant emotion from pressure
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
        return FText::FromString(TEXT("Hello."));
    }

    FMythicDialogueVariables Vars;
    if (FactionDB) {
        FMythicFactionData FData;
        if (FactionDB->GetFaction(Faction, FData)) {
            Vars.FactionName = FData.DisplayName.ToString();
        }
    }
    if (Context.DominantPressureChannel != -1) {
        // Fallback or explicit mapping can be placed here
        Vars.SpeakerMood = FString("Emotional"); 
    }
    
    return FMythicDialogueSelector::ResolveVariables(Result.Template->DialogueText, Vars);
}

void UMythicCognitiveBrainComponent::InjectBelief(const FMythicBelief& Belief) {
    // Check for duplicate — merge by taking max confidence
    for (FMythicBelief& Existing : Beliefs) {
        if (Existing.SourceEventId == Belief.SourceEventId && Existing.SourceEventId != 0) {
            Existing.Confidence = FMath::Max(Existing.Confidence, Belief.Confidence);
            return;
        }
    }

    // Evict weakest belief if at capacity
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

void UMythicCognitiveBrainComponent::OnSignificantEvent(const FGameplayTag& EventTag, FMythicCellCoord EventCell) {
    // Force immediate re-think
    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(ThinkTimerHandle);
        Think();

        // Re-arm the timer
        GetWorld()->GetTimerManager().SetTimer(
            ThinkTimerHandle,
            this,
            &UMythicCognitiveBrainComponent::Think,
            ThinkInterval,
            /*bLoop=*/true);
    }
}

// ─────────────────────────────────────────────────────────────
// Core BDI Loop
// ─────────────────────────────────────────────────────────────

void UMythicCognitiveBrainComponent::Think() {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicCognitiveBrain_Think);

    if (!bInitialized || !CausalFabric || !FactionDB) {
        return;
    }

    // Skip if already thinking asynchronously
    if (bIsThinkingAsync.exchange(true)) {
        return;
    }

    const double WorldTime = GetWorld()->GetTimeSeconds();

    // Launch background task for heavy BDI evaluation
    TWeakObjectPtr<UMythicCognitiveBrainComponent> WeakThis(this);
    AsyncThinkTask = UE::Tasks::Launch(
        TEXT("BDI_Think"),
        [this, WorldTime, WeakThis]() {
            // Step 1: Update beliefs from causal fabric (lock-free read)
            UpdateBeliefs(WorldTime);

            // Step 2: Score all desires
            ScoreDesires(WorldTime);

            // Step 3: Validate current intention
            ValidateIntention(WorldTime);

            // Step 4: Commit best desire as intention (with hysteresis)
            CommitIntention(WorldTime);

            // Chain a lightweight completion task back to the GameThread to apply tags
            AsyncTask(ENamedThreads::GameThread, [WeakThis]() {
                if (UMythicCognitiveBrainComponent* StrongThis = WeakThis.Get()) {
                    StrongThis->OnAsyncThinkCompleted();
                }
            });
        },
        UE::Tasks::ETaskPriority::BackgroundNormal
    );
}

void UMythicCognitiveBrainComponent::OnAsyncThinkCompleted() {
    bIsThinkingAsync.store(false);

    // Step 5: Emit Behavioral Emotion Tags based on pressure
    if (Settings) {
        AActor* Owner = GetOwner();
        IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner);
        if (ASI && ASI->GetAbilitySystemComponent()) {
            UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();

            const float TotalPressure = PressureChannels[0] + PressureChannels[1] +
                                        PressureChannels[2] + PressureChannels[3] +
                                        PressureChannels[4] + PressureChannels[5];

            // Despair check
            if (TotalPressure >= Settings->DespairThreshold) {
                ASC->AddLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_DESPAIR);
            } else {
                ASC->RemoveLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_DESPAIR);
            }

            // Fear check (high Threat)
            if (PressureChannels[static_cast<int32>(EMythicPressureChannel::Threat)] > 3.0f) {
                ASC->AddLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_FEAR);
            } else {
                ASC->RemoveLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_FEAR);
            }

            // Grief check (high Grief)
            if (PressureChannels[static_cast<int32>(EMythicPressureChannel::Grief)] > 3.0f) {
                ASC->AddLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_GRIEF);
            } else {
                ASC->RemoveLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_GRIEF);
            }

            // Joy check (zero pressure + socializing or high baseline utility)
            if (TotalPressure < 1.0f && CurrentIntention.bValid && 
                (CurrentIntention.Desire.Type == EMythicDesireType::Socialize ||
                 CurrentIntention.Desire.Type == EMythicDesireType::Rest)) {
                 
                ASC->AddLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_JOY);
            } else {
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

    // Query recent events near home cell (personality biases which events are noticed)
    TArray<const FMythicWorldEvent*> NearbyEvents;
    CausalFabric->QueryEventsByCell(
        HomeCell,
        WorldTime - 120.0, // Look back 2 minutes
        WorldTime,
        8, // Budget: max 8 events per think
        NearbyEvents);

    for (const FMythicWorldEvent* Event : NearbyEvents) {
        if (!Event) continue;

        // Personality bias: skip events that don't match this NPC's personality
        // Fight-heavy personality notices combat; Tend-heavy notices healing
        float RelevanceWeight = 0.3f; // Base relevance

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

        // Only form belief if relevance passes threshold (personality filter)
        if (RelevanceWeight < 0.4f) {
            continue;
        }

        FMythicBelief NewBelief;
        NewBelief.EventTag = Event->EventTag;
        NewBelief.Cell = Event->Cell;
        NewBelief.InvolvedFaction = Event->PrimaryFaction;
        NewBelief.Confidence = FMath::Clamp(Event->Significance * RelevanceWeight, 0.1f, 1.0f);
        NewBelief.FormationTime = WorldTime;
        NewBelief.PropagationHops = 0;
        NewBelief.SourceEventId = Event->EventId;

        // Spy routing: Spies evaluate events based on their true faction's stance,
        // rather than their cover faction.
        if (TrueFaction.IsValid() && TrueFaction != Faction) {
            NewBelief.InvolvedFaction = TrueFaction;
        }

        InjectBelief(NewBelief);
    }

    // Decay old beliefs
    for (int32 i = Beliefs.Num() - 1; i >= 0; --i) {
        const double Age = WorldTime - Beliefs[i].FormationTime;
        Beliefs[i].Confidence *= FMath::Exp(-0.005f * static_cast<float>(Age));

        if (Beliefs[i].Confidence < 0.05f) {
            Beliefs.RemoveAtSwap(i);
        }
    }
}

void UMythicCognitiveBrainComponent::ScoreDesires(double WorldTime) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicCognitiveBrain_ScoreDesires);

    LastDesires.Reset();
    LastDesires.SetNum(DesireTypeCount);

    // Score each desire type via dedicated utility functions
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

    // Despair Penalty: if pressure is too high, wipe all active intentions except Flee and Rest
    float TotalPressure = 0.0f;
    for (int32 i = 0; i < PressureChannelCount; ++i) {
        TotalPressure += PressureChannels[i];
    }
    if (Settings && TotalPressure > Settings->DespairThreshold) {
        for (FMythicDesire& D : LastDesires) {
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

    // Check timeout
    const double Elapsed = WorldTime - CurrentIntention.CommitTime;
    if (Elapsed > static_cast<double>(CurrentIntention.TimeoutSeconds)) {
        UE_LOG(LogMythCognition, Verbose, TEXT("Intention timed out: %d"), static_cast<int32>(CurrentIntention.Desire.Type));
        CurrentIntention.Reset();
    }
}

void UMythicCognitiveBrainComponent::CommitIntention(double WorldTime) {
    // Find highest-utility desire
    int32 BestIndex = -1;
    float BestUtility = -1.0f;

    for (int32 i = 0; i < LastDesires.Num(); ++i) {
        if (LastDesires[i].Utility > BestUtility) {
            BestUtility = LastDesires[i].Utility;
            BestIndex = i;
        }
    }

    if (BestIndex < 0 || BestUtility <= 0.0f) {
        return; // No viable desire
    }

    // Hysteresis: new desire must beat current intention by a margin to override
    constexpr float HysteresisMargin = 0.2f;

    if (CurrentIntention.bValid) {
        // Current intention's utility from last scoring
        const int32 CurrentDesireIndex = static_cast<int32>(CurrentIntention.Desire.Type);
        if (CurrentDesireIndex < LastDesires.Num()) {
            const float CurrentUtility = LastDesires[CurrentDesireIndex].Utility;
            if (BestUtility < CurrentUtility + HysteresisMargin) {
                return; // Not enough improvement to override
            }
        }
    }

    // Commit new intention
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

// ─────────────────────────────────────────────────────────────
// Utility Scoring Functions
// Formula: Utility = BaseWeight × PressureFactor × PersonalityBias × ContextModifier
// ─────────────────────────────────────────────────────────────

float UMythicCognitiveBrainComponent::ScoreSurvive(double WorldTime) const {
    // High Threat pressure → strong survive desire
    const float Threat = PressureChannels[static_cast<int32>(EMythicPressureChannel::Threat)];
    const float FleeWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Flee)];

    // Survive urgency scales quadratically with threat (panic response)
    return Threat * Threat * (0.5f + FleeWeight) * 2.0f;
}

float UMythicCognitiveBrainComponent::ScoreDefend(double WorldTime) const {
    // Count combat beliefs near home cell
    float CombatNearHome = 0.0f;
    for (const FMythicBelief& B : Beliefs) {
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

    // Avenge = Grief→Wrath crystallization × combat personality
    return (Wrath * 0.7f + Grief * 0.3f) * FightWeight * 1.8f;
}

float UMythicCognitiveBrainComponent::ScorePatrol(double WorldTime) const {
    const float EnforceWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Enforce)];

    // Guards have high Enforce → patrol is their baseline
    return EnforceWeight * 0.6f;
}

float UMythicCognitiveBrainComponent::ScoreTrade(double WorldTime) const {
    const float ExploitWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Exploit)];

    // Merchants score trade from Exploit personality + low threat
    const float Threat = PressureChannels[static_cast<int32>(EMythicPressureChannel::Threat)];
    const float SafetyMod = FMath::Max(0.0f, 1.0f - Threat);

    return ExploitWeight * SafetyMod * 0.5f;
}

float UMythicCognitiveBrainComponent::ScoreSocialize(double WorldTime) const {
    const float TendWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Tend)];
    const float RallyWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Rally)];

    // Social NPCs want to interact — baseline desire
    return (TendWeight + RallyWeight) * 0.3f;
}

float UMythicCognitiveBrainComponent::ScoreJoinPlayer(double WorldTime) const {
    // Emergent recruitment — requires specific conditions:
    // 1. Social connection to player (life debt or friendship from SocialGraph)
    // 2. Faction in distress (collapse, annihilation, low territory)
    // 3. High personal threat (accumulated Threat/Grief pressure)
    // 4. Moral alignment with player (ideology dot product)

    if (!SocialGraph || !FactionDB) {
        return 0.0f;
    }

    float Utility = 0.0f;

    // ---- Factor 1: Social connection to a player ----
    // Check if any edge connected to this NPC involves a player entity (via Debt or Friend relation)
    TArray<FMythicSocialEdge> AllEdges;
    SocialGraph->GetEdges(SourceEntity, WorldTime, AllEdges);

    float BestPlayerRelationStrength = 0.0f;
    bool bHasLifeDebt = false;
    for (const FMythicSocialEdge& Edge : AllEdges) {
        // In a full implementation, we'd check if Edge.TargetEntity is a player.
        // For now, Debt edges and strong Friend edges contribute.
        if (Edge.Relation == EMythicSocialRelation::Debt) {
            bHasLifeDebt = true;
            BestPlayerRelationStrength = FMath::Max(BestPlayerRelationStrength, Edge.Strength);
        } else if (Edge.Relation == EMythicSocialRelation::Friend && Edge.Strength > 0.6f) {
            BestPlayerRelationStrength = FMath::Max(BestPlayerRelationStrength, Edge.Strength);
        }
    }

    // No social connection to anyone meaningful = no interest in joining
    if (BestPlayerRelationStrength < 0.1f) {
        return 0.0f;
    }

    // Life debt is a strong motivator
    Utility += bHasLifeDebt ? 0.5f : BestPlayerRelationStrength * 0.3f;

    // ---- Factor 2: Faction distress ----
    FMythicFactionData FactionData;
    if (FactionDB->GetFaction(Faction, FactionData)) {
        // Faction losing territory or about to be annihilated
        if (FactionData.ControlledCellCount <= 1) {
            Utility += 0.3f; // Faction is collapsing
        } else if (FactionData.Population < 10) {
            Utility += 0.2f; // Faction is tiny
        }
    }

    // ---- Factor 3: Personal pressure ----
    const float Threat = PressureChannels[static_cast<int32>(EMythicPressureChannel::Threat)];
    const float Grief = PressureChannels[static_cast<int32>(EMythicPressureChannel::Grief)];
    // High personal danger or grief makes joining a protector attractive
    Utility += FMath::Min((Threat + Grief) * 0.15f, 0.3f);

    // ---- Factor 4: Personality fit ----
    // Sociable and loyal personality types are more likely to join
    const float TendWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Tend)];
    const float RallyWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Rally)];
    Utility *= (0.5f + TendWeight * 0.3f + RallyWeight * 0.2f);

    return FMath::Max(0.0f, Utility);
}

float UMythicCognitiveBrainComponent::ScoreFlee(double WorldTime) const {
    const float Threat = PressureChannels[static_cast<int32>(EMythicPressureChannel::Threat)];
    const float FleeWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Flee)];
    const float FightWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Fight)];

    // Flee scales with threat, biased by personality (cowardly vs brave)
    // High Fight weight suppresses flee desire
    float FleeScore = Threat * FleeWeight * (1.0f - FightWeight * 0.5f) * 1.5f;

    // Sacrifice override: if loyalty to a nearby entity is high, we might sacrifice self-preservation
    if (Settings && SocialGraph) {
        TArray<FMythicSocialEdge> SocialEdges;
        if (SocialGraph->GetEdges(SourceEntity, GetWorld()->GetTimeSeconds(), SocialEdges)) {
            for (const FMythicSocialEdge& Edge : SocialEdges) {
                if (Edge.Relation == static_cast<EMythicSocialRelation>(1) /*LifeDebt/Family*/ || Edge.Relation == EMythicSocialRelation::Friend) {
                    if (Edge.Strength > Settings->SacrificeThreshold) {
                        FleeScore *= 0.1f; // Suppress flee to defend the friend
                        break;
                    }
                }
            }
        }
    }

    return FleeScore;
}

float UMythicCognitiveBrainComponent::ScoreRest(double WorldTime) const {
    // Low urgency baseline — becomes relevant during rest schedule phases
    return 0.1f;
}

float UMythicCognitiveBrainComponent::ScoreExploit(double WorldTime) const {
    const float ExploitWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Exploit)];
    const float Desire = PressureChannels[static_cast<int32>(EMythicPressureChannel::Desire)];

    // Opportunistic exploitation — higher when nearby events create opportunity
    float Opportunity = 0.0f;
    for (const FMythicBelief& B : Beliefs) {
        if (B.Confidence > 0.5f) {
            Opportunity += 0.1f;
        }
    }

    return (ExploitWeight * 0.5f + Desire * 0.3f) * (1.0f + Opportunity);
}

float UMythicCognitiveBrainComponent::ScoreRally(double WorldTime) const {
    const float RallyWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Rally)];
    const float Injustice = PressureChannels[static_cast<int32>(EMythicPressureChannel::Injustice)];

    // Leaders with high injustice rally others
    return RallyWeight * Injustice * 1.5f;
}

float UMythicCognitiveBrainComponent::ScoreReport(double WorldTime) const {
    const float ReportWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Report)];
    const float Injustice = PressureChannels[static_cast<int32>(EMythicPressureChannel::Injustice)];

    // Authority-compliant NPCs report crimes
    return ReportWeight * Injustice * 1.2f;
}

float UMythicCognitiveBrainComponent::ScoreFollowSchedule(double WorldTime) const {
    // Default low-priority fallback. Always available.
    // Adjusted down when other desires are more urgent.
    const float TotalPressure = PressureChannels[0] + PressureChannels[1] +
                                PressureChannels[2] + PressureChannels[3] +
                                PressureChannels[4] + PressureChannels[5];

    // Schedule becomes less attractive as pressure rises
    return FMath::Max(0.05f, 0.4f - TotalPressure * 0.1f);
}
