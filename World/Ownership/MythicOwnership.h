
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MythicOwnership.generated.h"


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicOwnership {
    GENERATED_BODY()

    /** The faction that owns this object (e.g. "Faction.Imperials"). Resolved to a live FMythicFactionId at crime time. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ownership", meta = (Categories = "Faction"))
    FGameplayTag OwnerFactionTag;

    /** Optional canonical key of a specific owning NPC (a named merchant's strongbox / pocket). Empty = faction-owned only. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ownership")
    FString OwnerNpcKey;

    bool IsOwned() const { return OwnerFactionTag.IsValid() || !OwnerNpcKey.IsEmpty(); }
};


namespace MythicTheftCrime {
    MYTHIC_API bool ShouldSubmitTheft(const FMythicOwnership &Ownership, const FGameplayTag &ThiefFactionTag, bool bEnabled);

    MYTHIC_API bool TrySubmitTheft(AActor *Instigator, AActor *OwnedActor, const FMythicOwnership &Ownership);
}


UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicOwnershipComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicOwnershipComponent();

    /** Designer-authored (or spawn-stamped) ownership. Replicated for read-side UI; unstamped default = unowned. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Ownership")
    FMythicOwnership Ownership;

    bool IsOwned() const { return Ownership.IsOwned(); }
    const FMythicOwnership &GetOwnership() const { return Ownership; }

    /** SERVER: submit a theft crime for THIS component's owner against Instigator (delegates to the free helper). */
    UFUNCTION(BlueprintCallable, Category = "Ownership")
    bool TrySubmitTheft(AActor *Instigator);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
};
