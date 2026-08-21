#include "MythicTags_Cooking.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Station_Cooking, "Itemization.Station.Cooking",
                               "Root of the canonical cooking-station ladder (C7). One tag family for the food lane.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Station_Cooking_Campfire, "Itemization.Station.Cooking.Campfire",
                               "T0-1: the portable, fuel-burning campfire (Wave N's campfire IS this station).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Station_Cooking_CookPot, "Itemization.Station.Cooking.CookPot",
                               "T2: the placeable cook pot.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Station_Cooking_Kitchen, "Itemization.Station.Cooking.Kitchen",
                               "T3: granted by the homestead shell tier (Wave K2 StationTags) - better dishes need a better home.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Station_Cooking_Grand, "Itemization.Station.Cooking.Grand",
                               "T4: the settlement Grand Kitchen.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Buff_Food, "Buff.Food",
                               "Root of the food buff lanes (C14: food = endurance/preparation lane; potions reactive; Rested = XP lane).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Buff_Food_Meal, "Buff.Food.Meal",
                               "The ONE active meal. Meal GEs grant this AND remove-effects-with it (authoring contract).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Buff_Food_Drink, "Buff.Food.Drink",
                               "The ONE active drink. Drink GEs grant this AND remove-effects-with it (authoring contract).");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_SetByCaller_Food_Potency, "SetByCaller.Food.Potency",
                               "Cooked potency (quality x freshness x Cooking level, HARD-clamped at 2.0). Written per-dish by UMythicCookingRecipe.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_SetByCaller_Food_Nourish, "SetByCaller.Food.Nourish",
                               "Dish nourishment (NourishValue x potency) for survival Nourishment restore GEs.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(STAT_COOKING_DISHES_COOKED, "Stat.Cooking.DishesCooked",
                               "Lifetime dishes cooked (per portion), recorded at produce time.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(STAT_COOKING_EXPERIMENTS, "Stat.Cooking.Experiments",
                               "Lifetime experiment cooks (the Questionable Stew path).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(STAT_COOKING_RECIPES_DISCOVERED, "Stat.Cooking.RecipesDiscovered",
                               "Recipes discovered by experimentation or schematics (counted once per recipe - the grant path is idempotent).");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Codex_Term_Recipe, "Codex.Term.Recipe",
                               "Root for codex recipe pages: one UMythicGlossaryEntry per recipe, discovered on learn.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Itemization_Recipe_Cooking, "Itemization.Recipe.Cooking",
                               "Root for cooking RecipeIds (content authors children).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Itemization_Schematic_Cooking, "Itemization.Schematic.Cooking",
                               "Root for cooking schematic tags (granted by ServerLearnRecipe; gates recipes via InstigatorTagQuery).");
