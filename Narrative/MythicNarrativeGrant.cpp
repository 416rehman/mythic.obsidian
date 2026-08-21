
#include "Narrative/MythicNarrativeGrant.h"

#include "Narrative/MythicNarrativeStateComponent.h"
#include "World/LivingWorld/MythicWorldStateSubsystem.h"
#include "Engine/World.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Story_World, "Story.World")

bool FMythicNarrativeGrant::IsWorldScopedGrant(FGameplayTag Tag) {
    return Tag.IsValid() && Tag.MatchesTag(TAG_Story_World);
}

void FMythicNarrativeGrant::RouteGrant(const UObject *WorldContext, UMythicNarrativeStateComponent *NarrativeState, FGameplayTag Tag) {
    if (!Tag.IsValid()) {
        return;
    }
    if (IsWorldScopedGrant(Tag)) {
        const UWorld *World = WorldContext ? WorldContext->GetWorld() : nullptr;
        if (UMythicWorldStateSubsystem *WorldState = World ? World->GetSubsystem<UMythicWorldStateSubsystem>() : nullptr) {
            WorldState->ServerSetFlag(Tag);
        }
        return;
    }
    if (NarrativeState) {
        NarrativeState->ServerSetStoryTag(Tag);
    }
}

void FMythicNarrativeGrant::RouteGrants(const UObject *WorldContext, UMythicNarrativeStateComponent *NarrativeState, const FGameplayTagContainer &Tags) {
    for (const FGameplayTag &Tag : Tags) {
        RouteGrant(WorldContext, NarrativeState, Tag);
    }
}
