
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "Templates/SubclassOf.h"
#include "Containers/ArrayView.h"
#include "Math/RandomStream.h"
#include "MonsterAffixTypes.generated.h"

class UGameplayEffect;
class UMythicGameplayAbility;

UE_DECLARE_GAMEPLAY_TAG_EXTERN(AFFIX_MOLTEN);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(AFFIX_SHIELDED);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(AFFIX_FROZEN);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(AFFIX_VORTEX);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(AFFIX_STATE_SHIELDED);

USTRUCT(BlueprintType)
struct FMonsterAffixDef {
    GENERATED_BODY()

    // Identity tag (Affix.*). Used to resolve this def when granting a selected affix, and for incompatibility matching.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
    FGameplayTag AffixTag;

    // Infinite/duration GameplayEffect granted to the enemy's own ASC (e.g. the Shielded bonus-MaxShield GE). Optional.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
    TSubclassOf<UGameplayEffect> GrantedGE;

    // Native GAS ability granted + activated on the enemy's ASC (e.g. the Molten burning-aura ability). Optional.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
    TSubclassOf<UMythicGameplayAbility> GrantedAbility;

    // How much of the enemy's affix budget this consumes. Must be >= 1 (a <= 0 cost is treated as ineligible so the
    // greedy fill can never loop forever / over-pick a free affix).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix", meta = (ClampMin = "1"))
    int32 BudgetCost = 1;

    // Affix tags this one cannot co-exist with (e.g. Molten excludes Frozen). Matching is SYMMETRIC in the selector:
    // if A lists B OR B lists A, they are never co-selected — authoring it on one side is enough.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
    FGameplayTagContainer IncompatibleWith;

    // Minimum enemy tier int (GetAITierInt: Normal=1..Boss=5) this affix may appear on. Defaults to Elite (3).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix", meta = (ClampMin = "1"))
    int32 MinTierInt = 3;

    // Minimum region danger (EMythicDangerTier as uint8: Safe=0..Extreme=4) this affix may appear at. Stored as a raw
    // uint8 so this header stays dependency-free (the enum lives in the World/LivingWorld danger module).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Affix")
    uint8 MinDanger = 0;
};

struct FMonsterAffixSelector {
    static int32 ComputeAffixBudget(int32 EnemyTierInt, uint8 Danger) {
        if (EnemyTierInt <= 2) {
            return 0;
        }
        int32 TierBudget;
        switch (EnemyTierInt) {
            case 3:  TierBudget = 2; break;
            case 4:  TierBudget = 4; break;
            default: TierBudget = 6; break;
        }
        return TierBudget + static_cast<int32>(Danger);
    }

    static TArray<FGameplayTag> Select(int32 EnemyTierInt, uint8 Danger, int32 Budget,
                                       TConstArrayView<FMonsterAffixDef> Pool, FRandomStream &Rng) {
        TArray<FGameplayTag> Picked;
        if (EnemyTierInt <= 1 || Budget <= 0 || Pool.Num() == 0) {
            return Picked;
        }

        TArray<int32> Eligible;
        Eligible.Reserve(Pool.Num());
        for (int32 i = 0; i < Pool.Num(); ++i) {
            const FMonsterAffixDef &Def = Pool[i];
            if (!Def.AffixTag.IsValid()) { continue; }
            if (EnemyTierInt < Def.MinTierInt) { continue; }
            if (Danger < Def.MinDanger) { continue; }
            if (Def.BudgetCost <= 0) { continue; }
            Eligible.Add(i);
        }

        for (int32 i = Eligible.Num() - 1; i > 0; --i) {
            const int32 j = Rng.RandRange(0, i);
            Eligible.Swap(i, j);
        }

        int32 Remaining = Budget;
        FGameplayTagContainer PickedTags;
        FGameplayTagContainer BlockedTags;
        for (const int32 Idx : Eligible) {
            const FMonsterAffixDef &Def = Pool[Idx];
            if (Def.BudgetCost > Remaining) { continue; }
            if (PickedTags.HasTagExact(Def.AffixTag)) { continue; }
            if (BlockedTags.HasTagExact(Def.AffixTag)) { continue; }
            if (Def.IncompatibleWith.HasAnyExact(PickedTags)) { continue; }

            Picked.Add(Def.AffixTag);
            PickedTags.AddTag(Def.AffixTag);
            BlockedTags.AppendTags(Def.IncompatibleWith);
            Remaining -= Def.BudgetCost;
            if (Remaining <= 0) { break; }
        }
        return Picked;
    }
};
