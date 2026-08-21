
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/Territory/MythicBiome.h"
#include "ActivityTypes.generated.h"

enum class EMythicSchedulePhase : uint8;


UENUM(BlueprintType)
enum class EMythicActivityTimeWindow : uint8 {
    Any = 0 UMETA(DisplayName = "Any"),
    Day UMETA(DisplayName = "Day Only"),
    Night UMETA(DisplayName = "Night Only")
};

UENUM(BlueprintType)
enum class EMythicActivitySchedulePhase : uint8 {
    Any = 0 UMETA(DisplayName = "Any"),
    Work UMETA(DisplayName = "Work"),
    Rest UMETA(DisplayName = "Rest"),
    Social UMETA(DisplayName = "Social")
};

UENUM(BlueprintType)
enum class EMythicActivityTargetKind : uint8 {
    HomeCell = 0 UMETA(DisplayName = "Home Cell"),
    WorkCell UMETA(DisplayName = "Work Cell"),
    CurrentCell UMETA(DisplayName = "Current Cell (in place)"),
    SettlementCenter UMETA(DisplayName = "Settlement Center"),
    NearbyMerchant UMETA(DisplayName = "Nearby Merchant"),
    BiomeWander UMETA(DisplayName = "Biome Wander")
};


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicActivityDef {
    GENERATED_BODY()

    /** Identifies this activity (e.g. "NPC.Activity.Fish"). Also handed to the cosmetic OnPerformActivity BP hook. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity", meta = (Categories = "NPC.Activity"))
    FGameplayTag ActivityTag;

    /** Roles this activity is appropriate for (exact-or-child match). Empty = any role. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity", meta = (Categories = "NPC.Role"))
    FGameplayTagContainer EligibleRoles;

    /** Biomes this activity is appropriate for. Empty = any biome. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    TArray<EMythicBiome> EligibleBiomes;

    /** Require the NPC's current biome to be water-adjacent (v1 honest stub: Biome == Wetland). For fishing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    uint8 bRequiresWaterAdjacent : 1;

    /** Day/night gate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    EMythicActivityTimeWindow TimeWindow = EMythicActivityTimeWindow::Any;

    /** Committed-schedule-phase gate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    EMythicActivitySchedulePhase RequiredPhase = EMythicActivitySchedulePhase::Any;

    /** Where this activity steers the NPC. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    EMythicActivityTargetKind TargetKind = EMythicActivityTargetKind::CurrentCell;

    /** Require a merchant NPC nearby (the controller's bounded ScanNearbyMerchant must have found one). For bartering. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    uint8 bRequiresNearbyMerchant : 1;

    /** Relative weight among eligible activities in the weighted pick. 0 = effectively disabled. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity", meta = (ClampMin = "0.0"))
    float RelativeWeight = 1.0f;

    /** Human-readable label (debug/UI only). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Activity")
    FText DisplayName;

    FMythicActivityDef() : bRequiresWaterAdjacent(0), bRequiresNearbyMerchant(0) {}
};


struct FMythicActivityContext {
    FGameplayTag Role;

    EMythicBiome Biome = EMythicBiome::Plains;

    bool bIsDay = true;

    EMythicSchedulePhase Phase{};

    bool bHasNearbyMerchant = false;

    uint32 NameHash = 0;
};

struct FMythicActivityResult {
    int32 ChosenIndex = INDEX_NONE;
};


UCLASS(BlueprintType)
class MYTHIC_API UMythicActivityCatalog : public UDataAsset {
    GENERATED_BODY()

public:
    /** All activity defs in this catalog. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Activities")
    TArray<FMythicActivityDef> Activities;

    const FMythicActivityDef *FindByTag(const FGameplayTag &ActivityTag) const {
        for (const FMythicActivityDef &A : Activities) {
            if (A.ActivityTag.MatchesTagExact(ActivityTag)) {
                return &A;
            }
        }
        return nullptr;
    }
};


namespace MythicActivityDefaults {
    MYTHIC_API void BuildDefaultActivities(TArray<FMythicActivityDef> &Out);

    MYTHIC_API bool ActivityEligible(const FMythicActivityDef &Def, const FMythicActivityContext &Ctx);

    MYTHIC_API int32 PickActivityIndex(TConstArrayView<FMythicActivityDef> Activities, const FMythicActivityContext &Ctx);
}
