// 

#pragma once

#include "CoreMinimal.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "Itemization/Inventory/Fragments/ActionableItemFragment.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "AttackFragment.generated.h"

USTRUCT(Blueprintable, BlueprintType)
struct FAttackRuntimeServerOnlyData {
    GENERATED_BODY()

    UPROPERTY()
    TArray<int> InputBindings = TArray<int>();
};

USTRUCT(Blueprintable, BlueprintType)
struct FAttackRuntimeClientOnlyData {
    GENERATED_BODY()

    UPROPERTY()
    TArray<int> InputBindings = TArray<int>();
};

USTRUCT(Blueprintable, BlueprintType)
struct FAttackRuntimeReplicatedData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FRolledAttributeSpec RolledDamageSpec = FRolledAttributeSpec();

    UPROPERTY()
    UAbilitySystemComponent *ASC = nullptr;

    UPROPERTY()
    FGameplayAbilitySpecHandle AbilityHandle = FGameplayAbilitySpecHandle();
};

USTRUCT(Blueprintable, BlueprintType)
struct FAttackConfig {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<UGameplayAbility> TriggerAbility = nullptr;

    // The ability will be trigerred with this tag when the attack is executed
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Attack")
    FGameplayTag AttackBeginEventTag = GAS_EVENT_ATTACK_BEGIN;

    // The ability will be trigerred with this tag when the attack is finished
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Attack")
    FGameplayTag AttackEndEventTag = GAS_EVENT_ATTACK_END;

    // The animation montage to play when this attack is executed
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Attack")
    UAnimMontage *AttackMontage = nullptr;
};

USTRUCT(Blueprintable, BlueprintType)
struct FAttackBuildData {
    GENERATED_BODY()

    // The damage attribute, this is the attribute on the owner's ability system component that will be modified by this fragment
    UPROPERTY(EditAnywhere)
    FGameplayAttribute DamageAttribute = UMythicAttributeSet_Offense::GetDamagePerHitAttribute();

    // damage attribute roll definition - Modifier has no effect (Override is always used)
    UPROPERTY(EditAnywhere)
    FRollDefinition DamageRollDefinition = FRollDefinition();
};

/**
 * 
 */
UCLASS()
class MYTHIC_API UAttackFragment : public UActionableItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(Attack)

    /** Designer friendly configuration data that defines this fragment. */
    /** REPLICATED and fields should be BlueprintReadOnly */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
    FAttackConfig AttackConfig = FAttackConfig();

    /** This is used in the OnInstanced method to calculate/fill the rest of the data. */
    /** This should not be replicated or blueprint accessible and safely discarded after being used in the OnInstanced method. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties))
    FAttackBuildData AttackBuildData = FAttackBuildData();

    /** Contains the runtime state of the fragment (replicated to client) */
    /** REPLICATED */
    UPROPERTY(Replicated, BlueprintReadOnly)
    FAttackRuntimeReplicatedData AttackRuntimeReplicatedData = FAttackRuntimeReplicatedData();

    /** Contains the runtime client side state of the fragment for use in methods like OnActiveItemClient */
    /** Shouldn't be accessed on server side methods like OnActiveItem. */
    UPROPERTY(BlueprintReadOnly)
    FAttackRuntimeClientOnlyData AttackRuntimeClientOnlyData = FAttackRuntimeClientOnlyData();

    /** Contains the runtime server-only state of the fragment for use in methods like OnActiveItem */
    /** Shouldn't be accessed on client side methods like OnActiveItemClient. */
    UPROPERTY(BlueprintReadOnly)
    FAttackRuntimeServerOnlyData AttackRuntimeServerOnlyData = FAttackRuntimeServerOnlyData();

    // Overrides
    virtual bool IsValidFragment(FText &OutErrorMessage) const override;
    virtual void OnInstanced(UMythicItemInstance *Instance) override;

    virtual void OnItemActivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnItemDeactivated(UMythicItemInstance *ItemInstance) override;
    virtual void OnClientActionBegin(UMythicItemInstance *ItemInst) override;
    virtual void OnClientActionEnd(UMythicItemInstance *ItemInst) override;

    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;
    //~ Overrides

    void TriggerAbilityWithEvent(FGameplayTag Tag);

    // Replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        REP_FRAGMENT_DATA(Attack)

        // DOREPLIFETIME_CONDITION(UAttackFragment, AttackBeginEventTag, COND_InitialOrOwner);
        // DOREPLIFETIME_CONDITION(UAttackFragment, AttackEndEventTag, COND_InitialOrOwner);
        // DOREPLIFETIME_CONDITION(UAttackFragment, AttackMontage, COND_InitialOrOwner);
    }
};
