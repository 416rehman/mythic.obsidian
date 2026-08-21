#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "MythicDigSite.generated.h"

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicDigSiteEntry {
    GENERATED_BODY()

    // Stable id — the consumed/dedup key + the treasure-map target key.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dig Site")
    int32 SiteId = -1;

    // World anchor of the buried find. A dig completed within ToleranceRadius of this yields it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dig Site")
    FVector Anchor = FVector::ZeroVector;

    // How close (cm) a dig must complete to this anchor to yield. Non-positive = disabled site.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dig Site", meta = (ClampMin = "0.0"))
    float ToleranceRadius = 300.0f;

    // Category/identity tag (e.g. Dig.Treasure / Dig.EasterEgg). Cosmetic/routing only.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dig Site")
    FGameplayTag SiteTag;

    // Player-facing name of the find ("Buried Chest").
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dig Site")
    FText DisplayName;

    // True = a treasure-map-gated find: the matching UTreasureMapFragment (TargetDigSiteId == SiteId) is consumed on dig.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dig Site")
    bool bRequiresTreasureMap = false;

    // Cosmetic: this is a hidden easter-egg find (for content/UX flavor). Not a gate.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dig Site")
    bool bIsEasterEgg = false;

    // The buried loot/XP bundle minted on a successful dig (at the dig location).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dig Site")
    FRewardsToGive Rewards;
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicDigSiteRegistry : public UDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dig Site")
    TArray<FMythicDigSiteEntry> Sites;

    bool FindSiteById(int32 SiteId, FMythicDigSiteEntry &OutEntry) const;

    bool FindSiteAtLocation(const FVector &DigLoc, FMythicDigSiteEntry &OutEntry) const;

    void GetAllSiteIds(TSet<int32> &OutIds) const;
};

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mythic Digging"))
class MYTHIC_API UMythicDiggingSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

    /** The authored dig-site registry the digging subsystem loads at runtime. */
    UPROPERTY(EditAnywhere, Config, Category = "Digging")
    TSoftObjectPtr<UMythicDigSiteRegistry> DigSiteRegistry;
};
