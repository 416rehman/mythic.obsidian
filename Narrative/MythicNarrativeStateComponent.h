
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MythicNarrativeStateComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicOnStoryTagEarned, FGameplayTag, Tag);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicNarrativeStateComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicNarrativeStateComponent();

    // SERVER: record an earned story tag (idempotent — a container add of an already-present tag is a no-op).
    // Authority-gated; mirrors UMythicFactionStandingComponent::ServerAdjustStanding (a plain call, not a client RPC).
    UFUNCTION(BlueprintCallable, Category = "Narrative")
    void ServerSetStoryTag(FGameplayTag Tag);

    // SERVER: clear a previously-earned story tag (authority-gated). No-op if not present.
    UFUNCTION(BlueprintCallable, Category = "Narrative")
    void ServerClearStoryTag(FGameplayTag Tag);

    // Pure: is Tag owned (hierarchical HasTag)?
    UFUNCTION(BlueprintPure, Category = "Narrative")
    bool HasStoryTag(FGameplayTag Tag) const { return StoryTags.HasTag(Tag); }

    // Pure: are ALL of Tags owned? (empty container → true, per FGameplayTagContainer::HasAll).
    UFUNCTION(BlueprintPure, Category = "Narrative")
    bool HasAll(const FGameplayTagContainer &Tags) const { return StoryTags.HasAll(Tags); }

    // Pure: is ANY of Tags owned? (empty container → false, per FGameplayTagContainer::HasAny).
    UFUNCTION(BlueprintPure, Category = "Narrative")
    bool HasAny(const FGameplayTagContainer &Tags) const { return StoryTags.HasAny(Tags); }

    const FGameplayTagContainer &GetOwnedTags() const { return StoryTags; }

    // Broadcast (server-side) the first time each story tag is earned. See delegate doc above.
    UPROPERTY(BlueprintAssignable, Category = "Narrative")
    FMythicOnStoryTagEarned OnStoryTagEarned;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    // The earned story tags. Owner-only: private per-player narrative state on a net-everyone PlayerState (mirrors
    // FactionStanding::Standings). In-place mutation replicates on the next net update.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Narrative")
    FGameplayTagContainer StoryTags;
};
