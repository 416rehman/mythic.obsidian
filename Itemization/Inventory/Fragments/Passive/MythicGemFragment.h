#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/Inventory/Fragments/ItemFragment.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "MythicGemFragment.generated.h"

UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class MYTHIC_API UMythicGemFragment : public UItemFragment {
    GENERATED_BODY()

public:
    DECLARE_FRAGMENT(MythicGem)

    /** This gem's type (e.g. Itemization.Gem.Ruby). Identity + the color a socket must accept + a runeword element. */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Gem", meta = (Categories = "Itemization.Gem"))
    FGameplayTag GemType;

    /** Affixes granted to the wearer while this gem is socketed into an equipped host item. */
    UPROPERTY(Replicated, EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Gem")
    TArray<FRolledAffix> GrantedAffixes;

    /** True when this fragment carries a usable gem (valid type + at least one granted affix). */
    UFUNCTION(BlueprintPure, Category = "Gem")
    bool IsGem() const { return GemType.IsValid() && GrantedAffixes.Num() > 0; }

    UFUNCTION(BlueprintPure, Category = "Gem")
    FGameplayTag GetGemType() const { return GemType; }

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME_CONDITION(ThisClass, GemType, COND_InitialOrOwner);
        DOREPLIFETIME_CONDITION(ThisClass, GrantedAffixes, COND_InitialOrOwner);
    }

    virtual bool CanBeStackedWith(const UItemFragment *Other) const override {
        if (!Super::CanBeStackedWith(Other)) {
            return false;
        }
        const UMythicGemFragment *OtherGem = Cast<UMythicGemFragment>(Other);
        if (!OtherGem) {
            return false;
        }
        return GemType == OtherGem->GemType && GrantedAffixes.Num() == OtherGem->GrantedAffixes.Num();
    }
};
