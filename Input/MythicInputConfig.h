// Copyright Mythic Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MythicInputConfig.generated.h"

class UInputAction;
class UObject;
struct FFrame;

/**
 * FMythicInputAction
 *
 *	Struct used to map a input action to a gameplay input tag.
 */
USTRUCT(BlueprintType)
struct FMythicInputAction {
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TObjectPtr<const UInputAction> InputAction = nullptr;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "Input"))
    FGameplayTag InputTag;
};

/**
 * UMythicInputConfig
 *
 *	Non-mutable data asset that contains input configuration properties.
 */
UCLASS(BlueprintType, Const)
class MYTHIC_API UMythicInputConfig : public UDataAsset {
    GENERATED_BODY()

public:
    UMythicInputConfig(const FObjectInitializer &ObjectInitializer);

    UFUNCTION(BlueprintCallable, Category = "Mythic|Pawn")
    const UInputAction *FindNativeInputActionForTag(const FGameplayTag &InputTag, bool bLogNotFound = true) const;

    UFUNCTION(BlueprintCallable, Category = "Mythic|Pawn")
    const UInputAction *FindAbilityInputActionForTag(const FGameplayTag &InputTag, bool bLogNotFound = true) const;

public:
    // List of input actions used by the owner.  These input actions are mapped to a gameplay tag and must be manually bound.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
    TArray<FMythicInputAction> NativeInputActions;

    // List of input actions used by the owner.  These input actions are mapped to a gameplay tag and are automatically bound to abilities with matching input tags.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (TitleProperty = "InputAction"))
    TArray<FMythicInputAction> AbilityInputActions;
};
