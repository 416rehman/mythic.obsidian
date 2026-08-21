#include "World/Trading/MythicTags_Trading.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(GAS_EVENT_DELIVER, "GAS.Event.Deliver",
                               "Fired on the deliverer's ASC when contract goods are handed to a delivery-accepting vendor. EventMagnitude = units; TargetTags carries the item type tag.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(STAT_TRADE_PROFIT, "Stat.Trade.Profit",
                               "Lifetime coins earned through trading verbs (delivery payouts + stall till collections).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(STAT_TRADE_CONTRACTS_COMPLETED, "Stat.Trade.ContractsCompleted",
                               "Lifetime delivery contracts completed.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(STAT_TRADE_UNITS_DELIVERED, "Stat.Trade.UnitsDelivered",
                               "Lifetime contract goods units delivered to vendors.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(STAT_TRADE_CONTRABAND_SOLD, "Stat.Trade.ContrabandSold",
                               "Lifetime contraband units sold to black-market vendors.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_TRADING_EVENT_RUMOR, "LivingWorld.Event.Trade.Rumor",
                               "A trade rumor beat: a notable price differential between two settlements surfaced through the chronicle.");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_TRADING_EVENT_DEFICIT_MATERIALS, "LivingWorld.Event.Trade.Deficit.Materials",
                               "Edge-triggered: a faction's Materials reserve fell below the trading deficit threshold (delivery-contract trigger).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_TRADING_EVENT_DEFICIT_ARMS, "LivingWorld.Event.Trade.Deficit.Arms",
                               "Edge-triggered: a faction's Arms reserve fell below the trading deficit threshold (war-demand delivery trigger).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_TRADING_EVENT_CONTRACT_POSTED, "LivingWorld.Event.Trade.ContractPosted",
                               "A delivery contract offer was posted to the trade board (chronicle announcement).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_TRADING_EVENT_CONTRACT_COMPLETED, "LivingWorld.Event.Trade.ContractCompleted",
                               "A player completed a delivery contract (famine/deficit relief actually landed).");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_TRADING_EVENT_STALL_SALE, "LivingWorld.Event.Trade.StallSale",
                               "A player stall sold goods into the local settlement economy.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_TRADING_ACTION_CONTRABAND, "LivingWorld.Action.Trade.Contraband",
                               "A contraband sale action — rides the crime-witness pipeline; real perceiving witnesses decide whether it is a crime.");
