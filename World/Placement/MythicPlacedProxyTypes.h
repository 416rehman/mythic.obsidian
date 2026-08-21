#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "MythicPlacedProxyTypes.generated.h"

UENUM(BlueprintType)
enum class EMythicProxyState : uint8 {
    Dormant,
    Promoted
};

USTRUCT(BlueprintType)
struct FMythicPlacedProxy {
    GENERATED_BODY()

    // Stable identity. Survives promotion and demotion, and is what save data and state deltas key on — NOT the actor,
    // which comes and goes.
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Proxy")
    FGuid Id;

    // What kind of thing this is (Interactable.Chest, Interactable.Vendor, …). Drives which actor class is spawned.
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Proxy")
    FGameplayTag Type;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Proxy")
    FTransform Transform;

    // Gameplay state that must outlive the actor: opened, looted, discovered, disarmed. Lives here precisely so
    // demoting an actor can never lose it.
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Proxy")
    int32 StateFlags = 0;

    // What this specific record looks like asleep and spawns back as.
    //
    // These live PER RECORD, not per type tag. Two chests can share the tag Interactable.Chest and still be different
    // Blueprints with different meshes — keying either off the tag alone would make every chest in the world render as
    // whichever one happened to be adopted first.
    UPROPERTY(BlueprintReadOnly, Category = "Proxy")
    TObjectPtr<class UStaticMesh> DormantMesh = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Proxy")
    TSubclassOf<AActor> ActorClass = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Proxy")
    EMythicProxyState State = EMythicProxyState::Dormant;

    // Index of this record's instance in its shared ISM, or INDEX_NONE when it has no instance (because it is currently
    // promoted, or has no mesh).
    UPROPERTY(BlueprintReadOnly, Category = "Proxy")
    int32 InstanceIndex = INDEX_NONE;

    bool IsPromoted() const {
        return State == EMythicProxyState::Promoted;
    }
};

struct FMythicPlacedProxyRules {
    static bool ShouldBePromoted(float NearestDistSq, float PromoteRadius, float DemoteRadius, bool bCurrentlyPromoted) {
        if (NearestDistSq < 0.0f) {
            return false;
        }

        const float PromoteSq = FMath::Square(FMath::Max(0.0f, PromoteRadius));
        const float EffectiveDemote = FMath::Max(DemoteRadius, PromoteRadius);
        const float DemoteSq = FMath::Square(FMath::Max(0.0f, EffectiveDemote));

        if (bCurrentlyPromoted) {
            return NearestDistSq <= DemoteSq;
        }
        return NearestDistSq <= PromoteSq;
    }

    static bool ShouldHaveInstance(bool bPromoted, bool bHasMesh) {
        return bHasMesh && !bPromoted;
    }
};
