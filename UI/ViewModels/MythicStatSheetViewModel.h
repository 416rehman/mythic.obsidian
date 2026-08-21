// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "MVVMViewModelBase.h"
#include "AttributeSet.h"
#include "MythicStatDisplay.h"
#include "MythicStatSheetViewModel.generated.h"

class UAbilitySystemComponent;
class UMythicAttributeSet;

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatSection {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    FText Heading;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    EMythicStatCategory Category = EMythicStatCategory::Utility;

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|Stats")
    TArray<FMythicStatLine> Lines;
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicStatSheetViewModel : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    // Bind to the local player's ASC and build the first snapshot. Idempotent — safe to call on every open.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Stats")
    void InitializeForASC(UAbilitySystemComponent *InASC);

    // Drop all bindings. Call when the sheet closes so a hidden panel costs nothing.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Stats")
    void Shutdown();

    // Force an immediate rebuild. Only needed if something changes a value without going through GAS.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Stats")
    void Refresh();

    // Progressive disclosure: off by default, the sheet shows only stats that are non-zero or actively modified.
    // Turning it on reveals every attribute including the ~30 that are inert for this build.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Stats")
    void SetShowUnmodified(bool bShow);

    // b-prefixed -> explicit accessor name, so the getter drops the 'b' (UHT will not infer this).
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter = GetShowUnmodified, meta = (AllowPrivateAccess))
    bool bShowUnmodified = false;

    bool GetShowUnmodified() const {
        return bShowUnmodified;
    }

    // The whole sheet, ordered: Vitality, Offense, Defense, Utility, Survival, Proficiencies. Empty sections are
    // omitted entirely rather than rendered as an empty heading.
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess))
    TArray<FMythicStatSection> Sections;

    const TArray<FMythicStatSection> &GetSections() const {
        return Sections;
    }

    // How many stats your gear, talents and buffs are currently changing. Drives a "12 stats modified" summary line —
    // a single number that tells a player their build is doing something before they read any of it.
    UPROPERTY(BlueprintReadOnly, FieldNotify, Getter, meta = (AllowPrivateAccess))
    int32 ModifiedStatCount = 0;

    int32 GetModifiedStatCount() const {
        return ModifiedStatCount;
    }

    virtual void BeginDestroy() override;

private:
    UFUNCTION()
    void HandleAttributeChanged(const FGameplayAttribute &Attribute, float OldValue, float NewValue);

    void ScheduleRebuild();
    void Rebuild();

    void SetSections(TArray<FMythicStatSection> &&In);
    void SetModifiedStatCount(int32 In);

    TWeakObjectPtr<UAbilitySystemComponent> ASC;

    static void GatherGearContributions(const UAbilitySystemComponent *InASC,
                                        TMap<FGameplayAttribute, float> &OutByAttribute);

    UPROPERTY()
    TArray<TWeakObjectPtr<UMythicAttributeSet>> BoundSets;

    bool bRebuildScheduled = false;
};
