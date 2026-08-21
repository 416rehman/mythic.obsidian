
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MythicSocialVerbs.generated.h"

struct FMythicPersonalityFragment;

UENUM(BlueprintType)
enum class EMythicSocialVerb : uint8 {
    Greet = 0 UMETA(DisplayName = "Greet"),
    Compliment UMETA(DisplayName = "Compliment"),
    Provoke UMETA(DisplayName = "Provoke"),
    Bully UMETA(DisplayName = "Bully"),
    Threaten UMETA(DisplayName = "Threaten"),

    COUNT UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EMythicSocialReaction : uint8 {
    Warm = 0 UMETA(DisplayName = "Warm"),
    Neutral UMETA(DisplayName = "Neutral"),
    Cold UMETA(DisplayName = "Cold"),
    Intimidated UMETA(DisplayName = "Intimidated"),
    Angered UMETA(DisplayName = "Angered"),
    CallGuards UMETA(DisplayName = "CallGuards"),

    COUNT UMETA(Hidden)
};

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

UCLASS()
class MYTHIC_API UMythicSocialVerbLibrary : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
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
