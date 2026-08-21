
#include "MythicNarrativeStateComponent.h"

#include "Mythic.h"
#include "Net/UnrealNetwork.h"

UMythicNarrativeStateComponent::UMythicNarrativeStateComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicNarrativeStateComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicNarrativeStateComponent, StoryTags, COND_OwnerOnly);
}

void UMythicNarrativeStateComponent::ServerSetStoryTag(FGameplayTag Tag) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Tag.IsValid()) {
        return;
    }
    if (StoryTags.HasTagExact(Tag)) {
        return;
    }
    StoryTags.AddTag(Tag);
    UE_LOG(Myth, Log, TEXT("NarrativeState: %s earned story tag %s"), *GetNameSafe(Owner), *Tag.ToString());
    OnStoryTagEarned.Broadcast(Tag);
}

void UMythicNarrativeStateComponent::ServerClearStoryTag(FGameplayTag Tag) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Tag.IsValid()) {
        return;
    }
    if (StoryTags.RemoveTag(Tag)) {
        UE_LOG(Myth, Log, TEXT("NarrativeState: %s cleared story tag %s"), *GetNameSafe(Owner), *Tag.ToString());
    }
}
