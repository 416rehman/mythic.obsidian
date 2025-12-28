// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "Components/ActorComponent.h"
#include "ProficiencyComponent.generated.h"

struct FMilestone;
class UProficiencyDefinition;

USTRUCT(BlueprintType, Blueprintable)
struct FProficiency {
    GENERATED_BODY()

    void Instantiate();
    void GenerateTrack();

    // The definition of the proficiency
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency")
    UProficiencyDefinition *Definition = nullptr;

    // The ASC attribute indicating the progress of this proficiency
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency")
    FGameplayAttribute ProgressAttribute = FGameplayAttribute();

    // The generated proficiency track
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proficiency")
    TArray<FMilestone> Track = TArray<FMilestone>();

    // XP value loaded from save (staged here until BeginPlay when ASC is ready)
    float SavedXP = 0.0f;

    // Cache of the max XP for this proficiency (calculated at runtime)
    float MaxXP = 0.0f;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UProficiencyComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, Category = "Proficiency")
    TArray<FProficiency> Proficiencies;

    UPROPERTY()
    UAbilitySystemComponent *ASC;

protected:
    // When true, skip normal reward logic in OnAttributeChanged (used during restore)
    bool bIsRestoring = false;

    void OnAttributeChanged(const FOnAttributeChangeData &OnAttributeChangeData);
    void ConfigureProgressionAttribute(FProficiency &Proficiency);

    /** Reapply only CanReapplyOnLoad rewards for levels up to given level */
    void ReapplyRewardsForLevel(FProficiency &Proficiency, int32 TargetLevel);

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
};
