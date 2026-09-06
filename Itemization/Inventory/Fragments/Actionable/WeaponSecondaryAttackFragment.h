#pragma once
#include "AttackFragment.h"
#include "WeaponSecondaryAttackFragment.generated.h"
/** Separate exact source/config/cache; the existing canonical weapon GA owns every heavy hit and montage. */
UCLASS(BlueprintType,Blueprintable,EditInlineNew,DefaultToInstanced)
class MYTHIC_API UWeaponSecondaryAttackFragment : public UAttackFragment {
    GENERATED_BODY()
public:
    UWeaponSecondaryAttackFragment();
    /** Current-definition tuning. Never saved on an item, never written into shared weapon stat bases. */
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Replicated,Category="Heavy",meta=(ClampMin="1.0",ClampMax="3.0"))
    float DamageMultiplier=1.75f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Replicated,Category="Heavy",meta=(ClampMin="0.35",ClampMax="1.0"))
    float AttackRateMultiplier=0.70f;
    bool HasValidHeavyTuning() const;
    virtual bool CanBeStackedWith(const UItemFragment* Other) const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
#if WITH_EDITOR
    virtual bool IsValidFragment(FText& OutErrorMessage) const override;
#endif
};
