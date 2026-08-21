
#include "World/LivingWorld/Creatures/CreatureSpeciesTypes.h"

namespace MythicCreatureDefaults {
namespace {
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

    const TArray<FMythicCreatureSpeciesRow> &DefaultSpeciesArray() {
        static const TArray<FMythicCreatureSpeciesRow> Species = {
            MakeSpecies(1, TEXT("Deer"),    EMythicBiome::Plains,    0.05f, true,  2,   3.0f,  3, 7),
            MakeSpecies(2, TEXT("Boar"),    EMythicBiome::Plains,    0.45f, false, 2,   1.0f,  1, 1),
            MakeSpecies(3, TEXT("Coyote"),  EMythicBiome::Plains,    0.55f, true,  3,   0.6f,  2, 4),
            MakeSpecies(4, TEXT("Stag"),    EMythicBiome::Forest,    0.05f, true,  2,   2.5f,  2, 5),
            MakeSpecies(5, TEXT("Wolf"),    EMythicBiome::Forest,    0.75f, true,  4,   1.2f,  3, 6),
            MakeSpecies(6, TEXT("Bear"),    EMythicBiome::Forest,    0.80f, false, 3,   0.5f,  1, 1),
            MakeSpecies(7, TEXT("Goat"),    EMythicBiome::Mountain,  0.10f, true,  2,   2.5f,  2, 5),
            MakeSpecies(8, TEXT("Cougar"),  EMythicBiome::Mountain,  0.78f, false, 4,   0.7f,  1, 1),
            MakeSpecies(9, TEXT("Ram"),     EMythicBiome::Mountain,  0.35f, true,  2,   1.0f,  2, 4),
            MakeSpecies(10, TEXT("Heron"),  EMythicBiome::Wetland,   0.05f, true,  2,   2.0f,  2, 5),
            MakeSpecies(11, TEXT("Boar"),   EMythicBiome::Wetland,   0.45f, false, 2,   1.5f,  1, 1),
            MakeSpecies(12, TEXT("Croc"),   EMythicBiome::Wetland,   0.85f, false, 3,   0.5f,  1, 1),
            MakeSpecies(13, TEXT("Jackal"), EMythicBiome::Wasteland, 0.50f, true,  3,   1.5f,  2, 4),
            MakeSpecies(14, TEXT("Vulture"),EMythicBiome::Wasteland, 0.20f, true,  2,   1.5f,  2, 4),
            MakeSpecies(15, TEXT("Hyena"),  EMythicBiome::Wasteland, 0.70f, true,  3,   0.8f,  3, 5),
            MakeSpecies(16, TEXT("Antelope"),EMythicBiome::Desert,   0.05f, true,  2,   2.5f,  3, 6),
            MakeSpecies(17, TEXT("Scorpion"),EMythicBiome::Desert,   0.40f, false, 2,   1.2f,  1, 1),
            MakeSpecies(18, TEXT("Jackal"), EMythicBiome::Desert,    0.55f, true,  3,   1.0f,  2, 4),
        };
        return Species;
    }
}

TConstArrayView<FMythicCreatureSpeciesRow> GetCodeDefaultSpecies() {
    return DefaultSpeciesArray();
}
}
