#pragma once
#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "MythicAbilityWindowTypes.generated.h"

UENUM(BlueprintType)
enum class EMythicDamageOrigin : uint8 { Other, DirectWeapon, Skill, Rune, Talent, Periodic };
UENUM(BlueprintType)
enum class EMythicAbilityIntent : uint8 { Self, Ground, Hostile, Party };
UENUM(BlueprintType)
enum class EMythicAbilityWindowEnd : uint8 { NaturalExpiry, Released, Completed, Cancelled, AvatarLost, Invalidated };

USTRUCT(BlueprintType)
struct FMythicAbilityHealResult {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) float Attempted = 0;
    UPROPERTY(BlueprintReadOnly) float Gained = 0;
    UPROPERTY(BlueprintReadOnly) float Unused = 0;
};

USTRUCT(BlueprintType)
struct FMythicAbilityQuery {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float Radius = 500;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) float AngleDegrees = 360;
    /** Use yaw for the cone angle while retaining full three-dimensional range and line of sight. */
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bHorizontalArc = false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) int32 MaxTargets = 8;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bParty = false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bRequireLOS = true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bLowestHealthFirst = false;
    UPROPERTY(EditAnywhere,BlueprintReadWrite) bool bPrioritizeSelf = false;
};

USTRUCT(BlueprintType)
struct FMythicAbilitySignal {
    GENERATED_BODY()
    /** Native dispatch provenance, retained by delayed signals; never authored by the graph. */
    bool bReaction = false;
    /** Raw observations have no causal effect context and cannot originate offensive child actions. */
    bool bHasReactionLineage = true;
    UPROPERTY(BlueprintReadOnly) FName Name;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> Source;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> Target;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<AActor> Actor;
    UPROPERTY(BlueprintReadOnly) FVector Location = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector PreviousLocation = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) float Magnitude = 0;
    UPROPERTY(BlueprintReadOnly) bool bCritical = false;
    UPROPERTY(BlueprintReadOnly) bool bDirectWeapon = false;
    UPROPERTY(BlueprintReadOnly) EMythicDamageOrigin Origin = EMythicDamageOrigin::Other;
    UPROPERTY(BlueprintReadOnly) int32 Generation = 0;
    UPROPERTY(BlueprintReadOnly) FGuid ActivationId;
    UPROPERTY(BlueprintReadOnly) FGuid ShieldHandle;
    UPROPERTY(BlueprintReadOnly) FGuid Receipt;
};

struct FMythicOwnedShieldAbsorption {
    FGuid Handle;
    float Amount = 0;
};
