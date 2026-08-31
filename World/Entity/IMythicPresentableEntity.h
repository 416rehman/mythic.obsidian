#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IMythicPresentableEntity.generated.h"

class UMythicEntityPresentationComponent;

/** Reflection marker for actors that expose the shared contextual entity-presentation adapter. */
UINTERFACE(BlueprintType)
class MYTHIC_API UMythicPresentableEntity : public UInterface {
    GENERATED_BODY()
};

/** Optional subject contract for systems that cannot receive a component directly from the push registry. */
class MYTHIC_API IMythicPresentableEntity {
    GENERATED_BODY()

public:
    /**
     * Returns this actor's single shared presentation adapter, or null when it is intentionally not presentable.
     * The returned component remains gameplay/domain state only and never owns a widget or viewer-specific projection.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Mythic|Entity Presentation")
    UMythicEntityPresentationComponent *GetEntityPresentationComponent() const;
};
