
#include "World/LivingWorld/Spawn/DesignerSpawnerRegistry.h"
#include "World/LivingWorld/LivingWorldTypes.h"

void UMythicDesignerSpawnerRegistry::Serialize(FArchive& Ar) {
    int32 LocalVersion = 1;
    Ar << LocalVersion;

    int32 Count = States.Num();
    Ar << Count;

    if (Ar.IsSaving()) {
        for (TPair<FName, FMythicDesignerSpawnerState>& Pair : States) {
            FString IdStr = Pair.Key.ToString();
            int32 SpawnsEver = Pair.Value.SpawnsEver;
            uint8 bPerma = Pair.Value.bPermaDead ? 1 : 0;
            double LastDeath = Pair.Value.LastDeathTime;

            Ar << IdStr;
            Ar << SpawnsEver;
            Ar << bPerma;
            Ar << LastDeath;
        }
    }
    else {
        if (Count < 0 || Count > 1000000) {
            Ar.SetError();
            return;
        }

        States.Empty(Count);
        for (int32 i = 0; i < Count; ++i) {
            FString IdStr;
            int32 SpawnsEver = 0;
            uint8 bPerma = 0;
            double LastDeath = 0.0;

            Ar << IdStr;
            Ar << SpawnsEver;
            Ar << bPerma;
            Ar << LastDeath;

            FMythicDesignerSpawnerState State;
            State.SpawnsEver = SpawnsEver;
            State.bPermaDead = (bPerma != 0);
            State.LastDeathTime = LastDeath;
            States.Add(FName(*IdStr), State);
        }

        UE_LOG(LogMythLivingWorld, Log, TEXT("DesignerSpawnerRegistry loaded %d spawner state(s)."), Count);
    }
}
