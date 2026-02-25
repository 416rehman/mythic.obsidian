// Mythic Living World — Specialized Role Types
// All roles are data configurations on existing systems — zero new code per role.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "RoleTypes.generated.h"

/**
 * Defines a specialized NPC role configuration.
 * Roles are data-driven overlays on the existing pressure/venting/dialogue pipeline.
 * The doc mandates: "All data configurations — zero new systems per role."
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicRoleDefinition {
    GENERATED_BODY()

    /** Unique role tag (e.g., "NPC.Role.Guard", "NPC.Role.Spy", "NPC.Role.Merchant") */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "NPC.Role"))
    FGameplayTag RoleTag;

    /** Human-readable display name for this role */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FText DisplayName;

    /**
     * Vent weight overrides for this role.
     * Applied as additive modifiers to faction personality template during NPC generation.
     * E.g., Guard: Enforce +0.5, Fight +0.3. Spy: Exploit +0.4, Report +0.3.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Personality")
    TMap<FName, float> VentWeightModifiers;

    /**
     * Moral surface modifiers — how this role shifts faction ideology tolerance.
     * E.g., Priest has lower Sacrilege tolerance. Smuggler has higher Theft tolerance.
     * Applied as additive modifiers to the faction's moral surface thresholds.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Morality")
    TMap<FName, float> MoralToleranceModifiers;

    /**
     * Schedule override — which phases this role is active during.
     * E.g., Merchant: active during Work only. Guard: active during Work + Night.
     * Empty = use default faction schedule.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Schedule")
    TArray<uint8> ActiveSchedulePhases;

    /**
     * Required faction tags for this role (empty = any faction can have this role).
     * Ensures only military factions have Guard Captains, only religious factions have Priests, etc.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Requirements")
    FGameplayTagContainer RequiredFactionTags;

    /**
     * Dialogue template tags this role uses.
     * The dialogue selector filters by these tags when selecting templates for this NPC.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue", meta = (Categories = "Dialogue"))
    FGameplayTagContainer DialogueTags;

    // ─── Spy-Specific ────────────────────────────────────

    /**
     * If true, this role is an undercover agent with dual faction identity.
     * The NPC's public Faction differs from TrueFaction. Belief routing goes to TrueFaction.
     * Discovery risk from NPCs with high Cunning/Paranoia personality traits.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spy")
    bool bIsDualIdentity = false;

    /**
     * Discovery chance per interaction with Cunning/Paranoia NPC (REQ-ROL-001).
     * Only used when bIsDualIdentity is true.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SpyDiscoveryChance = 0.05f;

    // ─── Merchant-Specific ───────────────────────────────

    /**
     * If true, this role has a trade inventory generated from faction economy cache.
     * Pricing = supply/demand ratio × reputation modifier.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Merchant")
    bool bHasTradeInventory = false;

    /**
     * Inventory template tag — determines which items this merchant stocks.
     * The faction economy system generates stock based on available resources.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Merchant", meta = (Categories = "Inventory"))
    FGameplayTag InventoryTemplateTag;

    // ─── Leadership/Rally ────────────────────────────────

    /**
     * If true, this role can use the Rally vent to reduce nearby entities' Threat pressure.
     * Guard Captain, Faction Leader have this enabled.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leadership")
    bool bCanRally = false;

    /**
     * Rally multiplier — how much this role's Rally vent reduces Threat in nearby entities.
     * Higher = more effective leader. Faction leaders get the highest value.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leadership", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RallyEffectiveness = 0.3f;
};

/**
 * Designer-authored database of role definitions.
 * Referenced from DA_LivingWorldSettings → RoleDatabase.
 * Roles are data-driven — adding a new role is adding a new entry, not new code.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicRoleDatabase : public UDataAsset {
    GENERATED_BODY()

public:
    /** All role definitions in this database */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Roles")
    TArray<FMythicRoleDefinition> Roles;

    /** Find a role definition by tag. Returns nullptr if not found. */
    const FMythicRoleDefinition* FindRole(const FGameplayTag& RoleTag) const {
        for (const FMythicRoleDefinition& Role : Roles) {
            if (Role.RoleTag.MatchesTagExact(RoleTag)) {
                return &Role;
            }
        }
        return nullptr;
    }
};
