#include "MythicTags_GAS.h"

/** States */
// State tags are applied to entities to indicate their current state, such as dead, in combat, etc.
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_DEAD, "GAS.State.Dead", "The entity is dead");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_DYING, "GAS.State.Dying", "The entity is dying");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_INCOMBAT, "GAS.State.InCombat", "The entity was damaged recently");

/** Debuffs */
// Debuff tags are applied to entities to indicate that they are suffering from a negative effect, such as bleeding, burning, etc.
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_BLEEDING, "GAS.Debuff.Bleeding", "The entity is bleeding and taking damage over time");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_BURNING, "GAS.Debuff.Burning", "The entity is burning and taking damage over time");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_POISONED, "GAS.Debuff.Poisoned", "The entity is poisoned and taking damage over time");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_STUNNED, "GAS.Debuff.Stunned", "The entity is stunned and cannot act");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_SLOWED, "GAS.Debuff.Slowed", "The entity is slowed and moves at a reduced speed");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_WEAKENED, "GAS.Debuff.Weakened", "The entity is weakened and deals reduced damage");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_FROZEN, "GAS.Debuff.Frozen", "The entity is frozen and cannot act");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_TERRIFIED, "GAS.Debuff.Terrified", "The entity is terrified and receives increased damage");

/** Buffs */
// Buff tags are applied to entities to indicate that they are benefiting from a positive effect, such as healing, increased damage, etc.
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_HEALING, "GAS.Buff.Healing", "The entity is healing and gaining health over time");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_RAGE, "GAS.Buff.Rage", "The entity is enraged and deals increased damage");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_HASTE, "GAS.Buff.Haste", "The entity is hasted and moves at an increased speed");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_FORTIFY, "GAS.Buff.Fortify", "The entity is fortified and receives increased damage");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_ENLIGHTEN, "GAS.Buff.Enlighten", "The entity is enlightened and receives increased damage");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_INVINCIBLE, "GAS.Buff.Invincible", "The entity is invincible and cannot be damaged");

/// Gameplay Buffs
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_GAMEPLAY_CONDUIT_OF_LIGHTNING, "GAS.Buff.Gameplay.ConduitOfLightning",
                               "The entity is a conduit of lightning and deals increased damage");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_GAMEPLAY_GAMBIT, "GAS.Buff.Gameplay.Gambit", "Every 10 second, a random buff is applied to the entity");

/** GAS Abilities */
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_ABILITY_HEAL, "GAS.Ability.Heal", "The entity heals another entity");

/** SetByCaller */
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SETBYCALLER_GENERIC, "SetByCaller.Generic", "Generic SetByCaller tag to use when no other tag is appropriate");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SETBYCALLER_DAMAGE, "SetByCaller.Damage", "Used by effects that deal damage");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SETBYCALLER_HEAL, "SetByCaller.Heal", "Used by effects that heal");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SETBYCALLER_DURATION, "SetByCaller.Duration", "When an effect has a duration, this tag is used to set the duration");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SETBYCALLER_COOLDOWN, "SetByCaller.Cooldown", "Cooldown SetByCaller tag");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SETBYCALLER_COST, "SetByCaller.Cost", "Cost SetByCaller tag");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(SETBYCALLER_RANGE, "SetByCaller.Range", "Distance, Radius, or generic Area of Effect");

/** GAS Events */
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_ACTIVATE_LEVELUP, "GAS.Event.Activate.LevelUp", "Event called when player levels up");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_ACTIVATE_XPGAIN, "GAS.Event.Activate.XPGain", "Event called when player gains XP");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_HITBOX, "GAS.Event.Hitbox", "Event called when owner's animation's hitbox overlaps with another entity");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DMG_PRE, "GAS.Event.Dmg.Pre", "Event called before owner deals damage to another entity");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DMG_DELIVERED, "GAS.Event.Dmg.Delivered", "Event called when owner deals damage to another entity");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DMG_RECEIVED, "GAS.Event.Dmg.Received", "Event called when owner receives damage from another entity");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DEATH, "GAS.Event.Death", "Event called when owner dies");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_KILL, "GAS.Event.Kill", "Event called when owner kills another entity");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_HEAL_DELIVERED, "GAS.Event.Heal.Delivered", "Event called when owner heals another entity");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_HEAL_RECEIVED, "GAS.Event.Heal.Received", "Event called when owner is healed by another entity");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_ATTACK_BEGIN, "GAS.Event.Attack.Begin", "Event called when owner's attack begins");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_ATTACK_END, "GAS.Event.Attack.End", "Event called when owner's attack ends");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DMG_DESTRUCTIBLE, "GAS.Event.Dmg.Destructible", "Event called when the owner's attack hits a destructible object");

/** GameplayCues */
UE_DEFINE_GAMEPLAY_TAG(GAMEPLAYCUE_ABILITY_HEAL_ACTIVATED, "GameplayCue.Ability.Heal.Activated");
UE_DEFINE_GAMEPLAY_TAG(GAMEPLAYCUE_ABILITY_HEAL_RECEIVED, "GameplayCue.Ability.Heal.Received");
UE_DEFINE_GAMEPLAY_TAG(GAMEPLAYCUE_ABILITY_LEVELUP, "GameplayCue.Ability.LevelUp");

// Internal Notifies - format NOTIFY_<SYSTEM>_<ACTION>_<RESULT>_<REASON>
UE_DEFINE_GAMEPLAY_TAG(NOTIFY_ABILITY_ACTIVATION_FAILED_COST, "Notify.Ability.Activation.Failed.Cost");
UE_DEFINE_GAMEPLAY_TAG(NOTIFY_ABILITY_ACTIVATION_FAILED_ISDEAD, "Notify.Ability.Activation.Failed.IsDead");

/** Input */
// Input tags control ability input processing behavior
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_INPUT_BLOCKED, "GAS.Input.Blocked", "When present on ASC, all ability input is blocked and cleared");
