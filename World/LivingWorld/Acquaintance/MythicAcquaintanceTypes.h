
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "MythicAcquaintanceTypes.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_NPC_WARMTH_HOSTILE)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_NPC_WARMTH_WARY)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_NPC_WARMTH_STRANGER)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_NPC_WARMTH_ACQUAINTANCE)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_NPC_WARMTH_FRIEND)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_NPC_WARMTH_CONFIDANT)

UENUM(BlueprintType)
enum class EMythicNpcInteraction : uint8 {
    Met,
    Traded,
    QuestHelped,
    Saved,
    Attacked,
    Killed,
    KilledKin
};

UENUM(BlueprintType)
enum class EMythicWarmthTier : uint8 {
    Hostile,
    Wary,
    Stranger,
    Acquaintance,
    Friend,
    Confidant
};

enum class EMythicNpcRelationFlags : uint32 {
    None = 0,
    Met = 1 << 0,
    Traded = 1 << 1,
    QuestHelped = 1 << 2,
    Saved = 1 << 3,
    Wronged = 1 << 4,
};
ENUM_CLASS_FLAGS(EMythicNpcRelationFlags)

USTRUCT(BlueprintType)
struct FMythicNpcRelation {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    uint32 NpcNameHash = 0;

    // Affiliation of the NPC (AI.Affiliation.*) at last contact — grudges outlive the pawn, so cache the tag.
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Acquaintance")
    FGameplayTag Faction;

    // Accumulated warmth toward the player, -100 (blood grudge) .. +100 (devoted). Decays LAZILY toward 0.
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Acquaintance")
    float Warmth = 0.0f;

    UPROPERTY(SaveGame)
    uint32 Flags = 0;

    UPROPERTY(SaveGame)
    double LastInteractionTime = 0.0;

    bool HasFlag(EMythicNpcRelationFlags Flag) const { return (Flags & static_cast<uint32>(Flag)) != 0; }
};

USTRUCT(BlueprintType)
struct FMythicAcquaintanceConfig {
    GENERATED_BODY()

    /** Hard cap on remembered NPCs per player (LRU eviction beyond it — a person only holds so many faces). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquaintance", meta = (ClampMin = "1"))
    int32 MaxRelations = 64;

    /** Warmth decayed toward neutral (0) per in-world day of no contact. 0 disables decay. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquaintance", meta = (ClampMin = "0.0"))
    float DecayPerDay = 1.0f;

    /** World-seconds per in-world "day" for the decay rate (matches the cemetery's epitaph day-length default). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquaintance", meta = (ClampMin = "1.0"))
    float SecondsPerWorldDay = 1200.0f;

    // Per-interaction warmth deltas.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquaintance") float MetDelta = 2.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquaintance") float TradedDelta = 5.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquaintance") float QuestHelpedDelta = 15.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquaintance") float SavedDelta = 30.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquaintance") float AttackedDelta = -40.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquaintance") float KilledDelta = -80.0f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Acquaintance") float KilledKinDelta = -60.0f;
};

struct FMythicAcquaintanceRules {
    static constexpr float WarmthMin = -100.0f;
    static constexpr float WarmthMax = 100.0f;

    static float WarmthDeltaFor(EMythicNpcInteraction Interaction, const FMythicAcquaintanceConfig &Config) {
        switch (Interaction) {
        case EMythicNpcInteraction::Met: return Config.MetDelta;
        case EMythicNpcInteraction::Traded: return Config.TradedDelta;
        case EMythicNpcInteraction::QuestHelped: return Config.QuestHelpedDelta;
        case EMythicNpcInteraction::Saved: return Config.SavedDelta;
        case EMythicNpcInteraction::Attacked: return Config.AttackedDelta;
        case EMythicNpcInteraction::Killed: return Config.KilledDelta;
        case EMythicNpcInteraction::KilledKin: return Config.KilledKinDelta;
        default: return 0.0f;
        }
    }

    static EMythicNpcRelationFlags FlagFor(EMythicNpcInteraction Interaction) {
        switch (Interaction) {
        case EMythicNpcInteraction::Met: return EMythicNpcRelationFlags::Met;
        case EMythicNpcInteraction::Traded: return EMythicNpcRelationFlags::Traded;
        case EMythicNpcInteraction::QuestHelped: return EMythicNpcRelationFlags::QuestHelped;
        case EMythicNpcInteraction::Saved: return EMythicNpcRelationFlags::Saved;
        case EMythicNpcInteraction::Attacked:
        case EMythicNpcInteraction::Killed:
        case EMythicNpcInteraction::KilledKin: return EMythicNpcRelationFlags::Wronged;
        default: return EMythicNpcRelationFlags::None;
        }
    }

    static float DecayedWarmth(float Warmth, double LastInteractionTime, double Now, float DecayPerDay, float SecondsPerWorldDay) {
        if (DecayPerDay <= 0.0f || SecondsPerWorldDay <= 0.0f) {
            return Warmth;
        }
        const double Elapsed = Now - LastInteractionTime;
        if (Elapsed <= 0.0) {
            return Warmth;
        }
        const float Decay = DecayPerDay * static_cast<float>(Elapsed / SecondsPerWorldDay);
        if (Warmth > 0.0f) {
            return FMath::Max(0.0f, Warmth - Decay);
        }
        if (Warmth < 0.0f) {
            return FMath::Min(0.0f, Warmth + Decay);
        }
        return 0.0f;
    }

    static FMythicNpcRelation *FindRelation(TArray<FMythicNpcRelation> &Relations, uint32 NpcNameHash) {
        return Relations.FindByPredicate([NpcNameHash](const FMythicNpcRelation &R) { return R.NpcNameHash == NpcNameHash; });
    }
    static const FMythicNpcRelation *FindRelation(const TArray<FMythicNpcRelation> &Relations, uint32 NpcNameHash) {
        return Relations.FindByPredicate([NpcNameHash](const FMythicNpcRelation &R) { return R.NpcNameHash == NpcNameHash; });
    }

    static float ApplyInteraction(TArray<FMythicNpcRelation> &Relations, uint32 NpcNameHash, FGameplayTag Faction,
                                  EMythicNpcInteraction Interaction, double Now, const FMythicAcquaintanceConfig &Config) {
        if (NpcNameHash == 0) {
            return 0.0f;
        }
        FMythicNpcRelation *Rel = FindRelation(Relations, NpcNameHash);
        if (!Rel) {
            const int32 Cap = FMath::Max(1, Config.MaxRelations);
            while (Relations.Num() >= Cap) {
                int32 OldestIdx = 0;
                for (int32 i = 1; i < Relations.Num(); ++i) {
                    if (Relations[i].LastInteractionTime < Relations[OldestIdx].LastInteractionTime) {
                        OldestIdx = i;
                    }
                }
                Relations.RemoveAt(OldestIdx, 1, EAllowShrinking::No);
            }
            FMythicNpcRelation NewRel;
            NewRel.NpcNameHash = NpcNameHash;
            Rel = &Relations[Relations.Add(NewRel)];
        }

        Rel->Warmth = DecayedWarmth(Rel->Warmth, Rel->LastInteractionTime, Now, Config.DecayPerDay, Config.SecondsPerWorldDay);
        Rel->Warmth = FMath::Clamp(Rel->Warmth + WarmthDeltaFor(Interaction, Config), WarmthMin, WarmthMax);
        Rel->Flags |= static_cast<uint32>(FlagFor(Interaction));
        Rel->LastInteractionTime = Now;
        if (Faction.IsValid()) {
            Rel->Faction = Faction;
        }
        return Rel->Warmth;
    }

    static EMythicWarmthTier WarmthTier(float Warmth) {
        if (Warmth <= -50.0f) { return EMythicWarmthTier::Hostile; }
        if (Warmth <= -15.0f) { return EMythicWarmthTier::Wary; }
        if (Warmth < 15.0f) { return EMythicWarmthTier::Stranger; }
        if (Warmth < 45.0f) { return EMythicWarmthTier::Acquaintance; }
        if (Warmth < 80.0f) { return EMythicWarmthTier::Friend; }
        return EMythicWarmthTier::Confidant;
    }

    static FGameplayTag TagForTier(EMythicWarmthTier Tier) {
        switch (Tier) {
        case EMythicWarmthTier::Hostile: return TAG_NPC_WARMTH_HOSTILE.GetTag();
        case EMythicWarmthTier::Wary: return TAG_NPC_WARMTH_WARY.GetTag();
        case EMythicWarmthTier::Acquaintance: return TAG_NPC_WARMTH_ACQUAINTANCE.GetTag();
        case EMythicWarmthTier::Friend: return TAG_NPC_WARMTH_FRIEND.GetTag();
        case EMythicWarmthTier::Confidant: return TAG_NPC_WARMTH_CONFIDANT.GetTag();
        case EMythicWarmthTier::Stranger:
        default: return TAG_NPC_WARMTH_STRANGER.GetTag();
        }
    }
};
