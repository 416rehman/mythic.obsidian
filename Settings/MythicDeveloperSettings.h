#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MythicDeveloperSettings.generated.h"

class UMythicAbilityTagRelationshipMapping;

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
     * Returns the loaded AbilityTagRelationshipMapping, or nullptr if not yet loaded.
     * The asset is preloaded during game startup - this should always be valid during gameplay.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Abilities")
    UMythicAbilityTagRelationshipMapping *GetAbilityTagRelationshipMapping() const;

    /** Get all soft object paths that should be preloaded at startup */
    void GetStartupAssetPaths(TArray<FSoftObjectPath> &OutPaths) const;
};
