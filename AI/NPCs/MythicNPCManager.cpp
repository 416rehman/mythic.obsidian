

#include "MythicNPCManager.h"

#include "Mythic.h"
#include "MythicNPCCharacter.h"
#include "Engine/World.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Settings/MythicCombatSettings.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "NPCDefinition.h"

int32 UMythicNPCManager::ResolveCombatLevelAt(const FVector &SpawnLocation) const {
    return MythicCombat::ResolveCombatLevelAt(GetWorld(), SpawnLocation);
}

void UMythicNPCManager::Initialize(FSubsystemCollectionBase &Collection) {
    if (GetWorld()->GetNetMode() >= NM_Client) {
        UE_LOG(Myth, Log, TEXT("Skipping NPC Manager initialization on client"));
        return;
    }

    Super::Initialize(Collection);
}

AMythicNPCCharacter *UMythicNPCManager::GetFromPool(FGameplayTag NPCType) {
    if (!NPCType.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("UMythicNPCManager::GetFromPool: NPCType is invalid."));
        return nullptr;
    }


    if (NPCCharacterPool.Num() > 0) {
        if (AMythicNPCCharacter *PooledNPC = NPCCharacterPool.Pop()) {
            return PooledNPC;
        }

        NPCCharacterPool.Remove(nullptr);
        UE_LOG(Myth, Warning, TEXT("Found and removed nullptr from NPC pool for type %s."), *NPCType.ToString());
    }
    return nullptr;
}

void UMythicNPCManager::ReturnToPool(AMythicNPCCharacter *NPC, bool bShouldCache) {
    if (!NPC) {
        UE_LOG(Myth, Error, TEXT("UMythicNPCManager::ReturnToPool: Attempted to return a null NPC."));
        return;
    }

    const FGuid NPCId = NPC->GetNPCId();

    if (bShouldCache) {
        auto NPCData = NPC->GetNPCData();
        CacheNPC(NPCData);
    }

    NPC->OnReturnedToPool();
    ActiveNPCs.Remove(NPCId);
    NPCCharacterPool.Add(NPC);

    UE_LOG(Myth, Verbose, TEXT("NPC returned to pool (ID: %s)"), *NPCId.ToString());
}

void UMythicNPCManager::BuildDefinitionIndex() {
    if (bDefinitionIndexBuilt) {
        return;
    }
    bDefinitionIndexBuilt = true;

    const FAssetRegistryModule &Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> Assets;
    Registry.Get().GetAssetsByClass(UNPCDefinition::StaticClass()->GetClassPathName(), Assets, true);

    int32 Indexed = 0;
    for (const FAssetData &Asset : Assets) {
        UNPCDefinition *Def = Cast<UNPCDefinition>(Asset.GetAsset());
        if (!Def || !Def->NPCType.IsValid()) {
            continue;
        }
        DefinitionsByType.FindOrAdd(Def->NPCType).Defs.Add(Def);
        ++Indexed;
    }
    UE_LOG(Myth, Log, TEXT("NPCManager: definition index built - %d definitions across %d types."),
           Indexed, DefinitionsByType.Num());
}

int32 UMythicNPCManager::CountDefinitionsForType(const FGameplayTag NPCType) {
    BuildDefinitionIndex();
    int32 Count = 0;
    for (const TPair<FGameplayTag, FMythicNPCDefinitionBucket> &Bucket : DefinitionsByType) {
        if (Bucket.Key.MatchesTag(NPCType)) {
            Count += Bucket.Value.Defs.Num();
        }
    }
    return Count;
}

bool UMythicNPCManager::ReclaimNPC(AMythicNPCCharacter *NPC) {
    if (!NPC || !ActiveNPCs.Contains(NPC->GetNPCId())) {
        return false;
    }
    // Emergent NPCs die and are done: no cache, straight back to the pool for the next ambush.
    ReturnToPool(NPC, false);
    return true;
}

AMythicNPCCharacter *UMythicNPCManager::SpawnPredefinedNPC(UNPCDefinition *NPCDef, FVector SpawnLocation, FRotator SpawnRotation) {
    if (!GetWorld()) {
        return nullptr;
    }
    if (!NPCDef) {
        UE_LOG(Myth, Error, TEXT("UMythicNPCManager::SpawnPredefinedNPC: NPCDefinitionAsset is null."));
        return nullptr;
    }
    if (!NPCDef->NPCId.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UMythicNPCManager::SpawnPredefinedNPC: NPCDefinitionAsset has an invalid NPCId. Asset: %s"), *NPCDef->GetName());
        return nullptr;
    }
    if (!NPCDef->NPCType.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UMythicNPCManager::SpawnPredefinedNPC: NPCDefinitionAsset has an invalid NPCType. Asset: %s"), *NPCDef->GetName());
        return nullptr;
    }

    if (CachedNPCs.Contains(NPCDef->NPCId)) {
        UE_LOG(Myth, Log, TEXT("SpawnPredefinedNPC: NPC ID %s found in cache, redirecting to SpawnCachedNPC."), *NPCDef->NPCId.ToString());
        return SpawnCachedNPC(NPCDef->NPCId, SpawnLocation, SpawnRotation);
    }

    AMythicNPCCharacter *SpawnedNPC = GetFromPool(NPCDef->NPCType);

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    if (SpawnedNPC) {
        SpawnedNPC->SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
    }
    else {
        UClass *NPCClassToSpawn = AMythicNPCCharacter::StaticClass();

        SpawnedNPC = GetWorld()->SpawnActor<AMythicNPCCharacter>(NPCClassToSpawn, SpawnLocation, SpawnRotation, SpawnInfo);
        if (!SpawnedNPC) {
            UE_LOG(Myth, Error, TEXT("UMythicNPCManager::SpawnPredefinedNPC: Failed to spawn new actor for NPC ID %s, Type %s."), *NPCDef->NPCId.ToString(),
                   *NPCDef->NPCType.ToString());
            return nullptr;
        }
        UE_LOG(Myth, Log, TEXT("Spawned new NPC %s for Predefined ID %s, Type %s."), *SpawnedNPC->GetName(), *NPCDef->NPCId.ToString(),
               *NPCDef->NPCType.ToString());
    }

    FMythicNPCData Data(NPCDef);
    Data.CombatLevel = ResolveCombatLevelAt(SpawnLocation);
    SpawnedNPC->OnSpawnedFromPool(Data);

    ActiveNPCs.Add(NPCDef->NPCId, SpawnedNPC);
    return SpawnedNPC;
}

AMythicNPCCharacter *UMythicNPCManager::SpawnRandomNPC(FGameplayTag NPCType, FVector SpawnLocation, FRotator SpawnRotation) {
    if (!GetWorld()) {
        return nullptr;
    }
    if (!NPCType.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UMythicNPCManager::SpawnRandomNPC: NPCType is invalid."));
        return nullptr;
    }


    // A random spawn is a fresh individual of the requested type: pick among every authored definition
    // whose type sits under the request, so asking for NPC.Type.Bandit can produce any bandit variant.
    BuildDefinitionIndex();
    TArray<UNPCDefinition *> Candidates;
    for (const TPair<FGameplayTag, FMythicNPCDefinitionBucket> &Bucket : DefinitionsByType) {
        if (Bucket.Key.MatchesTag(NPCType)) {
            for (const TObjectPtr<UNPCDefinition> &Def : Bucket.Value.Defs) {
                Candidates.Add(Def);
            }
        }
    }
    if (Candidates.Num() == 0) {
        UE_LOG(Myth, Error, TEXT("UMythicNPCManager::SpawnRandomNPC: no NPCDefinition authored for type %s."),
               *NPCType.ToString());
        return nullptr;
    }
    UNPCDefinition *Chosen = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];

    AMythicNPCCharacter *SpawnedNPC = GetFromPool(NPCType);

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    if (SpawnedNPC) {
        SpawnedNPC->SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
    }
    else {
        UClass *NPCClassToSpawn = AMythicNPCCharacter::StaticClass();

        SpawnedNPC = GetWorld()->SpawnActor<AMythicNPCCharacter>(NPCClassToSpawn, SpawnLocation, SpawnRotation, SpawnInfo);
        if (!SpawnedNPC) {
            UE_LOG(Myth, Error, TEXT("UMythicNPCManager::SpawnRandomNPC: Failed to spawn new actor for NPCType %s."), *NPCType.ToString());
            return nullptr;
        }

        UE_LOG(Myth, Log, TEXT("Spawned new NPC %s for Random Type %s."), *SpawnedNPC->GetName(), *NPCType.ToString());
    }

    // The definition is the template, not the individual: a fresh id per spawn so two bandits from one
    // definition never collide in the active map, and the ambush in Extreme territory fights at its level.
    FMythicNPCData Data(Chosen);
    Data.NPCId = FGuid::NewGuid();
    Data.CombatLevel = ResolveCombatLevelAt(SpawnLocation);
    SpawnedNPC->OnSpawnedFromPool(Data);

    ActiveNPCs.Add(Data.NPCId, SpawnedNPC);
    return SpawnedNPC;
}

AMythicNPCCharacter *UMythicNPCManager::SpawnCachedNPC(FGuid NPCId, FVector SpawnLocation, FRotator SpawnRotation) {
    if (!GetWorld()) {
        return nullptr;
    }

    if (!NPCId.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UMythicNPCManager::SpawnCachedNPC: NPCId is invalid."));
        return nullptr;
    }

    const FMythicCachedNPCData *CachedData = CachedNPCs.Find(NPCId);
    if (!CachedData) {
        UE_LOG(Myth, Error, TEXT("UMythicNPCManager::SpawnCachedNPC: No cached data found for NPC ID %s."), *NPCId.ToString());
        return nullptr;
    }

    FGameplayTag NPCType = CachedData->NPCData.NPCType;
    if (!NPCType.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UMythicNPCManager::SpawnCachedNPC: NPCType in cached data for ID %s is invalid. Cannot determine pool category."),
               *NPCId.ToString());
        return nullptr;
    }

    AMythicNPCCharacter *SpawnedNPC = GetFromPool(NPCType);

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    if (SpawnedNPC) {
        SpawnedNPC->SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
    }
    else {
        UClass *NPCClassToSpawn = AMythicNPCCharacter::StaticClass();

        SpawnedNPC = GetWorld()->SpawnActor<AMythicNPCCharacter>(NPCClassToSpawn, SpawnLocation, SpawnRotation, SpawnInfo);
        if (!SpawnedNPC) {
            UE_LOG(Myth, Error, TEXT("UMythicNPCManager::SpawnCachedNPC: Failed to spawn new actor for NPC ID %s, Type %s."), *NPCId.ToString(),
                   *NPCType.ToString());
            return nullptr;
        }
        UE_LOG(Myth, Log, TEXT("Spawned new NPC %s for Cached ID %s, Type %s."), *SpawnedNPC->GetName(), *NPCId.ToString(), *NPCType.ToString());
    }

    // Level reflects where the NPC stands NOW, not where it was cached.
    FMythicNPCData Data = CachedData->NPCData;
    Data.CombatLevel = ResolveCombatLevelAt(SpawnLocation);
    SpawnedNPC->OnSpawnedFromPool(Data);

    ActiveNPCs.Add(NPCId, SpawnedNPC);
    CachedNPCs.Remove(NPCId);

    return SpawnedNPC;
}

bool UMythicNPCManager::GetCachedNPCData(const FGuid NPCId, FMythicNPCData &NPCData) {
    NPCData = CachedNPCs.FindRef(NPCId).NPCData;
    return NPCData.NPCId == NPCId;
}

bool UMythicNPCManager::GetCachedFamily(FGuid FamilyId, FFamilySpec &FamilySpec) {
    FamilySpec = CachedFamilies.FindRef(FamilyId);
    return FamilySpec.FamilyId == FamilyId;
}

void UMythicNPCManager::CacheNPC(FMythicNPCData NPCData) {
    auto CachedData = FMythicCachedNPCData(NPCData);
    CachedNPCs.Add(NPCData.NPCId, CachedData);

    if (NPCData.NPCFamilyId.IsValid()) {
        auto FamilySpec = this->ActiveFamilySpecs.FindRef(NPCData.NPCFamilyId);
        if (FamilySpec.FamilyId.IsValid()) {
            CachedFamilies.Add(FamilySpec.FamilyId, FamilySpec);

            auto FatherNPC = ActiveNPCs.FindRef(FamilySpec.FatherId);
            if (FatherNPC) {
                CachedNPCs.Add(FamilySpec.FatherId, FatherNPC->GetNPCData());
            }

            auto MotherNPC = ActiveNPCs.FindRef(FamilySpec.MotherId);
            if (MotherNPC) {
                CachedNPCs.Add(FamilySpec.MotherId, MotherNPC->GetNPCData());
            }

            for (auto ChildId : FamilySpec.ChildrenIds) {
                auto ChildNPC = ActiveNPCs.FindRef(ChildId);
                if (ChildNPC) {
                    CachedNPCs.Add(ChildId, ChildNPC->GetNPCData());
                }
            }
        }
    }
}

void UMythicNPCManager::RemoveCachedNPC(FGuid NPCId) {
    auto CachedData = CachedNPCs.FindRef(NPCId);

    if (CachedData.NPCData.NPCFamilyId.IsValid()) {
        auto FamilySpec = CachedFamilies.FindRef(CachedData.NPCData.NPCFamilyId);
        if (FamilySpec.FamilyId.IsValid()) {
            bool AllFamilyMembersUncached = true;
            if (FamilySpec.FatherId.IsValid()) {
                auto FatherData = CachedNPCs.FindRef(FamilySpec.FatherId);
                if (FatherData.NPCData.NPCId.IsValid() && FatherData.NPCData.NPCId != NPCId) {
                    AllFamilyMembersUncached = false;
                    UE_LOG(Myth, Log, TEXT("Removed NPC %s from family but family will remain cached because Father is still cached."), *NPCId.ToString());
                }
            }
            else if (FamilySpec.MotherId.IsValid()) {
                auto MotherData = CachedNPCs.FindRef(FamilySpec.MotherId);
                if (MotherData.NPCData.NPCId.IsValid() && MotherData.NPCData.NPCId != NPCId) {
                    AllFamilyMembersUncached = false;
                    UE_LOG(Myth, Log, TEXT("Removed NPC %s from family but family will remain cached because Mother is still cached."), *NPCId.ToString());
                }
            }
            else if (FamilySpec.ChildrenIds.Num() > 0) {
                for (auto ChildId : FamilySpec.ChildrenIds) {
                    auto ChildData = CachedNPCs.FindRef(ChildId);
                    if (ChildData.NPCData.NPCId.IsValid() && ChildData.NPCData.NPCId != NPCId) {
                        AllFamilyMembersUncached = false;
                        UE_LOG(Myth, Log, TEXT("Removed NPC %s from family but family will remain cached because Child is still cached."), *NPCId.ToString());
                        break;
                    }
                }
            }

            if (AllFamilyMembersUncached) {
                CachedFamilies.Remove(FamilySpec.FamilyId);
                UE_LOG(Myth, Log, TEXT("NPC %s was the last member of family %s, removing family."), *NPCId.ToString(), *FamilySpec.FamilyId.ToString());
            }
        }
    }

    CachedNPCs.Remove(NPCId);
    UE_LOG(Myth, Log, TEXT("Removed NPC: %s"), *NPCId.ToString());
}
