
#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "ActionableItemFragment.generated.h"

/** Abstract item-fragment base for server-authoritative activation and deactivation behavior. */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced, Abstract, meta=(ShowOnlyInnerProperties))
class MYTHIC_API UActionableItemFragment : public UItemFragment {
    GENERATED_BODY()

public:
    /** Optional enhanced-input action tag for fragments that grant a GAS input ability while active. */
    UPROPERTY(Replicated, EditAnywhere, SaveGame,
              meta = (ShowOnlyInnerProperties, Categories = "Input",
                      ToolTip = "Optional enhanced-input action tag. Leave unset for actions invoked directly by inventory or hotbar UI; input-bound fragments validate their own required tag."))
    FGameplayTag InputTag;

    /** Player-facing verb for this item action, such as Attack, Drink, or Use. */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, SaveGame,
              meta = (ShowOnlyInnerProperties,
                      ToolTip = "Player-facing verb for this item action, such as Attack, Drink, or Use."))
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
    /** Grants one exact-fragment source spec; bBindInputTag controls only generic input routing, never SourceObject. */
    FGameplayAbilitySpecHandle GrantItemAbility(
        class UMythicAbilitySystemComponent *ASC,
        UMythicItemInstance *ItemInstance,
        TSubclassOf<class UMythicGameplayAbility> AbilityClass,
        bool bBindInputTag = true);
};
