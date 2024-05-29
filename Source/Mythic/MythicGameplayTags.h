#pragma once

#include "NativeGameplayTags.h"

/**Inventory.Slot*/
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Inventory_Slot_Default);	// Generic slot, can contain any item
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Inventory_Slot_Equipment);	// Equipment slot, can contain equipment items
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Inventory_Slot_Accessory);	// Accessory slot, can contain accessory items
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Inventory_Slot_Hotbar);	// Hotbar slot, can contain any item

/*Itemization.Type*/
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Type_Other);	// Other
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Type_Equipment);	// Equipment, can be equipped in the equipment slot
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Type_Food);	// can be consumed for a buff 
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Type_Resource);	// can be used to craft other items
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Type_Weapon);	// can be equipped in the equipment slot
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Type_Plan);	// can be used to craft other items
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Type_Scroll);	// can be used to learn a spell
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Type_Special);	// any special item
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Type_Accessory);	// can be equipped in the accessory slot
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Type_Placable); // can be placed in the world, i.e a box, crafting station, etc.
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Type_Tool);	// tools such as pickaxes, axes, shovels, etc.
/*Itemization.Rarity*/
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Rarity_Common);	// Common
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Rarity_Rare);	// Rare
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Rarity_Epic);	// Epic
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Rarity_Legendary);	// Legendary
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_Rarity_Exotic);	// Exotic
/**Itemization.ActionCondition*/
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_ActionType_InHand);	// The item can be used while in hand
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Itemization_ActionType_InInventory);	// The item can be used from the inventory UI

/** GAS Attributes */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAS_ATTRIBUTE_HEALTH);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAS_ATTRIBUTE_PROGRESSION_XP);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAS_ATTRIBUTE_PROGRESSION_LEVEL);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAS_ATTRIBUTE_PLAYER_HEALTH);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAS_ATTRIBUTE_PLAYER_MAXHEALTH);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAS_ATTRIBUTE_PLAYER_MOVEMENTSPEED);

/** GAS Abilities */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAS_ABILITY_HEAL);

/** GAS Events */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAS_EVENT_ACTIVATE_LEVELUP);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAS_EVENT_ACTIVATE_XPGAIN);

/** GameplayCues */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAMEPLAYCUE_ABILITY_HEAL_ACTIVATED);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAMEPLAYCUE_ABILITY_HEAL_RECEIVED);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_GAMEPLAYCUE_ABILITY_LEVELUP);