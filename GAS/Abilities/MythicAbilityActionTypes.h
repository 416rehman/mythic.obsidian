#pragma once
#include "CoreMinimal.h"
#include "Containers/StaticArray.h"
#include "UObject/StrongObjectPtr.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "MythicDamageContainer.h"
#include "MythicAbilityActionTypes.generated.h"

UENUM(BlueprintType)
enum class EMythicAbilityStatusApplication : uint8 { None, Buildup, Direct };

UENUM(BlueprintType)
enum class EMythicAbilityActionWork : uint8 { Offensive, Utility };

/** Definition-local parameter. Gameplay tags classify behavior; they are not variable names. */
USTRUCT(BlueprintType)
struct FMythicAbilityParameter {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Name;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float Value=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float Minimum=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float Maximum=1;
};

/** One independently classified operation within a mixed ability. */
USTRUCT(BlueprintType)
struct FMythicAbilityActionProfile {
    GENERATED_BODY()
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FName ActionId;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EMythicAbilityActionWork Work=EMythicAbilityActionWork::Offensive;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FGameplayTagContainer Facets;
    /** Utility values are frozen with this action row; no live stat or equipment fallback. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FMythicAbilityParameter> Parameters;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FGameplayAttribute> SourceAttributes;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FMythicDamageContainer Damage;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float MinimumDamage=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float MaximumDamage=0;
    /** Use canonical authored primary-stat contribution rows; never the equipped weapon's roll or class. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bScaleWithPrimaryStats=true;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MaximumTargets=32;
    /** Each member spends its own fixed share. Misses cannot redistribute a budget. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<float> Shares={1.0f};
    UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bOneTargetPerShare=false;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) EMythicAbilityStatusApplication StatusApplication=EMythicAbilityStatusApplication::None;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) FGameplayTag StatusType;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float StatusChance=0;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float StatusDuration=0;
    /** Canonical status buildup GE with its magnitude supplied through SetByCaller.Generic. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly) TSubclassOf<UGameplayEffect> BuildupEffect;
    UPROPERTY(EditAnywhere,BlueprintReadOnly) float BuildupAmount=0;
    bool Validate(TArray<FText>& Errors) const;
};

USTRUCT(BlueprintType)
struct FMythicAbilityDamageResult {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bAccepted=false;
    UPROPERTY(BlueprintReadOnly) bool bDodged=false;
    UPROPERTY(BlueprintReadOnly) bool bKilled=false;
    UPROPERTY(BlueprintReadOnly) float Attempted=0;
    UPROPERTY(BlueprintReadOnly) float HealthLost=0;
    UPROPERTY(BlueprintReadOnly) float ShieldAbsorbed=0;
};

UENUM(BlueprintType)
enum class EMythicAbilityMovementOutcome : uint8 { Reached, Blocked, Cancelled, AvatarLost, StartFailed };

/** Authority facts about one claimed operation. Requested distance is never presented as actual travel. */
USTRUCT(BlueprintType)
struct FMythicAbilityMovementReceipt {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName ActionId;
    UPROPERTY(BlueprintReadOnly) FGuid ActionGroupId;
    UPROPERTY(BlueprintReadOnly) FGuid ReceiptId;
    UPROPERTY(BlueprintReadOnly) FGuid ActivationId;
    UPROPERTY(BlueprintReadOnly) FGameplayTagContainer Facets;
    UPROPERTY(BlueprintReadOnly) EMythicAbilityMovementOutcome Outcome=EMythicAbilityMovementOutcome::StartFailed;
    UPROPERTY(BlueprintReadOnly) FVector Start=FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector AdmittedEnd=FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) FVector ActualEnd=FVector::ZeroVector;
};
DECLARE_MULTICAST_DELEGATE_OneParam(FMythicAbilityUtilityMovementEvent,const FMythicAbilityMovementReceipt&);

/** Server-only immutable source snapshot; no equipment rescan or private source-array replication per hit. */
struct FMythicAbilityActionSource {
    void RetainEffectDefinitions(const UAbilitySystemComponent* ASC);
    static bool IsSupportedStat(const FGameplayAttribute& Attribute);
    static bool CaptureStat(UAbilitySystemComponent* ASC,const FGameplayAttribute& Attribute,const FGameplayTagContainer& SourceTags,
        const TMap<FGameplayAttribute,float>& ReplacementBases,float& Out);
    TMap<FGameplayAttribute,float> Stats;
    TMap<FGameplayAttribute,FGameplayEffectAttributeCaptureSpec> Captures;
    TMap<FGameplayAttribute,float> EquipmentBases;
    /** GAS snapshot mods retain raw tag-requirement pointers into these definitions. Shared by all actions in the cast. */
    TSharedPtr<const TArray<TStrongObjectPtr<const UGameplayEffect>>> EffectDefinitions;
    bool EvaluateStat(const FGameplayAttribute& Attribute,const FGameplayTagContainer& SourceTags,const FGameplayTagContainer* TargetTags,float& Out) const;
    FGameplayTagContainer StateTags;
    FGameplayTagContainer AbilityFacets;
    float ControlDamageMultiplier = 1;
};

struct FMythicAbilityActionSnapshot {
    TSharedPtr<const FMythicAbilityActionSource> Source;
    FName ActionId;
    FGameplayTagContainer Facets;
    float RolledDamage=0;
    bool bScaleWithPrimaryStats=false;
    TStaticArray<float,9> IntentRolls{};
};
