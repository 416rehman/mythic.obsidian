
#include "World/LivingWorld/Roles/ArchetypeTypes.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Territory/MythicBiome.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"

FMythicArchetypeContext::FMythicArchetypeContext()
    : Economy(EMythicSettlementEconomy::Generic)
    , Biome(EMythicBiome::Plains) {}

namespace MythicArchetypeDefaults {
namespace {
    constexpr int32 EconomyCount = static_cast<int32>(EMythicSettlementEconomy::COUNT);
    constexpr int32 BiomeCount = MythicBiomeCount;

    TArray<float> EconomyW(EMythicSettlementEconomy Favored, float Weight) {
        TArray<float> Out;
        Out.Init(1.0f, EconomyCount);
        const int32 Idx = static_cast<int32>(Favored);
        if (Out.IsValidIndex(Idx)) {
            Out[Idx] = Weight;
        }
        return Out;
    }

    TArray<float> BiomeW(EMythicBiome Favored, float Weight) {
        TArray<float> Out;
        Out.Init(1.0f, BiomeCount);
        const int32 Idx = static_cast<int32>(Favored);
        if (Out.IsValidIndex(Idx)) {
            Out[Idx] = Weight;
        }
        return Out;
    }

    const TArray<FMythicArchetypeRow> &DefaultArchetypeArray() {
        static const TArray<FMythicArchetypeRow> Archetypes = [] {
            TArray<FMythicArchetypeRow> A;

            auto Add = [&A](const FGameplayTag &Role, const TCHAR *Name, float Base) -> FMythicArchetypeRow & {
                FMythicArchetypeRow Row;
                Row.RoleTag = Role;
                Row.DisplayName = FName(Name);
                Row.BaseWeight = Base;
                A.Add(Row);
                return A.Last();
            };

            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_CIVILIAN, TEXT("Civilian"), 1.0f);
                R.bAllowedAlone = true;
            }

            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_FARMER, TEXT("Farmer"), 1.0f);
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Farming, 4.0f);
                R.BiomeWeights = BiomeW(EMythicBiome::Plains, 1.5f);
                R.DayWeight = 1.4f;
                R.NightWeight = 0.4f;
            }

            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_MERCHANT, TEXT("Merchant"), 1.0f);
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Trade, 4.0f);
                R.WealthFavor = 2.0f;
                R.DayWeight = 1.4f;
                R.NightWeight = 0.3f;
            }

            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_LABORER, TEXT("Laborer"), 1.0f);
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Mining, 4.0f);
                R.WealthDisfavor = 1.6f;
                R.BiomeWeights = BiomeW(EMythicBiome::Mountain, 1.5f);
                R.DayWeight = 1.3f;
                R.NightWeight = 0.5f;
            }

            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_FISHER, TEXT("Fisher"), 1.0f);
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Fishing, 5.0f);
                R.BiomeWeights = BiomeW(EMythicBiome::Wetland, 2.0f);
                R.bWaterCapable = true;
                R.DayWeight = 1.3f;
                R.NightWeight = 0.4f;
            }

            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_GUARD, TEXT("Guard"), 0.8f);
                R.MilitaryFavor = 3.0f;
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Military, 2.0f);
                R.DayWeight = 1.0f;
                R.NightWeight = 1.0f;
            }

            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_SOLDIER, TEXT("Soldier"), 0.6f);
                R.MilitaryFavor = 4.0f;
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Military, 4.0f);
                R.DayWeight = 1.0f;
                R.NightWeight = 0.8f;
            }

            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_BEGGAR, TEXT("Beggar"), 0.5f);
                R.WealthDisfavor = 3.0f;
                R.WealthFavor = 0.2f;
                R.bRequiresSettlement = true;
                R.DayWeight = 1.2f;
                R.NightWeight = 0.3f;
            }

            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_SOCIALITE, TEXT("Socialite"), 0.4f);
                R.WealthFavor = 3.0f;
                R.WealthDisfavor = 0.3f;
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Trade, 1.8f);
                R.bRequiresSettlement = true;
                R.DayWeight = 0.5f;
                R.NightWeight = 1.4f;
            }

            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_NOBLE, TEXT("Noble"), 0.25f);
                R.WealthFavor = 4.0f;
                R.WealthDisfavor = 0.1f;
                R.bRequiresSettlement = true;
                R.bAllowedAlone = false;
                R.MinGroupSize = 3;
                R.MaxGroupSize = 4;
            }

            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_TRAVELER, TEXT("Traveler"), 0.5f);
                R.WealthFavor = 1.4f;
                R.DayWeight = 1.3f;
                R.NightWeight = 0.5f;
            }

            return A;
        }();
        return Archetypes;
    }
}

TConstArrayView<FMythicArchetypeRow> GetCodeDefaultArchetypes() {
    return DefaultArchetypeArray();
}
}
