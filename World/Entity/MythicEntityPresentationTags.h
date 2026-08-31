#pragma once

#include "NativeGameplayTags.h"

/** Public entity-kind tags describe only what the current embodiment visibly is. */
namespace MythicEntityPresentationTags {
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EntityKindHumanoid);
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EntityKindCreature);
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EntityKindPlayer);
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EntityKindConstruct);
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(EntityKindWorldObject);

    /** Stateful observable slot owned by LivingWorld activity execution. */
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ObservableSlotActivity);

    /** Stateful observable slot owned by executed AI/combat behavior. */
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ObservableSlotBehavior);

    /** Stateful observable slot owned by the authoritative life component. */
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ObservableSlotLifeState);

    /** Public behavior value emitted only after the subject actually begins fighting. */
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ObservableBehaviorFighting);

    /** Public behavior value emitted only after the subject actually begins a flee move. */
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ObservableBehaviorFleeing);

    /** Public behavior value emitted only after the subject visibly surrenders. */
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ObservableBehaviorSurrendering);

    /** Public life value emitted while a revivable subject is visibly downed. */
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ObservableLifeDowned);

    /** Public life value emitted during an observable terminal dying transition. */
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ObservableLifeDying);

    /** Public life value emitted after authoritative death has occurred. */
    MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ObservableLifeDead);
}
