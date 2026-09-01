#include "Itemization/Inventory/ViewModels/MythicTags_ItemMetrics.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(ITEM_METRIC_WEAPON_DAMAGE_PER_SECOND, "ItemMetric.Weapon.DamagePerSecond",
                               "Canonical item-local sustained weapon damage per second.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ITEM_METRIC_WEAPON_AVERAGE_DAMAGE_PER_HIT, "ItemMetric.Weapon.AverageDamagePerHit",
                               "Canonical expected item-local damage for one uniform weapon hit.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ITEM_METRIC_WEAPON_ATTACKS_PER_SECOND, "ItemMetric.Weapon.AttacksPerSecond",
                               "Canonical effective item-local attacks per second after the combat AttackSpeed clamp.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(ITEM_METRIC_DURABILITY, "ItemMetric.Durability", "Maximum durability comparison identity.");
