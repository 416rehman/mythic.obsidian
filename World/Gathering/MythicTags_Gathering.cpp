#include "MythicTags_Gathering.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Itemization_Quality, "Itemization.Quality",
                               "Root for the def-level yield-quality tier tags (C1 manifestation law: quality on stackables = distinct item defs, one tag each).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Itemization_Quality_Ragged, "Itemization.Quality.Ragged",
                               "Botched-kill tier — HUNTING ONLY (Wave P). Crops/produce never mint Ragged (no-total-loss rule).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Itemization_Quality_Common, "Itemization.Quality.Common",
                               "Neutral baseline tier: 1.0x potency / 1.0x price. Every source rolls Common until its wave lands (M6 contract).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Itemization_Quality_Fine, "Itemization.Quality.Fine",
                               "Above-baseline tier (mastery/inputs earned).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Itemization_Quality_Pristine, "Itemization.Quality.Pristine",
                               "Top tier (rare rolls, mastery floors, perfect kills).");
