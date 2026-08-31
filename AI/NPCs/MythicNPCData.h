#pragma once
#include "FamilyDefinition.h"
#include "GameplayTagContainer.h"
#include "NPCDefinition.h"
#include "World/Entity/MythicEntityId.h"
#include "MythicNPCData.generated.h"

class UMythicEntityIdentityDefinition;

USTRUCT(BlueprintType, Blueprintable)
struct FMythicNPCData {
    GENERATED_BODY()

    /**
     * Legacy actor-cache and authored-family key. This value is never a canonical LivingWorld identity; authority
     * systems use EntityId, which is deliberately hidden from Blueprint.
     */
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data")
    FGuid NPCId = FGuid::NewGuid();

    /** Authority-only canonical identity allocated by UMythicPersistentNPCRegistry for this logical person. */
    UPROPERTY(SaveGame)
    FMythicEntityId EntityId;

    // Name of the NPC
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data")
    FString NPCName;

    // NPC Type
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data", meta = (Categories = "NPC.Type"))
    FGameplayTag NPCType;

    /** Authored public cover identity; it is independent from private NPCType and may intentionally be empty. */
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data")
    TSoftObjectPtr<UMythicEntityIdentityDefinition> PublicIdentityDefinition;

    // Actor class to spawn for this NPC (copied from UNPCDefinition::NPCClass). Carried on the runtime
    // struct so cached respawns rebuild the same body.
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data")
    TSubclassOf<AMythicNPCCharacter> NPCClass;

    // Cognitive faction (copied from UNPCDefinition::Faction). Resolved to an FMythicFactionId and fed to
    // the brain on spawn.
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data", meta = (Categories = "Faction"))
    FGameplayTag Faction;

    // Behavior traits, applied as loose tags on the NPC's ASC at spawn (stripped again on pool return).
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data", meta = (Categories = "NPC.Trait"))
    FGameplayTagContainer Traits;

    // Tags required to recruit
    UPROPERTY(EditAnywhere, Category = "NPC Data", meta = (Categories = "NPC.Recruitment"))
    FGameplayTagContainer TagsRequiredToRecruit;

    // Per-faction stance delta on the -100..+100 standing scale, composed with the live diplomacy
    // relation in AMythicAIController::GetTeamAttitudeTowards.
    UPROPERTY(EditAnywhere, Category = "NPC Data", meta = (Categories = "Faction"))
    TMap<FGameplayTag, float> AffiliationOverrides;

    // Per-faction fight probability [0,1]; collapsed into the brain's Fight/Flee vent weights at spawn.
    UPROPERTY(EditAnywhere, Category = "NPC Data", meta = (Categories = "Faction"))
    TMap<FGameplayTag, float> FlightOrFightOverrides;

    // The NPC's Family
    UPROPERTY(BlueprintReadOnly)
    FGuid NPCFamilyId = FGuid();

    // Combat level, stamped at spawn from the territory danger at the spawn site plus the world tier.
    // Drives the level half of ApplyCombatScaling and, downstream, the item level of anything it drops.
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data")
    int32 CombatLevel = 1;

    // Rolled base-attribute specs seeded onto this NPC's ASC on spawn (copied from the source
    // UNPCDefinition::Proficiencies). Applied authority-side in AMythicNPCCharacter::SeedAttributesFromData.
    // Carried on the runtime struct (not via a UNPCDefinition* backpointer) so pooled reuse retains them.
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data")
    TArray<FRolledAttributeSpec> Proficiencies;

    // Per-NPC sight perception (copied from UNPCDefinition; defaults match the previous hardcoded AIController
    // values so a default-constructed / MASS-baseline NPC perceives exactly as before). Applied by the AIController
    // on possess. Carried on the runtime struct so pooled reuse retains it.
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data")
    float SightRadius = 1500.0f;

    /** Distance at which sight perception finally forgets a target that has left the acquisition radius. */
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data")
    float LoseSightRadius = 2000.0f;

    /** Horizontal half-angle of this NPC's authored visual perception cone in degrees. */
    UPROPERTY(BlueprintReadOnly, Category = "NPC Data")
    float PeripheralVisionAngleDegrees = 90.0f;

    void ClearAll() {
        NPCId.Invalidate();
        EntityId.Reset();
        NPCName.Empty();
        NPCType = FGameplayTag();
        PublicIdentityDefinition.Reset();
        NPCClass = nullptr;
        Faction = FGameplayTag();
        Traits = FGameplayTagContainer();
        TagsRequiredToRecruit = FGameplayTagContainer();
        AffiliationOverrides.Empty();
        FlightOrFightOverrides.Empty();
        NPCFamilyId.Invalidate();
        CombatLevel = 1;
        Proficiencies.Empty();
        SightRadius = 1500.0f;
        LoseSightRadius = 2000.0f;
        PeripheralVisionAngleDegrees = 90.0f;
    }

    FMythicNPCData() {
        ClearAll();
    }

    FMythicNPCData(UNPCDefinition *NPCDef) {
        this->NPCId = NPCDef->NPCId;
        this->NPCName = NPCDef->Name;
        this->NPCType = NPCDef->NPCType;
        this->PublicIdentityDefinition = NPCDef->PublicIdentityDefinition;
        this->NPCClass = NPCDef->NPCClass;
        this->Faction = NPCDef->Faction;
        this->Traits = NPCDef->Traits;
        this->TagsRequiredToRecruit = NPCDef->TagsRequiredToRecruit;
        this->AffiliationOverrides = NPCDef->AffiliationOverrides;
        this->FlightOrFightOverrides = NPCDef->FlightOrFightOverrides;
        this->Proficiencies = NPCDef->Proficiencies;
        this->SightRadius = NPCDef->SightRadius;
        this->LoseSightRadius = NPCDef->LoseSightRadius;
        this->PeripheralVisionAngleDegrees = NPCDef->PeripheralVisionAngleDegrees;
        auto FamilyDef = NPCDef->FamilyDef.Get();
        if (FamilyDef) {
            this->NPCFamilyId = FamilyDef->FamilyId;
        }
    }
};
