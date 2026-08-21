

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DestinationComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UDestinationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDestinationComponent();

protected:
	virtual void BeginPlay() override;

public:
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
};
