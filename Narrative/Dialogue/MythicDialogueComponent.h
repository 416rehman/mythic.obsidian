
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MythicDialogueComponent.generated.h"

class UMythicDialogueGraph;
struct FMythicDialogueNode;
class UMythicNarrativeImportSubsystem;

USTRUCT(BlueprintType)
struct FMythicDialogueClientNode {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    FString NodeId;

    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    FGameplayTag Speaker;

    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    FText Line;

    // Parallel arrays: the offerable choices' display texts + their REAL indices into the server node's Choices.
    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    TArray<FText> ChoiceTexts;

    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    TArray<int32> ChoiceIndices;

    UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
    bool bActive = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicOnDialogueNodeChanged, const FMythicDialogueClientNode &, Node);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnDialogueEnded);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicDialogueComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicDialogueComponent();


    // Open a conversation with NPC: server-validates range, resolves the graph (named tag → role+faction → role →
    // faction via the import subsystem), gates the entry node on the player's owned story tags (ledger ∪ world flags —
    // renown tier mirrors are already ledger tags), and replicates the filtered node. Silently no-ops (with an end
    // broadcast if a conversation was live) when nothing is authored/eligible — the BP bark still plays, so an
    // NPC without a graph just barks as before.
    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Dialogue")
    void ServerStartDialogue(AActor *NPC);

    // Pick choice ChoiceIndex (a REAL index from ChoiceIndices) of node NodeId. The server re-validates EVERYTHING
    // against its own state (active session, matching node, index in range, Condition vs the CURRENT tag set) — a
    // stale pick (node already advanced) or forged index (gated/one-shot choice) is dropped. On success applies the
    // consequence plan, then advances (GotoNodeId whose EntryCondition passes vs the UPDATED tags, choices re-filtered)
    // or ends.
    UFUNCTION(Server, Reliable, WithValidation, BlueprintCallable, Category = "Dialogue")
    void ServerPickChoice(const FString &NodeId, int32 ChoiceIndex);

    // Walk away: clear the session and broadcast the end. Safe to spam (no-op when idle).
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Dialogue")
    void ServerEndDialogue();


    UFUNCTION(BlueprintPure, Category = "Dialogue")
    const FMythicDialogueClientNode &GetCurrentNode() const { return ClientNode; }

    UFUNCTION(BlueprintPure, Category = "Dialogue")
    bool IsDialogueActive() const { return ClientNode.bActive; }


    UPROPERTY(BlueprintAssignable, Category = "Dialogue")
    FMythicOnDialogueNodeChanged OnDialogueNodeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Dialogue")
    FMythicOnDialogueEnded OnDialogueEnded;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_ClientNode)
    FMythicDialogueClientNode ClientNode;

    UFUNCTION()
    void OnRep_ClientNode();


    UPROPERTY(Transient)
    TObjectPtr<UMythicDialogueGraph> ActiveGraph;

    FString ActiveNodeId;

    TWeakObjectPtr<AActor> ActiveNpc;

    TSet<FString> ConsumedRewardChoiceKeys;

private:
    FGameplayTagContainer GatherOwnedTags(const AActor *NpcContext = nullptr) const;

    void WriteClientNode(const FMythicDialogueNode &Node, const FGameplayTagContainer &Owned);

    void EndDialogueInternal();

    void NotifyClientNodeChanged();

    UMythicNarrativeImportSubsystem *ResolveImportSubsystem() const;

    bool bLastNotifiedActive = false;
};
