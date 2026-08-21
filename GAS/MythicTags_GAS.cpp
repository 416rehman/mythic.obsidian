#include "MythicTags_GAS.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_DEAD, "GAS.State.Dead", "The entity is dead");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_DYING, "GAS.State.Dying", "The entity is dying");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_DOWNED, "GAS.State.Downed", "Co-op down state: incapacitated but revivable (bleeds out if not revived)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_INCOMBAT, "GAS.State.InCombat", "The entity was damaged recently");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_SPRINTING, "GAS.State.Sprinting", "The entity is sprinting");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_EXHAUSTED, "GAS.State.Exhausted", "Winded: out of stamina from sprinting — the sprint speed bonus is suppressed until stamina recovers");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_HEALTH, "GAS.State.Health", "Parent of the health bands an entity currently sits in");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_HEALTH_CRITICAL, "GAS.State.Health.Critical", "At death's door");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_HEALTH_LOW, "GAS.State.Health.Low", "Badly hurt");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_HEALTH_WOUNDED, "GAS.State.Health.Wounded", "Has taken a meaningful wound");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_STATE_HEALTH_UNHURT, "GAS.State.Health.Unhurt", "Effectively untouched");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF, "GAS.Debuff", "Parent category for all debuffs");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_BLEEDING, "GAS.Debuff.Bleeding", "The entity is bleeding and taking damage over time");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_BURNING, "GAS.Debuff.Burning", "The entity is burning and taking damage over time");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_POISONED, "GAS.Debuff.Poisoned", "The entity is poisoned and taking damage over time");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_STUNNED, "GAS.Debuff.Stunned", "The entity is stunned and cannot act");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_SLOWED, "GAS.Debuff.Slowed", "The entity is slowed and moves at a reduced speed");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_WEAKENED, "GAS.Debuff.Weakened", "The entity is weakened and deals reduced damage");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_FROZEN, "GAS.Debuff.Frozen", "The entity is frozen and cannot act");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_DEBUFF_TERRIFIED, "GAS.Debuff.Terrified", "The entity is terrified and receives increased damage");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_HEALING, "GAS.Buff.Healing", "The entity is healing and gaining health over time");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_RAGE, "GAS.Buff.Rage", "The entity is enraged and deals increased damage");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_HASTE, "GAS.Buff.Haste", "The entity is hasted and moves at an increased speed");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_FORTIFY, "GAS.Buff.Fortify", "The entity is fortified and receives increased damage");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_ENLIGHTEN, "GAS.Buff.Enlighten", "The entity is enlightened and gains increased proficiency XP");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_INVINCIBLE, "GAS.Buff.Invincible", "The entity is invincible and cannot be damaged");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_GAMEPLAY_CONDUIT_OF_LIGHTNING, "GAS.Buff.Gameplay.ConduitOfLightning",
                               "The entity is a conduit of lightning and deals increased damage");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_BUFF_GAMEPLAY_GAMBIT, "GAS.Buff.Gameplay.Gambit", "Every 10 second, a random buff is applied to the entity");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_ABILITY_HEAL, "GAS.Ability.Heal", "The entity heals another entity");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_ABILITY_TYPE_SKILL, "GAS.Ability.Type.Skill", "Marks an outgoing hit as delivered by a skill");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_GENERIC, "SetByCaller.Generic", "Generic SetByCaller tag to use when no other tag is appropriate");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_DAMAGE, "SetByCaller.Damage", "Used by effects that deal damage");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_HEAL, "SetByCaller.Heal", "Used by effects that heal");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_DURATION, "SetByCaller.Duration", "When an effect has a duration, this tag is used to set the duration");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_COOLDOWN, "SetByCaller.Cooldown", "Cooldown SetByCaller tag");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_COST, "SetByCaller.Cost", "Cost SetByCaller tag");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_SETBYCALLER_RANGE, "SetByCaller.Range", "Distance, Radius, or generic Area of Effect");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_PIPELINE_DEATH_HANDLED, "GAS.Pipeline.Death.Handled",
                               "Indicates that death has been handled and further death processing should be skipped");


UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_HITBOX, "GAS.Event.Hitbox", "Event called when owner's animation's hitbox overlaps with another entity");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DMG_PRE, "GAS.Event.Dmg.Pre", "Event called before owner deals damage to another entity");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DMG_DELIVERED, "GAS.Event.Dmg.Delivered", "Event called when owner deals damage to another entity");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_HIT_CRITICAL, "GAS.Hit.Critical",
                               "Rides on a damage event when the hit was critical, so a talent can gate on one");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DMG_RECEIVED, "GAS.Event.Dmg.Received", "Event called when owner receives damage from another entity");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DEATH_PRE, "GAS.Event.Death.Pre",
                               "First event in the death pipeline, i.e. a talent could give owner 'GAS_PIPELINE_DEATH_HANDLED' tag");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DEATH, "GAS.Event.Death", "Event called when owner dies");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DEATH_POST, "GAS.Event.Death.Post",
                               "Last event in the death pipeline, i.e. a talent could remove 'GAS_PIPELINE_DEATH_HANDLED' tag from owner");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_KILL, "GAS.Event.Kill", "Event called when owner kills another entity");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_HEAL_DELIVERED, "GAS.Event.Heal.Delivered", "Event called when owner heals another entity");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_HEAL_RECEIVED, "GAS.Event.Heal.Received", "Event called when owner is healed by another entity");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_ATTACK_BEGIN, "GAS.Event.Attack.Begin", "Event called when owner's attack begins");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_ATTACK_END, "GAS.Event.Attack.End", "Event called when owner's attack ends");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_SKILL_BEGIN, "GAS.Event.Skill.Begin",
                               "A skill began. InstigatorTags carry the ability's own tags, Skill.* among them.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_SKILL_END, "GAS.Event.Skill.End",
                               "A skill finished. InstigatorTags carry the ability's own tags, Skill.* among them.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DMG_DESTRUCTIBLE, "GAS.Event.Dmg.Destructible", "Event called when the owner's attack hits a destructible object");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_ITEM_ACQUIRED, "GAS.Event.Item.Acquired",
                               "Fired server-side on a player's ASC when they genuinely acquire item(s). EventMagnitude = quantity; TargetTags carries the item's ItemType. Drives 'collect N' objectives.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_REACHED_LOCATION, "GAS.Event.ReachedLocation",
                               "Fired server-side on a player's ASC when they first enter an AMythicLocationObjectiveVolume. TargetTags carries the volume's LocationTag. Drives non-combat 'reach/visit X' objectives.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_TALKED_TO_NPC, "GAS.Event.TalkedToNPC",
                               "Fired server-side on a player's ASC when they talk to a quest-relevant NPC. TargetTags carries the NPC's QuestNpcTag. Drives non-combat 'talk to X' objectives.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_ITEM_USED, "GAS.Event.Item.Used",
                               "Fired server-side on a player's ASC when they USE/consume an item (the generic consumable ability). EventMagnitude = quantity; TargetTags carries the item's ItemType. Drives 'use N <type>' objectives (distinct from 'collect N').");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_ITEM_EQUIPPED, "GAS.Event.Item.Equipped",
                               "Fired server-side on a player's ASC on the first genuine weapon equip (per-item SaveGame marker suppresses save-restore re-fires). TargetTags carries the item's ItemType. Drives 'equip N <type>' objectives.");

UE_DEFINE_GAMEPLAY_TAG(GAMEPLAYCUE_ABILITY_HEAL_ACTIVATED, "GameplayCue.Ability.Heal.Activated");
UE_DEFINE_GAMEPLAY_TAG(GAMEPLAYCUE_ABILITY_HEAL_RECEIVED, "GameplayCue.Ability.Heal.Received");
UE_DEFINE_GAMEPLAY_TAG(GAMEPLAYCUE_ABILITY_LEVELUP, "GameplayCue.Ability.LevelUp");

UE_DEFINE_GAMEPLAY_TAG(NOTIFY_ABILITY_ACTIVATION_FAILED_COST, "Notify.Ability.Activation.Failed.Cost");
UE_DEFINE_GAMEPLAY_TAG(NOTIFY_ABILITY_ACTIVATION_FAILED_ISDEAD, "Notify.Ability.Activation.Failed.IsDead");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_INPUT_BLOCKED, "GAS.Input.Blocked", "When present on ASC, all ability input is blocked and cleared");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(AI_KIND_CREATURE, "AI.Kind.Creature",
                               "Huntable creature — its corpse is skinnable for hides/meat/trophies (Hunting F2). Owner adds this loose tag to the creature ASC on combat init.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(AI_KIND_HUMANOID, "AI.Kind.Humanoid",
                               "Humanoid — its corpse is NOT skinnable (the default when a dying pawn owns no AI.Kind tag).");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_PROFICIENCY_GAINED, "GAS.Event.Proficiency.Gained",
                               "Any proficiency track gained XP. The single non-combat hook: fires for gathering, crafting, farming, fishing, hunting, trading and building alike. InstigatorTags carry the track's TrackTag.");
