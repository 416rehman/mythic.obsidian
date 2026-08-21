#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MythicProxyRegistrationComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicProxyRegistrationComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicProxyRegistrationComponent();

    /**
     * What kind of placed thing this is (Interactable.Chest, Interactable.Vendor, Interactable.Secret …). Actors
     * sharing a Type share one instanced-mesh group, so give genuinely different meshes different tags.
     * UNSET = this component does nothing at all and the actor stays a normal actor.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proxy")
    FGameplayTag ProxyType;

    /**
     * Master switch. FALSE (default) means this component is inert — the actor behaves exactly as it does today.
     * Turning it on per-Blueprint is how the conversion is rolled out one class at a time instead of all at once.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proxy")
    bool bEnabled = false;

    /** Durable state handed to the record on adoption (opened, looted, discovered). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proxy")
    int32 InitialStateFlags = 0;

protected:
    virtual void BeginPlay() override;
};
