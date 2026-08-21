#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Containers/Set.h"
#include "World/Digging/MythicDigSite.h"
#include "MythicDiggingSubsystem.generated.h"

UCLASS()
class MYTHIC_API UMythicDiggingSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    bool ServerResolveAndConsumeAt(const FVector &DigLoc, FMythicDigSiteEntry &OutEntry);

    bool IsConsumed(int32 SiteId) const { return ConsumedSiteIds.Contains(SiteId); }

    const UMythicDigSiteRegistry *GetRegistry();

    void GetConsumedSiteIds(TArray<int32> &Out) const { Out = ConsumedSiteIds.Array(); }

    void LoadConsumedSiteIds(const TArray<int32> &In) { ConsumedSiteIds.Append(In); }

private:
    void EnsureRegistryLoaded();

    UPROPERTY()
    TObjectPtr<UMythicDigSiteRegistry> Registry;

    bool bRegistryResolved = false;

    TSet<int32> ConsumedSiteIds;
};
