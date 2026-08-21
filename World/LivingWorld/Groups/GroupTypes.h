
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "GroupTypes.generated.h"

static constexpr int32 MaxGroupMembers = 6;


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicGroupMemberSpec {
    GENERATED_BODY()

    /** Role stamped on this member's FMythicIdentityFragment.RoleTag (same role family the archetype catalog uses). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group", meta = (Categories = "NPC.Role"))
    FGameplayTag RoleTag;

    /** Minimum members of this role (inclusive). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group", meta = (ClampMin = "1"))
    int32 MinCount = 1;

    /** Maximum members of this role (inclusive). The rolled count is clamped to the template's remaining headroom. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group", meta = (ClampMin = "1"))
    int32 MaxCount = 1;

    /** If true, this spec is the group's leader (social edges orient toward it; it is spawned first). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group")
    bool bIsLeader = false;
};


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicGroupTemplate {
    GENERATED_BODY()

    /** Identifies this group kind (e.g. "NPC.Group.Retinue"). Also stamped on each member's FMythicGroupFragment so the
     *  debugger can tally a group's activity/kind. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group", meta = (Categories = "NPC.Group"))
    FGameplayTag GroupTag;

    /** Human-readable label (debug/UI only). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group")
    FText DisplayName;

    /** The member slots that make up this group (one should be flagged bIsLeader). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group")
    TArray<FMythicGroupMemberSpec> Members;

    /** The social relation written between members at spawn (guard→noble = Subordinate, merchant↔porter = Associate,
     *  friends = Friend). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group")
    EMythicSocialRelation IntraRelation = EMythicSocialRelation::Friend;

    /** Initial strength of each intra-group social edge [0,1]. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float IntraEdgeStrength = 0.6f;

    /** Minimum governing-faction MilitaryStrength [0,1] to field this group (e.g. a retinue needs a strong faction). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group|Requirements", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinFactionMilitaryStrength = 0.0f;

    /** Minimum governing-faction Population to field this group. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group|Requirements", meta = (ClampMin = "0"))
    int32 MinFactionPopulation = 0;

    /** Minimum governing-faction Reserves.Wealth to field this group (a noble retinue needs a wealthy faction). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group|Requirements")
    float MinReserveWealth = 0.0f;

    /** Economies this group may appear in (resolved settlement economy). Empty = any economy. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group|Requirements")
    TArray<EMythicSettlementEconomy> AllowedEconomies;

    /** Relative weight among eligible templates in the weighted pick. 0 = effectively disabled. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Group", meta = (ClampMin = "0.0"))
    float RelativeWeight = 1.0f;
};


UCLASS(BlueprintType)
class MYTHIC_API UMythicGroupTemplateDatabase : public UDataAsset {
    GENERATED_BODY()

public:
    /** All group templates in this database. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Groups")
    TArray<FMythicGroupTemplate> Templates;

    const FMythicGroupTemplate *FindByTag(const FGameplayTag &GroupTag) const {
        for (const FMythicGroupTemplate &T : Templates) {
            if (T.GroupTag.MatchesTagExact(GroupTag)) {
                return &T;
            }
        }
        return nullptr;
    }
};


namespace MythicGroupDefaults {
    MYTHIC_API void BuildDefaultTemplates(TArray<FMythicGroupTemplate> &Out);
}
