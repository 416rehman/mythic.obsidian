// Mythic Living World — Social Interaction Verbs (RDR2-style player→NPC verbs + trait-driven reactions)
// The PURE trait→reaction core lives here as free statics on a UBlueprintFunctionLibrary so it is unit-testable
// (no engine/world state). The server-authoritative wiring (RPC, range/auth, escalation) lives on
// AMythicPlayerController + AMythicNPCCharacter; this file only owns the enums, the result struct, and the
// deterministic mapping personality(VentWeights) + standing → reaction. See LIVING_WORLD_NPC_SPEC.md §6.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MythicSocialVerbs.generated.h"

struct FMythicPersonalityFragment;

/**
 * The set of player-initiated social verbs an NPC can be targeted with (a BP radial menu calls the server RPC with
 * one of these). FROZEN order — used as a BP contract and as an array index nowhere, but kept stable for save/UI.
 * Greet/Compliment are friendly; Provoke/Bully/Threaten are hostile-escalating (in increasing intent).
 */
UENUM(BlueprintType)
enum class EMythicSocialVerb : uint8 {
    Greet = 0 UMETA(DisplayName = "Greet"),
    Compliment UMETA(DisplayName = "Compliment"),
    Provoke UMETA(DisplayName = "Provoke"),
    Bully UMETA(DisplayName = "Bully"),
    Threaten UMETA(DisplayName = "Threaten"),

    COUNT UMETA(Hidden)
};

/**
 * The NPC's reaction band to a social verb, decided by its personality + the player's standing. Drives the bark line,
 * the standing delta sign, and whether the encounter escalates to aggro / a guard alert. FROZEN order (BP contract).
 */
UENUM(BlueprintType)
enum class EMythicSocialReaction : uint8 {
    Warm = 0 UMETA(DisplayName = "Warm"),          // friendly, gains standing
    Neutral UMETA(DisplayName = "Neutral"),        // indifferent
    Cold UMETA(DisplayName = "Cold"),              // rebuffed, no/low standing change
    Intimidated UMETA(DisplayName = "Intimidated"),// cowed (high Flee/Submit) — backs down, may flee
    Angered UMETA(DisplayName = "Angered"),        // provoked (high Fight) — turns hostile
    CallGuards UMETA(DisplayName = "CallGuards"),  // alerts allies/guards (high Enforce)

    COUNT UMETA(Hidden)
};

/**
 * The full resolved consequence of a social verb. Pure data — produced by ResolveReaction (no engine state) and then
 * applied server-side by AMythicNPCCharacter::ApplySocialReaction (standing mutation, aggro, guard alert).
 */
USTRUCT(BlueprintType)
struct FMythicSocialReactionResult {
    GENERATED_BODY()

    // The reaction band (drives the bark + the apply behavior).
    UPROPERTY(BlueprintReadOnly, Category = "Social")
    EMythicSocialReaction Reaction = EMythicSocialReaction::Neutral;

    // Signed change to the player's standing toward this NPC's faction (already includes sign: + for liked verbs,
    // - for hostile verbs). Zero when the verb produced no reputation movement (e.g. a Cold compliment).
    UPROPERTY(BlueprintReadOnly, Category = "Social")
    float StandingDelta = 0.0f;

    // The NPC should turn hostile on the interacting player (drives ForceEngageTarget).
    UPROPERTY(BlueprintReadOnly, Category = "Social")
    bool bSetHostile = false;

    // Nearby allied NPCs/guards should be alerted (drives the bounded radius scan + OnSignificantEvent).
    UPROPERTY(BlueprintReadOnly, Category = "Social")
    bool bAlertGuards = false;
};

/**
 * Pure library for the social-verb reaction mapping. All methods are stateless statics → unit-testable with no world.
 * The reaction is deterministic for a given (verb, personality, standing, thresholds): NO RNG, NO engine reads.
 */
UCLASS()
class MYTHIC_API UMythicSocialVerbLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    /**
     * PURE: map a social verb against an NPC's personality (its VentWeights) and the player's current standing toward
     * the NPC's faction into a full reaction result.
     *
     * Reads P.VentWeights[Fight / Flee / Submit / Enforce]. Friendly verbs (Greet/Compliment) gate on standing bands
     * (HostileThreshold / FriendlyThreshold). Hostile verbs (Provoke/Bully/Threaten) test the NPC's Fight weight
     * against a per-verb anger threshold (Bully angers easier than Threaten than Provoke) to decide Angered (+ aggro);
     * a high Enforce weight escalates to CallGuards; a high Flee/Submit & low Fight weight yields Intimidated.
     *
     * @param V                 the verb the player used.
     * @param P                 the NPC's personality fragment (VentWeights).
     * @param Standing          the player's standing toward the NPC's faction (0 if none).
     * @param HostileThreshold  standing at/below which the player is Hostile (from the standing component).
     * @param FriendlyThreshold standing at/above which the player is Friendly (from the standing component).
     *
     * NOTE: NOT a UFUNCTION — it takes FMythicPersonalityFragment, a plain USTRUCT() (not BlueprintType), so UHT
     * cannot expose it to Blueprint. It is a plain public static (the unit-tested pure core); BP reaches the system
     * through the server RPC (AMythicPlayerController::ServerPerformSocialVerb), not this raw mapping.
     */
    static FMythicSocialReactionResult ResolveReaction(
        EMythicSocialVerb V,
        const FMythicPersonalityFragment &P,
        float Standing,
        float HostileThreshold,
        float FriendlyThreshold);

    /**
     * PURE: a placeholder English fallback bark line for a (verb, reaction) pair. HONEST ART BOUNDARY — real,
     * localized, personality-flavored lines are authored content (the NPC's brain/bark tables); this is the
     * code-default so the system is functional before any line is written. Never empty.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Social")
    static FText DefaultBarkFor(EMythicSocialVerb V, EMythicSocialReaction Reaction);
};
