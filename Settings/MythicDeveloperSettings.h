#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MythicDeveloperSettings.generated.h"

class UMythicAbilityTagRelationshipMapping;
class UMythicDamageNumberConfig;

/**
 * Settings for Mythic gameplay systems.
 * Configure these in Project Settings > Game > Mythic.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Mythic"))
class MYTHIC_API UMythicDeveloperSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    UMythicDeveloperSettings();

    //~UDeveloperSettings interface
    virtual FName GetCategoryName() const override { return FName("Game"); }
    //~End of UDeveloperSettings interface

    /** 
     * Default ability tag relationship mapping applied to all AbilitySystemComponents.
     * Defines how ability tags interact (block, cancel, require other abilities).
     * Can be overridden per-ASC via SetTagRelationshipMapping().
     * 
     * NOTE: This is loaded asynchronously at startup. Access via GetAbilityTagRelationshipMapping().
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Abilities")
    TSoftObjectPtr<UMythicAbilityTagRelationshipMapping> DefaultAbilityTagRelationshipMapping;

    /** 
     * Default generic ability used for items that have an Input Tag but no specific Gameplay Ability.
     * Often used for Consumables that apply effects or simple actions.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Abilities")
    TSoftClassPtr<class UMythicGameplayAbility> DefaultItemInputAbility;

    /** 
     * Returns the loaded AbilityTagRelationshipMapping, or nullptr if not yet loaded.
     * The asset is preloaded during game startup - this should always be valid during gameplay.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Abilities")
    UMythicAbilityTagRelationshipMapping *GetAbilityTagRelationshipMapping() const;

    /** Get all soft object paths that should be preloaded at startup */
    void GetStartupAssetPaths(TArray<FSoftObjectPath> &OutPaths) const;

    /**
     * Configuration for screen-space damage numbers.
     * Controls font, colors, animation, and formatting.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "UI|Damage Numbers")
    TSoftObjectPtr<UMythicDamageNumberConfig> DamageNumberConfig;
};
