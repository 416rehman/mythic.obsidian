
#include "AI/Party/PartySubsystem.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/NPCs/MythicAIController.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicPlayerRegistrySubsystem.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
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
#include "GameFramework/PlayerController.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "World/EnvironmentController/MythicEnvironmentController.h"

DEFINE_LOG_CATEGORY(LogMythParty);

namespace {
void SerializePartyEntityId(FArchive &Ar, FMythicEntityId &EntityId) {
    uint8 Domain = static_cast<uint8>(EntityId.GetDomain());
    FGuid Guid = EntityId.GetAuthorityGuid();
    Ar << Domain;
    Ar << Guid.A;
    Ar << Guid.B;
    Ar << Guid.C;
    Ar << Guid.D;
    if (Ar.IsLoading()) {
        EntityId = FMythicEntityId::FromAuthorityGuid(
            static_cast<EMythicEntityDomain>(Domain), Guid);
    }
}
}


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
            LivingWorld = LW;
            CausalFabric = LW->GetCausalFabric();
            SocialGraph = LW->GetSocialGraph();
            Settings = LW->GetSettings();
        }
    }

    if (!CausalFabric || !Settings) {
        UE_LOG(LogMythParty, Warning, TEXT("PartySubsystem: Living world references not available. Party disabled."));
        return;
    }

    MaxPartySize = Settings->MaxPartySize;
    LoyaltyDepartureThreshold = Settings->LoyaltyDepartureThreshold;
    BetrayalThreshold = Settings->BetrayalPressureThreshold;
    RestLoyaltyRecovery = Settings->CompanionRestLoyaltyRecovery;
    RestBetrayalDecay = Settings->CompanionRestBetrayalDecay;
    BetrayalTriggerDelta = Settings->CompanionBetrayalTriggerDelta;
    BetrayalPressureMultiplier = Settings->CompanionBetrayalPressureMultiplier;
    BeliefPropagationDecay = Settings->BeliefPropagationDecay;
    MaxBeliefPropagationHops = Settings->MaxBeliefPropagationHops;

    UE_LOG(LogMythParty, Log, TEXT("PartySubsystem initialized: MaxPartySize=%d"), MaxPartySize);
}

void UMythicPartySubsystem::Deinitialize() {
    if (AMythicEnvironmentController *Controller = BoundEnvController.Get()) {
        Controller->DayTimeChangeDelegate.RemoveDynamic(this, &UMythicPartySubsystem::OnDaytimeChanged);
    }
    BoundEnvController.Reset();
    PlayerParties.Empty();
    if (CompanionRebuildTickHandle.IsValid()) {
        FTSTicker::GetCoreTicker().RemoveTicker(CompanionRebuildTickHandle);
        CompanionRebuildTickHandle.Reset();
    }
    Super::Deinitialize();
}

void UMythicPartySubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);

    if (InWorld.GetNetMode() == NM_Client) {
        return;
    }
    UGameInstance *GI = InWorld.GetGameInstance();
    UMythicEnvironmentSubsystem *EnvSubsystem = GI ? GI->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr;
    if (!EnvSubsystem) {
        return;
    }
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
        return;
    }
    for (const TPair<FString, TArray<FMythicPartyMember>> &Pair : PlayerParties) {
        if (bEnteringNight) {
            EnterRestPhase(Pair.Key);
        }
        else {
            ExitRestPhase(Pair.Key);
        }
    }
}


FString UMythicPartySubsystem::MakeLegacyPartyKey(int32 LegacyPlayerId) {
    return AMythicPlayerState::ResolveCanonicalPlayerKey(FString(), LegacyPlayerId);
}

void UMythicPartySubsystem::SerializePartyKey(FArchive &Ar, FString &Key, int32 Version) {
    if (Ar.IsLoading() && Version < 4) {
        int32 LegacyId = 0;
        Ar << LegacyId;
        Key = MakeLegacyPartyKey(LegacyId);
        UE_LOG(LogMythParty, Log, TEXT("Party load: migrated legacy int32 key %d -> '%s' (v%d save)."), LegacyId, *Key, Version);
        return;
    }
    Ar << Key;
}

bool UMythicPartySubsystem::AnyPartyContainsEntityIdentity(
    const TMap<FString, TArray<FMythicPartyMember>> &AllParties,
    const FMythicEntityId &EntityId) {
    if (!EntityId.IsValid()) {
        return false;
    }
    for (const TPair<FString, TArray<FMythicPartyMember>> &Pair : AllParties) {
        for (const FMythicPartyMember &Member : Pair.Value) {
            if (Member.PersistedEntityId == EntityId) {
                return true;
            }
        }
    }
    return false;
}

bool UMythicPartySubsystem::AddCompanion(const FString &PlayerKey, AMythicNPCCharacter *NPC, FMassEntityHandle SourceEntity) {
    if (!NPC) {
        UE_LOG(LogMythParty, Warning, TEXT("AddCompanion: Null NPC"));
        return false;
    }

    TArray<FMythicPartyMember> &Party = PlayerParties.FindOrAdd(PlayerKey);
    if (Party.Num() >= MaxPartySize) {
        UE_LOG(LogMythParty, Warning,
               TEXT("AddCompanion: Party full for Player %s (%d/%d)"),
               *PlayerKey, Party.Num(), MaxPartySize);
        return false;
    }

    FMythicEntityId IncomingEntityId;
    if (SourceEntity.IsValid()) {
        if (const UMassEntitySubsystem *ES = GetWorld()->GetSubsystem<UMassEntitySubsystem>()) {
            const FMassEntityManager &EM = ES->GetEntityManager();
            if (EM.IsEntityValid(SourceEntity)) {
                if (const FMythicIdentityFragment *Id = EM.GetFragmentDataPtr<FMythicIdentityFragment>(SourceEntity)) {
                    IncomingEntityId = Id->EntityId;
                }
            }
        }
    }

    for (const TPair<FString, TArray<FMythicPartyMember>> &PartyPair : PlayerParties) {
        for (const FMythicPartyMember &Member : PartyPair.Value) {
            if (Member.NPCActor.Get() == NPC) {
                UE_LOG(LogMythParty, Warning, TEXT("AddCompanion: NPC already in Player %s's party (actor)"), *PartyPair.Key);
                return false;
            }
        }
    }
    if (!IncomingEntityId.IsValid()) {
        UE_LOG(LogMythParty, Warning,
               TEXT("AddCompanion: NPC has no canonical LivingWorld identity"));
        return false;
    }
    if (AnyPartyContainsEntityIdentity(PlayerParties, IncomingEntityId)) {
        UE_LOG(LogMythParty, Warning,
               TEXT("AddCompanion: canonical NPC identity already belongs to a party"));
        return false;
    }

    FMythicPartyMember NewMember;
    NewMember.NPCActor = NPC;
    NewMember.SourceEntity = SourceEntity;
    NewMember.LoyaltyScore = 0.5f;
    NewMember.BetrayalPressure = 0.0f;
    NewMember.JoinTime = GetWorld()->GetTimeSeconds();

    if (UMythicCognitiveBrainComponent *Brain = NPC->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
        NewMember.OriginalFaction = Brain->GetFaction();
        NewMember.CachedDisplayName = Brain->GetDisplayName();
    }

    if (SourceEntity.IsValid()) {
        if (UMassEntitySubsystem *EntitySubsystem = GetWorld()->GetSubsystem<UMassEntitySubsystem>()) {
            const FMassEntityManager &EM = EntitySubsystem->GetEntityManager();
            if (EM.IsEntityValid(SourceEntity)) {
                if (const FMythicIdentityFragment *Id = EM.GetFragmentDataPtr<FMythicIdentityFragment>(SourceEntity)) {
                    NewMember.PersistedEntityId = Id->EntityId;
                    NewMember.PersistedNameSeed = Id->NameSeed;
                    NewMember.OriginalFaction = Id->Faction;
                    NewMember.PersistedTrueFaction = Id->TrueFaction;
                    NewMember.PersistedRoleTag = Id->RoleTag;
                    if (const FMythicScheduleFragment *Sched = EM.GetFragmentDataPtr<FMythicScheduleFragment>(SourceEntity)) {
                        NewMember.PersistedSpawnCell = Sched->HomeCell;
                    }
                    else {
                        NewMember.PersistedSpawnCell = Id->Cell;
                    }
                }
            }
        }
    }

    Party.Add(MoveTemp(NewMember));

    if (SourceEntity.IsValid()) {
        CompanionEntities.Add(SourceEntity);
    }

    if (AMythicAIController *AIC = Cast<AMythicAIController>(NPC->GetController())) {
        AIC->SetCompanionFollow(true, PlayerKey);
    }

    if (SocialGraph && SourceEntity.IsValid()) {
    }

    UE_LOG(LogMythParty, Log,
           TEXT("Companion added to Player %s party (%d/%d)"),
           *PlayerKey, Party.Num(), MaxPartySize);

    return true;
}

bool UMythicPartySubsystem::RemoveCompanion(const FString &PlayerKey, AMythicNPCCharacter *NPC, bool bVoluntary) {
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerKey);
    if (!Party) {
        return false;
    }

    for (int32 i = 0; i < Party->Num(); ++i) {
        if ((*Party)[i].NPCActor.Get() == NPC) {
            UE_LOG(LogMythParty, Log,
                   TEXT("Companion removed from Player %s party (%s)"),
                   *PlayerKey, bVoluntary ? TEXT("voluntary") : TEXT("forced"));

            RemoveMemberAt(PlayerKey, *Party, i);
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
                UE_LOG(LogMythParty, Log, TEXT("Companion removed from Player %s party (death/de-embody, %s)"),
                       *Pair.Key, bVoluntary ? TEXT("voluntary") : TEXT("forced"));
                RemoveMemberAt(Pair.Key, Pair.Value, i);
                return true;
            }
        }
    }
    return false;
}

int32 UMythicPartySubsystem::GetPartyMembers(const FString &PlayerKey, TArray<FMythicPartyMember> &OutMembers) const {
    OutMembers.Reset();

    const TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerKey);
    if (!Party) {
        return 0;
    }

    OutMembers = *Party;
    return OutMembers.Num();
}

int32 UMythicPartySubsystem::GetPartySize(const FString &PlayerKey) const {
    const TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerKey);
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

APawn *UMythicPartySubsystem::GetLeaderPawn(const FString &PlayerKey) const {
    const UWorld *World = GetWorld();
    const UMythicPlayerRegistrySubsystem *Registry = World ? World->GetSubsystem<UMythicPlayerRegistrySubsystem>() : nullptr;
    return Registry ? Registry->GetPawnForKey(PlayerKey) : nullptr;
}

bool UMythicPartySubsystem::IsCompanionEntity(FMassEntityHandle Entity) const {
    return Entity.IsValid() && CompanionEntities.Contains(Entity);
}

void UMythicPartySubsystem::NotifyCompanionLost(const FString &PlayerKey, const FMythicPartyMember &Member, bool bBetrayed) {
    APawn *Leader = GetLeaderPawn(PlayerKey);
    if (!Leader) {
        return;
    }
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(Leader->GetController());
    if (!PC) {
        return;
    }
    const FVector Loc = Member.NPCActor.IsValid() ? Member.NPCActor->GetActorLocation() : Leader->GetActorLocation();
    if (bBetrayed) {
        PC->ClientNotifyCompanionBetrayed(Member.CachedDisplayName, Loc);
    }
    else {
        PC->ClientNotifyCompanionDeparted(Member.CachedDisplayName, Loc);
    }
}

void UMythicPartySubsystem::RemoveMemberAt(const FString &PlayerKey, TArray<FMythicPartyMember> &Party, int32 Index) {
    if (!Party.IsValidIndex(Index)) {
        return;
    }
    const FMythicPartyMember &Member = Party[Index];
    CompanionEntities.Remove(Member.SourceEntity);
    if (AMythicNPCCharacter *NPC = Member.NPCActor.Get()) {
        if (AMythicAIController *AIC = Cast<AMythicAIController>(NPC->GetController())) {
            AIC->SetCompanionFollow(false, FString());
        }
    }
    Party.RemoveAtSwap(Index);
}


void UMythicPartySubsystem::OnPlayerAction(
    const FString &PlayerKey,
    const FGameplayTag &EventTag,
    const FMythicMoralAction &MoralAction) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicParty_OnPlayerAction);

    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerKey);
    if (!Party || Party->Num() == 0) {
        return;
    }

    TWeakObjectPtr<AMythicNPCCharacter> Commenter;
    float CommenterImpact = CompanionCommentaryLoyaltyDelta;

    for (int32 i = Party->Num() - 1; i >= 0; --i) {
        FMythicPartyMember &Member = (*Party)[i];

        const float LoyaltyDelta = EvaluateLoyaltyImpact(Member, MoralAction);
        Member.LoyaltyScore = FMath::Clamp(Member.LoyaltyScore + LoyaltyDelta, 0.0f, 1.0f);

        Member.BetrayalPressure += ComputeBetrayalPressureGain(LoyaltyDelta, BetrayalTriggerDelta, BetrayalPressureMultiplier);

        const float AbsImpact = FMath::Abs(LoyaltyDelta);
        if (AbsImpact > CommenterImpact && Member.LoyaltyScore > LoyaltyDepartureThreshold && Member.NPCActor.IsValid()) {
            CommenterImpact = AbsImpact;
            Commenter = Member.NPCActor;
        }

        CheckCompanionThresholds(PlayerKey, i);
    }

    if (AMythicNPCCharacter *CommenterActor = Commenter.Get()) {
        const TArray<FMythicPartyMember> *NowParty = PlayerParties.Find(PlayerKey);
        bool bStillInParty = false;
        if (NowParty) {
            for (const FMythicPartyMember &M : *NowParty) {
                if (M.NPCActor.Get() == CommenterActor) {
                    bStillInParty = true;
                    break;
                }
            }
        }
        if (bStillInParty) {
            if (const APawn *Leader = GetLeaderPawn(PlayerKey)) {
                if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(Leader->GetController())) {
                    if (const UMythicCognitiveBrainComponent *Brain = CommenterActor->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
                        const FText Line = Brain->SelectDialogue(PC, true, CommenterImpact);
                        if (!Line.IsEmpty()) {
                            PC->ClientReceiveNpcDialogue(CommenterActor, Line);
                        }
                    }
                }
            }
        }
    }
}

void UMythicPartySubsystem::EnterRestPhase(const FString &PlayerKey) {
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerKey);
    if (!Party) {
        return;
    }

    for (FMythicPartyMember &Member : *Party) {
        Member.bInRestPhase = true;
    }

    PropagateBeliefs(PlayerKey);

    for (FMythicPartyMember &Member : *Party) {
        Member.LoyaltyScore = ComputeRestedLoyalty(Member.LoyaltyScore, RestLoyaltyRecovery);
        Member.BetrayalPressure = ComputeDecayedBetrayal(Member.BetrayalPressure, RestBetrayalDecay);
    }

    UE_LOG(LogMythParty, Verbose,
           TEXT("Player %s party entered rest. %d members, beliefs propagated."),
           *PlayerKey, Party->Num());
}

void UMythicPartySubsystem::ExitRestPhase(const FString &PlayerKey) {
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerKey);
    if (!Party) {
        return;
    }

    for (FMythicPartyMember &Member : *Party) {
        Member.bInRestPhase = false;
    }
}


void UMythicPartySubsystem::PropagateBeliefs(const FString &PlayerKey) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicParty_PropagateBeliefs);

    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerKey);
    if (!Party || Party->Num() < 2) {
        return;
    }

    for (int32 i = 0; i < Party->Num(); ++i) {
        FMythicPartyMember &Source = (*Party)[i];

        AMythicNPCCharacter *SourceNPC = Source.NPCActor.Get();
        if (!SourceNPC) {
            continue;
        }

        UMythicCognitiveBrainComponent *SourceBrain = SourceNPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
        if (!SourceBrain) {
            continue;
        }

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

            for (const FMythicBelief &Belief : SourceBeliefs) {
                if (!ShouldShareBelief(Belief, MaxBeliefPropagationHops)) {
                    continue;
                }

                FMythicBelief PropagatedBelief = Belief;
                PropagatedBelief.Confidence *= (1.0f - BeliefPropagationDecay);
                PropagatedBelief.PropagationHops += 1;

                TargetBrain->InjectBelief(PropagatedBelief);
            }
        }
    }
}

bool UMythicPartySubsystem::ShouldShareBelief(const FMythicBelief &Belief, int32 MaxHops) {
    constexpr float MinConfidenceToShare = 0.3f;
    if (Belief.Confidence < MinConfidenceToShare) {
        return false;
    }

    if (Belief.PropagationHops >= MaxHops) {
        return false;
    }

    return true;
}

void UMythicPartySubsystem::SerializeBelief(FArchive &Ar, FMythicBelief &Belief, int32 Version) {
    Ar << Belief.EventTag;
    Ar << Belief.Confidence;
    Ar << Belief.FormationTime;

    if (Version >= 3) {
        Ar << Belief.Cell.X;
        Ar << Belief.Cell.Y;
        Ar << Belief.InvolvedFaction.Index;
        Ar << Belief.LastDecayTime;
        Ar << Belief.PropagationHops;
        Ar << Belief.SourceEventId;
    }
}

float UMythicPartySubsystem::EvaluateLoyaltyImpact(
    const FMythicPartyMember &Member,
    const FMythicMoralAction &MoralAction) const {
    AMythicNPCCharacter *NPC = Member.NPCActor.Get();
    if (!NPC) {
        return 0.0f;
    }

    UMythicCognitiveBrainComponent *Brain = NPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
    if (!Brain) {
        return 0.0f;
    }

    FMythicFactionData CompanionFactionData;
    UMythicLivingWorldSubsystem *LWS = GetWorld()->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>();
    if (!LWS || !LWS->GetFactionDatabase() || !LWS->GetFactionDatabase()->GetFaction(Member.OriginalFaction, CompanionFactionData)) {
        return 0.0f;
    }

    const EMythicMoralSeverity Severity = FMythicMoralSignature::EvaluateActionSeverity(
        MoralAction, CompanionFactionData.Ideology,
        CompanionFactionData.DisapproveThreshold,
        CompanionFactionData.CondemnThreshold,
        CompanionFactionData.HostileThreshold);

    const FMythicPersonalityFragment &PersonalityRef = Brain->GetPersonality();
    const float TendWeight = PersonalityRef.VentWeights[static_cast<int32>(EMythicVentChannel::Tend)];
    const float MercyVal = MoralAction.AxisValues[static_cast<int32>(EMythicMoralAxis::Mercy)];
    return ComputeLoyaltyDelta(Severity, MercyVal, TendWeight);
}

float UMythicPartySubsystem::ComputeLoyaltyDelta(EMythicMoralSeverity Severity, float MercyAxisValue, float TendWeight) {
    switch (Severity) {
    case EMythicMoralSeverity::Hostile:
        return -0.10f;
    case EMythicMoralSeverity::Condemn:
        return -0.06f;
    case EMythicMoralSeverity::Disapprove:
        return -0.02f;
    case EMythicMoralSeverity::Ignore:
    default:
        if (MercyAxisValue > 0.0f) {
            return 0.03f * (0.5f + TendWeight);
        }
        return 0.01f;
    }
}

float UMythicPartySubsystem::ComputeRestedLoyalty(float CurrentLoyalty, float Recovery) {
    return FMath::Min(CurrentLoyalty + Recovery, 1.0f);
}

float UMythicPartySubsystem::ComputeDecayedBetrayal(float CurrentPressure, float Decay) {
    return FMath::Max(0.0f, CurrentPressure - Decay);
}

float UMythicPartySubsystem::ComputeBetrayalPressureGain(float LoyaltyDelta, float TriggerDelta, float Multiplier) {
    if (LoyaltyDelta < TriggerDelta) {
        return FMath::Abs(LoyaltyDelta) * Multiplier;
    }
    return 0.0f;
}

float UMythicPartySubsystem::ComputeForcedComplianceGain(float LoyaltyDelta, float TriggerDelta, float BaseMultiplier, float ForcedComplianceScale) {
    return ComputeBetrayalPressureGain(LoyaltyDelta, TriggerDelta, BaseMultiplier * ForcedComplianceScale);
}

bool UMythicPartySubsystem::DoesOrderConflict(const FMythicPartyMember &Member, EMythicCompanionOrder Order,
                                              AActor *OrderTarget, float &OutLoyaltyDelta) const {
    OutLoyaltyDelta = 0.0f;

    if (Order != EMythicCompanionOrder::AttackTarget) {
        return false;
    }

    const FMythicMoralAction AttackVector = FMythicMoralSignature::MakeKillActionMoralVector();
    OutLoyaltyDelta = EvaluateLoyaltyImpact(Member, AttackVector);

    if (OrderTarget) {
        if (const UMythicCognitiveBrainComponent *TargetBrain = OrderTarget->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
            const FMythicFactionId TargetFaction = TargetBrain->GetFaction();
            if (TargetFaction.IsValid() && TargetFaction == Member.OriginalFaction) {
                OutLoyaltyDelta = FMath::Min(OutLoyaltyDelta, BetrayalTriggerDelta - 0.01f);
            }
        }
    }

    return OutLoyaltyDelta < BetrayalTriggerDelta;
}

bool UMythicPartySubsystem::ResolveOrderBelief(const FString &PlayerKey, const FMythicPartyMember &Member,
                                               EMythicCompanionOrder Order, AActor *OrderTarget,
                                               FGameplayTag &OutTag, FMythicCellCoord &OutCell) const {
    if (!LivingWorld) {
        return false;
    }
    const UMythicTerritoryGrid *Grid = LivingWorld->GetTerritoryGrid();
    if (!Grid) {
        return false;
    }
    AMythicNPCCharacter *NPC = Member.NPCActor.Get();

    switch (Order) {
    case EMythicCompanionOrder::AttackTarget:
        if (!OrderTarget) {
            return false;
        }
        OutTag = TAG_LIVINGWORLD_ACTION_VIOLENCE_ATTACK;
        OutCell = Grid->WorldToCell(OrderTarget->GetActorLocation());
        return true;

    case EMythicCompanionOrder::MoveTo:
        if (!OrderTarget) {
            return false;
        }
        OutTag = TAG_LIVINGWORLD_ACTION_SOCIAL_RALLY;
        OutCell = Grid->WorldToCell(OrderTarget->GetActorLocation());
        return true;

    case EMythicCompanionOrder::Follow: {
        OutTag = TAG_LIVINGWORLD_ACTION_SOCIAL_RALLY;
        if (const APawn *Leader = GetLeaderPawn(PlayerKey)) {
            OutCell = Grid->WorldToCell(Leader->GetActorLocation());
            return true;
        }
        if (NPC) {
            OutCell = Grid->WorldToCell(NPC->GetActorLocation());
            return true;
        }
        return false;
    }

    case EMythicCompanionOrder::Hold:
        if (!NPC) {
            return false;
        }
        OutTag = TAG_LIVINGWORLD_ACTION_SOCIAL_RALLY;
        OutCell = Grid->WorldToCell(NPC->GetActorLocation());
        return true;

    default:
        return false;
    }
}

bool UMythicPartySubsystem::IssueCompanionOrder(const FString &PlayerKey, AMythicNPCCharacter *Companion,
                                                EMythicCompanionOrder Order, AActor *OrderTarget) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicParty_IssueCompanionOrder);

    if (!Companion) {
        return false;
    }
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerKey);
    if (!Party) {
        return false;
    }

    int32 MemberIndex = INDEX_NONE;
    for (int32 i = 0; i < Party->Num(); ++i) {
        if ((*Party)[i].NPCActor.Get() == Companion) {
            MemberIndex = i;
            break;
        }
    }
    if (MemberIndex == INDEX_NONE) {
        return false;
    }

    FMythicPartyMember &Member = (*Party)[MemberIndex];
    UMythicCognitiveBrainComponent *Brain = Companion->FindComponentByClass<UMythicCognitiveBrainComponent>();
    if (!Brain) {
        return false;
    }

    FGameplayTag OrderTag;
    FMythicCellCoord OrderCell;
    if (!ResolveOrderBelief(PlayerKey, Member, Order, OrderTarget, OrderTag, OrderCell)) {
        UE_LOG(LogMythParty, Warning,
               TEXT("IssueCompanionOrder: could not resolve order belief (order=%d — grid/target missing); no-op."),
               static_cast<int32>(Order));
        return false;
    }

    float OrderLoyaltyDelta = 0.0f;
    const bool bConflict = DoesOrderConflict(Member, Order, OrderTarget, OrderLoyaltyDelta);
    if (bConflict) {
        const EMythicPartyEventType EventType = EMythicPartyEventType::ForcedCompliance;
        Member.LoyaltyScore = FMath::Clamp(Member.LoyaltyScore + OrderLoyaltyDelta, 0.0f, 1.0f);
        Member.BetrayalPressure += ComputeForcedComplianceGain(
            OrderLoyaltyDelta, BetrayalTriggerDelta, BetrayalPressureMultiplier, ForcedComplianceScale);

        UE_LOG(LogMythParty, Warning,
               TEXT("ForcedCompliance (event=%d): companion '%s' ordered against its morals (LoyaltyDelta=%.3f, Pressure=%.2f)"),
               static_cast<int32>(EventType), *GetNameSafe(Companion), OrderLoyaltyDelta, Member.BetrayalPressure);

        if (LivingWorld) {
            FMythicWorldEvent Event;
            Event.WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
            Event.EventTag = TAG_WORLD_ACTION_BETRAYAL;
            Event.PrimaryFaction = Member.OriginalFaction;
            Event.Significance = 0.6f;
            Event.CategoryFlags = EMythicEventCategory::Social;
            LivingWorld->SubmitWorldEvent(Event);
        }
    }

    FMythicBelief OrderBelief;
    OrderBelief.EventTag = OrderTag;
    OrderBelief.Cell = OrderCell;
    OrderBelief.InvolvedFaction = Member.OriginalFaction;
    OrderBelief.Confidence = 1.0f;
    OrderBelief.FormationTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    OrderBelief.PropagationHops = 0;
    Brain->InjectBelief(OrderBelief);
    Brain->OnSignificantEvent(OrderTag, OrderCell);

    UE_LOG(LogMythParty, Log,
           TEXT("IssueCompanionOrder: '%s' order=%d belief=%s cell=%s conflict=%d"),
           *GetNameSafe(Companion), static_cast<int32>(Order), *OrderTag.ToString(), *OrderCell.ToString(),
           bConflict ? 1 : 0);

    if (bConflict) {
        CheckCompanionThresholds(PlayerKey, MemberIndex);
    }
    return true;
}

void UMythicPartySubsystem::CheckCompanionThresholds(const FString &PlayerKey, int32 MemberIndex) {
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerKey);
    if (!Party || !Party->IsValidIndex(MemberIndex)) {
        return;
    }

    const FMythicPartyMember &Member = (*Party)[MemberIndex];

    if (Member.BetrayalPressure >= BetrayalThreshold) {
        HandleCompanionBetrayal(PlayerKey, MemberIndex);
        return;
    }

    if (Member.LoyaltyScore <= LoyaltyDepartureThreshold) {
        HandleCompanionDeparture(PlayerKey, MemberIndex);
    }
}

void UMythicPartySubsystem::HandleCompanionDeparture(const FString &PlayerKey, int32 MemberIndex) {
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerKey);
    if (!Party || !Party->IsValidIndex(MemberIndex)) {
        return;
    }

    const FMythicPartyMember &Member = (*Party)[MemberIndex];

    UE_LOG(LogMythParty, Log,
           TEXT("Companion departing Player %s party (Loyalty=%.2f below threshold %.2f)"),
           *PlayerKey, Member.LoyaltyScore, LoyaltyDepartureThreshold);

    if (LivingWorld) {
        FMythicWorldEvent Event;
        Event.WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
        Event.PrimaryFaction = Member.OriginalFaction;
        Event.Significance = 0.4f;
        Event.CategoryFlags = EMythicEventCategory::Social;
        LivingWorld->SubmitWorldEvent(Event);
    }

    NotifyCompanionLost(PlayerKey, Member, false);
    RemoveMemberAt(PlayerKey, *Party, MemberIndex);
}

void UMythicPartySubsystem::HandleCompanionBetrayal(const FString &PlayerKey, int32 MemberIndex) {
    TArray<FMythicPartyMember> *Party = PlayerParties.Find(PlayerKey);
    if (!Party || !Party->IsValidIndex(MemberIndex)) {
        return;
    }

    FMythicPartyMember &Member = (*Party)[MemberIndex];

    UE_LOG(LogMythParty, Warning,
           TEXT("Companion BETRAYAL! Player %s (Pressure=%.2f exceeded threshold %.2f)"),
           *PlayerKey, Member.BetrayalPressure, BetrayalThreshold);

    if (LivingWorld) {
        FMythicWorldEvent Event;
        Event.WorldTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
        Event.EventTag = TAG_WORLD_ACTION_BETRAYAL;
        Event.PrimaryFaction = Member.OriginalFaction;
        Event.Significance = 0.9f;
        Event.CategoryFlags = EMythicEventCategory::Social | EMythicEventCategory::Combat;
        LivingWorld->SubmitWorldEvent(Event);
    }

    AMythicNPCCharacter *NPC = Member.NPCActor.Get();
    if (NPC) {
        UMythicCognitiveBrainComponent *Brain = NPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
        if (Brain) {
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
                FMythicBelief HostileBelief;
                HostileBelief.EventTag = TAG_WORLD_ACTION_BETRAYAL;
                HostileBelief.Cell = BetrayalCell;
                HostileBelief.Confidence = 1.0f;
                HostileBelief.FormationTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
                HostileBelief.PropagationHops = 0;
                Brain->InjectBelief(HostileBelief);

                Brain->OnSignificantEvent(
                    TAG_WORLD_ACTION_BETRAYAL,
                    BetrayalCell);

                UE_LOG(LogMythParty, Log,
                       TEXT("Companion betrayal: injected hostile belief, triggered immediate re-think"));
            }
        }
    }

    NotifyCompanionLost(PlayerKey, Member, true);
    RemoveMemberAt(PlayerKey, *Party, MemberIndex);
}

void UMythicPartySubsystem::Serialize(FArchive &Ar) {
    constexpr int32 CurrentPartySaveVersion = 5;
    int32 Version = CurrentPartySaveVersion;
    Ar << Version;
    if (Ar.IsLoading() && Version != CurrentPartySaveVersion) {
        Ar.SetError();
        return;
    }

    int32 PartyCount = PlayerParties.Num();
    Ar << PartyCount;

    if (Ar.IsLoading() && (PartyCount < 0 || PartyCount > 1024)) {
        Ar.SetError();
        return;
    }

    if (Ar.IsLoading()) {
        PlayerParties.Empty(PartyCount);
        CompanionEntities.Empty();
    }

    if (Ar.IsSaving()) {
        for (auto &Pair : PlayerParties) {
            FString PlayerKey = Pair.Key;
            SerializePartyKey(Ar, PlayerKey, Version);

            int32 MemberCount = Pair.Value.Num();
            Ar << MemberCount;

            for (auto &Member : Pair.Value) {
                Ar << Member.OriginalFaction.Index;
                Ar << Member.LoyaltyScore;
                Ar << Member.BetrayalPressure;
                Ar << Member.JoinTime;
                Ar << Member.bInRestPhase;
                Ar << Member.CachedDisplayName;

                SerializePartyEntityId(Ar, Member.PersistedEntityId);
                Ar << Member.PersistedNameSeed;
                Ar << Member.PersistedTrueFaction.Index;
                Ar << Member.PersistedSpawnCell.X;
                Ar << Member.PersistedSpawnCell.Y;
                Ar << Member.PersistedRoleTag;

                if (AMythicNPCCharacter *NPC = Member.NPCActor.Get()) {
                    if (UMythicCognitiveBrainComponent *Brain = NPC->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
                        Member.SharedBeliefs = Brain->GetBeliefsCopy();
                    }
                }

                int32 BeliefCount = Member.SharedBeliefs.Num();
                Ar << BeliefCount;
                for (auto &Belief : Member.SharedBeliefs) {
                    SerializeBelief(Ar, Belief, Version);
                }
            }
        }
    }
    else {
        const UMythicPersistentNPCRegistry *IdentityRegistry = LivingWorld
            ? LivingWorld->GetPersistentNPCRegistry()
            : nullptr;
        TSet<FString> RestoredPlayerKeys;
        TSet<FMythicEntityId> RestoredEntityIds;
        for (int32 p = 0; p < PartyCount; ++p) {
            FString PlayerKey;
            SerializePartyKey(Ar, PlayerKey, Version);

            if (PlayerKey.IsEmpty() || RestoredPlayerKeys.Contains(PlayerKey)) {
                PlayerParties.Empty();
                Ar.SetError();
                return;
            }
            RestoredPlayerKeys.Add(PlayerKey);

            int32 MemberCount = 0;
            Ar << MemberCount;
            if (MemberCount < 0 || MemberCount > 1024) {
                Ar.SetError();
                return;
            }

            TArray<FMythicPartyMember> &Party = PlayerParties.Add(PlayerKey);
            Party.SetNum(MemberCount);

            for (int32 m = 0; m < MemberCount; ++m) {
                FMythicPartyMember &Member = Party[m];
                Ar << Member.OriginalFaction.Index;
                Ar << Member.LoyaltyScore;
                Ar << Member.BetrayalPressure;
                Ar << Member.JoinTime;
                Ar << Member.bInRestPhase;
                Ar << Member.CachedDisplayName;

                SerializePartyEntityId(Ar, Member.PersistedEntityId);
                if (!IdentityRegistry
                    || !IdentityRegistry->ContainsEntityIdentity(
                        Member.PersistedEntityId)
                    || IdentityRegistry->IsPermaDead(Member.PersistedEntityId)
                    || RestoredEntityIds.Contains(Member.PersistedEntityId)) {
                    PlayerParties.Empty();
                    Ar.SetError();
                    return;
                }
                RestoredEntityIds.Add(Member.PersistedEntityId);
                Ar << Member.PersistedNameSeed;
                Ar << Member.PersistedTrueFaction.Index;
                Ar << Member.PersistedSpawnCell.X;
                Ar << Member.PersistedSpawnCell.Y;
                Ar << Member.PersistedRoleTag;
                if (Member.PersistedEntityId.IsValid()) {
                    Member.RebuildState = EMythicCompanionRebuildState::NotCreated;
                }

                int32 BeliefCount = 0;
                Ar << BeliefCount;
                if (BeliefCount < 0 || BeliefCount > 4096) {
                    Ar.SetError();
                    return;
                }
                Member.SharedBeliefs.SetNum(BeliefCount);
                for (int32 b = 0; b < BeliefCount; ++b) {
                    SerializeBelief(Ar, Member.SharedBeliefs[b], Version);
                }
            }
        }
    }

    if (Ar.IsLoading()) {
        RebindCompanionsAfterLoad();
    }
}


void UMythicPartySubsystem::RebindCompanionsAfterLoad() {
    check(IsInGameThread());
    if (CompanionRebuildTickHandle.IsValid()) {
        FTSTicker::GetCoreTicker().RemoveTicker(CompanionRebuildTickHandle);
        CompanionRebuildTickHandle.Reset();
    }

    bool bAny = false;
    for (const TPair<FString, TArray<FMythicPartyMember>> &Pair : PlayerParties) {
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
    CompanionRebuildTickHandle = FTSTicker::GetCoreTicker().AddTicker(
        FTickerDelegate::CreateUObject(this, &UMythicPartySubsystem::TickCompanionRebuild), 0.0f);
}

bool UMythicPartySubsystem::TickCompanionRebuild(float) {
    check(IsInGameThread());
    ++CompanionRebuildAttempts;

    UWorld *World = GetWorld();
    UMythicLivingWorldSubsystem *LWS = LivingWorld;
    if (!World || !LWS) {
        CompanionRebuildTickHandle.Reset();
        return false;
    }

    bool bAnyPending = false;

    for (TPair<FString, TArray<FMythicPartyMember>> &Pair : PlayerParties) {
        const FString &PlayerKey = Pair.Key;
        TArray<FMythicPartyMember> &Party = Pair.Value;

        for (int32 i = 0; i < Party.Num(); ++i) {
            FMythicPartyMember &Member = Party[i];

            if (Member.RebuildState == EMythicCompanionRebuildState::Bound) {
                continue;
            }
            else if (Member.RebuildState == EMythicCompanionRebuildState::NotCreated) {
                const FMassEntityHandle NewEntity = CreateLoadedCompanionEntity(Member);
                if (NewEntity.IsValid()) {
                    Member.SourceEntity = NewEntity;
                    CompanionEntities.Add(NewEntity);
                    Member.RebuildState = EMythicCompanionRebuildState::EntityCreated;
                }
                else {
                    bAnyPending = true;
                    if (CompanionRebuildAttempts == 1
                        || CompanionRebuildAttempts % 120 == 0) {
                        UE_LOG(LogMythParty, Warning,
                           TEXT("Companion rebuild: entity create skipped/failed for %s (dead, unregistered, or no Mass) — companion not restored."),
                               *Member.PersistedEntityId.ToDebugString());
                    }
                    continue;
                }
                bAnyPending = true;
            }
            else {
                if (AMythicNPCCharacter *Actor = LWS->FindEmbodiedActor(Member.SourceEntity)) {
                    RebindLoadedCompanion(PlayerKey, Member, Actor);
                    Member.RebuildState = EMythicCompanionRebuildState::Bound;
                }
                else {
                    bAnyPending = true;
                }
            }
        }
    }

    if (!bAnyPending) {
        CompanionRebuildTickHandle.Reset();
        return false;
    }

    if (CompanionRebuildAttempts >= 600) {
        TArray<FMythicEntityId> UnrestorableEntityIds;
        for (TPair<FString, TArray<FMythicPartyMember>> &Pair : PlayerParties) {
            TArray<FMythicPartyMember> &Party = Pair.Value;
            for (int32 Index = Party.Num() - 1; Index >= 0; --Index) {
                if (Party[Index].RebuildState
                    == EMythicCompanionRebuildState::NotCreated) {
                    UnrestorableEntityIds.Add(Party[Index].PersistedEntityId);
                    Party.RemoveAt(Index);
                }
            }
        }
        for (const FMythicEntityId &EntityId : UnrestorableEntityIds) {
            LWS->HandleUnrestorableLogicalEntity(EntityId);
        }
        UE_LOG(LogMythParty, Warning,
               TEXT("Companion rebuild: timed out after %d attempts with members still pending embodiment — giving up."),
               CompanionRebuildAttempts);
        CompanionRebuildTickHandle.Reset();
        return false;
    }

    return true;
}

FMassEntityHandle UMythicPartySubsystem::CreateLoadedCompanionEntity(const FMythicPartyMember &Member) {
    check(IsInGameThread());
    UWorld *World = GetWorld();
    if (!World) { return FMassEntityHandle(); }

    UMassEntitySubsystem *MassSubsystem = World->GetSubsystem<UMassEntitySubsystem>();
    UMythicLivingWorldSubsystem *LWS = LivingWorld;
    if (!MassSubsystem || !LWS) { return FMassEntityHandle(); }

    UMythicPersistentNPCRegistry *Registry = LWS->GetPersistentNPCRegistry();
    if (!Registry || !Registry->ContainsEntityIdentity(Member.PersistedEntityId)
        || Registry->IsPermaDead(Member.PersistedEntityId)) {
        return FMassEntityHandle();
    }

    FMassEntityManager &EntityManager = MassSubsystem->GetMutableEntityManager();

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

    FMythicIdentityFragment &Identity = EntityManager.GetFragmentDataChecked<FMythicIdentityFragment>(Entity);
    Identity.EntityId = Member.PersistedEntityId;
    Identity.NameSeed = Member.PersistedNameSeed;
    Identity.Faction = Member.OriginalFaction;
    Identity.TrueFaction = Member.PersistedTrueFaction;
    Identity.Cell = Member.PersistedSpawnCell;
    Identity.RoleTag = Member.PersistedRoleTag;
    Identity.VisualArchetype = FMythicNPCGenerator::GenerateVisualArchetype(Member.PersistedNameSeed, 8);

    FMythicScheduleFragment &Schedule = EntityManager.GetFragmentDataChecked<FMythicScheduleFragment>(Entity);
    Schedule.Phase = EMythicSchedulePhase::Idle;
    Schedule.HomeCell = Member.PersistedSpawnCell;
    Schedule.WorkCell = Member.PersistedSpawnCell;

    FMythicSignificanceFragment &Sig = EntityManager.GetFragmentDataChecked<FMythicSignificanceFragment>(Entity);
    Sig.Tier = EMythicSignificanceTier::Tier2_Cognitive;
    Sig.Score = 1.0f;
    Sig.bDirty = false;

    TSharedPtr<FMassCommandBuffer> CmdBuffer = MakeShared<FMassCommandBuffer>();
    CmdBuffer->AddTag<FMythicHydratedTag>(Entity);
    CmdBuffer->AddFragment<FMythicPsychodynamicFragment>(Entity);
    CmdBuffer->AddFragment<FMythicPersonalityFragment>(Entity);
    CmdBuffer->AddFragment<FMythicSocialFragment>(Entity);
    EntityManager.FlushCommands(CmdBuffer);

    UMythicFactionDatabase *FactionDB = LWS->GetFactionDatabase();
    FMythicPersonalityFragment *Personality = EntityManager.GetFragmentDataPtr<FMythicPersonalityFragment>(Entity);
    if (Personality && FactionDB && Identity.Faction.IsValid()) {
        FMythicFactionData FData;
        if (FactionDB->GetFaction(Identity.Faction, FData)) {
            *Personality = FMythicNPCGenerator::GeneratePersonality(
                Identity.NameSeed, FData.Ideology, Identity.RoleTag);
        }
    }

    if (!Personality) {
        UE_LOG(LogMythParty, Error,
               TEXT("Companion rebuild: Personality fragment missing after hydrate for %s — aborting embodiment."),
               *Member.PersistedEntityId.ToDebugString());
        EntityManager.DestroyEntity(Entity);
        return FMassEntityHandle();
    }

    TSharedPtr<FMassCommandBuffer> SpawnCmd = MakeShared<FMassCommandBuffer>();
    SpawnCmd->AddTag<FMythicActorSpawnRequestTag>(Entity);
    EntityManager.FlushCommands(SpawnCmd);

    UE_LOG(LogMythParty, Log,
           TEXT("Companion rebuild: re-created entity (%s) at cell (%d,%d), Tier2 + spawn-request tagged."),
           *Member.PersistedEntityId.ToDebugString(), Member.PersistedSpawnCell.X, Member.PersistedSpawnCell.Y);

    return Entity;
}

void UMythicPartySubsystem::RebindLoadedCompanion(const FString &PlayerKey, FMythicPartyMember &Member, AMythicNPCCharacter *Actor) {
    Member.NPCActor = Actor;

    if (UMythicCognitiveBrainComponent *Brain = Actor->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
        for (const FMythicBelief &Belief : Member.SharedBeliefs) {
            Brain->InjectBelief(Belief);
        }
    }

    if (AMythicAIController *AIC = Cast<AMythicAIController>(Actor->GetController())) {
        AIC->SetCompanionFollow(true, PlayerKey);
    }

    UE_LOG(LogMythParty, Log,
           TEXT("Companion rebuild: rebound Player %s slot to re-embodied actor (%s, Loyalty=%.2f)."),
           *PlayerKey, *Member.PersistedEntityId.ToDebugString(), Member.LoyaltyScore);
}
