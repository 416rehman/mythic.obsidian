
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"

class UMythicNarrativeStateComponent;

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Story_World)

struct MYTHIC_API FMythicNarrativeGrant {
    static bool IsWorldScopedGrant(FGameplayTag Tag);

    static void RouteGrant(const UObject *WorldContext, UMythicNarrativeStateComponent *NarrativeState, FGameplayTag Tag);

    static void RouteGrants(const UObject *WorldContext, UMythicNarrativeStateComponent *NarrativeState, const FGameplayTagContainer &Tags);
};
