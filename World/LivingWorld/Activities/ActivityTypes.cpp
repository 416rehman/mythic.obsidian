// Mythic Living World — Context-driven Activity Catalog implementation
// Code-default activity set + the pure eligibility/selection helpers. NO engine/world/UObject access in the helpers
// (FMythicActivityContext carries everything they need), so they are deterministic + unit-testable.

#include "World/LivingWorld/Activities/ActivityTypes.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"               // TAG_NPC_ACTIVITY_* + TAG_NPC_ROLE_*
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"           // FMythicNPCGenerator::HashStep (deterministic tie-break)
#include "Mass/Fragments/MythicMassFragments.h"                     // EMythicSchedulePhase (phase gate mapping)

namespace {
    // Map the coarse activity-phase gate onto the entity's actual schedule phase. Work-gated activities run during the
    // Work part of the day; Rest-gated during Rest; Social-gated during Social. Travel/Idle never satisfy a phase gate
    // (an NPC mid-commute / with no schedule shouldn't lock into a phase-specific activity) — Any-phase activities
    // (e.g. Wander) still surface for them via the Any short-circuit in ActivityEligible.
    bool SchedulePhaseSatisfies(EMythicActivitySchedulePhase Required, EMythicSchedulePhase Actual) {
        switch (Required) {
        case EMythicActivitySchedulePhase::Any:
            return true;
        case EMythicActivitySchedulePhase::Work:
            return Actual == EMythicSchedulePhase::Work;
        case EMythicActivitySchedulePhase::Rest:
            return Actual == EMythicSchedulePhase::Rest;
        case EMythicActivitySchedulePhase::Social:
            return Actual == EMythicSchedulePhase::Social;
        default:
            return false;
        }
    }
} // namespace

void MythicActivityDefaults::BuildDefaultActivities(TArray<FMythicActivityDef> &Out) {
    Out.Reset();

    // Fish — a fisher by the water, in daylight. Stands at the water's edge (current cell) and casts.
    {
        FMythicActivityDef Fish;
        Fish.ActivityTag = TAG_NPC_ACTIVITY_FISH;
        Fish.EligibleRoles.AddTag(TAG_NPC_ROLE_FISHER);
        Fish.bRequiresWaterAdjacent = 1; // v1 stub: Biome == Wetland
        Fish.TimeWindow = EMythicActivityTimeWindow::Day;
        Fish.RequiredPhase = EMythicActivitySchedulePhase::Work;
        Fish.TargetKind = EMythicActivityTargetKind::CurrentCell;
        Fish.RelativeWeight = 2.0f;
        Fish.DisplayName = NSLOCTEXT("MythicActivity", "Fish", "Fishing");
        Out.Add(Fish);
    }

    // Barter — any townsperson drifting to a nearby merchant to trade. Gated on a merchant being in range.
    {
        FMythicActivityDef Barter;
        Barter.ActivityTag = TAG_NPC_ACTIVITY_BARTER;
        // Roles empty => any role; commoners + travelers all browse a stall.
        Barter.bRequiresNearbyMerchant = 1;
        Barter.TimeWindow = EMythicActivityTimeWindow::Day;
        Barter.TargetKind = EMythicActivityTargetKind::NearbyMerchant;
        Barter.RelativeWeight = 1.5f;
        Barter.DisplayName = NSLOCTEXT("MythicActivity", "Barter", "Bartering");
        Out.Add(Barter);
    }

    // Work — a laborer/farmer heading to its work cell during the working part of the day.
    {
        FMythicActivityDef Work;
        Work.ActivityTag = TAG_NPC_ACTIVITY_WORK;
        Work.EligibleRoles.AddTag(TAG_NPC_ROLE_FARMER);
        Work.EligibleRoles.AddTag(TAG_NPC_ROLE_LABORER);
        Work.RequiredPhase = EMythicActivitySchedulePhase::Work;
        Work.TimeWindow = EMythicActivityTimeWindow::Day;
        Work.TargetKind = EMythicActivityTargetKind::WorkCell;
        Work.RelativeWeight = 1.5f;
        Work.DisplayName = NSLOCTEXT("MythicActivity", "Work", "Working");
        Out.Add(Work);
    }

    // Rest — anyone winding down at home during the Rest phase (incl. night).
    {
        FMythicActivityDef Rest;
        Rest.ActivityTag = TAG_NPC_ACTIVITY_REST;
        Rest.RequiredPhase = EMythicActivitySchedulePhase::Rest;
        Rest.TargetKind = EMythicActivityTargetKind::HomeCell;
        Rest.RelativeWeight = 1.0f;
        Rest.DisplayName = NSLOCTEXT("MythicActivity", "Rest", "Resting");
        Out.Add(Rest);
    }

    // Socialize — gathering at the settlement square during the Social phase.
    {
        FMythicActivityDef Social;
        Social.ActivityTag = TAG_NPC_ACTIVITY_SOCIALIZE;
        Social.RequiredPhase = EMythicActivitySchedulePhase::Social;
        Social.TargetKind = EMythicActivityTargetKind::SettlementCenter;
        Social.RelativeWeight = 1.5f;
        Social.DisplayName = NSLOCTEXT("MythicActivity", "Socialize", "Socializing");
        Out.Add(Social);
    }

    // Patrol — guards/soldiers walking their post (any time of day).
    {
        FMythicActivityDef Patrol;
        Patrol.ActivityTag = TAG_NPC_ACTIVITY_PATROL;
        Patrol.EligibleRoles.AddTag(TAG_NPC_ROLE_GUARD);
        Patrol.EligibleRoles.AddTag(TAG_NPC_ROLE_SOLDIER);
        Patrol.TargetKind = EMythicActivityTargetKind::HomeCell; // anchor; the controller walks its patrol ring around it
        Patrol.RelativeWeight = 2.0f;
        Patrol.DisplayName = NSLOCTEXT("MythicActivity", "Patrol", "Patrolling");
        Out.Add(Patrol);
    }

    // Wander — the ALWAYS-eligible fallback so PickActivityIndex never starves: any role, any biome, any time, any phase,
    // a short scatter around the current cell. Low weight so a specific activity wins whenever one is eligible.
    {
        FMythicActivityDef Wander;
        Wander.ActivityTag = TAG_NPC_ACTIVITY_WANDER;
        Wander.TargetKind = EMythicActivityTargetKind::BiomeWander;
        Wander.RelativeWeight = 0.5f;
        Wander.DisplayName = NSLOCTEXT("MythicActivity", "Wander", "Wandering");
        Out.Add(Wander);
    }
}

bool MythicActivityDefaults::ActivityEligible(const FMythicActivityDef &Def, const FMythicActivityContext &Ctx) {
    // Disabled-weight activities are never eligible (mirrors GroupSpawnerProcessor::TemplateEligible's weight guard).
    if (Def.RelativeWeight <= 0.0f) {
        return false;
    }

    // Role gate: empty => any role; otherwise the NPC's role must match (exact-or-child) one of the eligible roles.
    if (!Def.EligibleRoles.IsEmpty()) {
        if (!Ctx.Role.IsValid() || !Ctx.Role.MatchesAny(Def.EligibleRoles)) {
            return false;
        }
    }

    // Biome gate: empty => any biome; otherwise the NPC's current biome must be one of the eligible biomes.
    if (Def.EligibleBiomes.Num() > 0 && !Def.EligibleBiomes.Contains(Ctx.Biome)) {
        return false;
    }

    // Water-adjacency gate (v1 honest stub: Wetland is the water-adjacent biome).
    if (Def.bRequiresWaterAdjacent && Ctx.Biome != EMythicBiome::Wetland) {
        return false;
    }

    // Time-of-day gate.
    if (Def.TimeWindow == EMythicActivityTimeWindow::Day && !Ctx.bIsDay) {
        return false;
    }
    if (Def.TimeWindow == EMythicActivityTimeWindow::Night && Ctx.bIsDay) {
        return false;
    }

    // Committed-schedule-phase gate.
    if (!SchedulePhaseSatisfies(Def.RequiredPhase, Ctx.Phase)) {
        return false;
    }

    // Nearby-merchant requirement (the controller's bounded scan must have found one).
    if (Def.bRequiresNearbyMerchant && !Ctx.bHasNearbyMerchant) {
        return false;
    }

    return true;
}

int32 MythicActivityDefaults::PickActivityIndex(TConstArrayView<FMythicActivityDef> Activities,
                                                const FMythicActivityContext &Ctx) {
    // Single pass to total eligible weight; second cumulative walk to pick — allocation-free, identical predicate both
    // passes so the winner-selection is consistent with the total (mirrors GroupSpawnerProcessor::PickTemplateIndex).
    float Total = 0.0f;
    for (const FMythicActivityDef &A : Activities) {
        if (ActivityEligible(A, Ctx)) {
            Total += FMath::Max(0.0f, A.RelativeWeight);
        }
    }
    if (Total <= UE_KINDA_SMALL_NUMBER) {
        return INDEX_NONE;
    }

    // Deterministic roll in [0,1) from the stable NameHash, mixed through HashStep so two NPCs with adjacent hashes don't
    // collapse to the same fraction. Large-prime modulus matches the ComputeStaggeredHour spread idiom.
    const uint32 Mixed = FMythicNPCGenerator::HashStep(Ctx.NameHash ^ 0x00AC1717u); // salt distinct from other systems
    const float Roll = (static_cast<float>(Mixed % 100003u) / 100003.0f) * Total; // [0, Total)

    float Cumulative = 0.0f;
    int32 LastEligible = INDEX_NONE;
    for (int32 i = 0; i < Activities.Num(); ++i) {
        const FMythicActivityDef &A = Activities[i];
        if (!ActivityEligible(A, Ctx)) {
            continue;
        }
        const float W = FMath::Max(0.0f, A.RelativeWeight);
        if (W <= 0.0f) {
            continue;
        }
        LastEligible = i;
        Cumulative += W;
        if (Roll < Cumulative) {
            return i;
        }
    }

    // Floating-point boundary guard: Roll can equal Total — fall to the last eligible activity.
    return LastEligible;
}
