// 


#include "MythicNPCManager.h"

#include "Mythic.h"
#include "MythicNPCCharacter.h"

void UMythicNPCManager::Initialize(FSubsystemCollectionBase &Collection) {
    // Server Only Subsystem
    if (GetWorld()->GetNetMode() >= NM_Client) {
        UE_LOG(Mythic, Log, TEXT("Skipping NPC Manager initialization on client"));
        return;
    }

    Super::Initialize(Collection);
}

AMythicNPCCharacter *UMythicNPCManager::GetFromPool(FGameplayTag NPCType) {
    if (!NPCType.IsValid()) {
        UE_LOG(Mythic, Warning, TEXT("UMythicNPCManager::GetFromPool: NPCType is invalid."));
        return nullptr;
    }


    if (NPCCharacterPool.Num() > 0) {
        if (AMythicNPCCharacter *PooledNPC = NPCCharacterPool.Pop()) {
            UE_LOG(Mythic, Log, TEXT("Reusing NPC %s from pool for type %s."), *PooledNPC->GetName(), *NPCType.ToString());
            return PooledNPC;
        }

        // If the popped NPC is invalid, that means pool has nulls. Remove them.
        NPCCharacterPool.Remove(nullptr);
        UE_LOG(Mythic, Warning, TEXT("Found and removed nullptr from NPC pool for type %s."), *NPCType.ToString());
    }
    return nullptr;
}

void UMythicNPCManager::ReturnToPool(AMythicNPCCharacter *NPC, bool bShouldCache) {
    if (!NPC) {
        UE_LOG(Mythic, Error, TEXT("UMythicNPCManager::ReturnToPool: Attempted to return a null NPC."));
        return;
    }

    auto NPCId = NPC->GetNPCId();
    auto NPCIdAsStr = NPCId.ToString();
    auto NPCTypeAsStr = NPC->GetNPCType().ToString();

    if (bShouldCache) {
        UE_LOG(Mythic, Log, TEXT("Caching NPC (ID: %s, Type: %s) for later use."), *NPCIdAsStr, *NPCTypeAsStr);

        auto NPCData = NPC->GetNPCData();
        CacheNPC(NPCData);
    }

    UE_LOG(Mythic, Log, TEXT("Returning NPC (ID: %s, Type: %s) to pool."), *NPCIdAsStr, *NPCTypeAsStr);
    NPC->OnReturnedToPool(); // After this, the NPCData will not be valid.

    ActiveNPCs.Remove(NPCId);
    UE_LOG(Mythic, Log, TEXT("Removed NPC (ID: %s, Type: %s) from active list."), *NPCIdAsStr, *NPCTypeAsStr);

    NPCCharacterPool.Add(NPC);
    UE_LOG(Mythic, Log, TEXT("Returned NPC (ID: %s, Type: %s) to pool."), *NPCIdAsStr, *NPCTypeAsStr);
}

AMythicNPCCharacter *UMythicNPCManager::SpawnPredefinedNPC(UNPCDefinition *NPCDef, FVector SpawnLocation, FRotator SpawnRotation) {
    if (!GetWorld()) {
        return nullptr; // Basic safety check
    }
    if (!NPCDef) {
        UE_LOG(Mythic, Error, TEXT("UMythicNPCManager::SpawnPredefinedNPC: NPCDefinitionAsset is null."));
        return nullptr;
    }
    if (!NPCDef->NPCId.IsValid()) {
        UE_LOG(Mythic, Error, TEXT("UMythicNPCManager::SpawnPredefinedNPC: NPCDefinitionAsset has an invalid NPCId. Asset: %s"), *NPCDef->GetName());
        return nullptr;
    }
    if (!NPCDef->NPCType.IsValid()) {
        UE_LOG(Mythic, Error, TEXT("UMythicNPCManager::SpawnPredefinedNPC: NPCDefinitionAsset has an invalid NPCType. Asset: %s"), *NPCDef->GetName());
        return nullptr;
    }

    // If the NPC is already cached, redirect to SpawnCachedNPC
    if (CachedNPCs.Contains(NPCDef->NPCId)) {
        UE_LOG(Mythic, Log, TEXT("SpawnPredefinedNPC: NPC ID %s found in cache, redirecting to SpawnCachedNPC."), *NPCDef->NPCId.ToString());
        return SpawnCachedNPC(NPCDef->NPCId, SpawnLocation, SpawnRotation);
    }

    AMythicNPCCharacter *SpawnedNPC = GetFromPool(NPCDef->NPCType);

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    if (SpawnedNPC) {
        SpawnedNPC->SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
    }
    else {
        // Should NPCDef have a class to spawn?
        UClass *NPCClassToSpawn = AMythicNPCCharacter::StaticClass();

        SpawnedNPC = GetWorld()->SpawnActor<AMythicNPCCharacter>(NPCClassToSpawn, SpawnLocation, SpawnRotation, SpawnInfo);
        if (!SpawnedNPC) {
            UE_LOG(Mythic, Error, TEXT("UMythicNPCManager::SpawnPredefinedNPC: Failed to spawn new actor for NPC ID %s, Type %s."), *NPCDef->NPCId.ToString(),
                   *NPCDef->NPCType.ToString());
            return nullptr;
        }
        UE_LOG(Mythic, Log, TEXT("Spawned new NPC %s for Predefined ID %s, Type %s."), *SpawnedNPC->GetName(), *NPCDef->NPCId.ToString(),
               *NPCDef->NPCType.ToString());
    }

    // After this OnSpawnedFromPool, the NPC's data should be set.
    SpawnedNPC->OnSpawnedFromPool(FMythicNPCData(NPCDef));

    ActiveNPCs.Add(NPCDef->NPCId, SpawnedNPC);
    return SpawnedNPC;
}

AMythicNPCCharacter *UMythicNPCManager::SpawnRandomNPC(FGameplayTag NPCType, FVector SpawnLocation, FRotator SpawnRotation) {
    if (!GetWorld()) {
        return nullptr;
    }
    if (!NPCType.IsValid()) {
        UE_LOG(Mythic, Error, TEXT("UMythicNPCManager::SpawnRandomNPC: NPCType is invalid."));
        return nullptr;
    }

    // // TODO: 
    // // might want to look up NPCType in NPCTypeDataTable to get UNPCTypeDefinition and pass that to FMythicNPCData()
    // // which would then generate the NPCData for the NPC.
    // auto NPCTypeDef = NPCTypeDataTable.FindRow<UNPCTypeDefinition>(NPCType);
    // if (!NPCTypeDef) {
    //     UE_LOG(Mythic, Error, TEXT("UMythicNPCManager::SpawnRandomNPC: NPCType %s not found in NPCTypeDataTable."), *NPCType.ToString());
    //     return nullptr;
    // }
    // auto NPCTypeDefinition = NPCTypeDef->NPCTypeDefinition;
    // auto NPCData = FMythicNPCData(NPCTypeDefinition);

    AMythicNPCCharacter *SpawnedNPC = GetFromPool(NPCType);

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    if (SpawnedNPC) {
        SpawnedNPC->SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
    }
    else { // NPC came from pool
        UClass *NPCClassToSpawn = AMythicNPCCharacter::StaticClass();

        SpawnedNPC = GetWorld()->SpawnActor<AMythicNPCCharacter>(NPCClassToSpawn, SpawnLocation, SpawnRotation, SpawnInfo);
        if (!SpawnedNPC) {
            UE_LOG(Mythic, Error, TEXT("UMythicNPCManager::SpawnRandomNPC: Failed to spawn new actor for NPCType %s."), *NPCType.ToString());
            return nullptr;
        }

        UE_LOG(Mythic, Log, TEXT("Spawned new NPC %s for Random Type %s."), *SpawnedNPC->GetName(), *NPCType.ToString());
    }

    // if (IPoolableNPC *PoolableNPC = Cast<IPoolableNPC>(SpawnedNPC)) {
    // PoolableNPC->OnSpawnedFromPool(NPCData);
    // }
    // else {
    // UE_LOG(Mythic, Error, TEXT("NPC %s (ID: %s) does not implement IPoolableNPC. Cannot call OnSpawnedFromPool."), *SpawnedNPC->GetName(), *NPCData.NPCId.ToString());
    // }

    // ActiveNPCs.Add(SpawnedNPC->GetNPCDataRef()->NPCId, SpawnedNPC);

    return SpawnedNPC;
}

AMythicNPCCharacter *UMythicNPCManager::SpawnCachedNPC(FGuid NPCId, FVector SpawnLocation, FRotator SpawnRotation) {
    if (!GetWorld()) {
        return nullptr;
    }

    if (!NPCId.IsValid()) {
        UE_LOG(Mythic, Error, TEXT("UMythicNPCManager::SpawnCachedNPC: NPCId is invalid."));
        return nullptr;
    }

    const FMythicCachedNPCData *CachedData = CachedNPCs.Find(NPCId);
    if (!CachedData) {
        UE_LOG(Mythic, Error, TEXT("UMythicNPCManager::SpawnCachedNPC: No cached data found for NPC ID %s."), *NPCId.ToString());
        return nullptr;
    }

    // Get NPCType from cached data
    FGameplayTag NPCType = CachedData->NPCData.NPCType;
    if (!NPCType.IsValid()) {
        UE_LOG(Mythic, Error, TEXT("UMythicNPCManager::SpawnCachedNPC: NPCType in cached data for ID %s is invalid. Cannot determine pool category."),
               *NPCId.ToString());
        return nullptr;
    }

    AMythicNPCCharacter *SpawnedNPC = GetFromPool(NPCType);

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    if (SpawnedNPC) { // NPC came from pool
        SpawnedNPC->SetActorLocationAndRotation(SpawnLocation, SpawnRotation);
    }
    else {
        UClass *NPCClassToSpawn = AMythicNPCCharacter::StaticClass();
        // For example: UClass* ClassFromData = GetClassFromCachedData(CachedData->NPCData); if (ClassFromData) NPCClassToSpawn = ClassFromData;

        SpawnedNPC = GetWorld()->SpawnActor<AMythicNPCCharacter>(NPCClassToSpawn, SpawnLocation, SpawnRotation, SpawnInfo);
        if (!SpawnedNPC) {
            UE_LOG(Mythic, Error, TEXT("UMythicNPCManager::SpawnCachedNPC: Failed to spawn new actor for NPC ID %s, Type %s."), *NPCId.ToString(),
                   *NPCType.ToString());
            return nullptr;
        }
        UE_LOG(Mythic, Log, TEXT("Spawned new NPC %s for Cached ID %s, Type %s."), *SpawnedNPC->GetName(), *NPCId.ToString(), *NPCType.ToString());
    }

    // After this OnSpawnedFromPool, the NPC's data should be set.
    SpawnedNPC->OnSpawnedFromPool(CachedData->NPCData);

    // Once the NPC is spawned, it should be added to the ActiveNPCs map, and not cached anymore (NPC can be cached again later with updated data)
    ActiveNPCs.Add(NPCId, SpawnedNPC);
    CachedNPCs.Remove(NPCId);

    return SpawnedNPC;
}

bool UMythicNPCManager::GetCachedNPCData(const FGuid NPCId, FMythicNPCData &NPCData) {
    auto CachedData = CachedNPCs.FindRef(NPCId);
    return CachedData.NPCData.NPCId == NPCId;
}

bool UMythicNPCManager::GetCachedFamily(FGuid FamilyId, FFamilySpec &FamilySpec) {
    FamilySpec = CachedFamilies.FindRef(FamilyId);
    return FamilySpec.FamilyId == FamilyId;
}

void UMythicNPCManager::CacheNPC(FMythicNPCData NPCData) {
    auto CachedData = FMythicCachedNPCData(NPCData);
    CachedNPCs.Add(NPCData.NPCId, CachedData);

    // If the NPC has a Family, cache the Family as well and its members
    if (NPCData.NPCFamilyId.IsValid()) {
        auto FamilySpec = this->ActiveFamilySpecs.FindRef(NPCData.NPCFamilyId);
        if (FamilySpec.FamilyId.IsValid()) {
            // Cache the family spec itself
            CachedFamilies.Add(FamilySpec.FamilyId, FamilySpec);

            // Cache the Father
            auto FatherNPC = ActiveNPCs.FindRef(FamilySpec.FatherId);
            if (FatherNPC) {
                CachedNPCs.Add(FamilySpec.FatherId, FatherNPC->GetNPCData());
            }

            // Cache the Mother
            auto MotherNPC = ActiveNPCs.FindRef(FamilySpec.MotherId);
            if (MotherNPC) {
                CachedNPCs.Add(FamilySpec.MotherId, MotherNPC->GetNPCData());
            }

            // Cache the Children
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

    // If the NPC member has a family, check if all of its family members are uncached, we can uncache the entire family.
    if (CachedData.NPCData.NPCFamilyId.IsValid()) {
        auto FamilySpec = CachedFamilies.FindRef(CachedData.NPCData.NPCFamilyId);
        if (FamilySpec.FamilyId.IsValid()) {
            // Check if all of the family members are uncached
            bool AllFamilyMembersUncached = true;
            if (FamilySpec.FatherId.IsValid()) {
                auto FatherData = CachedNPCs.FindRef(FamilySpec.FatherId);
                if (FatherData.NPCData.NPCId.IsValid() && FatherData.NPCData.NPCId != NPCId) {
                    AllFamilyMembersUncached = false;
                    UE_LOG(Mythic, Log, TEXT("Removed NPC %s from family but family will remain cached because Father is still cached."), *NPCId.ToString());
                }
            }
            else if (FamilySpec.MotherId.IsValid()) {
                auto MotherData = CachedNPCs.FindRef(FamilySpec.MotherId);
                if (MotherData.NPCData.NPCId.IsValid() && MotherData.NPCData.NPCId != NPCId) {
                    AllFamilyMembersUncached = false;
                    UE_LOG(Mythic, Log, TEXT("Removed NPC %s from family but family will remain cached because Mother is still cached."), *NPCId.ToString());
                }
            }
            else if (FamilySpec.ChildrenIds.Num() > 0) {
                for (auto ChildId : FamilySpec.ChildrenIds) {
                    auto ChildData = CachedNPCs.FindRef(ChildId);
                    if (ChildData.NPCData.NPCId.IsValid() && ChildData.NPCData.NPCId != NPCId) {
                        AllFamilyMembersUncached = false;
                        UE_LOG(Mythic, Log, TEXT("Removed NPC %s from family but family will remain cached because Child is still cached."), *NPCId.ToString());
                        break;
                    }
                }
            }

            if (AllFamilyMembersUncached) {
                CachedFamilies.Remove(FamilySpec.FamilyId);
                UE_LOG(Mythic, Log, TEXT("NPC %s was the last member of family %s, removing family."), *NPCId.ToString(), *FamilySpec.FamilyId.ToString());
            }
        }
    }

    CachedNPCs.Remove(NPCId);
    UE_LOG(Mythic, Log, TEXT("Removed NPC: %s"), *NPCId.ToString());
}
