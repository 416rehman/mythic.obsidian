
#include "World/LivingWorld/Activities/ActivityTypes.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/NPCGeneration/NPCGenerator.h"
#include "Mass/Fragments/MythicMassFragments.h"

namespace {
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
}

void MythicActivityDefaults::BuildDefaultActivities(TArray<FMythicActivityDef> &Out) {
    Out.Reset();

    {
        FMythicActivityDef Fish;
        Fish.ActivityTag = TAG_NPC_ACTIVITY_FISH;
        Fish.EligibleRoles.AddTag(TAG_NPC_ROLE_FISHER);
        Fish.bRequiresWaterAdjacent = 1;
        Fish.TimeWindow = EMythicActivityTimeWindow::Day;
        Fish.RequiredPhase = EMythicActivitySchedulePhase::Work;
        Fish.TargetKind = EMythicActivityTargetKind::CurrentCell;
        Fish.RelativeWeight = 2.0f;
        Fish.DisplayName = NSLOCTEXT("MythicActivity", "Fish", "Fishing");
        Out.Add(Fish);
    }

    {
        FMythicActivityDef Barter;
        Barter.ActivityTag = TAG_NPC_ACTIVITY_BARTER;
        Barter.bRequiresNearbyMerchant = 1;
        Barter.TimeWindow = EMythicActivityTimeWindow::Day;
        Barter.TargetKind = EMythicActivityTargetKind::NearbyMerchant;
        Barter.RelativeWeight = 1.5f;
        Barter.DisplayName = NSLOCTEXT("MythicActivity", "Barter", "Bartering");
        Out.Add(Barter);
    }

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

    {
        FMythicActivityDef Rest;
        Rest.ActivityTag = TAG_NPC_ACTIVITY_REST;
        Rest.RequiredPhase = EMythicActivitySchedulePhase::Rest;
        Rest.TargetKind = EMythicActivityTargetKind::HomeCell;
        Rest.RelativeWeight = 1.0f;
        Rest.DisplayName = NSLOCTEXT("MythicActivity", "Rest", "Resting");
        Out.Add(Rest);
    }

    {
        FMythicActivityDef Social;
        Social.ActivityTag = TAG_NPC_ACTIVITY_SOCIALIZE;
        Social.RequiredPhase = EMythicActivitySchedulePhase::Social;
        Social.TargetKind = EMythicActivityTargetKind::SettlementCenter;
        Social.RelativeWeight = 1.5f;
        Social.DisplayName = NSLOCTEXT("MythicActivity", "Socialize", "Socializing");
        Out.Add(Social);
    }

    {
        FMythicActivityDef Patrol;
        Patrol.ActivityTag = TAG_NPC_ACTIVITY_PATROL;
        Patrol.EligibleRoles.AddTag(TAG_NPC_ROLE_GUARD);
        Patrol.EligibleRoles.AddTag(TAG_NPC_ROLE_SOLDIER);
        Patrol.TargetKind = EMythicActivityTargetKind::HomeCell;
        Patrol.RelativeWeight = 2.0f;
        Patrol.DisplayName = NSLOCTEXT("MythicActivity", "Patrol", "Patrolling");
        Out.Add(Patrol);
    }

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
    if (Def.RelativeWeight <= 0.0f) {
        return false;
    }

    if (!Def.EligibleRoles.IsEmpty()) {
        if (!Ctx.Role.IsValid() || !Ctx.Role.MatchesAny(Def.EligibleRoles)) {
            return false;
        }
    }

    if (Def.EligibleBiomes.Num() > 0 && !Def.EligibleBiomes.Contains(Ctx.Biome)) {
        return false;
    }

    if (Def.bRequiresWaterAdjacent && Ctx.Biome != EMythicBiome::Wetland) {
        return false;
    }

    if (Def.TimeWindow == EMythicActivityTimeWindow::Day && !Ctx.bIsDay) {
        return false;
    }
    if (Def.TimeWindow == EMythicActivityTimeWindow::Night && Ctx.bIsDay) {
        return false;
    }

    if (!SchedulePhaseSatisfies(Def.RequiredPhase, Ctx.Phase)) {
        return false;
    }

    if (Def.bRequiresNearbyMerchant && !Ctx.bHasNearbyMerchant) {
        return false;
    }

    return true;
}

int32 MythicActivityDefaults::PickActivityIndex(TConstArrayView<FMythicActivityDef> Activities,
                                                const FMythicActivityContext &Ctx) {
    float Total = 0.0f;
    for (const FMythicActivityDef &A : Activities) {
        if (ActivityEligible(A, Ctx)) {
            Total += FMath::Max(0.0f, A.RelativeWeight);
        }
    }
    if (Total <= UE_KINDA_SMALL_NUMBER) {
        return INDEX_NONE;
    }

    const uint32 Mixed = FMythicNPCGenerator::HashStep(Ctx.NameHash ^ 0x00AC1717u);
    const float Roll = (static_cast<float>(Mixed % 100003u) / 100003.0f) * Total;

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

    return LastEligible;
}
