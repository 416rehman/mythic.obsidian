
#pragma once

#include "CoreMinimal.h"
#include "EnhancedInput/Public/InputAction.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "ActionableItemFragment.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, Abstract, meta=(ShowOnlyInnerProperties))
class MYTHIC_API UActionableItemFragment : public UItemFragment {
    GENERATED_BODY()

public:
    UPROPERTY(Replicated, EditAnywhere, meta=(ShowOnlyInnerProperties), SaveGame, meta=(Categories="Input"))
    FGameplayTag InputTag;

    /** Display name for the action associated with this item (e.g., "Attack", "Drink", "Use") */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, meta=(ShowOnlyInnerProperties), SaveGame)
    FText ActionDisplayName;

#if WITH_EDITOR
    virtual bool IsValidFragment(FText &OutErrorMessage) const override;
#endif
    virtual bool CanBeStackedWith(const UItemFragment *Other) const override;

    /** Called when action input starts. Override to implement item-specific behavior. */
    UFUNCTION(BlueprintCallable)
    virtual void OnClientActionBegin(UMythicItemInstance *ItemInst) {}

    /** Called when action input ends. Override to implement item-specific behavior. */
    UFUNCTION(BlueprintCallable)
    virtual void OnClientActionEnd(UMythicItemInstance *ItemInstance) {}

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(UActionableItemFragment, InputTag);
        DOREPLIFETIME(UActionableItemFragment, ActionDisplayName);
    }

    virtual void ExecuteGenericAction(UMythicItemInstance *ItemInstance);

protected:
    FGameplayAbilitySpecHandle GrantItemAbility(class UMythicAbilitySystemComponent *ASC, UMythicItemInstance *ItemInstance,
                                                TSubclassOf<class UMythicGameplayAbility> AbilityClass);
};
