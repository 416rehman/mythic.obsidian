// Mythic Living World — Code-default encounter-clear OBJECTIVE.
//
// Companion to MythicEncounterDefaults (encounter TEMPLATE defaults): builds a working UObjectiveDefinition in code so
// the "clear the encounter" quest loop is FUNCTIONAL out of the box when no UMythicLivingWorldSettings::EncounterClear-
// Objective asset is authored. Before this, GetEncounterClearObjective() returned null in every shipped config (the
// soft-ptr defaults null), so even the data hook produced nothing and there was no encounter-driven "stuff to do".
//
// HONEST SCOPE — code default vs authored content:
//  - The code-default objective triggers on GAS.Event.Kill (the proven server-emitted kill event the ObjectiveTracker
//    already subscribes to) and completes after RequiredCount kills, with a player-facing display/completed line.
//  - Its Rewards holder is LEFT EMPTY: a real reward needs an authored asset (UXPReward needs a UProficiencyDefinition,
//    UItemReward needs a UItemDefinition) which cannot be fabricated in code. So the loop OFFERS, TRACKS, COMPLETES, and
//    fires the completion callout — but grants nothing until a reward asset is assigned. This is the same honest gap the
//    EncounterClearObjective soft-ptr documents; an authored UObjectiveDefinition assigned to that setting ALWAYS wins.
//  - The objective does NOT distinguish WHICH encounter's members were killed (no per-encounter kill attribution exists
//    on the GAS.Event.Kill payload here) — it counts any kills while the objective is active. Per-encounter target
//    attribution is a downstream slice.

#pragma once

#include "CoreMinimal.h"

class UObjectiveDefinition;

namespace MythicEncounterObjectiveDefaults {
/**
 * Construct a transient code-default "clear the encounter" UObjectiveDefinition, owned by `Outer` (so it shares the
 * outer's lifetime and the returned pointer stays stable for the ObjectiveTracker's pointer-keyed dedup). RequiredCount
 * is clamped to >= 1. Returns null only if Outer is null. Pure aside from the NewObject allocation → callers should
 * cache the result (the EncounterDirector builds it ONCE and reuses the pointer across all offers).
 *
 * @param Outer          UObject to own the transient definition (e.g. the EncounterDirector).
 * @param RequiredKills  Kills needed to complete (e.g. an encounter's EntityCount). Clamped to >= 1.
 */
MYTHIC_API UObjectiveDefinition *BuildDefaultEncounterClearObjective(class UObject *Outer, int32 RequiredKills);
} // namespace MythicEncounterObjectiveDefaults
