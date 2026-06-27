// Mythic Living World — Significance Rescore & Promotion Implementation

#include "Mass/Processors/SignificanceProcessor.h"
#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"
#include "MassCommandBuffer.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "AI/Party/PartySubsystem.h" // companion despawn-exemption query
#include "AI/NPCs/MythicNPCCharacter.h" // embodied-actor location for the demote (no-vanish-in-front) gate
#include "GameFramework/Pawn.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

namespace {
    // Despawn-side no-vanish-in-front gate for the Tier2->Tier1 demotion. Resolves the entity's REAL embodied actor
    // (via the LWS reverse link) and view-tests its actual location — more accurate than the entity cell for a moving
    // NPC, and consistent with the actor-side guard in UMythicActorSpawnProcessor::IsActorInCloseView. Returns false
    // (allow despawn) when the gate is off, there are no cameras, or the actor is already gone (nothing visible to
    // protect). Game-thread only (FindEmbodiedActor + GetActorLocation). File-local so it can pull the LWS + actor
    // types the static member helper deliberately avoids depending on.
    bool IsEmbodiedActorInCloseView(
        const UMythicLivingWorldSubsystem *LWS,
        FMassEntityHandle Entity,
        bool bDespawnGateActive,
        TConstArrayView<FMythicPlayerView> PlayerViews,
        float MinSpawnDistance) {
        if (!bDespawnGateActive || !LWS) {
            return false;
        }
        const AMythicNPCCharacter *Actor = LWS->FindEmbodiedActor(Entity);
        if (!Actor) {
            return false; // No body to protect from a visible pop.
        }
        return UMythicSignificanceProcessor::IsInCloseView(Actor->GetActorLocation(), PlayerViews, MinSpawnDistance);
    }
}

UMythicSignificanceProcessor::UMythicSignificanceProcessor() {
    ProcessingPhase = EMassProcessingPhase::PrePhysics;
    ExecutionFlags = static_cast<uint8>(EProcessorExecutionFlags::Server | EProcessorExecutionFlags::Standalone);
    bRequiresGameThreadExecution = true; // Accesses player controllers + fragment mutation
    bAutoRegisterWithProcessingPhases = true;

    // Run after pressure processing completes (so scores include latest pressure data)
    ExecutionOrder.ExecuteAfter.Add(TEXT("UMythicPressureProcessor"));

    AllSignificanceQuery.RegisterWithProcessor(*this);
}

void UMythicSignificanceProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) {
    AllSignificanceQuery.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
    AllSignificanceQuery.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadWrite);
    // Optional: present on Tier1+ (hydrated) entities, absent on Tier0 (ambient).
    // GetFragmentView returns an empty view when the chunk lacks this fragment.
    AllSignificanceQuery.AddRequirement<FMythicPsychodynamicFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
}

void UMythicSignificanceProcessor::Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicSignificance_Execute);

    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    UGameInstance *GI = World->GetGameInstance();
    if (!GI) {
        return;
    }

    UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LWS || !LWS->IsSystemActive()) {
        return;
    }

    const UMythicLivingWorldSettings *Settings = LWS->GetSettings();
    if (!Settings) {
        return;
    }

    // Party companions are EXEMPT from Tier2→Tier1 dehydration below — a companion that follows its player out of its
    // significance zone must not be silently despawned. This processor is game-thread (bRequiresGameThreadExecution),
    // so a direct query of the (game-thread) party subsystem by entity is safe — mirrors the IsPermaDead gate below.
    const UMythicPartySubsystem *PartySubsystem = World->GetSubsystem<UMythicPartySubsystem>();

    // Timer throttle — don't rescore every frame
    TimeSinceLastTick += Context.GetDeltaTimeSeconds();
    if (TimeSinceLastTick < Settings->SignificanceIntervalSeconds) {
        return;
    }
    TimeSinceLastTick = 0.0f;

    const float PromotionThreshold = Settings->PromotionThreshold;
    const float DemotionThreshold = Settings->DemotionThreshold;
    const float Hysteresis = Settings->SignificanceHysteresisMargin;
    const float ProxWeight = Settings->ProximityWeight;
    const float EventWeight = Settings->EventCountWeight;
    const float EmotionWeight = Settings->EmotionalIntensityWeight;
    const float SpawnRadius = Settings->PopulationSpawnRadius;
    const float EventCountFullScore = Settings->EventCountFullScore;
    const float EmotionalPressureFullScore = Settings->EmotionalPressureFullScore;
    // Proximity-driven embodiment: populate the world near players regardless of narrative score (see the floor below).
    const bool bProxEmbodiment = Settings->bProximityForcesEmbodiment;
    const int32 EmbodimentRadiusCells = Settings->EmbodimentRadiusCells;
    int32 RescoreBudget = Settings->MaxRescoresPerFrame;
    int32 PromotionBudget = Settings->MaxPromotionsPerFrame;
    // Demotions get their OWN per-frame budget so a tick saturated by promotions can never starve demotions
    // (which is what FREES cognitive slots) — see the embodiment-arc review. Tier1->Tier0 and Tier2->Tier1
    // draw this counter; the two promotion branches draw PromotionBudget.
    int32 DemotionBudget = Settings->MaxPromotionsPerFrame;

    // ─── View-gate / stream-in grace settings (embodiment-service-LOCK-v1 §5b, §5c) ───
    const bool bViewGate = Settings->bViewGateEmbodiment;
    const float ViewGateMinDist = Settings->ViewGateMinSpawnDistance;
    const float ViewConeMarginRad = FMath::DegreesToRadians(FMath::Max(0.0f, Settings->ViewConeMarginDeg));
    const float StreamInGrace = Settings->StreamInGraceSeconds;
    const int32 RegionEnterJump = FMath::Max(1, Settings->RegionEnterCellJump);
    const double NowSeconds = World->GetTimeSeconds();

    // Cache player cell coordinates for proximity computation, AND (for the view-gate) a per-player camera view.
    TArray<FMythicCellCoord, TInlineAllocator<4>> PlayerCells;
    TArray<FMythicPlayerView, TInlineAllocator<4>> PlayerViews;
    // Set of currently-live controllers so the grace maps can be pruned of stale (destroyed / disconnected) keys.
    TSet<TWeakObjectPtr<const APlayerController>> LivePlayers;
    // True if ANY local player is inside its stream-in grace window — while true the view-gate is BYPASSED so a town
    // bulk-embodies behind the fade-in (the gate only governs incremental spawning during active play). Grace is a
    // GLOBAL bypass (not per-player): with split-screen co-op, a region-jump by one player streams the shared world in
    // for both, which is the desired pre-population behavior.
    bool bGraceActive = false;

    const UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid();
    if (Grid) {
        for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
            const APlayerController *PC = It->Get();
            if (!PC || !PC->GetPawn()) {
                continue;
            }

            const FVector PawnLoc = PC->GetPawn()->GetActorLocation();
            const FMythicCellCoord PlayerCell = Grid->WorldToCell(PawnLoc);
            PlayerCells.Add(PlayerCell);

            // ── View cone (only built when the gate is on; mirrors UMythicActorSpawnProcessor::IsActorInCloseView) ──
            if (bViewGate && PC->PlayerCameraManager) {
                FMythicPlayerView View;
                View.CamLocation = PC->PlayerCameraManager->GetCameraLocation();
                View.CamForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
                const float HalfFOVRad = FMath::DegreesToRadians(0.5f * PC->PlayerCameraManager->GetFOVAngle());
                const float HalfConeRad = FMath::Min(PI, HalfFOVRad + ViewConeMarginRad);
                View.CosHalfCone = FMath::Cos(HalfConeRad);
                PlayerViews.Add(View);
            }

            // ── Stream-in grace (only tracked when the gate is on; pure bookkeeping otherwise) ──
            if (bViewGate) {
                const TWeakObjectPtr<const APlayerController> Key(PC);
                LivePlayers.Add(Key);

                bool bArmGrace = false;
                if (const FMythicCellCoord *LastCell = PlayerLastCell.Find(Key)) {
                    // Region-enter teleport: a single-tick Manhattan cell jump >= RegionEnterCellJump re-arms grace.
                    const int32 CellJump = FMath::Abs(PlayerCell.X - LastCell->X) + FMath::Abs(PlayerCell.Y - LastCell->Y);
                    if (CellJump >= RegionEnterJump) {
                        bArmGrace = true;
                    }
                }
                else {
                    // First time we've seen this player this session → arm grace so the initial town pre-populates.
                    bArmGrace = true;
                }
                PlayerLastCell.Add(Key, PlayerCell);

                if (bArmGrace) {
                    PlayerFirstSeenTime.Add(Key, NowSeconds);
                }

                if (const double *FirstSeen = PlayerFirstSeenTime.Find(Key)) {
                    if (NowSeconds - *FirstSeen < static_cast<double>(StreamInGrace)) {
                        bGraceActive = true;
                    }
                }
            }
        }
    }

    // Prune the grace maps of controllers no longer live (destroyed / disconnected) so they never grow unbounded.
    // Only meaningful when the gate is on (the maps are written only in that branch); harmless no-op otherwise.
    if (bViewGate && (PlayerFirstSeenTime.Num() > 0 || PlayerLastCell.Num() > 0)) {
        for (auto MapIt = PlayerFirstSeenTime.CreateIterator(); MapIt; ++MapIt) {
            if (!LivePlayers.Contains(MapIt.Key())) {
                MapIt.RemoveCurrent();
            }
        }
        for (auto MapIt = PlayerLastCell.CreateIterator(); MapIt; ++MapIt) {
            if (!LivePlayers.Contains(MapIt.Key())) {
                MapIt.RemoveCurrent();
            }
        }
    }

    // The view-gate ACTIVELY blocks an embody/despawn only when the master toggle is on AND we are NOT inside a
    // stream-in grace window AND there is at least one camera to test against. (Spawn-gate respects grace; the despawn
    // no-vanish-in-front guard does NOT — grace is for bulk pre-population, never for vanishing visible actors.)
    const bool bSpawnGateActive = bViewGate && !bGraceActive && PlayerViews.Num() > 0;
    const bool bDespawnGateActive = bViewGate && PlayerViews.Num() > 0;

    // ─── Pass 1: Rescore dirty entities ───
    AllSignificanceQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (RescoreBudget <= 0) {
            return;
        }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto IdentityView = ChunkContext.GetFragmentView<FMythicIdentityFragment>();
        auto SignificanceView = ChunkContext.GetMutableFragmentView<FMythicSignificanceFragment>();

        // Check if this chunk has psychodynamic fragments (hydrated entities)
        const bool bHasPsycho = ChunkContext.GetFragmentView<FMythicPsychodynamicFragment>().Num() > 0;
        const FMythicPsychodynamicFragment *PsychoData = nullptr;
        if (bHasPsycho) {
            PsychoData = ChunkContext.GetFragmentView<FMythicPsychodynamicFragment>().GetData();
        }

        for (int32 i = 0; i < NumEntities && RescoreBudget > 0; ++i) {
            FMythicSignificanceFragment &Sig = SignificanceView[i];

            // Nearest player cell (Manhattan), reused for both the proximity score and proximity-driven embodiment.
            // No players → stays MAX_int32 (never inside an embodiment band).
            int32 NearestCellDist = MAX_int32;
            for (const FMythicCellCoord &PCell : PlayerCells) {
                NearestCellDist = FMath::Min(NearestCellDist,
                    FMath::Abs(IdentityView[i].Cell.X - PCell.X) + FMath::Abs(IdentityView[i].Cell.Y - PCell.Y));
            }

            // Rescore dirty AMBIENT entities AND always the (capped) hydrated hot-set — the latter so a stationary
            // embodied NPC re-evaluates proximity and DEMOTES when players leave (else its stale high score leaks a
            // cognitive slot forever). See ShouldRescore. NOTE: hydrated entities now consume RescoreBudget every tick,
            // so MaxRescoresPerFrame should be >= MaxCognitiveActors to fully cover the hot-set each significance tick.
            // ADDITIONALLY rescore any ambient within the embodiment band (+1 cell so an edge entity can drop out and
            // demote): proximity embodiment (below) needs nearby ambients re-evaluated as players move, not just on
            // events. Still bounded by RescoreBudget.
            const bool bNearForEmbody = bProxEmbodiment && NearestCellDist <= EmbodimentRadiusCells + 1;
            if (!ShouldRescore(Sig.bDirty, Sig.Tier) && !bNearForEmbody) {
                continue;
            }

            --RescoreBudget;

            // ─── Proximity Score ───
            // Inverse cell distance to the nearest player [0.0, 1.0] — see ComputeProximityScore.
            const float ProximityScore = ComputeProximityScore(IdentityView[i].Cell, PlayerCells, SpawnRadius);

            // ─── Event Count Score ───
            // Normalized by the designer-tunable saturation count (default 16 events = 1.0).
            const float EventScore = FMath::Min(1.0f, static_cast<float>(Sig.RelevantEventCount) / EventCountFullScore);

            // ─── Emotional Intensity Score ───
            // Sum of all pressure channels (only for hydrated entities)
            float EmotionScore = 0.0f;
            if (bHasPsycho && PsychoData) {
                float TotalPressure = 0.0f;
                for (int32 c = 0; c < PressureChannelCount; ++c) {
                    TotalPressure += PsychoData[i].Pressure[c];
                }
                // Normalize by the designer-tunable saturation pressure (default 3.0 total = 1.0 score).
                EmotionScore = FMath::Min(1.0f, TotalPressure / EmotionalPressureFullScore);
            }

            // ─── Weighted (narrative) Score ───
            const float WeightedScore = FMath::Clamp(
                ProxWeight * ProximityScore + EventWeight * EventScore + EmotionWeight * EmotionScore,
                0.0f, 1.0f);

            // ─── Proximity-driven embodiment ───
            // A quiet ambient NPC caps at ProximityWeight (< Tier2PromotionThreshold) and so could never become a
            // visible body, leaving the world unpopulated. Embodiment is a SPATIAL concern: any NPC within
            // EmbodimentRadiusCells of a player should be a real, visible actor. Floor the score to 1.0 inside that
            // band (with 1-cell hysteresis once embodied, so a player jittering at the edge doesn't flicker it) — Pass 2
            // keys promotion AND demotion off Sig.Score, so this both embodies on approach and demotes on departure.
            // Bounded by MaxCognitiveActors regardless of how many entities are in-band. (KNOWN minor: a proximity-
            // embodied entity reports as a faction leader-candidate with this floored score — a spatial accident, not a
            // narrative one; harmless, see Docs/BACKLOG.md.)
            float EmbodimentScore = 0.0f;
            if (bNearForEmbody) {
                const int32 Cutoff = EmbodimentRadiusCells + (Sig.Tier == EMythicSignificanceTier::Tier2_Cognitive ? 1 : 0);
                if (NearestCellDist <= Cutoff) {
                    EmbodimentScore = 1.0f;
                }
            }

            Sig.Score = FMath::Max(WeightedScore, EmbodimentScore);
            Sig.bDirty = false;
        }
    });

    // ─── Pass 2: Promotion / Demotion ───
    // First, count existing actors per tier to enforce the two hard caps. ONE O(entities) read-walk tallies BOTH:
    //   - CognitiveActorCount = Tier1+ (hydrated)  → bounded by MaxCognitiveActors at the Tier0->Tier1 site.
    //   - EmbodiedActorCount  = Tier2  (embodied)  → bounded by MaxEmbodiedActors at BOTH Tier2-promotion sites.
    // Fused into the existing pre-pass (no second walk) per the no-hitch budget contract.
    // Counts are split humanoid vs CREATURE so wildlife gets its OWN budget (MaxCreatureActors) and can't starve the
    // humanoid cognitive cap — a creature-dense wilderness was filling MaxCognitiveActors and blocking travelers/NPCs
    // from ever promoting. CognitiveActorCount/EmbodiedActorCount now count NON-creature (humanoid) entities only;
    // CreatureActiveCount counts hydrated (Tier1+) creatures (one budget covering creature hydrate + embody).
    int32 CognitiveActorCount = 0;
    int32 EmbodiedActorCount = 0;
    int32 CreatureActiveCount = 0;
    AllSignificanceQuery.ForEachEntityChunk(Context, [&CognitiveActorCount, &EmbodiedActorCount, &CreatureActiveCount](FMassExecutionContext &ChunkContext) {
        const bool bCreatureChunk = ChunkContext.DoesArchetypeHaveTag<FMythicCreatureTag>();
        const int32 NumEntities = ChunkContext.GetNumEntities();
        const auto SignificanceView = ChunkContext.GetFragmentView<FMythicSignificanceFragment>();
        for (int32 i = 0; i < NumEntities; ++i) {
            if (SignificanceView[i].Tier >= EMythicSignificanceTier::Tier1_Reactive) {
                if (bCreatureChunk) { ++CreatureActiveCount; } else { ++CognitiveActorCount; }
            }
            if (!bCreatureChunk && SignificanceView[i].Tier >= EMythicSignificanceTier::Tier2_Cognitive) {
                ++EmbodiedActorCount;
            }
        }
    });

    const int32 MaxCognitiveActors = Settings->MaxCognitiveActors;
    const int32 MaxEmbodiedActors = Settings->MaxEmbodiedActors;
    const int32 MaxCreatureActors = Settings->MaxCreatureActors;

    AllSignificanceQuery.ForEachEntityChunk(Context, [&](FMassExecutionContext &ChunkContext) {
        if (PromotionBudget <= 0 && DemotionBudget <= 0) {
            return;
        }

        const int32 NumEntities = ChunkContext.GetNumEntities();
        auto SignificanceView = ChunkContext.GetMutableFragmentView<FMythicSignificanceFragment>();

        // ─── Creature-hydration gate (embodiment-service-LOCK-v1 §5a) ───
        // Whole chunks are homogeneous by archetype, so this is computed ONCE per chunk. Humanoid BDI hydration
        // (Psychodynamic / Personality / Social fragments + GeneratePersonality + leader-candidate reporting) is
        // skipped for creature entities below — a wolf must NOT get a faction personality. The SHARED parts of
        // promotion (FMythicHydratedTag, the tier decision, FMythicActorSpawnRequestTag) still apply so the creature
        // hydrates + routes through the ActorSpawnProcessor's creature query exactly as before.
        const bool bChunkIsCreature = ChunkContext.DoesArchetypeHaveTag<FMythicCreatureTag>();

        // ─── spawn view-gate (embodiment-service-LOCK-v1 §5b) ───
        // Would embodying this entity pop an actor INTO a player's close view? Tested at the candidate spawn position
        // (the entity's cell center — the actor doesn't exist yet). Honors the stream-in grace bypass: during grace
        // bSpawnGateActive is false, so a gated candidate embodies immediately (bulk pre-population). A gated candidate
        // is NOT promoted to Tier2 this tick — it stays Tier1 (hydrated, bodyless) and is rescored every tick, so it
        // embodies the instant the player looks away or moves past ViewGateMinSpawnDistance (no permanent deferral).
        auto WouldPopInView = [&](const FMythicIdentityFragment &Id) -> bool {
            if (!bSpawnGateActive || !Grid) {
                return false;
            }
            const FVector CandidatePos = Grid->CellToWorld(Id.Cell);
            return IsInCloseView(CandidatePos, PlayerViews, ViewGateMinDist);
        };

        for (int32 i = 0; i < NumEntities && (PromotionBudget > 0 || DemotionBudget > 0); ++i) {
            FMythicSignificanceFragment &Sig = SignificanceView[i];

            // ─── Perma-dead cleanup (combat death of an embodied NPC) — evaluated FIRST ───
            // AMythicNPCCharacter::HandleNPCDeath marks a combat-killed NPC perma-dead, but its entity is still
            // hydrated (Tier1+) and, if Tier2, still embodied. Fully dehydrate it back to ambient Tier0 here so it
            // FREES its cognitive slot. (Dropping only to Tier1 could get STUCK near a player: the Tier1->Tier2
            // IsPermaDead guard `continue`s before the Tier1->Tier0 demotion is ever reached.) The IsPermaDead
            // guards on both promotion branches then permanently block re-embodiment as a zombie.
            if (Sig.Tier >= EMythicSignificanceTier::Tier1_Reactive) {
                const FMythicIdentityFragment &DeadId = ChunkContext.GetFragmentView<FMythicIdentityFragment>()[i];
                UMythicPersistentNPCRegistry *DeadReg = LWS->GetPersistentNPCRegistry();
                if (DeadReg && DeadReg->IsPermaDead(DeadId.NameHash)) {
                    const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                    // Only Tier2 entities carry FMythicCognitiveTag / an embodied actor — request the despawn so the
                    // existing ActorSpawnProcessor consumer removes the tag + destroys the actor. Combat death is
                    // terminal: this is NOT gated by the no-vanish-in-front view-guard (a perma-dead embodied NPC is
                    // being torn down regardless — the actor's own death/corpse path owns the visible removal).
                    if (Sig.Tier == EMythicSignificanceTier::Tier2_Cognitive) {
                        Context.Defer().AddTag<FMythicActorDespawnRequestTag>(Entity);
                        // Drops out of the embodied (Tier2) set — humanoids only count against MaxEmbodiedActors.
                        if (!bChunkIsCreature) {
                            --EmbodiedActorCount;
                        }
                    }
                    // Remove the hydration fragments + drop to Tier0, mirroring the Tier1->Tier0 demotion (single
                    // source of truth for de-hydration), and free the cognitive/creature slot. Creatures carry ONLY the
                    // HydratedTag (the BDI fragments were skipped on their promotion), so only strip those for humanoids.
                    Context.Defer().RemoveTag<FMythicHydratedTag>(Entity);
                    if (!bChunkIsCreature) {
                        Context.Defer().RemoveFragment<FMythicPsychodynamicFragment>(Entity);
                        Context.Defer().RemoveFragment<FMythicPersonalityFragment>(Entity);
                        Context.Defer().RemoveFragment<FMythicSocialFragment>(Entity);
                    }
                    Sig.Tier = EMythicSignificanceTier::Tier0_Ambient;
                    Sig.Score = 0.0f;
                    Sig.RelevantEventCount = 0;
                    if (bChunkIsCreature) { --CreatureActiveCount; } else { --CognitiveActorCount; }
                    continue;
                }
            }

            // ─── Tier 0 → Tier 1 Promotion ───
            if (PromotionBudget > 0
                && Sig.Tier == EMythicSignificanceTier::Tier0_Ambient
                && QualifiesForPromotion(Sig.Score, PromotionThreshold, Hysteresis)) {

                // Enforce the hard cap — creatures against their OWN budget (MaxCreatureActors) so wildlife never
                // starves the humanoid cognitive cap (the bug: a creature-dense wild filled MaxCognitiveActors and a
                // crossing merchant could never get a slot to promote).
                if (bChunkIsCreature ? (CreatureActiveCount >= MaxCreatureActors)
                                     : (CognitiveActorCount >= MaxCognitiveActors)) {
                    continue;
                }

                // Promote: add hydration fragments via deferred commands
                const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                const FMythicIdentityFragment &Identity = ChunkContext.GetFragmentView<FMythicIdentityFragment>()[i];

                // Check PersistentNPCRegistry — dead NPCs cannot be promoted. This MUST run BEFORE drawing the promotion
                // budget + cognitive count below: a perma-dead Tier0 NPC selected for promotion would otherwise leak a
                // per-frame promotion slot (--PromotionBudget) + inflate CognitiveActorCount before the `continue` bails,
                // starving real promotions that frame.
                UMythicPersistentNPCRegistry *Registry = LWS->GetPersistentNPCRegistry();
                if (Registry && Registry->IsPermaDead(Identity.NameHash)) {
                    Sig.Score = 0.0f; // Suppress score
                    continue; // Skip promotion, this NPC is dead forever
                }

                --PromotionBudget;
                if (bChunkIsCreature) { ++CreatureActiveCount; } else { ++CognitiveActorCount; }


                // SHARED hydration marker — both humanoids AND creatures become Tier1+ entities (the creature gate
                // below skips only the HUMANOID BDI work, not the hydrated-tag / tier decision / spawn-request).
                Context.Defer().AddTag<FMythicHydratedTag>(Entity);

                // ─── Humanoid-only BDI hydration (embodiment-service-LOCK-v1 §5a) ───
                // Creatures (FMythicCreatureTag chunks) do NOT get a psyche, a faction-biased personality, a social
                // graph, or leader-candidacy — those are human concepts. Skipping the AddFragment calls also keeps the
                // creature archetype clean (no Psychodynamic/Personality/Social columns), which the gameplay debugger
                // surfaces. A wolf still hydrates (Tier1) + can embody (Tier2) via the shared paths above/below.
                if (!bChunkIsCreature) {
                    Context.Defer().AddFragment<FMythicPsychodynamicFragment>(Entity);
                    Context.Defer().AddFragment<FMythicPersonalityFragment>(Entity);
                    Context.Defer().AddFragment<FMythicSocialFragment>(Entity);

                    // ─── Generate personality from faction ideology ───
                    // Full NPC generation pipeline: NameHash + faction ideology → personality
                    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();
                    if (FactionDB && Identity.Faction.IsValid()) {
                        FMythicFactionData FData;
                        if (FactionDB->GetFaction(Identity.Faction, FData)) {
                            // Generate personality biased by faction ideology
                            FMythicPersonalityFragment GenPersonality = FMythicNPCGenerator::GeneratePersonality(
                                Identity.NameHash, FData.Ideology, Identity.RoleTag);

                            // Apply via deferred command (fragment data is set after archetype change)
                            Context.Defer().PushCommand<FMassDeferredChangeCompositionCommand>(
                                [Entity, GenPersonality](FMassEntityManager &Manager) {
                                    if (Manager.IsEntityValid(Entity)) {
                                        FMythicPersonalityFragment *Personality = Manager.GetFragmentDataPtr<FMythicPersonalityFragment>(Entity);
                                        if (Personality) {
                                            *Personality = GenPersonality;
                                        }
                                    }
                                });
                        }
                    }

                    // ─── Leadership succession reporting ───
                    // Report this entity as a potential leader candidate for its faction.
                    // ReportLeaderCandidate only accepts if the score exceeds the current leader's.
                    if (FactionDB && Identity.Faction.IsValid()) {
                        // Route through the subsystem's SimulationLock-guarded wrapper — a direct FactionDB call here
                        // (game thread) would race the sim thread's leader writes (succession + AnnihilateFaction).
                        LWS->ReportLeaderCandidate(
                            Identity.Faction,
                            Entity.Index,
                            Sig.Score);
                    }
                }

                // ─── Promotion to Tier 2 (Actor Spawn) ─── (no hysteresis margin on the spawn threshold)
                // GATED: (1) under the MaxEmbodiedActors ceiling AND (2) embodying would not pop an actor into a
                // player's close view (unless inside the stream-in grace window). A refused/gated candidate stays
                // Tier1 (hydrated, bodyless) and is rescored every tick, so it embodies the moment a slot frees / the
                // player looks away — no permanent deferral. Short-circuit ORDER matters: the && evaluates the cheap
                // QualifiesForPromotion first, then the O(players) view test, so a far/quiet entity costs ~nothing.
                if (QualifiesForPromotion(Sig.Score, Settings->Tier2PromotionThreshold, 0.0f)
                    && (bChunkIsCreature || EmbodiedActorCount < MaxEmbodiedActors)
                    && !WouldPopInView(Identity)) {
                    Sig.Tier = EMythicSignificanceTier::Tier2_Cognitive;
                    if (!bChunkIsCreature) { ++EmbodiedActorCount; } // creatures are bounded by CreatureActiveCount instead

                    // Trigger actual actor spawn here or in PopulationSpawnerProcessor
                    Context.Defer().AddTag<FMythicActorSpawnRequestTag>(Entity);

                    UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Promoted entity DIRECT to Tier2_Cognitive (score=%.2f, NameHash=%u, embodied=%d/%d)"),
                           Sig.Score, Identity.NameHash, EmbodiedActorCount, MaxEmbodiedActors);
                }
                else {
                    Sig.Tier = EMythicSignificanceTier::Tier1_Reactive;

                    UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Promoted entity to Tier1_Reactive (score=%.2f, cognitive=%d/%d)"),
                           Sig.Score, CognitiveActorCount, MaxCognitiveActors);
                }
            }
            // ─── Tier 1 → Tier 2 Promotion ───
            // GATED in the CONDITION (embodiment-service-LOCK-v1 §5b/§5d): under the MaxEmbodiedActors ceiling AND not
            // popping into a player's close view. A gated entity FAILS this `else if` and falls through — but it cannot
            // hit the Tier1->Tier0 demotion below (its score is >= the Tier2 threshold, so QualifiesForDemotion is
            // false), so it simply STAYS Tier1 (hydrated, bodyless) and is rescored every tick. It embodies the moment
            // an embodied slot frees OR the player looks away / moves past ViewGateMinSpawnDistance — no permanent
            // deferral, no leaked PromotionBudget (the && short-circuits BEFORE --PromotionBudget below). Cheap test
            // first (Qualifies), then ceiling, then the O(players) view test last (so a quiet/far Tier1 entity costs
            // ~nothing). The per-i Identity fetch in the condition is a trivial view-index; it's re-fetched in the body.
            else if (PromotionBudget > 0
                && Sig.Tier == EMythicSignificanceTier::Tier1_Reactive
                && QualifiesForPromotion(Sig.Score, Settings->Tier2PromotionThreshold, 0.0f)
                && (bChunkIsCreature || EmbodiedActorCount < MaxEmbodiedActors)
                && !WouldPopInView(ChunkContext.GetFragmentView<FMythicIdentityFragment>()[i])) {

                const FMythicIdentityFragment &Identity = ChunkContext.GetFragmentView<FMythicIdentityFragment>()[i];

                // Check PersistentNPCRegistry — dead NPCs cannot be promoted
                UMythicPersistentNPCRegistry *Registry = LWS->GetPersistentNPCRegistry();
                if (Registry && Registry->IsPermaDead(Identity.NameHash)) {
                    Sig.Score = 0.0f; // Suppress score
                    continue; // Skip promotion, this NPC is dead forever
                }

                // Draw the per-frame promotion budget — every sibling tier-transition branch does. Without this the
                // Tier1->Tier2 path bypassed MaxPromotionsPerFrame and burst-spawned every qualifying entity's cognitive
                // actor in ONE frame (while DemotionBudget kept the outer loop alive), causing a hitch.
                --PromotionBudget;
                Sig.Tier = EMythicSignificanceTier::Tier2_Cognitive;
                if (!bChunkIsCreature) { ++EmbodiedActorCount; } // humanoids count against MaxEmbodiedActors; creatures use CreatureActiveCount

                const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                Context.Defer().AddTag<FMythicActorSpawnRequestTag>(Entity);

                UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Promoted entity to Tier2_Cognitive (score=%.2f, NameHash=%u, embodied=%d/%d)"),
                       Sig.Score, Identity.NameHash, EmbodiedActorCount, MaxEmbodiedActors);
            }
            // ─── Tier 1 → Tier 0 Demotion ───
            else if (DemotionBudget > 0
                && Sig.Tier == EMythicSignificanceTier::Tier1_Reactive
                && QualifiesForDemotion(Sig.Score, DemotionThreshold, Hysteresis)) {
                --DemotionBudget;
                if (bChunkIsCreature) { --CreatureActiveCount; } else { --CognitiveActorCount; }

                // Demote: remove hydration fragments. Creatures carry ONLY the HydratedTag (BDI fragments were skipped
                // on their promotion), so only strip those for humanoids.
                const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                Context.Defer().RemoveTag<FMythicHydratedTag>(Entity);
                if (!bChunkIsCreature) {
                    Context.Defer().RemoveFragment<FMythicPsychodynamicFragment>(Entity);
                    Context.Defer().RemoveFragment<FMythicPersonalityFragment>(Entity);
                    Context.Defer().RemoveFragment<FMythicSocialFragment>(Entity);
                }

                Sig.Tier = EMythicSignificanceTier::Tier0_Ambient;
                Sig.RelevantEventCount = 0;

                UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Demoted entity to Tier0_Ambient (score=%.2f, cognitive=%d/%d)"),
                       Sig.Score, CognitiveActorCount, MaxCognitiveActors);
            }
            // ─── Tier 2 → Tier 1 Demotion (dehydrate: despawn the embodied actor) ───
            // no-vanish-in-front (embodiment-service-LOCK-v1 §5b): do NOT request a despawn while the embodied
            // actor is in a player's close view — keep it Tier2 (still embodied) and re-evaluate next tick; it
            // despawns the instant the player looks away or moves past ViewGateMinSpawnDistance. Grace is NOT applied
            // to despawn (bDespawnGateActive ignores it) — grace exists only to BULK-EMBODY on arrival, never to keep
            // a de-significant actor alive longer. The actor's REAL location is the gate target (more accurate than the
            // entity cell for a moving NPC, and it matches the actor-side guard in ActorSpawnProcessor); if the actor is
            // already gone (no reverse link) there's nothing visible to protect, so the gate is skipped. This is the
            // FINAL clause of the if/else chain, so a gated entity falls through and simply stays Tier2 this tick.
            else if (DemotionBudget > 0
                && Sig.Tier == EMythicSignificanceTier::Tier2_Cognitive
                && QualifiesForDemotion(Sig.Score, DemotionThreshold, Hysteresis)
                // EXEMPT party companions — they stay embodied while following the player (see resolve above).
                && !(PartySubsystem && PartySubsystem->IsCompanionEntity(ChunkContext.GetEntity(i)))
                && !IsEmbodiedActorInCloseView(LWS, ChunkContext.GetEntity(i), bDespawnGateActive, PlayerViews, ViewGateMinDist)) {
                // Drop to Tier1_Reactive (stays hydrated + still counted in CognitiveActorCount, which counts
                // Tier1+) and request that the MASS->actor bridge despawn its actor. Do NOT decrement
                // CognitiveActorCount here - the entity remains Tier1+, so only the Tier1->Tier0 step leaves
                // that set. The freed ACTOR is handled by the bridge's despawn consumer.
                --DemotionBudget;
                if (!bChunkIsCreature) { --EmbodiedActorCount; } // only humanoids count against MaxEmbodiedActors (creature stays in CreatureActiveCount)
                const FMassEntityHandle Entity = ChunkContext.GetEntity(i);
                Context.Defer().AddTag<FMythicActorDespawnRequestTag>(Entity);
                Sig.Tier = EMythicSignificanceTier::Tier1_Reactive;

                UE_LOG(LogMythLivingWorld, Log, TEXT("Significance: Demoted entity Tier2->Tier1, despawn requested (score=%.2f, cognitive=%d/%d)"),
                       Sig.Score, CognitiveActorCount, MaxCognitiveActors);
            }
        }
    });
}

float UMythicSignificanceProcessor::ComputeProximityScore(const FMythicCellCoord &EntityCell, TConstArrayView<FMythicCellCoord> PlayerCells, float SpawnRadius) {
    // Inverse cell (Manhattan) distance to the NEAREST player, normalized by the spawn radius → [0,1]. No players → 0.
    float ProximityScore = 0.0f;
    const float SafeRadius = FMath::Max(SpawnRadius, 1.0f);
    for (const FMythicCellCoord &PlayerCell : PlayerCells) {
        const float CellDist = static_cast<float>(
            FMath::Abs(EntityCell.X - PlayerCell.X) + FMath::Abs(EntityCell.Y - PlayerCell.Y));
        ProximityScore = FMath::Max(ProximityScore, FMath::Max(0.0f, 1.0f - (CellDist / SafeRadius)));
    }
    return ProximityScore;
}

bool UMythicSignificanceProcessor::IsInCloseView(const FVector &WorldPos, TConstArrayView<FMythicPlayerView> PlayerViews, float MinSpawnDistance) {
    // Mirrors UMythicActorSpawnProcessor::IsActorInCloseView so the spawn-gate (here) and the despawn-gate (there) use
    // identical math. For each cached player view: if WorldPos is within MinSpawnDistance AND inside the precomputed
    // (margin-widened) view cone, it would pop/vanish in that player's close view. Pure dot+compare, no allocation,
    // no traces — O(players). Empty PlayerViews → false (nothing to gate against).
    const float MinDistSq = MinSpawnDistance * MinSpawnDistance;

    for (const FMythicPlayerView &View : PlayerViews) {
        const FVector ToPos = WorldPos - View.CamLocation;
        const float DistSq = ToPos.SizeSquared();

        // Beyond ViewGateMinSpawnDistance a pop/vanish is imperceptible — not gated.
        if (DistSq > MinDistSq) {
            continue;
        }

        const FVector DirToPos = ToPos.GetSafeNormal();
        if (DirToPos.IsNearlyZero()) {
            // Camera essentially on top of the candidate position — treat as in-view (don't pop/vanish here).
            return true;
        }
        const float CosAngle = FVector::DotProduct(View.CamForward, DirToPos);
        if (CosAngle >= View.CosHalfCone) {
            return true; // Inside a player's close view cone.
        }
    }

    return false;
}

bool UMythicSignificanceProcessor::QualifiesForPromotion(float Score, float Threshold, float Hysteresis) {
    // Promote only once the score clears the threshold by the full hysteresis margin (>= so an exact hit promotes).
    return Score >= Threshold + Hysteresis;
}

bool UMythicSignificanceProcessor::QualifiesForDemotion(float Score, float Threshold, float Hysteresis) {
    // Demote only once the score has fallen the full hysteresis margin below the threshold (<= so an exact hit demotes).
    return Score <= Threshold - Hysteresis;
}

bool UMythicSignificanceProcessor::ShouldRescore(bool bDirty, EMythicSignificanceTier Tier) {
    // Ambient (Tier0) entities are rescored only on an event (bDirty) — there are vast numbers of them and they only
    // matter when something happens. The cognitive hot-set (Tier1+) is CAPPED (MaxCognitiveActors) and always
    // re-evaluated so its proximity stays current and it demotes once players move away (no stale-score slot leak).
    return bDirty || Tier != EMythicSignificanceTier::Tier0_Ambient;
}
