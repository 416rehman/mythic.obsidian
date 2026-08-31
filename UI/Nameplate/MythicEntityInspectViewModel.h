#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UI/Nameplate/MythicEntityInspectTypes.h"

#include "MythicEntityInspectViewModel.generated.h"

/** Allocation-stable local state consumed by one entity Inspect page. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicEntityInspectViewModel : public UObject {
    GENERATED_BODY()

public:
    /** Returns the immutable viewer-safe dossier currently bound to the page. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Inspect")
    const FMythicEntityInspectProjection &GetProjection() const {
        return Projection;
    }

    /** Applies an already-sanitized dossier; only the local nameplate/Inspect coordinator should call this. */
    void Apply(const FMythicEntityInspectProjection &InProjection) {
        Projection = InProjection;
    }

    /** Clears the dossier before the page closes or changes embodiment. */
    void Reset() { Projection = FMythicEntityInspectProjection(); }

private:
    /** Current player-knowledge DTO; it contains no gameplay object or private simulation identifier. */
    UPROPERTY(Transient)
    FMythicEntityInspectProjection Projection;
};

