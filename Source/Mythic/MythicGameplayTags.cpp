#include "MythicGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG(TAG_Inventory_Slot_Default, "Inventory.Slot.Default");
UE_DEFINE_GAMEPLAY_TAG(TAG_Inventory_Slot_Equipment, "Inventory.Slot.Equipment");
UE_DEFINE_GAMEPLAY_TAG(TAG_Inventory_Slot_Accessory, "Inventory.Slot.Accessory");
UE_DEFINE_GAMEPLAY_TAG(TAG_Inventory_Slot_Hotbar, "Inventory.Slot.Hotbar");
/*Itemization.Type*/
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Type_Other, "Itemization.Type.Other");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Type_Equipment, "Itemization.Type.Equipment");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Type_Food, "Itemization.Type.Food");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Type_Resource, "Itemization.Type.Resource");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Type_Weapon, "Itemization.Type.Weapon");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Type_Plan, "Itemization.Type.Plan");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Type_Scroll, "Itemization.Type.Scroll");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Type_Special, "Itemization.Type.Special");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Type_Accessory, "Itemization.Type.Accessory");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Type_Placable, "Itemization.Type.Placable");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Type_Tool, "Itemization.Type.Tool");
/*Itemization.Rarity*/
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Rarity_Common, "Itemization.Rarity.Common");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Rarity_Rare, "Itemization.Rarity.Rare");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Rarity_Epic, "Itemization.Rarity.Epic");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Rarity_Legendary, "Itemization.Rarity.Legendary");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_Rarity_Exotic, "Itemization.Rarity.Exotic");
/*Itemization.ActionType*/
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_ActionType_InHand, "Itemization.ActionType.InHand");
UE_DEFINE_GAMEPLAY_TAG(TAG_Itemization_ActionType_InInventory, "Itemization.ActionType.InInventory");

/** GAS Attributes */
UE_DEFINE_GAMEPLAY_TAG(TAG_GAS_ATTRIBUTE_HEALTH, "GAS.Attribute.Health");
UE_DEFINE_GAMEPLAY_TAG(TAG_GAS_ATTRIBUTE_PROGRESSION_XP, "GAS.Attribute.Progression.XP");
UE_DEFINE_GAMEPLAY_TAG(TAG_GAS_ATTRIBUTE_PROGRESSION_LEVEL, "GAS.Attribute.Progression.Level");
UE_DEFINE_GAMEPLAY_TAG(TAG_GAS_ATTRIBUTE_PLAYER_HEALTH, "GAS.Attribute.Player.Health");
UE_DEFINE_GAMEPLAY_TAG(TAG_GAS_ATTRIBUTE_PLAYER_MAXHEALTH, "GAS.Attribute.Player.MaxHealth");
UE_DEFINE_GAMEPLAY_TAG(TAG_GAS_ATTRIBUTE_PLAYER_MOVEMENTSPEED, "GAS.Attribute.Player.MovementSpeed");

/** GAS Abilities */
UE_DEFINE_GAMEPLAY_TAG(TAG_GAS_ABILITY_HEAL, "GAS.Ability.Heal");

/** GAS Events */
UE_DEFINE_GAMEPLAY_TAG(TAG_GAS_EVENT_ACTIVATE_LEVELUP, "GAS.Event.Activate.LevelUp");
UE_DEFINE_GAMEPLAY_TAG(TAG_GAS_EVENT_ACTIVATE_XPGAIN, "GAS.Event.Activate.XPGain");

/** GameplayCues */
UE_DEFINE_GAMEPLAY_TAG(TAG_GAMEPLAYCUE_ABILITY_HEAL_ACTIVATED, "GameplayCue.Ability.Heal.Activated");
UE_DEFINE_GAMEPLAY_TAG(TAG_GAMEPLAYCUE_ABILITY_HEAL_RECEIVED, "GameplayCue.Ability.Heal.Received");
UE_DEFINE_GAMEPLAY_TAG(TAG_GAMEPLAYCUE_ABILITY_LEVELUP, "GameplayCue.Ability.LevelUp");