
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MythicCemeteryRules.generated.h"

USTRUCT(BlueprintType)
struct FMythicCemeteryConfig {
    GENERATED_BODY()

    // Roles that ALWAYS warrant a grave regardless of combat significance — leaders/named notables (nobles, the named
    // shopkeepers a settlement remembers). Matched child-of-or-equal, so listing a parent (NPC.Role.Merchant) covers
    // every merchant sub-role. Left empty, notability falls back purely to the significance gate below.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cemetery|Notability", meta = (Categories = "NPC.Role"))
    FGameplayTagContainer NotableRoleTags;

    // Significance at/above which a death warrants a grave irrespective of role (slain champions/bosses). Significance
    // is sourced at the death site from the enemy combat tier (Normal=1..Boss=5). Default 3 → Champion-and-above.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cemetery|Notability", meta = (ClampMin = "0.0"))
    float NotabilityGate = 3.0f;

    // Centre-to-centre spacing (cm) between adjacent grave slots in a cemetery's grid.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cemetery|Layout", meta = (ClampMin = "1.0"))
    float GraveSpacing = 150.0f;

    // Graves per row before the grid wraps to the next row — keeps a growing graveyard a compact rectangle.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cemetery|Layout", meta = (ClampMin = "1"))
    int32 GravesPerRow = 5;

    // Hard cap on graves materialized per settlement cemetery (bounds save size + actor count over a long campaign).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cemetery|Layout", meta = (ClampMin = "1"))
    int32 MaxGravesPerCemetery = 64;

    // World-seconds per in-world "day", used only to render the {day} number stamped into an epitaph. Flavour, not sim.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cemetery|Epitaph", meta = (ClampMin = "1.0"))
    float SecondsPerWorldDay = 1200.0f;

    // Fixed offset (cm) from a settlement's origin at which its cemetery grid begins — places the graveyard beside the
    // settlement rather than on top of it. A death with no owning settlement buries at the death location instead.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cemetery|Layout")
    FVector CemeteryAnchorOffset = FVector(2000.0f, 2000.0f, 0.0f);
};

struct FMythicCemeteryRules {
    static bool IsNotableDeath(FGameplayTag RoleTag, float Significance, const FMythicCemeteryConfig &Config) {
        if (RoleTag.IsValid()) {
            for (const FGameplayTag &Notable : Config.NotableRoleTags) {
                if (Notable.IsValid() && RoleTag.MatchesTag(Notable)) {
                    return true;
                }
            }
        }
        return Significance >= Config.NotabilityGate;
    }

    static FVector ComputeGraveSlotOffset(int32 GraveIndex, float Spacing, int32 PerRow) {
        if (GraveIndex < 0) {
            GraveIndex = 0;
        }
        const int32 Row = (PerRow > 0) ? (GraveIndex / PerRow) : GraveIndex;
        const int32 Col = (PerRow > 0) ? (GraveIndex % PerRow) : 0;
        return FVector(static_cast<float>(Row) * Spacing, static_cast<float>(Col) * Spacing, 0.0f);
    }

    static int32 WorldDayForSeconds(double DeathTimeSeconds, float SecondsPerWorldDay) {
        if (SecondsPerWorldDay <= 0.0f) {
            return 1;
        }
        const double Days = DeathTimeSeconds / static_cast<double>(SecondsPerWorldDay);
        return 1 + FMath::Max(0, FMath::FloorToInt(static_cast<float>(Days)));
    }
};
