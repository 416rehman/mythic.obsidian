
#include "MythicDialogueComponent.h"

#include "MythicDialogueCore.h"
#include "MythicDialogueGraphTypes.h"
#include "Mythic.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "Narrative/MythicNarrativeImportSubsystem.h"
#include "Narrative/MythicNarrativeStateComponent.h"
#include "Narrative/MythicNarrativeGrant.h"
#include "Narrative/MythicQuestJournalComponent.h"
#include "Narrative/MythicQuestDefinition.h"
#include "Narrative/MythicStorylineDefinition.h"
#include "Player/MythicPlayerState.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/MythicWorldStateSubsystem.h"
#include "World/LivingWorld/Acquaintance/MythicAcquaintanceComponent.h"
#include "World/LivingWorld/Chronicle/MythicDossierComponent.h"

#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"

UMythicDialogueComponent::UMythicDialogueComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicDialogueComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicDialogueComponent, ClientNode, COND_OwnerOnly);
}


bool UMythicDialogueComponent::ServerStartDialogue_Validate(AActor *NPC) {
    return NPC != nullptr;
}

void UMythicDialogueComponent::ServerStartDialogue_Implementation(AActor *NPC) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !IsValid(NPC) || NPC->GetWorld() != GetWorld()) {
        return;
    }
    const APlayerState *PS = Cast<APlayerState>(Owner);
    APlayerController *PC = PS ? PS->GetPlayerController() : nullptr;
    APawn *Pawn = PC ? PC->GetPawn() : nullptr;

    const AMythicNPCCharacter *NpcChar = Cast<AMythicNPCCharacter>(NPC);
    if (NpcChar) {
        if (!NpcChar->IsActorInTradeRange(Pawn)) {
            return;
        }
    }
    else if (!Pawn || FVector::DistSquared(Pawn->GetActorLocation(), NPC->GetActorLocation()) > 250000.0f) {
        return;
    }

    UMythicNarrativeImportSubsystem *Import = ResolveImportSubsystem();
    if (!Import) {
        return;
    }

    const FGameplayTag NamedTag = NpcChar ? NpcChar->GetQuestNpcTag() : FGameplayTag();
    FGameplayTag RoleTag;
    FGameplayTag FactionTag;
    const UMythicCognitiveBrainComponent *Brain = NPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
    if (Brain) {
        RoleTag = Brain->GetRole();
        if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
            if (const UMythicLivingWorldSubsystem *LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                FMythicFactionData FactionData;
                if (LW->GetFactionDatabase() && LW->GetFactionDatabase()->GetFaction(Brain->GetFaction(), FactionData)) {
                    FactionTag = FactionData.FactionTag;
                }
            }
        }
    }

    UMythicDialogueGraph *Graph = Import->ResolveGraphForNpc(NamedTag, RoleTag, FactionTag);
    if (!Graph) {
        return;
    }

    const FGameplayTagContainer Owned = GatherOwnedTags(NPC);
    const FMythicDialogueNode *Entry = FMythicDialogueCore::ResolveEntryNode(*Graph, Owned);
    if (!Entry) {
        EndDialogueInternal();
        return;
    }

    ActiveGraph = Graph;
    ActiveNodeId = Entry->Id;
    ActiveNpc = NPC;
    WriteClientNode(*Entry, Owned);

    if (AMythicPlayerState *MPS = Cast<AMythicPlayerState>(GetOwner())) {
        const uint32 NpcNameHash = GetTypeHash(NPC->GetFName());
        if (UMythicAcquaintanceComponent *Acquaintance = MPS->GetAcquaintanceComponent()) {
            Acquaintance->ServerRecordInteraction(NpcNameHash, FactionTag, EMythicNpcInteraction::Met);
        }
        if (UMythicDossierComponent *DossierComp = MPS->GetDossierComponent()) {
            DossierComp->ServerObserveNpc(NpcNameHash, Brain ? Brain->GetDisplayName() : FText(), FactionTag, RoleTag);
        }
    }
}

bool UMythicDialogueComponent::ServerPickChoice_Validate(const FString &NodeId, int32 ChoiceIndex) {
    return ChoiceIndex >= 0 && ChoiceIndex < 256 && NodeId.Len() <= 256;
}

void UMythicDialogueComponent::ServerPickChoice_Implementation(const FString &NodeId, int32 ChoiceIndex) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (!ClientNode.bActive || !ActiveGraph || NodeId != ActiveNodeId) {
        return;
    }
    const FMythicDialogueNode *Node = ActiveGraph->FindNode(ActiveNodeId);
    if (!Node) {
        EndDialogueInternal();
        return;
    }
    const FGameplayTagContainer Owned = GatherOwnedTags();
    if (!FMythicDialogueCore::IsChoiceValid(*Node, ChoiceIndex, Owned)) {
        UE_LOG(Myth, Warning, TEXT("Dialogue: %s sent invalid choice %d on node '%s' (stale or forged) — dropped."),
               *GetNameSafe(Owner), ChoiceIndex, *NodeId);
        return;
    }

    const FMythicDialogueChoice &Choice = Node->Choices[ChoiceIndex];
    const FMythicDialogueConsequencePlan Plan = FMythicDialogueCore::PlanChoiceConsequences(Choice);

    AMythicPlayerState *MythicPS = Cast<AMythicPlayerState>(GetOwner());

    FMythicNarrativeGrant::RouteGrants(this, MythicPS ? MythicPS->GetNarrativeState() : nullptr, Plan.GrantTags);

    if (Plan.bHasRewards) {
        const FString ConsumedKey = FMythicDialogueCore::MakeChoiceConsumedKey(
            ActiveGraph ? ActiveGraph->GraphId : FString(), ActiveNodeId, ChoiceIndex);
        if (ConsumedRewardChoiceKeys.Contains(ConsumedKey)) {
            UE_LOG(Myth, Log, TEXT("Dialogue: reward choice %d on node '%s' already consumed for %s — reward skipped (idempotent)."),
                   ChoiceIndex, *NodeId, *GetNameSafe(Owner));
        }
        else if (APlayerController *PC = MythicPS ? MythicPS->GetPlayerController() : nullptr) {
            Choice.Rewards.Give(PC);
            ConsumedRewardChoiceKeys.Add(ConsumedKey);
        }
    }

    if (!Plan.QuestOfferId.IsEmpty()) {
        if (UMythicNarrativeImportSubsystem *Import = ResolveImportSubsystem()) {
            if (UMythicQuestDefinition *Quest = Import->GetQuestById(Plan.QuestOfferId)) {
                if (UMythicQuestJournalComponent *Journal = MythicPS ? MythicPS->GetQuestJournal() : nullptr) {
                    Journal->ServerStartQuest(Quest);
                }
            }
            else {
                UE_LOG(Myth, Warning, TEXT("Dialogue: unknown questOfferId '%s' on node '%s' — no quest started."),
                       *Plan.QuestOfferId, *NodeId);
            }
        }
    }

    if (!Plan.StorylineOfferId.IsEmpty()) {
        if (UMythicNarrativeImportSubsystem *Import = ResolveImportSubsystem()) {
            if (UMythicStorylineDefinition *Storyline = Import->GetStorylineById(Plan.StorylineOfferId)) {
                if (UMythicQuestJournalComponent *Journal = MythicPS ? MythicPS->GetQuestJournal() : nullptr) {
                    Journal->ServerStartStoryline(Storyline);
                }
            }
            else {
                UE_LOG(Myth, Warning, TEXT("Dialogue: unknown storylineOfferId '%s' on node '%s' — no storyline started."),
                       *Plan.StorylineOfferId, *NodeId);
            }
        }
    }

    if (Plan.bEnds) {
        EndDialogueInternal();
        return;
    }
    if (!Plan.GotoNodeId.IsEmpty()) {
        if (const FMythicDialogueNode *Next = ActiveGraph->FindNode(Plan.GotoNodeId)) {
            const FGameplayTagContainer OwnedNow = GatherOwnedTags();
            if (FMythicStoryCondition::Evaluate(Next->EntryCondition, OwnedNow)) {
                ActiveNodeId = Next->Id;
                WriteClientNode(*Next, OwnedNow);
                return;
            }
            UE_LOG(Myth, Log, TEXT("Dialogue: goto '%s' entry-gated off for %s — ending conversation."),
                   *Plan.GotoNodeId, *GetNameSafe(Owner));
        }
        else {
            UE_LOG(Myth, Warning, TEXT("Dialogue: unknown gotoNodeId '%s' on node '%s' — ending conversation."),
                   *Plan.GotoNodeId, *NodeId);
        }
    }
    EndDialogueInternal();
}

void UMythicDialogueComponent::ServerEndDialogue_Implementation() {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    EndDialogueInternal();
}


FGameplayTagContainer UMythicDialogueComponent::GatherOwnedTags(const AActor *NpcContext) const {
    FGameplayTagContainer Owned;
    const AMythicPlayerState *MythicPS = Cast<AMythicPlayerState>(GetOwner());
    if (MythicPS) {
        if (const UMythicNarrativeStateComponent *Ledger = MythicPS->GetNarrativeState()) {
            Owned.AppendTags(Ledger->GetOwnedTags());
        }
    }
    if (const UWorld *World = GetWorld()) {
        if (const UMythicWorldStateSubsystem *WorldState = World->GetSubsystem<UMythicWorldStateSubsystem>()) {
            Owned.AppendTags(WorldState->GetWorldFlags());
        }
    }
    if (MythicPS) {
        const AActor *Npc = NpcContext ? NpcContext : ActiveNpc.Get();
        if (Npc) {
            if (const UMythicAcquaintanceComponent *Acquaintance = MythicPS->GetAcquaintanceComponent()) {
                const FGameplayTag WarmthTag =
                    FMythicAcquaintanceRules::TagForTier(Acquaintance->GetWarmthTierForActor(Npc));
                if (WarmthTag.IsValid()) {
                    Owned.AddTag(WarmthTag);
                }
            }
        }
    }
    return Owned;
}

void UMythicDialogueComponent::WriteClientNode(const FMythicDialogueNode &Node, const FGameplayTagContainer &Owned) {
    const TArray<int32> ValidIndices = FMythicDialogueCore::FilterValidChoices(Node, Owned);

    ClientNode = FMythicDialogueClientNode();
    ClientNode.NodeId = Node.Id;
    ClientNode.Speaker = Node.Speaker;
    ClientNode.Line = Node.Line;
    ClientNode.ChoiceTexts.Reserve(ValidIndices.Num());
    ClientNode.ChoiceIndices = ValidIndices;
    for (const int32 Idx : ValidIndices) {
        ClientNode.ChoiceTexts.Add(Node.Choices[Idx].Text);
    }
    ClientNode.bActive = true;
    NotifyClientNodeChanged();
}

void UMythicDialogueComponent::EndDialogueInternal() {
    ActiveGraph = nullptr;
    ActiveNodeId.Reset();
    ActiveNpc = nullptr;
    ClientNode = FMythicDialogueClientNode();
    NotifyClientNodeChanged();
}

void UMythicDialogueComponent::OnRep_ClientNode() {
    NotifyClientNodeChanged();
}

void UMythicDialogueComponent::NotifyClientNodeChanged() {
    if (ClientNode.bActive) {
        bLastNotifiedActive = true;
        OnDialogueNodeChanged.Broadcast(ClientNode);
    }
    else if (bLastNotifiedActive) {
        bLastNotifiedActive = false;
        OnDialogueEnded.Broadcast();
    }
}

UMythicNarrativeImportSubsystem *UMythicDialogueComponent::ResolveImportSubsystem() const {
    const UWorld *World = GetWorld();
    UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    return GI ? GI->GetSubsystem<UMythicNarrativeImportSubsystem>() : nullptr;
}
