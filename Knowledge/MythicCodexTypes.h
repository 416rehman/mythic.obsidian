
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/MythicTags_GAS.h"
#include "Knowledge/MythicTags_Knowledge.h"
#include "MythicCodexTypes.generated.h"

UENUM(BlueprintType)
enum class EMythicCodexTier : uint8 {
    Unknown,
    Sighted,
    Basic,
    Full
};

USTRUCT(BlueprintType)
struct FMythicBestiaryRecord {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Knowledge|Bestiary")
    FGameplayTag Key;

    UPROPERTY(BlueprintReadWrite, Category = "Knowledge|Bestiary")
    int32 KillCount = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Knowledge|Bestiary")
    bool bEncountered = false;
};

struct FMythicBestiaryRules {
    static EMythicCodexTier TierForKills(int32 Kills, bool bEncountered, int32 T1, int32 T2) {
        T1 = FMath::Max(1, T1);
        T2 = FMath::Max(T1, T2);
        if (Kills >= T2) {
            return EMythicCodexTier::Full;
        }
        if (Kills >= T1) {
            return EMythicCodexTier::Basic;
        }
        if (bEncountered || Kills > 0) {
            return EMythicCodexTier::Sighted;
        }
        return EMythicCodexTier::Unknown;
    }

    static bool RevealsResistances(EMythicCodexTier Tier) {
        return Tier == EMythicCodexTier::Full;
    }

    static FGameplayTag MakeBestiaryKeyFromOwnedTags(const FGameplayTagContainer &OwnedTags) {
        static const FGameplayTag BestiaryRoot = FGameplayTag::RequestGameplayTag(FName("Codex.Bestiary"), false);
        if (BestiaryRoot.IsValid()) {
            for (const FGameplayTag &Tag : OwnedTags) {
                if (Tag != BestiaryRoot && Tag.MatchesTag(BestiaryRoot)) {
                    return Tag;
                }
            }
        }

        static const FGameplayTag NPCTypeRoot = FGameplayTag::RequestGameplayTag(FName("NPC.Type"), false);
        if (NPCTypeRoot.IsValid()) {
            const FString NPCTypePrefix = NPCTypeRoot.ToString() + TEXT(".");
            for (const FGameplayTag &Tag : OwnedTags) {
                if (Tag == NPCTypeRoot || !Tag.MatchesTag(NPCTypeRoot)) {
                    continue;
                }
                // Walk up from the most specific type until one has a codex page. A bestiary has a page for
                // Bandit, not for every variant of bandit, so NPC.Type.Bandit.Ambusher has to credit its parent
                // rather than fall all the way through to the generic humanoid entry.
                for (FGameplayTag Current = Tag; Current.IsValid() && Current != NPCTypeRoot;
                     Current = Current.RequestDirectParent()) {
                    const FString Suffix = Current.ToString().RightChop(NPCTypePrefix.Len());
                    const FGameplayTag Mapped = FGameplayTag::RequestGameplayTag(
                        FName(*(FString(TEXT("Codex.Bestiary.Humanoid.")) + Suffix)), false);
                    if (Mapped.IsValid()) {
                        return Mapped;
                    }
                }
                return CODEX_BESTIARY_HUMANOID_GENERIC.GetTag();
            }
        }

        if (OwnedTags.HasTag(AI_KIND_CREATURE)) {
            return CODEX_BESTIARY_CREATURE_GENERIC.GetTag();
        }
        if (OwnedTags.HasTag(AI_KIND_HUMANOID)) {
            return CODEX_BESTIARY_HUMANOID_GENERIC.GetTag();
        }
        return FGameplayTag();
    }
};
