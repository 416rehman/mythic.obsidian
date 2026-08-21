#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameplayTagContainer.h"
#include "MythicInfluenceSourceComponent.generated.h"

class UAbilitySystemComponent;

struct FMythicInfluenceHit {
    TWeakObjectPtr<const class UMythicInfluenceSourceComponent> Source;
    float Magnitude = 0.0f;
    float DistSq = 0.0f;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicInfluenceSourceComponent : public USphereComponent {
    GENERATED_BODY()

public:
    UMythicInfluenceSourceComponent();

    /** The influence ROLE this source provides (Influence.Shelter now; Irrigation/Pollination/Scarecrow in L, stall
     *  traffic in O). The query discriminator — invalid = the source registers nothing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Influence")
    FGameplayTag RoleTag;

    /** Role-specific strength (meaning is the CONSUMER's: shelter quality, irrigation rate, fear power...). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Influence", meta = (ClampMin = "0.0"))
    float Magnitude = 1.0f;

    /** Aura radius (cm). Drives both the overlap sphere (tag grant) and the coverage query. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Influence", meta = (ClampMin = "0.0"))
    float InfluenceRadius = 400.0f;

    /** OPTIONAL loose tag granted (refcounted, server-side) to qualifying actors inside the radius — e.g. shelter
     *  grants Status.Sheltered. Unset = query-only source (no tag pushing; L's read-only roles). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Influence")
    FGameplayTag GrantedStatusTag;

    /** Actor filter for the TAG GRANT: when true (default) only player pawns (controller/pawn resolves a player ASC)
     *  qualify. False = any pawn with an ASC. (The pull-query is location-based and unaffected.) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Influence")
    bool bAffectPlayersOnly = true;

    /** OPTIONAL class filter for the TAG GRANT: overlapping actor must be (a subclass of) this. Empty = no gate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Influence")
    TSubclassOf<AActor> RequiredActorClass;

    static void GetInfluencesAt(const UWorld *World, const FVector &Location, const FGameplayTag &QueryRoleTag,
                                TArray<FMythicInfluenceHit> &OutHits);

    static float GetTotalInfluenceAt(const UWorld *World, const FVector &Location, const FGameplayTag &QueryRoleTag);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void OnInfluenceBeginOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp,
                                 int32 OtherBodyIndex, bool bFromSweep, const FHitResult &Sweep);

    UFUNCTION()
    void OnInfluenceEndOverlap(UPrimitiveComponent *OverlappedComp, AActor *OtherActor, UPrimitiveComponent *OtherComp,
                               int32 OtherBodyIndex);

private:
    UAbilitySystemComponent *ResolveQualifyingASC(AActor *Actor) const;

    TSet<TWeakObjectPtr<UAbilitySystemComponent>> GrantedASCs;

    static TArray<TWeakObjectPtr<UMythicInfluenceSourceComponent>> GLiveSources;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicShelterAuraComponent : public UMythicInfluenceSourceComponent {
    GENERATED_BODY()

public:
    UMythicShelterAuraComponent();
};


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicIrrigationAuraComponent : public UMythicInfluenceSourceComponent {
    GENERATED_BODY()

public:
    UMythicIrrigationAuraComponent();
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicPollinationAuraComponent : public UMythicInfluenceSourceComponent {
    GENERATED_BODY()

public:
    UMythicPollinationAuraComponent();
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicScarecrowAuraComponent : public UMythicInfluenceSourceComponent {
    GENERATED_BODY()

public:
    UMythicScarecrowAuraComponent();
};
