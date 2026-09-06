#pragma once
#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "Itemization/Inventory/Fragments/ActionableItemFragment.h"
#include "WeaponBlockFragment.generated.h"
class UGameplayEffect;
class UMythicAbilitySystemComponent;
/** Optional held fortification. A weapon may author this OR a secondary attack, never both. */
UCLASS(BlueprintType,Blueprintable,EditInlineNew,DefaultToInstanced)
class MYTHIC_API UWeaponBlockFragment : public UActionableItemFragment {
    GENERATED_BODY()
public:
    UWeaponBlockFragment();
    /** Definition-owned infinite nonperiodic self defense, excluded from SaveGame. */
    UPROPERTY(Replicated,EditAnywhere,BlueprintReadOnly,Category="Defense") TSubclassOf<UGameplayEffect> GuardEffect;
    virtual void OnItemActivated(UMythicItemInstance* Item) override;
    virtual void OnItemDeactivated(UMythicItemInstance* Item) override;
    virtual bool CanBeStackedWith(const UItemFragment* Other) const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    bool HasUsableEffect() const;
#if WITH_EDITOR
    virtual bool IsValidFragment(FText& Error) const override;
#endif
private:
    UPROPERTY(Transient,DuplicateTransient) TObjectPtr<UMythicAbilitySystemComponent> GrantedASC;
    FGameplayAbilitySpecHandle GrantedHandle;
};
