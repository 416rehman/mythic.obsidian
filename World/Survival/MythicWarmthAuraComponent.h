
#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "MythicWarmthAuraComponent.generated.h"

class UAbilitySystemComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicWarmthAuraComponent : public USphereComponent {
    GENERATED_BODY()

public:
    UMythicWarmthAuraComponent();

    // Aura radius (cm). Applied to the sphere in the ctor + BeginPlay so a designer can tune it per campfire.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warmth Aura")
    float WarmthRadius = 500.0f;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void OnAuraBeginOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp,
                            int32 OtherBodyIndex, bool bFromSweep, const FHitResult &Sweep);

    UFUNCTION()
    void OnAuraEndOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp,
                          int32 OtherBodyIndex);

private:
    static UAbilitySystemComponent *ResolvePlayerASC(AActor *Actor);

    TSet<TWeakObjectPtr<UAbilitySystemComponent>> WarmedASCs;
};
