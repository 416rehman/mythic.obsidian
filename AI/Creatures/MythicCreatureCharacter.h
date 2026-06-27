// Mythic Living World — Embodied Creature Actor
// The visible body of a wilderness creature (deer/wolf/boar/...) promoted out of the MASS creature layer.

#pragma once

#include "CoreMinimal.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "MythicCreatureCharacter.generated.h"

struct FMassEntityHandle;

/**
 * Embodied actor for a wilderness CREATURE (non-humanoid wildlife) entity.
 *
 * Deliberately a SUBCLASS of AMythicNPCCharacter so it reuses, with ZERO change, the entire embodiment plumbing the
 * humanoid path already has: the LivingWorldSubsystem's TMap<FMassEntityHandle, TWeakObjectPtr<AMythicNPCCharacter>>
 * EmbodiedActors map + Register/Find/Unregister, the ActorSpawnProcessor dehydration path (FindEmbodiedActor->Destroy),
 * AI possession (AMythicNPCAIController via the base ctor), and the GAS combat/death wiring. A creature IS-A
 * AMythicNPCCharacter, so it upcasts cleanly into all of that — no subsystem signature touches the creature type.
 *
 * What differs is INITIALIZATION: a creature carries no FMythicPersonalityFragment / FMythicScheduleFragment (it is
 * not part of the humanoid social/schedule simulation), so InitializeFromMassEntity must NOT build a humanoid cognitive
 * brain off those fragments. Instead it reads the FMythicCreatureFragment (species / pack / aggression / den) +
 * FMythicIdentityFragment and stamps lightweight per-species runtime state. Visual mesh selection is left to the
 * mesh-bearing Blueprint subclass a designer assigns to UMythicLivingWorldSettings::EmbodiedCreatureClass (the
 * SpeciesId is exposed via a BlueprintImplementableEvent so the BP can swap mesh/anim per species with no C++ asset
 * coupling); the bare C++ class is a functional, AI-possessed, invisible-but-real creature when no BP is authored.
 */
UCLASS(Blueprintable)
class MYTHIC_API AMythicCreatureCharacter : public AMythicNPCCharacter {
    GENERATED_BODY()

public:
    AMythicCreatureCharacter();

    /**
     * Bind this actor to its source CREATURE entity. Reads FMythicCreatureFragment (SpeciesId / PackId /
     * CurrentAggression / DenCell / TerritorialRadius) + FMythicIdentityFragment and caches them on the actor, then
     * fires OnCreatureInitialized so a Blueprint can apply the per-species mesh/anim. Overrides the humanoid version so
     * NO cognitive brain is initialized for a creature (creatures have no personality/schedule fragments — the base
     * impl would early-out on the null personality anyway, but the override makes the intent explicit and avoids a
     * stray fragment read on the creature archetype).
     */
    virtual void InitializeFromMassEntity(const FMassEntityHandle &InEntityHandle) override;

    /** Runtime species id this creature was embodied as (0 until InitializeFromMassEntity runs). */
    UFUNCTION(BlueprintPure, Category = "Mythic Creature")
    uint8 GetSpeciesId() const { return SpeciesId; }

    /** Pack/herd id this creature belongs to (0 = solitary). */
    UFUNCTION(BlueprintPure, Category = "Mythic Creature")
    int32 GetPackId() const { return PackId; }

    /** Effective aggression snapshotted at embodiment time [0,1] (the live value is owned by the MASS ecology layer). */
    UFUNCTION(BlueprintPure, Category = "Mythic Creature")
    float GetCurrentAggression() const { return CurrentAggression; }

protected:
    /**
     * Fired (server) once the creature has been bound to its entity, carrying its resolved SpeciesId. The mesh-bearing
     * creature Blueprint binds this to swap the skeletal mesh / anim blueprint per species — keeping species→asset
     * mapping in authored data, not in C++. Editor handoff, mirrors AMythicNPCCharacter's other BP-implementable events.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic Creature")
    void OnCreatureInitialized(uint8 InSpeciesId, int32 InPackId);

    /** Resolved per-species runtime data, cached at embodiment (see InitializeFromMassEntity). */
    uint8 SpeciesId = 0;
    int32 PackId = 0;
    float CurrentAggression = 0.0f;
};
