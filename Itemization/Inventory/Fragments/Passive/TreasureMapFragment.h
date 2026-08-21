#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Net/UnrealNetwork.h"
#include "TreasureMapFragment.generated.h"

class APlayerController;

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UTreasureMapFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(TreasureMap)

    /** The dig site this map points to (matches FMythicDigSiteEntry::SiteId). The dig at that site consumes this map. */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Treasure Map")
    int32 TargetDigSiteId = -1;

    /** World anchor of the buried site — the point the compass WAYPOINT marks. Replicated so the client compass can place
     *  the marker directly. If left zero, the server back-fills it from the dig registry when the map is read. */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Treasure Map")
    FVector TargetAnchor = FVector::ZeroVector;

    /** Radius (cm) around the anchor the holder must dig within. Mirrors the site's tolerance; client UX hint. */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Treasure Map", meta = (ClampMin = "0.0"))
    float ToleranceRadius = 300.0f;

    /** Player-facing map name ("Faded Treasure Map"). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Treasure Map")
    FText MapName;

    UFUNCTION(BlueprintPure, Category = "Treasure Map")
    int32 GetTargetDigSiteId() const { return TargetDigSiteId; }

    UFUNCTION(BlueprintPure, Category = "Treasure Map")
    FVector GetTargetAnchor() const { return TargetAnchor; }

    /** True when this fragment names a real dig site. */
    UFUNCTION(BlueprintPure, Category = "Treasure Map")
    bool IsTreasureMap() const { return TargetDigSiteId >= 0; }

    static bool ConsumeMatchingMap(APlayerController *PC, int32 SiteId);

    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME_CONDITION(ThisClass, TargetDigSiteId, COND_InitialOrOwner);
        DOREPLIFETIME_CONDITION(ThisClass, TargetAnchor, COND_InitialOrOwner);
        DOREPLIFETIME_CONDITION(ThisClass, ToleranceRadius, COND_InitialOrOwner);
    }
};
