#include "World/Entity/MythicEntityPresentationTags.h"

namespace MythicEntityPresentationTags {
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(EntityKindHumanoid, "Entity.Kind.Humanoid", "A visibly humanoid current embodiment.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(EntityKindCreature, "Entity.Kind.Creature", "A visibly non-humanoid creature current embodiment.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(EntityKindPlayer, "Entity.Kind.Player", "A player-controlled current embodiment.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(EntityKindConstruct, "Entity.Kind.Construct", "A visibly artificial or animated construct embodiment.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(EntityKindWorldObject, "Entity.Kind.WorldObject", "A presentable non-character world-object embodiment.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ObservableSlotActivity, "LivingWorld.Observable.Activity", "The subject's currently executed public activity.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ObservableSlotBehavior, "LivingWorld.Observable.Behavior", "The subject's currently executed public behavior.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ObservableSlotLifeState, "LivingWorld.Observable.LifeState", "The subject's current publicly observable life state.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ObservableBehaviorFighting, "LivingWorld.Observable.Behavior.Fighting", "The subject is visibly fighting.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ObservableBehaviorFleeing, "LivingWorld.Observable.Behavior.Fleeing", "The subject is visibly fleeing.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ObservableBehaviorSurrendering, "LivingWorld.Observable.Behavior.Surrendering", "The subject is visibly surrendering.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ObservableLifeDowned, "LivingWorld.Observable.LifeState.Downed", "The subject is visibly downed and may be revivable.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ObservableLifeDying, "LivingWorld.Observable.LifeState.Dying", "The subject is in an observable terminal dying transition.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(ObservableLifeDead, "LivingWorld.Observable.LifeState.Dead", "The subject has authoritatively died.");
}
