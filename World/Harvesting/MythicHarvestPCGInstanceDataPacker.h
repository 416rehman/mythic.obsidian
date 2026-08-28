#pragma once

#include "InstanceDataPackers/PCGInstanceDataPackerBase.h"

#include "MythicHarvestPCGInstanceDataPacker.generated.h"

/**
 * PCG static-mesh instance-data packer that derives one stable harvesting node id per selected point from typed native
 * seed/position data and writes it as eight exact uint16-in-float custom-data values. Invalid or duplicate inputs fail
 * closed to invalid zero ids; no mutable point index or string-selected metadata attribute participates.
 */
UCLASS(NotBlueprintable, ClassGroup = (Procedural),
       meta = (DisplayName = "Mythic Harvest Stable Identity"))
class MYTHIC_API UMythicHarvestPCGInstanceDataPacker final
    : public UPCGInstanceDataPackerBase {
    GENERATED_BODY()

public:
    virtual void PackInstances_Implementation(
        UPARAM(ref) FPCGContext &Context,
        const UPCGSpatialData *InSpatialData,
        UPARAM(ref) const FPCGMeshInstanceList &InstanceList,
        FPCGPackedCustomData &OutPackedCustomData) const override;

    virtual bool GetAttributeNames(TArray<FName> *OutNames) override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(
        FDataValidationContext &Context) const override;
#endif

    /** Persistent GUID of the graph or spawner that owns every identity emitted by this packer. */
    /**
     * Bumped when the packed custom-data layout changes. It participates in the PCG settings hash, so a layout change
     * invalidates serialized generation instead of silently leaving the old stride baked into the world.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Identity")
    int32 CustomDataLayoutVersion = 2;

    UPROPERTY(EditAnywhere, Category = "Harvesting|Identity")
    FGuid ProviderGuid;

    /** Persistent GUID of the provider domain, such as its deterministic generation cell. */
    UPROPERTY(EditAnywhere, Category = "Harvesting|Identity")
    FGuid DomainGuid;

};
