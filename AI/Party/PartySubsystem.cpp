// Mythic Living World — Party Subsystem Implementation
// Companion management with loyalty, betrayal, and belief sharing.

#include "AI/Party/PartySubsystem.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/NPCs/MythicAIController.h" // arm/disarm the companion follow timer
#include "Player/MythicPlayerController.h" // departure/betrayal loss callouts
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
// Slice-7: post-load companion entity re-creation (create + force-hydrate to Tier2 + request embodiment)
#include "MassEntitySubsystem.h"
#include "MassCommandBuffer.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h" // GetLeaderPawn: first local player's pawn
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h" // nightfall rest trigger (the only real one)
#include "World/EnvironmentController/MythicEnvironmentController.h" // DayTimeChangeDelegate

DEFINE_LOG_CATEGORY(LogMythParty);

// ─────────────────────────────────────────────────────────────
// Subsystem Lifecycle
// ─────────────────────────────────────────────────────────────

bool UMythicPartySubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    if (const UWorld *World = Cast<UWorld>(Outer)) {
        return World->IsGameWorld();
    }
    return false;
}

void UMythicPartySubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    if (UGameInstance *GI = GetWorld()->GetGameInstance()) {
        if (UMythicLivingWorldSubsystem *LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            LivingWorld = LW; // game-thread event writes route through LW->SubmitWorldEvent (thread-safe queue)
            CausalFabric = LW->GetCausalFabric();
            SocialGraph = LW->GetSocialGraph();
            Settings = LW->GetSettings();
        }
    }

    if (!CausalFabric || !Settings) {
        UE_LOG(LogMythParty, Warning, TEXT("PartySubsystem: Living world references not available. Party disabled."));
        return;
    }

    // Load configuration from settings
    MaxPartySize = Settings->MaxPartySize;
    LoyaltyDepartureThreshold = Settings->LoyaltyDepartureThreshold;
    BetrayalThreshold = Settings->BetrayalPressureThreshold;
    BeliefPropagationDecay = Settings->BeliefPropagationDecay;

    UE_LOG(LogMythParty, Log, TEXT("PartySubsystem initialized: MaxPartySize=%d"), MaxPartySize);
}

void UMythicPartySubsystem::Deinitialize() {
    if (AMythicEnvironmentController *Controller = BoundEnvController.Get()) {
        Controller->DayTimeChangeDelegate.RemoveDynamic(this, &UMythicPartySubsystem::OnDaytimeChanged);
    }
    BoundEnvController.Reset();
    PlayerParties.Empty();
    // Slice-7: tear down the post-load companion-rebuild ticker if it is still running.
    if (CompanionRebuildTickHandle.IsValid()) {
        FTSTicker::GetCoreTicker().RemoveTicker(CompanionRebuildTickHandle);
        CompanionRebuildTickHandle.Reset();
    }
    Super::Deinitialize();
}

void UMythicPartySubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);

    // Server-authoritative only: the DayTimeChangeDelegate fires on server AND clients (it's broadcast inside a
    // NetMulticast), and rest mutates authoritative loyalty/belief state — binding only on the server avoids
    // double-applying the rest deltas. (On a client PlayerParties is empty anyway, but never bind there.)
    if (InWorld.GetNetMode() == NM_Client) {
        return;
    }
    UGameInstance *GI = InWorld.GetGameInstance();
    UMythicEnvironmentSubsystem *EnvSubsystem = GI ? GI->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr;
    if (!EnvSubsystem) {
        return;
    }
    // Bind now if the world clock already exists, else catch it when it registers (mirrors the hazard component).
    if (AMythicEnvironmentController *Controller = EnvSubsystem->GetEnvironmentController()) {
        BindEnvironmentController(Controller);
    }
    else {
        EnvSubsystem->OnEnvironmentControllerRegisterDelegate.AddUniqueDynamic(
            this, &UMythicPartySubsystem::OnEnvironmentControllerRegistered);
    }
}

void UMythicPartySubsystem::OnEnvironmentControllerRegistered(AMythicEnvironmentController *Controller) {
    if (Controller) {
        BindEnvironmentController(Controller);
    }
}

void UMythicPartySubsystem::BindEnvironmentController(AMythicEnvironmentController *Controller) {
    BoundEnvController = Controller;
    Controller->DayTimeChangeDelegate.AddUniqueDynamic(this, &UMythicPartySubsystem::OnDaytimeChanged);
}

void UMythicPartySubsystem::OnDaytimeChanged(EDayTime PrevDayTime, EDayTime NewDayTime) {
    const bool bEnteringNight = (NewDayTime == EDayTime::Night && PrevDayTime != EDayTime::Night);
    const bool bLeavingNight = (PrevDayTime == EDayTime::Night && NewDayTime != EDayTime::Night);
    if (!bEnteringNight && !bLeavingNight) {
        return; // only the night boundary matters
    }
    // Rest every active party at nightfall (bonding over the campfire), wake them at dawn. EnterRestPhase /
    // ExitRestPhase mutate member VALUES only (no map structure change), so iterating the keys is safe.
    for (const TPair<int32, TArray<FMythicPartyMember>> &Pair : PlayerParties) {
        if (bEnteringNight) {
            EnterRestPhase(Pair.Key);
        }
        else {
            ExitRestPhase(Pair.Key);
        }
    }
}

// ─────────────────────────────────────────────────────────────
// Party Management
// ─────────────────────────────────────────────────────────────

bool UMythicPartySubsystem::AddCompanion(int32 PlayerId, AMythicNPCCharacter *NPC, FMassEntityHandle SourceEntity) {
    if (!NPC) {
        UE_LOG(LogMythParty, Warning, TEXT("AddCompanion: Null NPC"));
        return false;
    }

    // Check party size
    TArray<FMythicPartyMember> &Party = PlayerParties.FindOrAdd(PlayerId);
    if (Party.Num() >= MaxPartySize) {
        UE_LOG(LogMythParty, Warning,
               TEXT("AddCompanion: Party full for Player %d (%d/%d)"),
               PlayerId, Party.Num(), MaxPartySize);
        return false;
    }

    // Check if already in party
    for (const FMythicPartyMember &Member : Party) {
        if (Member.NPCActor.Get() == NPC) {
            UE_LOG(LogMythParty, Warning, TEXT("AddCompanion: NPC already in party"));
            return false;
        }
    }

    // Create party member
    FMythicPartyMember NewMember;
    NewMember.NPCActor = NPC;
    NewMember.SourceEntity = SourceEntity;
    NewMember.LoyaltyScore = 0.5f; // Start at neutral loyalty
    NewMember.BetrayalPressure = 0.0f;
    NewMember.JoinTime = GetWorld()->GetTimeSeconds();

    // Cache faction + display name from the NPC's brain (the name powers the departure/betrayal callouts + future UI).
    if (UMythicCognitiveBrainComponent *Brain = NPC->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
        NewMember.OriginalFaction = Brain->GetFaction();
        NewMember.CachedDisplayName = Brain->GetDisplayName();
    }

    // Slice-7: capture the persisted MASS identity from the LIVE source entity (canonical — Rule 3). Needed to
    // RE-CREATE the entity on load (the Mass world is transient; SourceEntity is stale after a save/load round-trip,
    // and the spawner never regenerates this NameHash). Entity Identity is authoritative over the brain for faction.
    if (SourceEntity.IsValid()) {
        if (UMassEntitySubsystem *EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>()) {
            const FMassEntityManager &EM = EntitySubsystem->GetEntityManager();
            if (EM.IsEntityValid(SourceEntity)) {
                if (const FMythicIdentityFragment *Id = EM.GetFragmentDataPtr<FMythicIdentityFragment>(SourceEntity)) {
                    NewMember.PersistedNameHash = Id->NameHash;
                    NewMember.OriginalFaction = Id->Faction; // entity Identity is authoritative
                    NewMember.PersistedTrueFaction = Id->TrueFaction;
                    NewMember.PersistedRoleTag = Id->RoleTag;
                    // #34-r2 fix: PersistedSpawnCell is the never-drifted SPAWN-ORIGIN (Schedule.HomeCell), NOT the now-live
                    // Identity.Cell — else a companion recruited away from its birth cell would persist its drifted cell as
                    // home + re-spawn there on load. Schedule.HomeCell is write-once-at-spawn (= the original Identity.Cell).
                    if (const FMythicScheduleFragment *Sched = EM.GetFragmentDataPtr<FMythicScheduleFragment>(SourceEntity)) {
                        NewMember.PersistedSpawnCell = Sched->HomeCell;
                    }
                    else {
                        NewMember.PersistedSpawnCell = Id->Cell; // fallback (Schedule frag is always present on an embodied NPC)
                    }
                }
            }
        }
    }

    Party.Add(MoveTemp(NewMember));

    // Register the MASS entity in the flat companion set so the SignificanceProcessor exempts it from dehydration.
    if (SourceEntity.IsValid()) {
        CompanionEntities.Add(SourceEntity);
    }

    // Arm the dedicated follow timer (server-side; the NPC is embodied at recruit time, so its AIController exists).
    if (AMythicAIController *AIC = Cast<AMythicAIController>(NPC->GetController())) {
        AIC->SetCompanionFollow(true, PlayerId);
    }

    // Create social edge between player entity and NPC
    if (SocialGraph && SourceEntity.IsValid()) {
        // Note: player→NPC edge would require a player entity handle
        // For now, this is tracked via the party system itself
    }

    UE_LOG(LogMythParty, Log,
           TEXT("Companion added to Player %d party (%d/%d)"),
           PlayerId, Party.Num(), MaxPartySize);

    return true;
}

bool UMythicPartySubsystem::RemoveCompanion(int32 PlayerId, AMythicNPCCharacter *NPC, bool bVoluntary) {
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerId);
    if (!Party) {
        return false;
    }

    for (int32 i = 0; i < Party->Num(); ++i) {
        if ((*Party)[i].NPCActor.Get() == NPC) {
            UE_LOG(LogMythParty, Log,
                   TEXT("Companion removed from Player %d party (%s)"),
                   PlayerId, bVoluntary ? TEXT("voluntary") : TEXT("forced"));

            RemoveMemberAt(PlayerId, *Party, i);
            return true;
        }
    }

    return false;
}

bool UMythicPartySubsystem::RemoveCompanionFromAnyParty(AMythicNPCCharacter *NPC, bool bVoluntary) {
    if (!NPC) {
        return false;
    }
    for (auto &Pair : PlayerParties) {
        for (int32 i = 0; i < Pair.Value.Num(); ++i) {
            if (Pair.Value[i].NPCActor.Get() == NPC) {
                UE_LOG(LogMythParty, Log, TEXT("Companion removed from Player %d party (death/de-embody, %s)"),
                       Pair.Key, bVoluntary ? TEXT("voluntary") : TEXT("forced"));
                RemoveMemberAt(Pair.Key, Pair.Value, i); // clears the despawn-exemption + follow timer + the slot
                return true;
            }
        }
    }
    return false;
}

int32 UMythicPartySubsystem::GetPartyMembers(int32 PlayerId, TArray<FMythicPartyMember> &OutMembers) const {
    OutMembers.Reset();

    const TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerId);
    if (!Party) {
        return 0;
    }

    OutMembers = *Party;
    return OutMembers.Num();
}

int32 UMythicPartySubsystem::GetPartySize(int32 PlayerId) const {
    const TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerId);
    return Party ? Party->Num() : 0;
}

bool UMythicPartySubsystem::IsInParty(const AMythicNPCCharacter *NPC) const {
    for (const auto &Pair : PlayerParties) {
        for (const FMythicPartyMember &Member : Pair.Value) {
            if (Member.NPCActor.Get() == NPC) {
                return true;
            }
        }
    }
    return false;
}

APawn *UMythicPartySubsystem::GetLeaderPawn(int32 PlayerId) const {
    // Slice-1 single-player mapping: only PlayerId 0 → the first local player's pawn. The real PlayerId→pawn resolver
    // is the MP slice (one shared source per Rule 3).
    if (PlayerId != 0) {
        return nullptr;
    }
    const UWorld *World = GetWorld();
    APlayerController *PC = World ? World->GetFirstPlayerController() : nullptr;
    return PC ? PC->GetPawn() : nullptr;
}

bool UMythicPartySubsystem::IsCompanionEntity(FMassEntityHandle Entity) const {
    return Entity.IsValid() && CompanionEntities.Contains(Entity);
}

void UMythicPartySubsystem::NotifyCompanionLost(int32 PlayerId, const FMythicPartyMember &Member, bool bBetrayed) {
    APawn *Leader = GetLeaderPawn(PlayerId);
    if (!Leader) {
        return;
    }
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(Leader->GetController());
    if (!PC) {
        return;
    }
    // Float over the departing companion if it is still around, else over the player.
    const FVector Loc = Member.NPCActor.IsValid() ? Member.NPCActor->GetActorLocation() : Leader->GetActorLocation();
    if (bBetrayed) {
        PC->ClientNotifyCompanionBetrayed(Member.CachedDisplayName, Loc);
    }
    else {
        PC->ClientNotifyCompanionDeparted(Member.CachedDisplayName, Loc);
    }
}

void UMythicPartySubsystem::RemoveMemberAt(int32 PlayerId, TArray<FMythicPartyMember> &Party, int32 Index) {
    if (!Party.IsValidIndex(Index)) {
        return;
    }
    const FMythicPartyMember &Member = Party[Index];
    // Drop the despawn-exemption signal + stop the follow timer BEFORE the swap — the one place all exit paths funnel
    // through, so neither cleanup can ever be bypassed (the bug the iter-137 review caught for departure/betrayal).
    CompanionEntities.Remove(Member.SourceEntity);
    if (AMythicNPCCharacter *NPC = Member.NPCActor.Get()) {
        if (AMythicAIController *AIC = Cast<AMythicAIController>(NPC->GetController())) {
            AIC->SetCompanionFollow(false, PlayerId);
        }
    }
    Party.RemoveAtSwap(Index);
}

// ─────────────────────────────────────────────────────────────
// Event Handling
// ─────────────────────────────────────────────────────────────

void UMythicPartySubsystem::OnPlayerAction(
    int32 PlayerId,
    const FGameplayTag &EventTag,
    const FMythicMoralAction &MoralAction) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicParty_OnPlayerAction);

    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerId);
    if (!Party || Party->Num() == 0) {
        return;
    }

    // Iterate DOWNWARD: CheckCompanionThresholds below can remove member i via RemoveAtSwap, which moves the LAST element
    // into slot i. Ascending iteration would then ++i past that swapped-in (unvisited) member, skipping its loyalty delta
    // and its own threshold check for this action (so a 2nd simultaneous departure/betrayal slipped to the next action).
    // Descending is safe — the swapped-in element always comes from a higher, already-processed index.
    for (int32 i = Party->Num() - 1; i >= 0; --i) {
        FMythicPartyMember &Member = (*Party)[i];

        // Evaluate loyalty impact of the full moral vector
        const float LoyaltyDelta = EvaluateLoyaltyImpact(Member, MoralAction);
        Member.LoyaltyScore = FMath::Clamp(Member.LoyaltyScore + LoyaltyDelta, 0.0f, 1.0f);

        // Track betrayal pressure for severe moral violations
        if (LoyaltyDelta < -0.1f) {
            // The action was significantly against the companion's morals
            Member.BetrayalPressure += FMath::Abs(LoyaltyDelta) * 2.0f;
        }

        // Check departure/betrayal thresholds
        CheckCompanionThresholds(PlayerId, i);
    }
}

void UMythicPartySubsystem::EnterRestPhase(int32 PlayerId) {
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerId);
    if (!Party) {
        return;
    }

    for (FMythicPartyMember &Member : *Party) {
        Member.bInRestPhase = true;
    }

    // Propagate beliefs between companions during rest
    PropagateBeliefs(PlayerId);

    // Slight loyalty recovery during rest (bonding over campfire)
    for (FMythicPartyMember &Member : *Party) {
        Member.LoyaltyScore = FMath::Min(Member.LoyaltyScore + 0.02f, 1.0f);

        // Slight betrayal pressure decay during rest
        Member.BetrayalPressure = FMath::Max(0.0f, Member.BetrayalPressure - 0.1f);
    }

    UE_LOG(LogMythParty, Verbose,
           TEXT("Player %d party entered rest. %d members, beliefs propagated."),
           PlayerId, Party->Num());
}

void UMythicPartySubsystem::ExitRestPhase(int32 PlayerId) {
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerId);
    if (!Party) {
        return;
    }

    for (FMythicPartyMember &Member : *Party) {
        Member.bInRestPhase = false;
    }
}

// ─────────────────────────────────────────────────────────────
// Internal Operations
// ─────────────────────────────────────────────────────────────

void UMythicPartySubsystem::PropagateBeliefs(int32 PlayerId) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicParty_PropagateBeliefs);

    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerId);
    if (!Party || Party->Num() < 2) {
        return; // Need at least 2 members for propagation
    }

    // Each member shares their top beliefs with every other member
    for (int32 i = 0; i < Party->Num(); ++i) {
        FMythicPartyMember &Source = (*Party)[i];

        // Get beliefs from the source's cognitive brain
        AMythicNPCCharacter *SourceNPC = Source.NPCActor.Get();
        if (!SourceNPC) {
            continue;
        }

        UMythicCognitiveBrainComponent *SourceBrain = SourceNPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
        if (!SourceBrain) {
            continue;
        }

        // Copy (not a live ref): the source brain's worker may be mid async-think, mutating its Beliefs while we iterate.
        const TArray<FMythicBelief> SourceBeliefs = SourceBrain->GetBeliefsCopy();

        for (int32 j = 0; j < Party->Num(); ++j) {
            if (i == j) {
                continue;
            }

            FMythicPartyMember &Target = (*Party)[j];
            AMythicNPCCharacter *TargetNPC = Target.NPCActor.Get();
            if (!TargetNPC) {
                continue;
            }

            UMythicCognitiveBrainComponent *TargetBrain = TargetNPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
            if (!TargetBrain) {
                continue;
            }

            // Propagate top beliefs with confidence decay
            for (const FMythicBelief &Belief : SourceBeliefs) {
                if (Belief.Confidence < 0.3f) {
                    continue; // Don't share weak beliefs
                }

                FMythicBelief PropagatedBelief = Belief;
                PropagatedBelief.Confidence *= (1.0f - BeliefPropagationDecay);
                PropagatedBelief.PropagationHops += 1;

                TargetBrain->InjectBelief(PropagatedBelief);
            }
        }
    }
}

float UMythicPartySubsystem::EvaluateLoyaltyImpact(
    const FMythicPartyMember &Member,
    const FMythicMoralAction &MoralAction) const {
    // The companion evaluates the player's action against their own faction's ideology.
    // Actions that align with the companion's moral surface boost loyalty.
    // Actions that violate it damage loyalty.

    AMythicNPCCharacter *NPC = Member.NPCActor.Get();
    if (!NPC) {
        return 0.0f;
    }

    UMythicCognitiveBrainComponent *Brain = NPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
    if (!Brain) {
        return 0.0f;
    }

    // Get companion's faction ideology to evaluate the moral action
    FMythicFactionData CompanionFactionData;
    UMythicLivingWorldSubsystem *LWS = GetWorld()->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LWS || !LWS->GetFactionDatabase() || !LWS->GetFactionDatabase()->GetFaction(Member.OriginalFaction, CompanionFactionData)) {
        return 0.0f;
    }

    // Evaluate the action's full moral vector using the companion's faction moral thresholds, from their perspective.
    const EMythicMoralSeverity Severity = FMythicMoralSignature::EvaluateActionSeverity(
        MoralAction, CompanionFactionData.Ideology,
        CompanionFactionData.DisapproveThreshold,
        CompanionFactionData.CondemnThreshold,
        CompanionFactionData.HostileThreshold);

    // Translate severity into loyalty impact
    switch (Severity) {
    case EMythicMoralSeverity::Hostile:
        return -0.10f; // Major violation
    case EMythicMoralSeverity::Condemn:
        return -0.06f; // Serious violation
    case EMythicMoralSeverity::Disapprove:
        return -0.02f; // Minor disapproval
    case EMythicMoralSeverity::Ignore:
    default:
        // If the companion's personality leans toward this axis, reward alignment
    {
        // Read the companion's actual personality VentWeights for nuanced evaluation
        const FMythicPersonalityFragment &PersonalityRef = Brain->GetPersonality();
        const float TendWeight = PersonalityRef.VentWeights[static_cast<int32>(EMythicVentChannel::Tend)];

        // Mercy actions are universally loyalty-positive for empathetic companions (read the Mercy axis off the
        // full vector — a positive Mercy contribution means the player showed mercy/healed/spared).
        if (MoralAction.AxisValues[static_cast<int32>(EMythicMoralAxis::Mercy)] > 0.0f) {
            return 0.03f * (0.5f + TendWeight);
        }
        return 0.01f; // Mild approval
    }
    }
}

void UMythicPartySubsystem::CheckCompanionThresholds(int32 PlayerId, int32 MemberIndex) {
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerId);
    if (!Party || !Party->IsValidIndex(MemberIndex)) {
        return;
    }

    const FMythicPartyMember &Member = (*Party)[MemberIndex];

    // Check betrayal first (more dramatic)
    if (Member.BetrayalPressure >= BetrayalThreshold) {
        HandleCompanionBetrayal(PlayerId, MemberIndex);
        return;
    }

    // Check departure
    if (Member.LoyaltyScore <= LoyaltyDepartureThreshold) {
        HandleCompanionDeparture(PlayerId, MemberIndex);
    }
}

void UMythicPartySubsystem::HandleCompanionDeparture(int32 PlayerId, int32 MemberIndex) {
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerId);
    if (!Party || !Party->IsValidIndex(MemberIndex)) {
        return;
    }

    const FMythicPartyMember &Member = (*Party)[MemberIndex];

    UE_LOG(LogMythParty, Log,
           TEXT("Companion departing Player %d party (Loyalty=%.2f below threshold %.2f)"),
           PlayerId, Member.LoyaltyScore, LoyaltyDepartureThreshold);

    // Write the departure event through the subsystem's THREAD-SAFE queue — NOT CausalFabric->AppendEvent directly.
    // This runs on the GAME thread, but the fabric is a lock-free single-writer owned by the sim thread (which drains
    // PendingEvents on its own thread); a direct game-thread append would race its ring/index mutation. WorldTime is
    // stamped so the event survives time-windowed fabric queries (which drop WorldTime==0 records).
    if (LivingWorld) {
        FMythicWorldEvent Event;
        Event.WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
        Event.PrimaryFaction = Member.OriginalFaction;
        Event.Significance = 0.4f;
        Event.CategoryFlags = EMythicEventCategory::Social;
        LivingWorld->SubmitWorldEvent(Event);
    }

    NotifyCompanionLost(PlayerId, Member, /*bBetrayed*/ false); // BEFORE RemoveMemberAt — Member is still valid
    RemoveMemberAt(PlayerId, *Party, MemberIndex);
}

void UMythicPartySubsystem::HandleCompanionBetrayal(int32 PlayerId, int32 MemberIndex) {
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerId);
    if (!Party || !Party->IsValidIndex(MemberIndex)) {
        return;
    }

    FMythicPartyMember &Member = (*Party)[MemberIndex];

    UE_LOG(LogMythParty, Warning,
           TEXT("Companion BETRAYAL! Player %d (Pressure=%.2f exceeded threshold %.2f)"),
           PlayerId, Member.BetrayalPressure, BetrayalThreshold);

    // Write the betrayal event — high significance — through the thread-safe queue (see HandleCompanionDeparture for
    // the single-writer rationale). Tagged World.Action.Betrayal (the same grounded tag the brain belief uses below)
    // + WorldTime stamped so it survives time-windowed fabric/witness queries.
    if (LivingWorld) {
        FMythicWorldEvent Event;
        Event.WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
        Event.EventTag = FGameplayTag::RequestGameplayTag(FName("World.Action.Betrayal"));
        Event.PrimaryFaction = Member.OriginalFaction;
        Event.Significance = 0.9f;
        Event.CategoryFlags = EMythicEventCategory::Social | EMythicEventCategory::Combat;
        LivingWorld->SubmitWorldEvent(Event);
    }

    // Trigger betrayal behavior via the companion's BDI brain
    AMythicNPCCharacter *NPC = Member.NPCActor.Get();
    if (NPC) {
        UMythicCognitiveBrainComponent *Brain = NPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
        if (Brain) {
            // Resolve the betrayer's REAL cell, and ONLY inject the belief if it resolves. The (0,0) value is a
            // LEGITIMATE grid corner, not a sentinel — so an unresolved cell would inject a Confidence=1.0 grievance at
            // the origin, which the iter-115 global-highest-confidence Avenge router would ALWAYS pick (1.0 is the
            // ceiling), deterministically marching the NPC to (0,0). A betrayal with no resolvable cell has no meaningful
            // spatial target, so skip the injection entirely rather than fabricate a max-confidence origin grievance.
            FMythicCellCoord BetrayalCell;
            bool bHaveCell = false;
            if (LivingWorld) {
                if (const UMythicTerritoryGrid *Grid = LivingWorld->GetTerritoryGrid()) {
                    BetrayalCell = Grid->WorldToCell(NPC->GetActorLocation());
                    bHaveCell = true;
                }
            }

            if (!bHaveCell) {
                UE_LOG(LogMythParty, Warning,
                       TEXT("Companion betrayal: territory grid unavailable; skipping hostile-belief injection (an "
                           "unresolved (0,0) cell would mis-route Avenge to the grid origin)."));
            }
            else {
                // Inject a high-confidence hostile belief: "player is my enemy" — the BDI brain naturally generates
                // Fight/Flee/Exploit/Avenge desires from it, routed to BetrayalCell.
                FMythicBelief HostileBelief;
                HostileBelief.EventTag = FGameplayTag::RequestGameplayTag(FName("World.Action.Betrayal"));
                HostileBelief.Cell = BetrayalCell;
                HostileBelief.Confidence = 1.0f;
                HostileBelief.FormationTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
                HostileBelief.PropagationHops = 0; // Direct experience
                Brain->InjectBelief(HostileBelief);

                // Force an immediate re-think so the NPC reacts right away
                Brain->OnSignificantEvent(
                    FGameplayTag::RequestGameplayTag(FName("World.Action.Betrayal")),
                    BetrayalCell);

                UE_LOG(LogMythParty, Log,
                       TEXT("Companion betrayal: injected hostile belief, triggered immediate re-think"));
            }
        }
    }

    NotifyCompanionLost(PlayerId, Member, /*bBetrayed*/ true); // BEFORE RemoveMemberAt — Member is still valid
    RemoveMemberAt(PlayerId, *Party, MemberIndex);
}

void UMythicPartySubsystem::Serialize(FArchive &Ar) {
    int32 Version = 2; // v2 (Slice-7): persists companion MASS identity (NameHash/TrueFaction/SpawnCell/RoleTag)
    Ar << Version;

    // Serialize the number of player parties
    int32 PartyCount = PlayerParties.Num();
    Ar << PartyCount;

    if (Ar.IsLoading()) {
        PlayerParties.Empty(PartyCount);
        // Slice-7: drop any stale companion exemption handles from a prior session (load-over-active-session) — the
        // rebuild re-adds the live ones. Without this, defunct handles linger in the despawn-exemption set.
        CompanionEntities.Empty();
    }

    if (Ar.IsSaving()) {
        for (auto &Pair : PlayerParties) {
            int32 PlayerId = Pair.Key;
            Ar << PlayerId;

            int32 MemberCount = Pair.Value.Num();
            Ar << MemberCount;

            for (auto &Member : Pair.Value) {
                Ar << Member.OriginalFaction.Index;
                Ar << Member.LoyaltyScore;
                Ar << Member.BetrayalPressure;
                Ar << Member.JoinTime;
                Ar << Member.bInRestPhase;
                Ar << Member.CachedDisplayName;

                // v2 (Slice-7) persisted MASS identity (OriginalFaction.Index already written above). Byte order MUST
                // mirror the load branch exactly, or the unframed stream desyncs the beliefs + every later member.
                Ar << Member.PersistedNameHash;
                Ar << Member.PersistedTrueFaction.Index;
                Ar << Member.PersistedSpawnCell.X;
                Ar << Member.PersistedSpawnCell.Y;
                Ar << Member.PersistedRoleTag;

                // Serialize shared beliefs count and data
                int32 BeliefCount = Member.SharedBeliefs.Num();
                Ar << BeliefCount;
                for (auto &Belief : Member.SharedBeliefs) {
                    Ar << Belief.EventTag;
                    Ar << Belief.Confidence;
                    Ar << Belief.FormationTime;
                }
            }
        }
    }
    else {
        // Loading
        for (int32 p = 0; p < PartyCount; ++p) {
            int32 PlayerId = 0;
            Ar << PlayerId;

            int32 MemberCount = 0;
            Ar << MemberCount;

            TArray<FMythicPartyMember> &Party = PlayerParties.Add(PlayerId);
            Party.SetNum(MemberCount);

            for (int32 m = 0; m < MemberCount; ++m) {
                FMythicPartyMember &Member = Party[m];
                Ar << Member.OriginalFaction.Index;
                Ar << Member.LoyaltyScore;
                Ar << Member.BetrayalPressure;
                Ar << Member.JoinTime;
                Ar << Member.bInRestPhase;
                Ar << Member.CachedDisplayName;

                // v2 (Slice-7) persisted MASS identity — same fields/order as the save branch, version-gated so v1
                // saves stay byte-aligned. A member carrying a real NameHash is marked for post-load entity re-creation.
                if (Version >= 2) {
                    Ar << Member.PersistedNameHash;
                    Ar << Member.PersistedTrueFaction.Index;
                    Ar << Member.PersistedSpawnCell.X;
                    Ar << Member.PersistedSpawnCell.Y;
                    Ar << Member.PersistedRoleTag;
                    if (Member.PersistedNameHash != 0) {
                        Member.RebuildState = EMythicCompanionRebuildState::NotCreated;
                    }
                }
                // v1 saves: no persisted identity → RebuildState stays Bound (default) → the ticker skips it (the
                // companion is lost on a legacy load, same as before this slice — no crash, no desync).

                int32 BeliefCount = 0;
                Ar << BeliefCount;
                Member.SharedBeliefs.SetNum(BeliefCount);
                for (int32 b = 0; b < BeliefCount; ++b) {
                    Ar << Member.SharedBeliefs[b].EventTag;
                    Ar << Member.SharedBeliefs[b].Confidence;
                    Ar << Member.SharedBeliefs[b].FormationTime;
                }
            }
        }
    }

    // Slice-7: after a load completes, re-create + re-embody + re-bind companions (the Mass world is transient and
    // their entities are gone). Game-thread, idempotent; arms a self-rescheduling ticker (NOT inline here — the
    // recreate needs a later processor pass to drain the embodiment request).
    if (Ar.IsLoading()) {
        RebindCompanionsAfterLoad();
    }
}

// ─────────────────────────────────────────────────────────────
// Slice-7 — Post-load companion entity re-creation + rebind
// ─────────────────────────────────────────────────────────────

void UMythicPartySubsystem::RebindCompanionsAfterLoad() {
    // Slice-7-r2: the rebuild path mutates the Mass EntityManager (create/hydrate/tag) + arms a core ticker — all
    // game-thread-only. Today the sole caller is Party->Serialize on load (game thread), but assert the invariant so a
    // future off-thread load path fails loudly here rather than corrupting Mass state silently.
    check(IsInGameThread());
    // Idempotent: if a rebuild ticker is already armed (e.g. a second load before the first finished), drop it first.
    if (CompanionRebuildTickHandle.IsValid()) {
        FTSTicker::GetCoreTicker().RemoveTicker(CompanionRebuildTickHandle);
        CompanionRebuildTickHandle.Reset();
    }

    // Are there any members to rebuild at all? (Avoid arming a ticker for nothing.)
    bool bAny = false;
    for (const TPair<int32, TArray<FMythicPartyMember>> &Pair : PlayerParties) {
        for (const FMythicPartyMember &Member : Pair.Value) {
            if (Member.RebuildState != EMythicCompanionRebuildState::Bound) {
                bAny = true;
                break;
            }
        }
        if (bAny) { break; }
    }
    if (!bAny) {
        return;
    }

    CompanionRebuildAttempts = 0;
    // 0.0f interval = fire every game-thread tick: create entities on the first fire, then poll for the two-tick
    // ActorSpawnProcessor handoff on subsequent fires.
    CompanionRebuildTickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UMythicPartySubsystem::TickCompanionRebuild), 0.0f);
}

bool UMythicPartySubsystem::TickCompanionRebuild(float /*DeltaTime*/) {
    // Self-rescheduling: return true to keep ticking, false to stop. Game thread (core ticker).
    check(IsInGameThread()); // Slice-7-r2: EntityManager create/hydrate/embody is game-thread-only
    ++CompanionRebuildAttempts;

    UWorld *World = GetWorld();
    UMythicLivingWorldSubsystem *LWS = LivingWorld; // cached in Initialize
    if (!World || !LWS) {
        CompanionRebuildTickHandle.Reset();
        return false; // no world/LWS -> nothing we can do; stop.
    }

    bool bAnyPending = false;

    for (TPair<int32, TArray<FMythicPartyMember>> &Pair : PlayerParties) {
        const int32 PlayerId = Pair.Key;
        TArray<FMythicPartyMember> &Party = Pair.Value;

        for (int32 i = 0; i < Party.Num(); ++i) {
            FMythicPartyMember &Member = Party[i];

            if (Member.RebuildState == EMythicCompanionRebuildState::Bound) {
                continue; // (c) done
            }
            else if (Member.RebuildState == EMythicCompanionRebuildState::NotCreated) {
                // (a) not-yet-created -> create entity + force Tier2 + register exemption + tag spawn-request.
                const FMassEntityHandle NewEntity = CreateLoadedCompanionEntity(Member);
                if (NewEntity.IsValid()) {
                    Member.SourceEntity = NewEntity;
                    // EXEMPTION RACE FIX: register SYNCHRONOUSLY now, BEFORE any significance/spawn pass can run.
                    CompanionEntities.Add(NewEntity);
                    Member.RebuildState = EMythicCompanionRebuildState::EntityCreated;
                }
                else {
                    // Perma-dead or create failed -> this companion cannot come back. Mark Bound so we stop retrying.
                    Member.RebuildState = EMythicCompanionRebuildState::Bound;
                    UE_LOG(LogMythParty, Warning,
                           TEXT("Companion rebuild: entity create skipped/failed for NameHash=%u (perma-dead or no mass) — companion not restored."),
                           Member.PersistedNameHash);
                    continue;
                }
                bAnyPending = true; // created this tick; embodiment is on a later pass
            }
            else { // EntityCreated
                // (b) created+tagged, actor not yet embodied -> poll. Do NOT re-create.
                if (AMythicNPCCharacter *Actor = LWS->FindEmbodiedActor(Member.SourceEntity)) {
                    RebindLoadedCompanion(PlayerId, Member, Actor);
                    Member.RebuildState = EMythicCompanionRebuildState::Bound;
                }
                else {
                    bAnyPending = true; // still waiting on ActorSpawnProcessor
                }
            }
        }
    }

    if (!bAnyPending) {
        CompanionRebuildTickHandle.Reset();
        return false; // all members Bound -> stop
    }

    // Timeout guard: never tick forever if embodiment never happens (e.g. ActorSpawnProcessor disabled, grid missing).
    // ~600 game-thread ticks is many seconds — generous for a 2-tick handoff, bounded for safety.
    if (CompanionRebuildAttempts >= 600) {
        UE_LOG(LogMythParty, Warning,
               TEXT("Companion rebuild: timed out after %d attempts with members still pending embodiment — giving up."),
               CompanionRebuildAttempts);
        // Leave any still-EntityCreated members as-is: their entity exists + is Tier2 + exempt, so a later natural
        // significance pass may still embody it. We just stop the ticker.
        CompanionRebuildTickHandle.Reset();
        return false;
    }

    return true; // keep ticking
}

FMassEntityHandle UMythicPartySubsystem::CreateLoadedCompanionEntity(const FMythicPartyMember &Member) {
    check(IsInGameThread()); // Slice-7-r2: creates + mutates Mass entities — game-thread-only
    UWorld *World = GetWorld();
    if (!World) { return FMassEntityHandle(); }

    UMassEntitySubsystem *MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
    UMythicLivingWorldSubsystem *LWS = LivingWorld;
    if (!MassSubsystem || !LWS) { return FMassEntityHandle(); }

    // IsPermaDead guard: never resurrect a permanently dead companion.
    if (UMythicPersistentNPCRegistry *Registry = LWS->GetPersistentNPCRegistry()) {
        if (Registry->IsPermaDead(Member.PersistedNameHash)) {
            return FMassEntityHandle();
        }
    }

    FMassEntityManager &EntityManager = MassSubsystem->GetMutableEntityManager();

    // Archetype = the ambient-citizen set the SignificanceProcessor + PopulationSpawner queries require
    // (Identity + Schedule + Significance + NPCTag). NO EncounterEntityTag (it must be a normal companion the
    // exemption set protects), NO AllocateSpawnSerial (we supply the persisted NameHash ourselves).
    const UScriptStruct *Composition[] = {
        FMythicIdentityFragment::StaticStruct(),
        FMythicScheduleFragment::StaticStruct(),
        FMythicSignificanceFragment::StaticStruct(),
        FMythicNPCTag::StaticStruct()
    };
    FMassArchetypeHandle Archetype = EntityManager.CreateArchetype(MakeArrayView(Composition));

    TArray<FMassEntityHandle> Spawned;
    EntityManager.BatchCreateEntities(Archetype, 1, Spawned);
    if (Spawned.Num() != 1) { return FMassEntityHandle(); }
    const FMassEntityHandle Entity = Spawned[0];

    // ─── Identity — restore the persisted values verbatim ───
    FMythicIdentityFragment &Identity = EntityManager.GetFragmentDataChecked<FMythicIdentityFragment>(Entity);
    Identity.NameHash = Member.PersistedNameHash;
    Identity.Faction = Member.OriginalFaction; // cover faction
    Identity.TrueFaction = Member.PersistedTrueFaction;
    Identity.Cell = Member.PersistedSpawnCell; // persisted spawn/home cell (Slice-7 cell decision)
    Identity.RoleTag = Member.PersistedRoleTag;
    Identity.VisualArchetype = FMythicNPCGenerator::GenerateVisualArchetype(Member.PersistedNameHash, 8); // match spawner exactly

    // ─── Schedule — home/work at the spawn cell, idle (the companion is follow-driven, not schedule-driven) ───
    FMythicScheduleFragment &Schedule = EntityManager.GetFragmentDataChecked<FMythicScheduleFragment>(Entity);
    Schedule.Phase = EMythicSchedulePhase::Idle;
    Schedule.HomeCell = Member.PersistedSpawnCell;
    Schedule.WorkCell = Member.PersistedSpawnCell;

    // ─── Significance — force Tier2 + max score (the next demotion pass won't drop it; the exemption also protects it) ───
    FMythicSignificanceFragment &Sig = EntityManager.GetFragmentDataChecked<FMythicSignificanceFragment>(Entity);
    Sig.Tier = EMythicSignificanceTier::Tier2_Cognitive;
    Sig.Score = 1.0f;
    Sig.bDirty = false;

    // ─── Force-hydrate to Tier2 via a command buffer (mirror MythicCheatManager.cpp:1181-1190) ───
    TSharedPtr<FMassCommandBuffer> CmdBuffer = MakeShared<FMassCommandBuffer>();
    CmdBuffer->AddTag<FMythicHydratedTag>(Entity);
    CmdBuffer->AddFragment<FMythicPsychodynamicFragment>(Entity);
    CmdBuffer->AddFragment<FMythicPersonalityFragment>(Entity);
    CmdBuffer->AddFragment<FMythicSocialFragment>(Entity);
    EntityManager.FlushCommands(CmdBuffer); // fragments are MATERIALIZED after this returns

    // ─── Personality — write the generated value now that the fragment exists (outside a processor, no deferral) ───
    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();
    FMythicPersonalityFragment *Personality = EntityManager.GetFragmentDataPtr<FMythicPersonalityFragment>(Entity);
    if (Personality && FactionDB && Identity.Faction.IsValid()) {
        FMythicFactionData FData;
        if (FactionDB->GetFaction(Identity.Faction, FData)) {
            *Personality = FMythicNPCGenerator::GeneratePersonality(
                Identity.NameHash, FData.Ideology, Identity.RoleTag);
        }
    }

    // Embodiment hard-requires BOTH Identity AND Personality (MythicNPCCharacter.cpp:449-461). If the personality
    // fragment is somehow missing, abort the create rather than embody a brainless actor.
    if (!Personality) {
        UE_LOG(LogMythParty, Error,
               TEXT("Companion rebuild: Personality fragment missing after hydrate for NameHash=%u — aborting embodiment."),
               Member.PersistedNameHash);
        return FMassEntityHandle();
    }

    // ─── Request embodiment — consumed by ActorSpawnProcessor on a LATER PrePhysics pass (the two-tick handoff) ───
    TSharedPtr<FMassCommandBuffer> SpawnCmd = MakeShared<FMassCommandBuffer>();
    SpawnCmd->AddTag<FMythicActorSpawnRequestTag>(Entity);
    EntityManager.FlushCommands(SpawnCmd);

    UE_LOG(LogMythParty, Log,
           TEXT("Companion rebuild: re-created entity (NameHash=%u) at cell (%d,%d), Tier2 + spawn-request tagged."),
           Member.PersistedNameHash, Member.PersistedSpawnCell.X, Member.PersistedSpawnCell.Y);

    return Entity;
}

void UMythicPartySubsystem::RebindLoadedCompanion(int32 PlayerId, FMythicPartyMember &Member, AMythicNPCCharacter *Actor) {
    // The actor now exists (ActorSpawnProcessor embodied it + InitializeFromMassEntity ran). Bind the slot, preserving
    // the ALREADY-LOADED loyalty/pressure/beliefs on Member (Serialize restored them; we do NOT touch them).
    Member.NPCActor = Actor;

    // Re-arm the dedicated follow timer (the NPC is embodied now, so its AIController exists).
    if (AMythicAIController *AIC = Cast<AMythicAIController>(Actor->GetController())) {
        AIC->SetCompanionFollow(true, PlayerId);
    }

    UE_LOG(LogMythParty, Log,
           TEXT("Companion rebuild: rebound Player %d slot to re-embodied actor (NameHash=%u, Loyalty=%.2f)."),
           PlayerId, Member.PersistedNameHash, Member.LoyaltyScore);
}
