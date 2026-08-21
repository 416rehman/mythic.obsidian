
#include "AI/MonsterAffixes/MonsterAffixGranter.h"
#include "AI/MonsterAffixes/MonsterAffixPool.h"
#include "AI/MonsterAffixes/MonsterAffixTypes.h"

#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"

#include "World/LivingWorld/Territory/MythicDanger.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(AFFIX_MOLTEN, "Affix.Molten", "Elite affix: periodic Burn aura (fire)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(AFFIX_SHIELDED, "Affix.Shielded", "Elite affix: infinite bonus-MaxShield GE");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(AFFIX_FROZEN, "Affix.Frozen", "Elite affix: periodic Slow aura (chill)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(AFFIX_VORTEX, "Affix.Vortex", "Elite affix: periodic pull toward the owner");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(AFFIX_STATE_SHIELDED, "Affix.State.Shielded",
                               "Granted by the Shielded GE so pool-return can strip it (mirrors GAS.State.CombatScaling)");

const TArray<FMonsterAffixDef> &UMonsterAffixPool::GetDefaultPool() {
    static const TArray<FMonsterAffixDef> Pool = [] {
        using DT = EMythicDangerTier;
        TArray<FMonsterAffixDef> P;

        {
            FMonsterAffixDef D;
            D.AffixTag = AFFIX_MOLTEN;
            D.BudgetCost = 2;
            D.MinTierInt = 3;
            D.MinDanger = static_cast<uint8>(DT::Safe);
            D.IncompatibleWith.AddTag(AFFIX_FROZEN);
            P.Add(D);
        }
        {
            FMonsterAffixDef D;
            D.AffixTag = AFFIX_FROZEN;
            D.BudgetCost = 2;
            D.MinTierInt = 3;
            D.MinDanger = static_cast<uint8>(DT::Low);
            P.Add(D);
        }
        {
            FMonsterAffixDef D;
            D.AffixTag = AFFIX_SHIELDED;
            D.BudgetCost = 1;
            D.MinTierInt = 3;
            D.MinDanger = static_cast<uint8>(DT::Safe);
            P.Add(D);
        }
        {
            FMonsterAffixDef D;
            D.AffixTag = AFFIX_VORTEX;
            D.BudgetCost = 3;
            D.MinTierInt = 4;
            D.MinDanger = static_cast<uint8>(DT::Moderate);
            P.Add(D);
        }
        return P;
    }();
    return Pool;
}

FMonsterAffixGrantHandles FMonsterAffixGranter::GrantMonsterAffixes(UAbilitySystemComponent *NpcASC,
                                                                   TConstArrayView<FGameplayTag> AffixTags,
                                                                   const UMonsterAffixPool *PoolAsset) {
    FMonsterAffixGrantHandles Out;
    if (!NpcASC || !NpcASC->IsOwnerActorAuthoritative() || AffixTags.Num() == 0) {
        return Out;
    }

    const TArray<FMonsterAffixDef> &Defs =
        (PoolAsset && PoolAsset->Defs.Num() > 0) ? PoolAsset->Defs : UMonsterAffixPool::GetDefaultPool();

    AActor *Owner = NpcASC->GetOwnerActor();
    for (const FGameplayTag &Tag : AffixTags) {
        const FMonsterAffixDef *Def =
            Defs.FindByPredicate([&Tag](const FMonsterAffixDef &D) { return D.AffixTag.MatchesTagExact(Tag); });
        if (!Def) {
            continue;
        }

        if (Def->GrantedAbility) {
            const FGameplayAbilitySpec Spec(Def->GrantedAbility.GetDefaultObject(), 1, INDEX_NONE, Owner);
            const FGameplayAbilitySpecHandle H = NpcASC->GiveAbility(Spec);
            if (H.IsValid()) {
                Out.AbilityHandles.Add(H);
                NpcASC->TryActivateAbility(H);
            }
        }

        if (Def->GrantedGE) {
            FGameplayEffectContextHandle Ctx = NpcASC->MakeEffectContext();
            Ctx.AddSourceObject(Owner);
            const FGameplayEffectSpecHandle Spec = NpcASC->MakeOutgoingSpec(Def->GrantedGE, 1.0f, Ctx);
            if (Spec.IsValid()) {
                const FActiveGameplayEffectHandle AH = NpcASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
                if (AH.IsValid()) {
                    Out.EffectHandles.Add(AH);
                }
            }
        }
    }
    return Out;
}

void FMonsterAffixGranter::RemoveMonsterAffixes(UAbilitySystemComponent *NpcASC, FMonsterAffixGrantHandles &Handles) {
    if (NpcASC) {
        for (const FActiveGameplayEffectHandle &H : Handles.EffectHandles) {
            if (H.IsValid()) {
                NpcASC->RemoveActiveGameplayEffect(H);
            }
        }
        for (const FGameplayAbilitySpecHandle &H : Handles.AbilityHandles) {
            if (H.IsValid()) {
                NpcASC->ClearAbility(H);
            }
        }
    }
    Handles.Reset();
}
