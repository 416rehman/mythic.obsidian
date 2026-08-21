
#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "PersistentNPCRegistry.generated.h"

class UMythicSocialGraph;
class UMythicCausalFabric;

USTRUCT()
struct FMythicPersistentDeathRecord {
    GENERATED_BODY()

    uint32 NameHash = 0;

    FMythicFactionId Faction;

    FGameplayTag RoleTag;

    double DeathTime = 0.0;

    FMythicCellCoord DeathCell;
};

UCLASS()
class MYTHIC_API UMythicPersistentNPCRegistry : public UObject {
    GENERATED_BODY()

public:
    void RegisterDeath(
        uint32 NameHash,
        const FMythicFactionId &Faction,
        const FGameplayTag &RoleTag,
        const FMythicCellCoord &Cell,
        double WorldTime,
        class UMythicLivingWorldSubsystem *OwningLWS);

    bool IsPermaDead(uint32 NameHash) const {
        return DeadNPCHashes.Contains(NameHash);
    }

    int32 AllocateSpawnSerial() {
        const int32 Serial = static_cast<int32>(NextSpawnSerial);
        ++NextSpawnSerial;
        return Serial;
    }

    const TArray<FMythicPersistentDeathRecord> &GetDeathRecords() const { return DeathRecords; }

    int32 GetDeathCount() const { return DeadNPCHashes.Num(); }

    virtual void Serialize(FArchive &Ar) override;

private:
    TSet<uint32> DeadNPCHashes;

    TArray<FMythicPersistentDeathRecord> DeathRecords;

    uint32 NextSpawnSerial = 0;
};
