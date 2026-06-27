// Mythic Living World — Code-default creature species set
// Built-in wildlife so the creature ecology runs unauthored. One static array, returned as a view.

#include "World/LivingWorld/Creatures/CreatureSpeciesTypes.h"

namespace MythicCreatureDefaults {

namespace {
    // Helper to keep the table terse and self-documenting.
    FMythicCreatureSpeciesRow MakeSpecies(uint8 Id, const TCHAR *Name, EMythicBiome Biome, float Aggression,
                                          bool bPack, uint8 TerritorialRadius, float Weight,
                                          uint8 MinPack, uint8 MaxPack) {
        FMythicCreatureSpeciesRow Row;
        Row.SpeciesId = Id;
        Row.DisplayName = FName(Name);
        Row.Biome = Biome;
        Row.BaseAggression = Aggression;
        Row.bIsPackAnimal = bPack;
        Row.DefaultTerritorialRadius = TerritorialRadius;
        Row.SpawnWeight = Weight;
        Row.MinPackSize = MinPack;
        Row.MaxPackSize = MaxPack;
        return Row;
    }

    // Built once on first use, immutable thereafter — safe to hand out as a const view from any thread.
    // Every biome has at least one low-aggression herbivore (common) + one predator (rarer).
    // SpeciesIds are stable + unique across the whole set.
    const TArray<FMythicCreatureSpeciesRow> &DefaultSpeciesArray() {
        static const TArray<FMythicCreatureSpeciesRow> Species = {
            //         Id  Name           Biome                    Aggr  Pack  TerrR Weight Min Max
            // ─ Plains ─
            MakeSpecies(1, TEXT("Deer"),    EMythicBiome::Plains,    0.05f, true,  2,   3.0f,  3, 7),
            MakeSpecies(2, TEXT("Boar"),    EMythicBiome::Plains,    0.45f, false, 2,   1.0f,  1, 1),
            MakeSpecies(3, TEXT("Coyote"),  EMythicBiome::Plains,    0.55f, true,  3,   0.6f,  2, 4),
            // ─ Forest ─
            MakeSpecies(4, TEXT("Stag"),    EMythicBiome::Forest,    0.05f, true,  2,   2.5f,  2, 5),
            MakeSpecies(5, TEXT("Wolf"),    EMythicBiome::Forest,    0.75f, true,  4,   1.2f,  3, 6),
            MakeSpecies(6, TEXT("Bear"),    EMythicBiome::Forest,    0.80f, false, 3,   0.5f,  1, 1),
            // ─ Mountain ─
            MakeSpecies(7, TEXT("Goat"),    EMythicBiome::Mountain,  0.10f, true,  2,   2.5f,  2, 5),
            MakeSpecies(8, TEXT("Cougar"),  EMythicBiome::Mountain,  0.78f, false, 4,   0.7f,  1, 1),
            MakeSpecies(9, TEXT("Ram"),     EMythicBiome::Mountain,  0.35f, true,  2,   1.0f,  2, 4),
            // ─ Wetland ─
            MakeSpecies(10, TEXT("Heron"),  EMythicBiome::Wetland,   0.05f, true,  2,   2.0f,  2, 5),
            MakeSpecies(11, TEXT("Boar"),   EMythicBiome::Wetland,   0.45f, false, 2,   1.5f,  1, 1),
            MakeSpecies(12, TEXT("Croc"),   EMythicBiome::Wetland,   0.85f, false, 3,   0.5f,  1, 1),
            // ─ Wasteland ─
            MakeSpecies(13, TEXT("Jackal"), EMythicBiome::Wasteland, 0.50f, true,  3,   1.5f,  2, 4),
            MakeSpecies(14, TEXT("Vulture"),EMythicBiome::Wasteland, 0.20f, true,  2,   1.5f,  2, 4),
            MakeSpecies(15, TEXT("Hyena"),  EMythicBiome::Wasteland, 0.70f, true,  3,   0.8f,  3, 5),
            // ─ Desert ─
            MakeSpecies(16, TEXT("Antelope"),EMythicBiome::Desert,   0.05f, true,  2,   2.5f,  3, 6),
            MakeSpecies(17, TEXT("Scorpion"),EMythicBiome::Desert,   0.40f, false, 2,   1.2f,  1, 1),
            MakeSpecies(18, TEXT("Jackal"), EMythicBiome::Desert,    0.55f, true,  3,   1.0f,  2, 4),
        };
        return Species;
    }
}

TConstArrayView<FMythicCreatureSpeciesRow> GetCodeDefaultSpecies() {
    // TArray -> TConstArrayView via the implicit view constructor; the backing array is a function-local static (built
    // once, immutable), so the view stays valid for the program lifetime.
    return DefaultSpeciesArray();
}

} // namespace MythicCreatureDefaults
