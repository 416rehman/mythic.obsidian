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
#include "World/LivingWorld/Chronicle/MythicWorldChronicleSubsystem.h" // EventTagToReadable for the {recent_event} var
#include "World/LivingWorld/Settlements/MythicSettlement.h" // FMythicSettlementData for the {settlement_name} var
#include "TimerManager.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY(LogMythCognition);

// ─────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────

UMythicCognitiveBrainComponent::UMythicCognitiveBrainComponent() {
    PrimaryComponentTick.bCanEverTick = false; // Uses timer, not tick
    bWantsInitializeComponent = false;
    // Default to Idle (NOT the enum's first value Work) so a not-yet-thought / non-embodied brain never scores the
    // Work-phase boost in ScoreFollowSchedule and routes to a default (0,0) WorkCell. Think() overwrites it.
    CachedSchedulePhase = EMythicSchedulePhase::Idle;
}

// ─────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────

void UMythicCognitiveBrainComponent::BeginPlay() {
    Super::BeginPlay();

    // Cache shared data references
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
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(ThinkTimerHandle);

        // Remove the entity->actor reverse link on ANY actor teardown path (combat death, level/world teardown,
        // seamless travel, explicit Destroy) — not just the demotion despawn loop, which is the only OTHER site
        // that unregisters. SourceEntity is only set on the server (the brain inits authority-side), so this is a
        // no-op on clients. Without this the map accumulates stale FMassEntityHandle keys over a long session.
        if (SourceEntity.IsSet()) {
            if (UGameInstance *GI = World->GetGameInstance()) {
                if (UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                    LWS->UnregisterEmbodiedActor(SourceEntity);
                }
            }
        }
    }

    // Join the in-flight background BDI task before destruction. Its body dereferences raw `this`
    // (PressureChannels[], Settings, Beliefs, Personality) on a worker thread; without this wait the component
    // can be GC'd mid-think on a no-delay teardown path → cross-thread use-after-free. Wait() is safe on the game
    // thread and the body is ~hundreds of ops, so the stall is negligible.
    if (AsyncThinkTask.IsValid() && !AsyncThinkTask.IsCompleted()) {
        AsyncThinkTask.Wait();
    }

    Super::EndPlay(EndPlayReason);
}

void UMythicCognitiveBrainComponent::StopThinking() {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(ThinkTimerHandle);
    }
    // Double-guard: any think tick already queued for this frame will see bInitialized=false and early-return.
    bInitialized = false;

    // Join any in-flight background BDI task so the worker is done touching `this` before the corpse is destroyed
    // (StopThinking is the documented pre-corpse-destroy halt). A weak-pointer check alone is insufficient — the
    // object can be freed mid-body — so we block here on the game thread.
    if (AsyncThinkTask.IsValid() && !AsyncThinkTask.IsCompleted()) {
        AsyncThinkTask.Wait();
    }
}

// ─────────────────────────────────────────────────────────────
// Initialization
// ─────────────────────────────────────────────────────────────

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

// ─────────────────────────────────────────────────────────────
// External Events
// ─────────────────────────────────────────────────────────────

FText UMythicCognitiveBrainComponent::GetDisplayName() const {
    // Server-side read of the source entity's identity fragment → the deterministic generated name (real, not
    // fabricated). The single source for {npc_name} (SelectDialogue) AND the companion party display name.
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
    // A brain that was never InitializeBrain'd (e.g. a pooled/designer NPC, or a client where the brain inits
    // server-side only) has default Role/Faction/pressure — scoring against that empty context would silently
    // return a generic line. Return the honest fallback deterministically instead. (Callers should pick the line
    // on the SERVER where the brain is initialized — see AMythicPlayerController::ServerRequestNpcDialogue.)
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

    // Companion moral commentary (REQ-PTY-002): when the party subsystem detects the player's action strongly moved a
    // companion's loyalty, it asks that companion to remark — this flips the template filter to the commentary family,
    // gated by how morally-charged the act was. Defaults (false/0) reproduce a normal player-initiated bark exactly.
    Context.bIsCompanionCommentary = bCompanionCommentary;
    Context.PlayerActionMoralScore = PlayerActionMoralScore;

    // Resolve the faction's gameplay tag from the faction DB BEFORE template selection — otherwise Context.FactionTag
    // stays empty and SelectTemplate hard-filters out EVERY faction-gated template (RequiredFaction never matches an
    // empty tag), so faction-specific dialogue could never be chosen. The data is reused below for FactionName.
    FMythicFactionData FactionData;
    const bool bHasFactionData = (FactionDB && FactionDB->GetFaction(Faction, FactionData));
    if (bHasFactionData) {
        Context.FactionTag = FactionData.FactionTag;
    }

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
        // In commentary mode, no matching commentary template means this companion has nothing to say about the act —
        // return empty so the caller skips the bark rather than forcing a generic "Hello.".
        return bCompanionCommentary ? FText::GetEmpty() : FText::FromString(TEXT("Hello."));
    }

    FMythicDialogueVariables Vars;
    if (bHasFactionData) {
        Vars.FactionName = FactionData.DisplayName.ToString();
    }

    // {speaker_mood}: derive a mood word from the dominant pressure channel (interpreting each channel's real
    // meaning — not invented lore). Replaces the old hardcoded "Emotional".
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

    // {player_reputation_descriptor}: bucket the interacting player's standing toward THIS NPC's faction using the
    // existing Hostile/Friendly thresholds (single source of truth — same buckets as the AI attitude routing).
    if (const APlayerController *PC = Cast<APlayerController>(InteractingPlayer)) {
        if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(PC->PlayerState)) {
            // {target_name}: who the NPC is ADDRESSING. This dialogue is always player-initiated (InteractingPlayer),
            // so the addressee is the player — their (character) name, the same one the save sets via SetPlayerName.
            // This was the ONE template var the brain never filled, so any "{target_name}" template rendered blank.
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

    // {npc_name}: the NPC's deterministic generated name (single source — see GetDisplayName).
    Vars.NPCName = GetDisplayName().ToString();

    // {recent_event}: the readable leaf of the NPC's single STRONGEST belief — the most salient event it witnessed —
    // via the SAME Chronicle transform the world news feed uses (single source: EventTagToReadable). Beliefs are formed
    // from real fabric events (UpdateBeliefs); GetBeliefsCopy() snapshots under BeliefsLock since this dialogue read
    // runs on the game thread while the brain may be mid async-think. Empty stays empty (template var just blanks).
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

    // {settlement_name}: the NPC's HOME-cell settlement display name. Snapshotted under SimulationLock via the
    // subsystem copy-out accessor (CopySettlementAtCell) — the sim thread mutates Settlements during conquest, so a
    // raw GetSettlementAtCell read here would race it. GetSubsystem returns a non-const ptr even from this const method.
    // Empty stays empty (an NPC whose home cell is not governed by a settlement just blanks the var).
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
    // Thread-safe entry for game-thread callers (party propagation / betrayal): serialize against the async think
    // worker, which mutates + iterates Beliefs under this same lock.
    FScopeLock Lock(&BeliefsLock);
    InjectBeliefInternal(Belief);
}

TArray<FMythicBelief> UMythicCognitiveBrainComponent::GetBeliefsCopy() const {
    FScopeLock Lock(&BeliefsLock);
    return Beliefs;
}

void UMythicCognitiveBrainComponent::InjectBeliefInternal(const FMythicBelief &Belief) {
    // Caller must hold BeliefsLock. Check for duplicate — merge by taking max confidence
    for (FMythicBelief &Existing : Beliefs) {
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

void UMythicCognitiveBrainComponent::OnSignificantEvent(const FGameplayTag &EventTag, FMythicCellCoord EventCell) {
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

    // Pull the live psychodynamic pressure from MASS into the brain's working array — on the GAME THREAD, BEFORE
    // the async BDI task launches below. The lambda (UpdateBeliefs/ScoreDesires) reads PressureChannels[] off a
    // background task, so the MASS EntityManager read must happen here (reading it inside the task would be an
    // unsafe cross-thread access). The PressureProcessor writes FMythicPsychodynamicFragment::Pressure[] every sim
    // tick; without this copy the entire emotional model (Fear/Grief/Despair -> desire utilities + the despair
    // penalty) scores against all-zeros. Indexed by the shared PressureChannelCount so the arrays are guaranteed
    // compatible (single source of truth). If the entity/fragment is absent (non-embodied), pressure stays zero —
    // the legitimate empty state, not a fabricated value.
    if (SourceEntity.IsSet()) {
        if (UMassEntitySubsystem *Ess = UWorld::GetSubsystem<UMassEntitySubsystem>(GetWorld())) {
            if (Ess->GetEntityManager().IsEntityValid(SourceEntity)) {
                if (const FMythicPsychodynamicFragment *Psyche =
                    Ess->GetEntityManager().GetFragmentDataPtr<FMythicPsychodynamicFragment>(SourceEntity)) {
                    for (int32 i = 0; i < PressureChannelCount; ++i) {
                        PressureChannels[i] = Psyche->Pressure[i];
                    }
                }
                // Cache the schedule phase the SAME cross-thread-safe way (game-thread read here; the async scorer
                // ScoreRest consumes it). The ScheduleTransitionProcessor writes Phase every sim tick.
                if (const FMythicScheduleFragment *Schedule =
                    Ess->GetEntityManager().GetFragmentDataPtr<FMythicScheduleFragment>(SourceEntity)) {
                    CachedSchedulePhase = Schedule->Phase;
                    CachedWorkCell = Schedule->WorkCell;
                }
            }
        }
    }

    // Launch background task for heavy BDI evaluation
    TWeakObjectPtr<UMythicCognitiveBrainComponent> WeakThis(this);
    AsyncThinkTask = UE::Tasks::Launch(
        TEXT("BDI_Think"),
        [this, WorldTime, WeakThis]() {
            {
                // Hold BeliefsLock across the whole belief-touching pass: UpdateBeliefs decays/adds + ScoreDesires
                // iterates Beliefs here on the worker, while the GAME thread may InjectBelief / GetBeliefsCopy. One
                // coarse lock (contention is rare — only party rest-phase/betrayal) avoids a TArray realloc racing an
                // iterator. UpdateBeliefs calls the lock-free InjectBeliefInternal (no recursion on this non-recursive lock).
                FScopeLock BeliefsScope(&BeliefsLock);

                // Step 1: Update beliefs from causal fabric (lock-free read)
                UpdateBeliefs(WorldTime);

                // Step 2: Score all desires
                ScoreDesires(WorldTime);
            }

            // Steps 3-4 (Validate + Commit) are deliberately NOT done here: they write CurrentIntention, which the
            // game thread reads (AIController flee gate + the emotion-tag logic). Doing them on this worker would be
            // a data race on CurrentIntention. They run in OnAsyncThinkCompleted on the game thread instead, so
            // CurrentIntention is only ever touched on the game thread. LastDesires (written above) is read there
            // too, safely, because this worker has finished before the completion task runs.
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
    // Steps 3-4 on the GAME THREAD: validate + commit the intention here (not on the worker) so CurrentIntention is
    // only ever written on the game thread — the AIController + emotion-tag logic read it here. Do this BEFORE
    // clearing bIsThinkingAsync, so a new Think() can't launch a worker that writes LastDesires while we read it.
    ValidateIntention(WorldTime);
    CommitIntention(WorldTime);

    bIsThinkingAsync.store(false);

    // Step 5: Emit Behavioral Emotion Tags based on pressure
    if (Settings) {
        AActor *Owner = GetOwner();
        IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(Owner);
        if (ASI && ASI->GetAbilitySystemComponent()) {
            UAbilitySystemComponent *ASC = ASI->GetAbilitySystemComponent();

            const float TotalPressure = PressureChannels[0] + PressureChannels[1] +
                PressureChannels[2] + PressureChannels[3] +
                PressureChannels[4] + PressureChannels[5];

            // Despair check
            if (TotalPressure >= Settings->DespairThreshold) {
                ASC->AddLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_DESPAIR);
            }
            else {
                ASC->RemoveLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_DESPAIR);
            }

            // Fear check (high Threat)
            if (PressureChannels[static_cast<int32>(EMythicPressureChannel::Threat)] > 3.0f) {
                ASC->AddLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_FEAR);
            }
            else {
                ASC->RemoveLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_FEAR);
            }

            // Grief check (high Grief)
            if (PressureChannels[static_cast<int32>(EMythicPressureChannel::Grief)] > 3.0f) {
                ASC->AddLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_GRIEF);
            }
            else {
                ASC->RemoveLooseGameplayTag(TAG_LIVINGWORLD_EMOTION_GRIEF);
            }

            // Joy check (zero pressure + socializing or high baseline utility)
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

    // Query recent events near home cell (personality biases which events are noticed)
    TArray<FMythicWorldEvent> NearbyEvents;
    CausalFabric->QueryEventsByCell(
        HomeCell,
        WorldTime - 120.0, // Look back 2 minutes
        WorldTime,
        8, // Budget: max 8 events per think
        NearbyEvents);

    for (const FMythicWorldEvent &EventRef : NearbyEvents) {
        const FMythicWorldEvent *Event = &EventRef; // value copies (no ReadBuffer aliasing); ptr alias keeps body unchanged

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
        NewBelief.LastDecayTime = WorldTime; // decay against the delta since this, NOT the full age (see the decay loop)
        NewBelief.PropagationHops = 0;
        NewBelief.SourceEventId = Event->EventId;

        // Spy routing: Spies evaluate events based on their true faction's stance,
        // rather than their cover faction.
        if (TrueFaction.IsValid() && TrueFaction != Faction) {
            NewBelief.InvolvedFaction = TrueFaction;
        }

        InjectBeliefInternal(NewBelief); // already under BeliefsLock (held across the whole think on the worker)
    }

    // Decay old beliefs by the time DELTA since their last decay, NOT the full age — otherwise the per-tick exp(-rate*age)
    // multipliers compound (Σ ages grows quadratically) and beliefs evaporate in ~tens of seconds instead of the intended
    // ~10 min half-life. Telescoping: Π exp(-rate*Δᵢ) = exp(-rate*Σ Δᵢ) = exp(-rate*totalAge), the intended single curve.
    // Rate + prune floor are data-driven (LivingWorldSettings); fall back to the prior hardcoded values if unset.
    const float DecayRate = Settings ? Settings->BeliefConfidenceDecayRate : 0.005f;
    const float PruneThreshold = Settings ? Settings->BeliefPruneThreshold : 0.05f;
    for (int32 i = Beliefs.Num() - 1; i >= 0; --i) {
        double &Last = Beliefs[i].LastDecayTime;
        if (Last <= 0.0) {
            // Seed for beliefs injected without LastDecayTime set (InjectBelief/InjectBeliefInternal callers) — use the
            // formation time so the first decay uses a correct delta, not a huge one-shot age.
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
    // Clamp the delta at 0 so a clock that went backwards (or an out-of-order LastDecayTime) can never AMPLIFY
    // confidence via a positive exponent. exp's additivity gives the telescoping property documented above.
    const double Delta = FMath::Max(0.0, DeltaSeconds);
    return Confidence * FMath::Exp(-DecayRate * static_cast<float>(Delta));
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

    // Give the HOME-ANCHORED desires a concrete move target so the AI controller's idle dispatch can act on them
    // (Defend = engage the threat near home, Rest = return home, Patrol = the ANCHOR the controller walks a ring
    // around). HomeCell is the NPC's real home cell. Report/Rally/etc. still have no authored cell source, so they
    // stay target-less (the controller's idle dispatch never moves on a defaulted (0,0) cell).
    //
    // Defend routes to the witnessed THREAT cell — the highest-confidence belief near home (same within-2-cells
    // predicate ScoreDefend scores from) — so the NPC moves to WHERE the threat is rather than passively retreating
    // home; it falls back to HomeCell when no belief sits near home. Beliefs are formed from real fabric events
    // (UpdateBeliefs), so this is a grounded producer, and Beliefs is read here on the same async think pass that
    // updated it — no cross-thread access.
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
    // Patrol anchors on HomeCell (the guard's post); the AI controller walks a bounded ring of neighbour cells around
    // it (TickIdleBehavior), so an Enforce-driven guard with nothing acute to do actively patrols instead of standing.
    LastDesires[static_cast<int32>(EMythicDesireType::Patrol)].TargetCell = HomeCell;
    // FollowSchedule heads to the WorkCell (set high only during the Work phase, which only an embodied brain with a
    // real cached WorkCell reaches — see ScoreFollowSchedule).
    LastDesires[static_cast<int32>(EMythicDesireType::FollowSchedule)].TargetCell = CachedWorkCell;

    // Avenge routes to the GRIEVANCE cell — the single highest-confidence belief, wherever it is (NO near-home
    // restriction like Defend: a vengeful NPC travels to the site of the wrong). Falls back to HomeCell when the brain
    // holds no beliefs. Drives OUT-OF-COMBAT movement only (in combat the AIController's engage logic owns Avenge); the
    // controller's TickIdleBehavior accepts Avenge alongside Defend/Rest/FollowSchedule.
    FMythicCellCoord AvengeCell = HomeCell;
    float BestGrievanceConfidence = 0.0f;
    for (const FMythicBelief &B : Beliefs) {
        if (B.Confidence > BestGrievanceConfidence) {
            BestGrievanceConfidence = B.Confidence;
            AvengeCell = B.Cell;
        }
    }
    LastDesires[static_cast<int32>(EMythicDesireType::Avenge)].TargetCell = AvengeCell;

    // Despair Penalty: if pressure is too high, wipe all active intentions except Flee and Rest
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

    // Check timeout
    const double Elapsed = WorldTime - CurrentIntention.CommitTime;
    if (Elapsed > static_cast<double>(CurrentIntention.TimeoutSeconds)) {
        UE_LOG(LogMythCognition, Verbose, TEXT("Intention timed out: %d"), static_cast<int32>(CurrentIntention.Desire.Type));
        CurrentIntention.Reset();
    }
}

bool UMythicCognitiveBrainComponent::ShouldOverrideIntention(float BestUtility, float CurrentUtility, float Hysteresis) {
    // Override only when the new utility clears the current by the full margin (mirrors the prior
    // `BestUtility < CurrentUtility + Hysteresis → keep`). >= so an exact tie-plus-margin still overrides.
    return BestUtility >= CurrentUtility + Hysteresis;
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

    // Hysteresis: a new desire must beat the current intention by a margin to override (prevents desire flicker when
    // two desires are near-tied). Data-driven (LivingWorldSettings::DesireHysteresis) — the margin was previously a
    // hardcoded 0.2 that silently shadowed the designer setting; fall back to that value if Settings is unset.
    const float Hysteresis = Settings ? Settings->DesireHysteresis : 0.2f;

    if (CurrentIntention.bValid) {
        // Current intention's utility from last scoring
        const int32 CurrentDesireIndex = static_cast<int32>(CurrentIntention.Desire.Type);
        if (CurrentDesireIndex < LastDesires.Num()) {
            const float CurrentUtility = LastDesires[CurrentDesireIndex].Utility;
            if (!ShouldOverrideIntention(BestUtility, CurrentUtility, Hysteresis)) {
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
    for (const FMythicSocialEdge &Edge : AllEdges) {
        // In a full implementation, we'd check if Edge.TargetEntity is a player.
        // For now, Debt edges and strong Friend edges contribute.
        if (Edge.Relation == EMythicSocialRelation::Debt) {
            bHasLifeDebt = true;
            BestPlayerRelationStrength = FMath::Max(BestPlayerRelationStrength, Edge.Strength);
        }
        else if (Edge.Relation == EMythicSocialRelation::Friend && Edge.Strength > 0.6f) {
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
        }
        else if (FactionData.Population < 10) {
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
        // Use the game-thread-captured WorldTime param — NOT GetWorld()->GetTimeSeconds(), which would read UWorld
        // state off the BDI worker thread (a non-atomic cross-thread read / UB). Matches ScoreJoinPlayer.
        if (SocialGraph->GetEdges(SourceEntity, WorldTime, SocialEdges)) {
            for (const FMythicSocialEdge &Edge : SocialEdges) {
                // Suppress flee to defend kin, life-debtors, and friends. (The prior static_cast<EMythicSocialRelation>(1)
                // "LifeDebt" was a magic-ordinal bug: 1 is Family, while a life debt is Debt(=3) — so debtors never
                // triggered self-sacrifice, unlike the companion scorer above which correctly uses the named Debt.)
                if (Edge.Relation == EMythicSocialRelation::Debt || Edge.Relation == EMythicSocialRelation::Family ||
                    Edge.Relation == EMythicSocialRelation::Friend) {
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
    // Rises sharply during the Rest schedule phase (night) so an idle NPC commits to Rest — the AI controller's idle
    // dispatch then routes it to its HomeCell (Rest's TargetCell, populated in ScoreDesires). A real combat/threat
    // desire (Survive/Flee/Defend) still scores higher and overrides it. CachedSchedulePhase is copied on the game
    // thread in Think(). Low baseline outside the Rest phase.
    if (CachedSchedulePhase == EMythicSchedulePhase::Rest) {
        return 0.8f;
    }
    return 0.1f;
}

float UMythicCognitiveBrainComponent::ScoreExploit(double WorldTime) const {
    const float ExploitWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Exploit)];
    const float Desire = PressureChannels[static_cast<int32>(EMythicPressureChannel::Desire)];

    // Opportunistic exploitation — higher when nearby events create opportunity
    float Opportunity = 0.0f;
    for (const FMythicBelief &B : Beliefs) {
        if (B.Confidence > 0.5f) {
            Opportunity += 0.1f;
        }
    }

    // Routine opportunism is a BASELINE desire, but the (1 + Opportunity) multiplier is unbounded (up to ~2.6 in a
    // belief-dense cell), which would let a high-Exploit NPC silently outscore the daily schedule (FollowSchedule=0.7,
    // Rest=0.8) and never walk to work/home. Cap it below the schedule anchors (RoutineDesireCeiling) so only ACUTE
    // threat desires (Survive/Flee/Defend/Avenge — intentionally unbounded) outscore the schedule. Patrol/Trade/
    // Socialize already top out ≤0.6; Rally/Report ARE capped too now (via ScoreRoutineDesire) — they were previously
    // unbounded (the earlier "other routine scorers already top out" note missed them).
    return FMath::Min((ExploitWeight * 0.5f + Desire * 0.3f) * (1.0f + Opportunity), RoutineDesireCeiling);
}

float UMythicCognitiveBrainComponent::ScoreRoutineDesire(float Weight, float Pressure, float Multiplier) {
    // Shared cap for the simple routine scorers (Rally/Report) so they never outscore the schedule/acute desires —
    // see RoutineDesireCeiling. Max(0) guards a future signed input; weights + pressures are >= 0 today.
    return FMath::Min(FMath::Max(0.0f, Weight * Pressure * Multiplier), RoutineDesireCeiling);
}

float UMythicCognitiveBrainComponent::ScoreRally(double WorldTime) const {
    const float RallyWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Rally)];
    const float Injustice = PressureChannels[static_cast<int32>(EMythicPressureChannel::Injustice)];

    // Leaders with high injustice rally others. Capped to RoutineDesireCeiling: Rally is ROUTINE (not in the acute
    // Survive/Flee/Defend/Avenge set), so a high-Injustice NPC must not abandon its schedule to commit Rally — the
    // controller has no movement for it, so it would idle in place. Was previously unbounded.
    return ScoreRoutineDesire(RallyWeight, Injustice, 1.5f);
}

float UMythicCognitiveBrainComponent::ScoreReport(double WorldTime) const {
    const float ReportWeight = Personality.VentWeights[static_cast<int32>(EMythicVentChannel::Report)];
    const float Injustice = PressureChannels[static_cast<int32>(EMythicPressureChannel::Injustice)];

    // Authority-compliant NPCs report crimes. Capped to RoutineDesireCeiling (routine desire — see ScoreRally); was
    // previously unbounded, letting a high-Injustice NPC outscore its schedule to commit Report (no controller
    // movement → idles in place).
    return ScoreRoutineDesire(ReportWeight, Injustice, 1.2f);
}

float UMythicCognitiveBrainComponent::ScoreFollowSchedule(double WorldTime) const {
    // Rises during the Work schedule phase so an idle NPC commits to it + the AI controller routes it to its WorkCell
    // (FollowSchedule's TargetCell, populated in ScoreDesires). A real threat (Survive/Flee/Defend) still outscores it.
    // Outside the Work phase it stays a near-zero baseline so it never wins idle arbitration with a stale/unset
    // WorkCell (which would steer to (0,0)). CachedSchedulePhase is copied on the game thread in Think().
    return (CachedSchedulePhase == EMythicSchedulePhase::Work) ? 0.7f : 0.05f;
}
