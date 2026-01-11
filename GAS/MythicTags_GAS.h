#pragma once

#include "NativeGameplayTags.h"

/** States */
// State tags are applied to entities to indicate their current state, such as dead, in combat, etc.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_STATE_DEAD); // The entity is dead
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_STATE_DYING); // The entity is dying
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_STATE_INCOMBAT); // The entity was damaged recently

/** Debuffs */
// Debuff tags are applied to entities to indicate that they are suffering from a negative effect, such as bleeding, burning, etc.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_DEBUFF_BLEEDING); // The entity is bleeding and taking damage over time
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_DEBUFF_BURNING); // The entity is burning and taking damage over time
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_DEBUFF_POISONED); // The entity is poisoned and taking damage over time
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_DEBUFF_STUNNED); // The entity is stunned and cannot act
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_DEBUFF_SLOWED); // The entity is slowed and moves at a reduced speed
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_DEBUFF_WEAKENED); // The entity is weakened and deals reduced damage
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_DEBUFF_FROZEN); // The entity is frozen and cannot act
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_DEBUFF_TERRIFIED); // The entity is terrified and receives increased damage

/** Buffs */
// Buff tags are applied to entities to indicate that they are benefiting from a temporary positive effect, such as healing, increased damage, etc.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_BUFF_HEALING); // The entity is healing and gaining health over time
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_BUFF_RAGE); // The entity is enraged and deals increased damage
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_BUFF_HASTE); // The entity is hasted and moves at an increased speed
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_BUFF_FORTIFY); // The entity is fortified and takes reduced damage
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_BUFF_ENLIGHTEN); // The entity is enlightened and gains increased experience
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_BUFF_INVINCIBLE); // The entity is invincible and cannot be damaged
/// Gameplay Buffs: Changes the entity's gameplay in some way i.e Conduit of Lightning turns the entity into a conduit of lightning
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_BUFF_GAMEPLAY_CONDUITOFLIGHTNING); // The entity has turned into a ball of lightning and deals damage to nearby enemies
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_BUFF_GAMEPLAY_GAMBIT); // The entity's attacks trigger a random ability, the ability is swapped every X seconds

/** SetByCaller */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_SETBYCALLER_GENERIC);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_SETBYCALLER_DAMAGE);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_SETBYCALLER_DURATION);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_SETBYCALLER_RADIUS);

/** Abilities */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_ABILITY_HEAL);

/** Events */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_ACTIVATE_LEVELUP);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_ACTIVATE_XPGAIN);
// The owner has hit an entity with a hitbox - Triggered by melee attacks, if the animation HITBOX hits a target.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_HITBOX);
// The owner is about to damage an entity - Triggered by Damage Pipeline DamageContainer's DamageContextEffect. Use cases: Modify damage context before applying
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_DMG_PRE);
// The owner has damaged an entity - Triggered by Damage Pipeline DamageContainer's DamageApplicationEffect. Use cases: Apply damage to target and propagate event to LifeComponent
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_DMG_DELIVERED);
// The owner has been damaged - Triggered by LifeComponent (LifeComponent decides how to interpret an actor's life and death)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_DMG_RECEIVED);
// The owner has died - Triggered by LifeComponent (LifeComponent decides how to interpret an actor's life and death)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_DEATH);
// The owner has killed an entity - Triggered by LifeComponent (LifeComponent decides how to interpret an actor's life and death)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_KILL);
// The owner has healed an entity - Triggered by Healing LifeComponent (LifeComponent decides how to interpret an actor's life and death)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_HEAL_DELIVERED);
// The owner has been healed - Triggered by Healing LifeComponent (LifeComponent decides how to interpret an actor's life and death)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_HEAL_RECEIVED);
// The owner has hit a destructible object - Triggered by ApplyDamageContainerSpec if it's target data has actors that implement the IDestructible interface
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_DMG_DESTRUCTIBLE);

// The owner has started an attack
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_ATTACK_BEGIN);
// The owner has ended an attack
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_EVENT_ATTACK_END);


/** GameplayCues */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_CUE_ABILITY_HEAL_ACTIVATED);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_CUE_ABILITY_HEAL_RECEIVED);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_CUE_ABILITY_LEVELUP);

// Internal Notifies - format NOTIFY_<SYSTEM>_<ACTION>_<RESULT>_<REASON>
UE_DECLARE_GAMEPLAY_TAG_EXTERN(NOTIFY_ABILITY_ACTIVATION_FAILED_COST);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(NOTIFY_ABILITY_ACTIVATION_FAILED_ISDEAD);
