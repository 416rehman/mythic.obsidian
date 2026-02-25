// Mythic Living World — Party Subsystem Implementation
// Companion management with loyalty, betrayal, and belief sharing.

#include "AI/Party/PartySubsystem.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogMythParty);

// ─────────────────────────────────────────────────────────────
// Subsystem Lifecycle
// ─────────────────────────────────────────────────────────────

bool UMythicPartySubsystem::ShouldCreateSubsystem(UObject* Outer) const {
    if (const UWorld* World = Cast<UWorld>(Outer)) {
        return World->IsGameWorld();
    }
    return false;
}

void UMythicPartySubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);

    if (UGameInstance* GI = GetWorld()->GetGameInstance()) {
        if (UMythicLivingWorldSubsystem* LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
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
    PlayerParties.Empty();
    Super::Deinitialize();
}

// ─────────────────────────────────────────────────────────────
// Party Management
// ─────────────────────────────────────────────────────────────

bool UMythicPartySubsystem::AddCompanion(int32 PlayerId, AMythicNPCCharacter* NPC, FMassEntityHandle SourceEntity) {
    if (!NPC) {
        UE_LOG(LogMythParty, Warning, TEXT("AddCompanion: Null NPC"));
        return false;
    }

    // Check party size
    TArray<FMythicPartyMember>& Party = PlayerParties.FindOrAdd(PlayerId);
    if (Party.Num() >= MaxPartySize) {
        UE_LOG(LogMythParty, Warning,
               TEXT("AddCompanion: Party full for Player %d (%d/%d)"),
               PlayerId, Party.Num(), MaxPartySize);
        return false;
    }

    // Check if already in party
    for (const FMythicPartyMember& Member : Party) {
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

    // Cache faction from the NPC's brain component
    if (UMythicCognitiveBrainComponent* Brain = NPC->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
        NewMember.OriginalFaction = Brain->GetFaction();
    }

    Party.Add(MoveTemp(NewMember));

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

bool UMythicPartySubsystem::RemoveCompanion(int32 PlayerId, AMythicNPCCharacter* NPC, bool bVoluntary) {
    TArray<FMythicPartyMember>* Party = PlayerParties.Find(PlayerId);
    if (!Party) {
        return false;
    }

    for (int32 i = 0; i < Party->Num(); ++i) {
        if ((*Party)[i].NPCActor.Get() == NPC) {
            UE_LOG(LogMythParty, Log,
                   TEXT("Companion removed from Player %d party (%s)"),
                   PlayerId, bVoluntary ? TEXT("voluntary") : TEXT("forced"));

            Party->RemoveAtSwap(i);
            return true;
        }
    }

    return false;
}

int32 UMythicPartySubsystem::GetPartyMembers(int32 PlayerId, TArray<FMythicPartyMember>& OutMembers) const {
    OutMembers.Reset();

    const TArray<FMythicPartyMember>* Party = PlayerParties.Find(PlayerId);
    if (!Party) {
        return 0;
    }

    OutMembers = *Party;
    return OutMembers.Num();
}

int32 UMythicPartySubsystem::GetPartySize(int32 PlayerId) const {
    const TArray<FMythicPartyMember>* Party = PlayerParties.Find(PlayerId);
    return Party ? Party->Num() : 0;
}

bool UMythicPartySubsystem::IsInParty(const AMythicNPCCharacter* NPC) const {
    for (const auto& Pair : PlayerParties) {
        for (const FMythicPartyMember& Member : Pair.Value) {
            if (Member.NPCActor.Get() == NPC) {
                return true;
            }
        }
    }
    return false;
}

// ─────────────────────────────────────────────────────────────
// Event Handling
// ─────────────────────────────────────────────────────────────

void UMythicPartySubsystem::OnPlayerAction(
    int32 PlayerId,
    const FGameplayTag& EventTag,
    EMythicMoralAxis MoralAxis,
    float MoralValue) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicParty_OnPlayerAction);

    TArray<FMythicPartyMember>* Party = PlayerParties.Find(PlayerId);
    if (!Party || Party->Num() == 0) {
        return;
    }

    for (int32 i = 0; i < Party->Num(); ++i) {
        FMythicPartyMember& Member = (*Party)[i];

        // Evaluate loyalty impact
        const float LoyaltyDelta = EvaluateLoyaltyImpact(Member, MoralAxis, MoralValue);
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
    TArray<FMythicPartyMember>* Party = PlayerParties.Find(PlayerId);
    if (!Party) return;

    for (FMythicPartyMember& Member : *Party) {
        Member.bInRestPhase = true;
    }

    // Propagate beliefs between companions during rest
    PropagateBeliefs(PlayerId);

    // Slight loyalty recovery during rest (bonding over campfire)
    for (FMythicPartyMember& Member : *Party) {
        Member.LoyaltyScore = FMath::Min(Member.LoyaltyScore + 0.02f, 1.0f);

        // Slight betrayal pressure decay during rest
        Member.BetrayalPressure = FMath::Max(0.0f, Member.BetrayalPressure - 0.1f);
    }

    UE_LOG(LogMythParty, Verbose,
           TEXT("Player %d party entered rest. %d members, beliefs propagated."),
           PlayerId, Party->Num());
}

void UMythicPartySubsystem::ExitRestPhase(int32 PlayerId) {
    TArray<FMythicPartyMember>* Party = PlayerParties.Find(PlayerId);
    if (!Party) return;

    for (FMythicPartyMember& Member : *Party) {
        Member.bInRestPhase = false;
    }
}

// ─────────────────────────────────────────────────────────────
// Internal Operations
// ─────────────────────────────────────────────────────────────

void UMythicPartySubsystem::PropagateBeliefs(int32 PlayerId) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicParty_PropagateBeliefs);

    TArray<FMythicPartyMember>* Party = PlayerParties.Find(PlayerId);
    if (!Party || Party->Num() < 2) {
        return; // Need at least 2 members for propagation
    }

    // Each member shares their top beliefs with every other member
    for (int32 i = 0; i < Party->Num(); ++i) {
        FMythicPartyMember& Source = (*Party)[i];

        // Get beliefs from the source's cognitive brain
        AMythicNPCCharacter* SourceNPC = Source.NPCActor.Get();
        if (!SourceNPC) continue;

        UMythicCognitiveBrainComponent* SourceBrain = SourceNPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
        if (!SourceBrain) continue;

        const TArray<FMythicBelief>& SourceBeliefs = SourceBrain->GetBeliefs();

        for (int32 j = 0; j < Party->Num(); ++j) {
            if (i == j) continue;

            FMythicPartyMember& Target = (*Party)[j];
            AMythicNPCCharacter* TargetNPC = Target.NPCActor.Get();
            if (!TargetNPC) continue;

            UMythicCognitiveBrainComponent* TargetBrain = TargetNPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
            if (!TargetBrain) continue;

            // Propagate top beliefs with confidence decay
            for (const FMythicBelief& Belief : SourceBeliefs) {
                if (Belief.Confidence < 0.3f) continue; // Don't share weak beliefs

                FMythicBelief PropagatedBelief = Belief;
                PropagatedBelief.Confidence *= (1.0f - BeliefPropagationDecay);
                PropagatedBelief.PropagationHops += 1;

                TargetBrain->InjectBelief(PropagatedBelief);
            }
        }
    }
}

float UMythicPartySubsystem::EvaluateLoyaltyImpact(
    const FMythicPartyMember& Member,
    EMythicMoralAxis MoralAxis,
    float MoralValue) const {
    // The companion evaluates the player's action against their own faction's ideology.
    // Actions that align with the companion's moral surface boost loyalty.
    // Actions that violate it damage loyalty.

    AMythicNPCCharacter* NPC = Member.NPCActor.Get();
    if (!NPC) return 0.0f;

    UMythicCognitiveBrainComponent* Brain = NPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
    if (!Brain) return 0.0f;

    // Get companion's faction ideology to evaluate the moral action
    FMythicFactionData CompanionFactionData;
    UMythicLivingWorldSubsystem* LWS = GetWorld()->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LWS || !LWS->GetFactionDatabase() || !LWS->GetFactionDatabase()->GetFaction(Member.OriginalFaction, CompanionFactionData)) {
        return 0.0f;
    }

    // Evaluate the action using the companion's faction moral thresholds
    // Build a moral action to evaluate
    FMythicMoralAction MoralAction;
    MoralAction.AxisValues[static_cast<int32>(MoralAxis)] = MoralValue;

    // Use severity evaluation from the companion's perspective
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
            const FMythicPersonalityFragment& PersonalityRef = Brain->GetPersonality();
            const float TendWeight = PersonalityRef.VentWeights[static_cast<int32>(EMythicVentChannel::Tend)];

            // Mercy actions are universally loyalty-positive for empathetic companions
            if (MoralAxis == EMythicMoralAxis::Mercy && MoralValue > 0.0f) {
                return 0.03f * (0.5f + TendWeight);
            }
            return 0.01f; // Mild approval
        }
    }
}

void UMythicPartySubsystem::CheckCompanionThresholds(int32 PlayerId, int32 MemberIndex) {
    TArray<FMythicPartyMember>* Party = PlayerParties.Find(PlayerId);
    if (!Party || !Party->IsValidIndex(MemberIndex)) {
        return;
    }

    const FMythicPartyMember& Member = (*Party)[MemberIndex];

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
    TArray<FMythicPartyMember>* Party = PlayerParties.Find(PlayerId);
    if (!Party || !Party->IsValidIndex(MemberIndex)) {
        return;
    }

    const FMythicPartyMember& Member = (*Party)[MemberIndex];

    UE_LOG(LogMythParty, Log,
           TEXT("Companion departing Player %d party (Loyalty=%.2f below threshold %.2f)"),
           PlayerId, Member.LoyaltyScore, LoyaltyDepartureThreshold);

    // Write departure event to Causal Fabric
    if (CausalFabric) {
        FMythicWorldEvent Event;
        Event.PrimaryFaction = Member.OriginalFaction;
        Event.Significance = 0.4f;
        Event.CategoryFlags = EMythicEventCategory::Social;

        CausalFabric->AppendEvent(Event);
    }

    Party->RemoveAtSwap(MemberIndex);
}

void UMythicPartySubsystem::HandleCompanionBetrayal(int32 PlayerId, int32 MemberIndex) {
    TArray<FMythicPartyMember>* Party = PlayerParties.Find(PlayerId);
    if (!Party || !Party->IsValidIndex(MemberIndex)) {
        return;
    }

    FMythicPartyMember& Member = (*Party)[MemberIndex];

    UE_LOG(LogMythParty, Warning,
           TEXT("Companion BETRAYAL! Player %d (Pressure=%.2f exceeded threshold %.2f)"),
           PlayerId, Member.BetrayalPressure, BetrayalThreshold);

    // Write betrayal event to Causal Fabric — high significance
    if (CausalFabric) {
        FMythicWorldEvent Event;
        Event.PrimaryFaction = Member.OriginalFaction;
        Event.Significance = 0.9f;
        Event.CategoryFlags = EMythicEventCategory::Social | EMythicEventCategory::Combat;

        CausalFabric->AppendEvent(Event);
    }

    // Trigger betrayal behavior via the companion's BDI brain
    AMythicNPCCharacter* NPC = Member.NPCActor.Get();
    if (NPC) {
        UMythicCognitiveBrainComponent* Brain = NPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
        if (Brain) {
            // Inject a high-confidence hostile belief: "player is my enemy"
            // This will cause the BDI brain to naturally generate Fight/Flee/Exploit desires
            FMythicBelief HostileBelief;
            HostileBelief.EventTag = FGameplayTag::RequestGameplayTag(FName("World.Action.Betrayal"));
            HostileBelief.Cell = FMythicCellCoord(0, 0); // Will be overridden by brain
            HostileBelief.Confidence = 1.0f;
            HostileBelief.FormationTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
            HostileBelief.PropagationHops = 0; // Direct experience
            Brain->InjectBelief(HostileBelief);

            // Force an immediate re-think so the NPC reacts right away
            Brain->OnSignificantEvent(
                FGameplayTag::RequestGameplayTag(FName("World.Action.Betrayal")),
                FMythicCellCoord(0, 0));

            UE_LOG(LogMythParty, Log,
                   TEXT("Companion betrayal: injected hostile belief, triggered immediate re-think"));
        }
    }

    Party->RemoveAtSwap(MemberIndex);
}

void UMythicPartySubsystem::Serialize(FArchive& Ar) {
    int32 Version = 1;
    Ar << Version;

    // Serialize the number of player parties
    int32 PartyCount = PlayerParties.Num();
    Ar << PartyCount;

    if (Ar.IsLoading()) {
        PlayerParties.Empty(PartyCount);
    }

    if (Ar.IsSaving()) {
        for (auto& Pair : PlayerParties) {
            int32 PlayerId = Pair.Key;
            Ar << PlayerId;

            int32 MemberCount = Pair.Value.Num();
            Ar << MemberCount;

            for (auto& Member : Pair.Value) {
                Ar << Member.OriginalFaction.Index;
                Ar << Member.LoyaltyScore;
                Ar << Member.BetrayalPressure;
                Ar << Member.JoinTime;
                Ar << Member.bInRestPhase;
                Ar << Member.CachedDisplayName;

                // Serialize shared beliefs count and data
                int32 BeliefCount = Member.SharedBeliefs.Num();
                Ar << BeliefCount;
                for (auto& Belief : Member.SharedBeliefs) {
                    Ar << Belief.EventTag;
                    Ar << Belief.Confidence;
                    Ar << Belief.FormationTime;
                }
            }
        }
    } else {
        // Loading
        for (int32 p = 0; p < PartyCount; ++p) {
            int32 PlayerId = 0;
            Ar << PlayerId;

            int32 MemberCount = 0;
            Ar << MemberCount;

            TArray<FMythicPartyMember>& Party = PlayerParties.Add(PlayerId);
            Party.SetNum(MemberCount);

            for (int32 m = 0; m < MemberCount; ++m) {
                FMythicPartyMember& Member = Party[m];
                Ar << Member.OriginalFaction.Index;
                Ar << Member.LoyaltyScore;
                Ar << Member.BetrayalPressure;
                Ar << Member.JoinTime;
                Ar << Member.bInRestPhase;
                Ar << Member.CachedDisplayName;

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
}

