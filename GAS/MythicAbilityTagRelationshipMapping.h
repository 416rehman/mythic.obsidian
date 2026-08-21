// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "UObject/UObjectGlobals.h"

#include "MythicAbilityTagRelationshipMapping.generated.h"

class UObject;

USTRUCT()
struct FMythicAbilityTagRelationship
{
    GENERATED_BODY()

    /** The tag that this container relationship is about. Single tag, but abilities can have multiple of these */
    UPROPERTY(EditAnywhere, Category = Ability)
    FGameplayTag AbilityTag;

    /** The other ability tags that will be blocked by any ability using this tag */
    UPROPERTY(EditAnywhere, Category = Ability)
    FGameplayTagContainer AbilityTagsToBlock;

    /** The other ability tags that will be canceled by any ability using this tag */
    UPROPERTY(EditAnywhere, Category = Ability)
    FGameplayTagContainer AbilityTagsToCancel;

    /** If an ability has the tag, this is implicitly added to the activation required tags of the ability */
    UPROPERTY(EditAnywhere, Category = Ability)
    FGameplayTagContainer ActivationRequiredTags;

    /** If an ability has the tag, this is implicitly added to the activation blocked tags of the ability */
    UPROPERTY(EditAnywhere, Category = Ability)
    FGameplayTagContainer ActivationBlockedTags;
};


UCLASS()
class UMythicAbilityTagRelationshipMapping : public UDataAsset
{
    GENERATED_BODY()

private:
    /** The list of relationships between different gameplay tags (which ones block or cancel others) */
    UPROPERTY(EditAnywhere, Category = Ability, meta=(TitleProperty="AbilityTag"))
    TArray<FMythicAbilityTagRelationship> AbilityTagRelationships;

public:
    void GetAbilityTagsToBlockAndCancel(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer* OutTagsToBlock, FGameplayTagContainer* OutTagsToCancel) const;

    void GetRequiredAndBlockedActivationTags(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer* OutActivationRequired, FGameplayTagContainer* OutActivationBlocked) const;

    bool IsAbilityCancelledByTag(const FGameplayTagContainer& AbilityTags, const FGameplayTag& ActionTag) const;
};
