// Mythic Living World — Code-default encounter templates.
//
// Provides a built-in encounter template set so the EncounterDirector produces encounters WITHOUT any authored
// EncounterTemplateDatabase asset. Before this, Settings->EncounterTemplateDatabase was null in every shipped config,
// so the director registered zero templates and the world had no spawned "stuff to do". This closes that gap with a
// minimal, all-prerequisites-zeroed set that spawns on any map/faction state.
//
// HONEST SCOPE — code default vs authored content:
//  - These four templates embody their members as the GENERIC EmbodiedNPCClass (the humanoid significance/embodiment
//    path), NOT bandit/wolf-specific meshes. A "Wildlife" default is a wolf in NAME (chronicle beat) only; its body is
//    the same generic NPC actor. KIND-per-encounter VISUALS (real wolf/bandit meshes per template) is a downstream
//    content slice and is intentionally NOT done here.
//  - Probabilities/counts/cooldowns are hand-tuned starting points, not balanced content.
//  - An authored UMythicEncounterTemplateDatabase ALWAYS wins: the director loads the asset first and only falls back
//    to these defaults when the asset is absent or yields zero templates (see EncounterDirector::Initialize).

#include "World/LivingWorld/Encounters/EncounterTemplate.h"
#include "World/LivingWorld/Encounters/MythicEncounterObjectiveDefaults.h" // code-default clear objective (companion)
#include "World/LivingWorld/MythicTags_LivingWorld.h" // TAG_LIVINGWORLD_ENCOUNTER_* (template KIND tags)
#include "World/LivingWorld/Factions/FactionDatabase.h" // EMythicFactionRelation
#include "Objectives/ObjectiveDefinition.h"             // UObjectiveDefinition + GAS_EVENT_KILL default trigger
#include "GAS/MythicTags_GAS.h"                          // GAS_EVENT_KILL (explicit — ObjectiveDefinition defaults to it)

namespace MythicEncounterDefaults {

void BuildDefaultTemplates(TArray<FMythicEncounterTemplate>& Out) {
    // Reserve exactly the count we append (4) so the caller's array doesn't reallocate mid-fill.
    Out.Reserve(Out.Num() + 4);

    // ── Patrol ───────────────────────────────────────────────
    // A faction patrol sweeping its own territory. Most common, lowest stakes. Neutral relation gate (no war needed).
    {
        FMythicEncounterTemplate T;
        T.EncounterTag = TAG_LIVINGWORLD_ENCOUNTER_PATROL;
        T.DisplayName = FText::FromString(TEXT("Faction Patrol"));
        T.MinFactionRelation = EMythicFactionRelation::Neutral; // backward-safe: imposes no relationship constraint
        T.MinMilitaryStrength = 0.0f;
        T.MinPopulation = 0;
        T.CooldownSeconds = 180.0f;
        T.MaxConcurrentInstances = 2;
        T.BaseProbability = 0.25f;
        T.EntityCount = 3;
        T.MaxDurationSeconds = 600.0f;
        Out.Add(MoveTemp(T));
    }

    // ── Bandit Ambush ────────────────────────────────────────
    // An ambush sprung on a controlled cell. Slightly rarer than a patrol, larger group.
    {
        FMythicEncounterTemplate T;
        T.EncounterTag = TAG_LIVINGWORLD_ENCOUNTER_BANDIT_AMBUSH;
        T.DisplayName = FText::FromString(TEXT("Bandit Ambush"));
        T.MinFactionRelation = EMythicFactionRelation::Neutral;
        T.MinMilitaryStrength = 0.0f;
        T.MinPopulation = 0;
        T.CooldownSeconds = 240.0f;
        T.MaxConcurrentInstances = 2;
        T.BaseProbability = 0.20f;
        T.EntityCount = 4;
        T.MaxDurationSeconds = 600.0f;
        Out.Add(MoveTemp(T));
    }

    // ── Wildlife ─────────────────────────────────────────────
    // A wilderness creature encounter. Highest base probability so the wilds feel alive. NOTE: a CODE-DEFAULT wildlife
    // encounter still spawns the GENERIC humanoid NPC body (the director's archetype is faction/NPC-based) — it is
    // "wildlife" in the chronicle beat only until a per-template creature-mesh slice lands.
    {
        FMythicEncounterTemplate T;
        T.EncounterTag = TAG_LIVINGWORLD_ENCOUNTER_WILDLIFE;
        T.DisplayName = FText::FromString(TEXT("Wildlife"));
        T.MinFactionRelation = EMythicFactionRelation::Neutral;
        T.MinMilitaryStrength = 0.0f;
        T.MinPopulation = 0;
        T.CooldownSeconds = 150.0f;
        T.MaxConcurrentInstances = 2;
        T.BaseProbability = 0.30f;
        T.EntityCount = 3;
        T.MaxDurationSeconds = 600.0f;
        Out.Add(MoveTemp(T));
    }

    // ── Raid ─────────────────────────────────────────────────
    // A hostile raid — the only conflict-gated default. MinFactionRelation=Hostile means it spawns ONLY where the
    // originating faction actually holds a Hostile-or-worse relationship with some other alive faction (a real war),
    // per EncounterDirector::EvaluateTemplate's relationship gate. Rarest, biggest group, longest cooldown.
    {
        FMythicEncounterTemplate T;
        T.EncounterTag = TAG_LIVINGWORLD_ENCOUNTER_RAID;
        T.DisplayName = FText::FromString(TEXT("Faction Raid"));
        T.MinFactionRelation = EMythicFactionRelation::Hostile; // conflict-gated: needs a hostile faction pair to exist
        T.MinMilitaryStrength = 0.0f;
        T.MinPopulation = 0;
        T.CooldownSeconds = 360.0f;
        T.MaxConcurrentInstances = 2;
        T.BaseProbability = 0.15f;
        T.EntityCount = 6;
        T.MaxDurationSeconds = 600.0f;
        Out.Add(MoveTemp(T));
    }
}

} // namespace MythicEncounterDefaults

namespace MythicEncounterObjectiveDefaults {

UObjectiveDefinition *BuildDefaultEncounterClearObjective(UObject *Outer, int32 RequiredKills) {
    if (!Outer) {
        return nullptr;
    }

    // Transient (RF_Transient): a runtime-built default, never serialized — the EncounterDirector owns it for the
    // session and the ObjectiveTracker keys dedup on the pointer, so it just needs a stable address, not an asset path.
    UObjectiveDefinition *Def = NewObject<UObjectiveDefinition>(Outer, NAME_None, RF_Transient);
    if (!Def) {
        return nullptr;
    }

    // GAS.Event.Kill is UObjectiveDefinition's own default, but stamp it explicitly so the intent is local & obvious
    // (and survives any future change to that field's default). It is the proven server-emitted kill event the
    // ObjectiveTracker already subscribes to (UMythicLifeComponent::TriggerGameplayEvent_Kill).
    Def->TriggerEventTag = GAS_EVENT_KILL;
    Def->RequiredCount = FMath::Max(1, RequiredKills);
    Def->bCountByEventMagnitude = false; // kills advance by occurrence (+1), not by the damage carried as magnitude
    Def->DisplayText = FText::FromString(TEXT("Clear the encounter"));
    Def->CompletedText = FText::FromString(TEXT("Encounter cleared!"));
    // Rewards intentionally EMPTY — see header. The loop offers/tracks/completes/calls-out without a reward asset; a
    // designer assigns UMythicLivingWorldSettings::EncounterClearObjective (with a real reward) to upgrade this.

    return Def;
}

} // namespace MythicEncounterObjectiveDefaults
