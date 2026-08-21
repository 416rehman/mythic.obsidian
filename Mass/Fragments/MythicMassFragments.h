
#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicMassFragments.generated.h"


USTRUCT()
struct MYTHIC_API FMythicIdentityFragment : public FMassFragment {
    GENERATED_BODY()

    FMythicFactionId Faction;

    FMythicFactionId TrueFaction;

    FMythicCellCoord Cell;

    FGameplayTag RoleTag;

    uint32 NameHash = 0;

    uint8 VisualArchetype = 0;

    uint8 DemographicFlags = 0;

    uint8 VisibilityGroup = 0;

    uint8 ActionCategory = 0;

    FVector SpawnOverridePos = FVector::ZeroVector;

    bool bHasSpawnOverride = false;

    bool IsSpy() const { return Faction.Index != TrueFaction.Index && TrueFaction.IsValid(); }
};


USTRUCT()
struct MYTHIC_API FMythicGroupFragment : public FMassFragment {
    GENERATED_BODY()

    uint32 GroupId = 0;

    FGameplayTag ActivityTag;

    uint8 bIsLeader : 1;

    FMythicGroupFragment() : bIsLeader(0) {}
};


UENUM()
enum class EMythicSchedulePhase : uint8 {
    Work,
    Rest,
    Social,
    Travel,
    Idle
};

USTRUCT()
struct MYTHIC_API FMythicScheduleFragment : public FMassFragment {
    GENERATED_BODY()

    EMythicSchedulePhase Phase = EMythicSchedulePhase::Idle;

    FMythicCellCoord WorkCell;
    FMythicCellCoord HomeCell;
};


USTRUCT()
struct MYTHIC_API FMythicSignificanceFragment : public FMassFragment {
    GENERATED_BODY()

    float Score = 0.0f;

    EMythicSignificanceTier Tier = EMythicSignificanceTier::Tier0_Ambient;

    uint16 RelevantEventCount = 0;

    uint8 bDirty : 1;

    FMythicSignificanceFragment() : bDirty(true) {}
};


USTRUCT()
struct MYTHIC_API FMythicPsychodynamicFragment : public FMassFragment {
    GENERATED_BODY()

    float Pressure[PressureChannelCount] = {};

    double LastEventTime = 0.0;

    int32 FightTargetEntity = INDEX_NONE;

    bool bDespaired = false;
};


USTRUCT()
struct MYTHIC_API FMythicPersonalityFragment : public FMassFragment {
    GENERATED_BODY()

    float VentWeights[static_cast<int32>(EMythicVentChannel::COUNT)] = {};
};

static constexpr int32 VentChannelCount = static_cast<int32>(EMythicVentChannel::COUNT);


USTRUCT()
struct MYTHIC_API FMythicCreatureFragment : public FMassFragment {
    GENERATED_BODY()

    uint8 SpeciesId = 0;

    uint16 PackId = 0;

    float BaseAggression = 0.0f;

    float CurrentAggression = 0.0f;

    FMythicCellCoord DenCell;

    uint8 TerritorialRadius = 2;
};


static constexpr int32 MaxSocialEdges = 16;

USTRUCT()
struct MYTHIC_API FMythicSocialFragment : public FMassFragment {
    GENERATED_BODY()

    uint16 EdgeEntityIndices[MaxSocialEdges] = {};

    int8 EdgeQuality[MaxSocialEdges] = {};

    uint8 EdgeCount = 0;

    bool bHasMetPlayer = false;

    float PlayerLoyalty[8] = {};

    static constexpr uint16 InvalidEdgeIndex = 0xFFFF;

    FMythicSocialFragment() {
        FMemory::Memset(EdgeEntityIndices, 0xFF, sizeof(EdgeEntityIndices));
        FMemory::Memzero(PlayerLoyalty, sizeof(PlayerLoyalty));
    }
};
