
#pragma once

#include "CoreMinimal.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "MythicCreatureCharacter.generated.h"

struct FMassEntityHandle;

UCLASS(Blueprintable)
class MYTHIC_API AMythicCreatureCharacter : public AMythicNPCCharacter {
    GENERATED_BODY()

public:
    AMythicCreatureCharacter();

    virtual void InitializeFromMassEntity(const FMassEntityHandle &InEntityHandle) override;

    virtual void OnReturnedToPool() override;

    /** Runtime species id this creature was embodied as (0 until InitializeFromMassEntity runs). */
    UFUNCTION(BlueprintPure, Category = "Mythic Creature")
    uint8 GetSpeciesId() const { return SpeciesId; }

    /** Pack/herd id this creature belongs to (0 = solitary). */
    UFUNCTION(BlueprintPure, Category = "Mythic Creature")
    int32 GetPackId() const { return PackId; }

    /** Effective aggression snapshotted at embodiment time [0,1] (the live value is owned by the MASS ecology layer). */
    UFUNCTION(BlueprintPure, Category = "Mythic Creature")
    float GetCurrentAggression() const { return CurrentAggression; }

    /** Canonical Creature.Species.* presentation/codex identity; invalid means the species data is not authored. */
    UFUNCTION(BlueprintPure, Category = "Mythic Creature")
    FGameplayTag GetSpeciesTag() const { return SpeciesTag; }

protected:
    virtual void BuildMassPublicIdentity(const FMythicIdentityFragment &Identity,
                                         FMythicPublicIdentitySnapshot &OutIdentity) const override;

    virtual void BuildDirectPublicIdentity(
        FMythicPublicIdentitySnapshot &OutIdentity) const override;

    /**
     * Fired (server) once the creature has been bound to its entity, carrying its resolved SpeciesId. The mesh-bearing
     * creature Blueprint binds this to swap the skeletal mesh / anim blueprint per species — keeping species→asset
     * mapping in authored data, not in C++. Editor handoff, mirrors AMythicNPCCharacter's other BP-implementable events.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic Creature")
    void OnCreatureInitialized(uint8 InSpeciesId, int32 InPackId);

    uint8 SpeciesId = 0;
    FGameplayTag SpeciesTag;
    FPrimaryAssetId PublicIdentityDefinitionId;
    int32 PackId = 0;
    float CurrentAggression = 0.0f;
};
