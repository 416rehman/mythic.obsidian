// Mythic Living World — Code-default archetype set + context defaults
// Built-in archetypes so role-from-context runs unauthored. One static array, returned as a view.

#include "World/LivingWorld/Roles/ArchetypeTypes.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h" // EMythicSettlementEconomy (full enum)
#include "World/LivingWorld/Territory/MythicBiome.h"        // EMythicBiome (full enum)
#include "World/LivingWorld/MythicTags_LivingWorld.h"       // NPC.Role.* native role tags

// The context's enum members can only be default-initialized to a real enumerator in the .cpp, where the full enum
// definitions are visible (the header only has forward decls). Generic settlement + Plains biome are the neutral
// baselines and match the spawner's own fallbacks.
FMythicArchetypeContext::FMythicArchetypeContext()
    : Economy(EMythicSettlementEconomy::Generic)
    , Biome(EMythicBiome::Plains) {}

namespace MythicArchetypeDefaults {

namespace {
    // Number of economy/biome slots so the helper can size the per-context weight arrays exactly.
    constexpr int32 EconomyCount = static_cast<int32>(EMythicSettlementEconomy::COUNT);
    constexpr int32 BiomeCount = MythicBiomeCount;

    // Build an economy-weight array with all-neutral (1.0) defaults, overriding a single index. Returns a fully-sized
    // array so DeriveArchetype's IsValidIndex guard always hits (and any unspecified economy stays neutral).
    TArray<float> EconomyW(EMythicSettlementEconomy Favored, float Weight) {
        TArray<float> Out;
        Out.Init(1.0f, EconomyCount);
        const int32 Idx = static_cast<int32>(Favored);
        if (Out.IsValidIndex(Idx)) {
            Out[Idx] = Weight;
        }
        return Out;
    }

    // Build a biome-weight array, all-neutral with a single index overridden.
    TArray<float> BiomeW(EMythicBiome Favored, float Weight) {
        TArray<float> Out;
        Out.Init(1.0f, BiomeCount);
        const int32 Idx = static_cast<int32>(Favored);
        if (Out.IsValidIndex(Idx)) {
            Out[Idx] = Weight;
        }
        return Out;
    }

    // Built once on first use, immutable thereafter — safe to hand out as a const view from any thread.
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

            // ─── Civilian: ALWAYS eligible, all-neutral. Guarantees Total > 0 for every context. ───
            // No economy/biome favor, no faction/settlement requirement, alone-allowed. This is the safety floor the
            // doctrine requires: even an unowned/empty context still has a positive-weight row to draw.
            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_CIVILIAN, TEXT("Civilian"), 1.0f);
                R.bAllowedAlone = true;
            }

            // ─── Farmer: strongly favored in Farming economies; thrives on Plains. Daytime worker. ───
            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_FARMER, TEXT("Farmer"), 1.0f);
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Farming, 4.0f);
                R.BiomeWeights = BiomeW(EMythicBiome::Plains, 1.5f);
                R.DayWeight = 1.4f;
                R.NightWeight = 0.4f;
            }

            // ─── Merchant: favored in Trade economies and where there is wealth. Daytime. ───
            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_MERCHANT, TEXT("Merchant"), 1.0f);
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Trade, 4.0f);
                R.WealthFavor = 2.0f;
                R.DayWeight = 1.4f;
                R.NightWeight = 0.3f;
            }

            // ─── Laborer: favored in Mining; favored by poverty (cheap labor). Daytime. ───
            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_LABORER, TEXT("Laborer"), 1.0f);
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Mining, 4.0f);
                R.WealthDisfavor = 1.6f;
                R.BiomeWeights = BiomeW(EMythicBiome::Mountain, 1.5f);
                R.DayWeight = 1.3f;
                R.NightWeight = 0.5f;
            }

            // ─── Fisher: favored in Fishing economies and Wetland biomes. Water-capable. ───
            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_FISHER, TEXT("Fisher"), 1.0f);
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Fishing, 5.0f);
                R.BiomeWeights = BiomeW(EMythicBiome::Wetland, 2.0f);
                R.bWaterCapable = true;
                R.DayWeight = 1.3f;
                R.NightWeight = 0.4f;
            }

            // ─── Guard: town defender. Favored by military strength + Military economy. Active day AND night. ───
            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_GUARD, TEXT("Guard"), 0.8f);
                R.MilitaryFavor = 3.0f;
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Military, 2.0f);
                R.DayWeight = 1.0f;
                R.NightWeight = 1.0f; // night watch
            }

            // ─── Soldier: field military. Strongly favored by military strength + Military economy. ───
            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_SOLDIER, TEXT("Soldier"), 0.6f);
                R.MilitaryFavor = 4.0f;
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Military, 4.0f);
                R.DayWeight = 1.0f;
                R.NightWeight = 0.8f;
            }

            // ─── Beggar: favored by poverty; suppressed by wealth. Settlement-only, daytime. ───
            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_BEGGAR, TEXT("Beggar"), 0.5f);
                R.WealthDisfavor = 3.0f;
                R.WealthFavor = 0.2f; // rich towns drive beggars out
                R.bRequiresSettlement = true;
                R.DayWeight = 1.2f;
                R.NightWeight = 0.3f;
            }

            // ─── Socialite: favored by wealth. Settlement-only. Evening/night creature. ───
            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_SOCIALITE, TEXT("Socialite"), 0.4f);
                R.WealthFavor = 3.0f;
                R.WealthDisfavor = 0.3f;
                R.EconomyWeights = EconomyW(EMythicSettlementEconomy::Trade, 1.8f);
                R.bRequiresSettlement = true;
                R.DayWeight = 0.5f;
                R.NightWeight = 1.4f;
            }

            // ─── Noble: rare, wealth-gated, settlement-only, NEVER appears alone (spawns with a retinue, Step 4). ───
            {
                FMythicArchetypeRow &R = Add(TAG_NPC_ROLE_NOBLE, TEXT("Noble"), 0.25f);
                R.WealthFavor = 4.0f;
                R.WealthDisfavor = 0.1f;
                R.bRequiresSettlement = true;
                R.bAllowedAlone = false; // group-only
                R.MinGroupSize = 3;
                R.MaxGroupSize = 4;
            }

            // ─── Traveler: the rare ambient that is at home OUTSIDE a settlement (the wilderness draw's anchor). ───
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
} // namespace

TConstArrayView<FMythicArchetypeRow> GetCodeDefaultArchetypes() {
    // TArray -> TConstArrayView via the implicit view constructor; the backing array is a function-local static (built
    // once, immutable), so the view stays valid for the program lifetime.
    return DefaultArchetypeArray();
}

} // namespace MythicArchetypeDefaults
