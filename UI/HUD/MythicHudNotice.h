// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MythicHudNotice.generated.h"

UENUM(BlueprintType)
enum class EMythicNoticeKind : uint8 {
    Loot,
    Objective,
    Progression,
    Combat,
    Warning,
    Celebration,
    // First time a status is inflicted on the player. Read once, then never again — so it is the only kind whose
    // job is to be READ rather than glanced at.
    Status,
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicHudNotice {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Mythic|HUD")
    EMythicNoticeKind Kind = EMythicNoticeKind::Loot;

    /** The line itself, already formatted and localised by whoever raised it. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|HUD")
    FText Text;

    /** Second line, used where a beat has a payoff worth naming (the milestone behind a level-up). */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|HUD")
    FText Detail;

    /** Item rarity, objective state, refusal red. The presenter never invents a colour of its own. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|HUD")
    FLinearColor Accent = FLinearColor::White;

    /**
     * Identity for STACKING. Two notices raised close together with the same key merge instead of pushing a second
     * row, which is what stops "+1 Wood" nine times from becoming nine rows.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|HUD")
    FName StackKey;

    /** How many this notice represents after any merging. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|HUD")
    int32 Count = 1;

    /** For counted beats: the target `Count` is measured against ("2 / 5"). 0 = not a counted beat. */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|HUD")
    int32 Total = 0;

    /**
     * This beat CONCLUDES its subject: the objective finished, the hazard passed, the item was repaired. Presenters
     * that hold a row open until it resolves (the objective tracker) use this to know when to let it go. Never infer
     * this from the accent colour.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Mythic|HUD")
    bool bTerminal = false;
};

struct MYTHIC_API FMythicHudNoticeRules {
    static float LifetimeFor(EMythicNoticeKind Kind) {
        switch (Kind) {
            case EMythicNoticeKind::Status:
                // A sentence to read mid-fight, not a number to glance at.
                return 6.0f;
            case EMythicNoticeKind::Celebration:
                return 4.5f;
            case EMythicNoticeKind::Progression:
                return 3.5f;
            case EMythicNoticeKind::Warning:
                return 3.0f;
            case EMythicNoticeKind::Objective:
                return 3.0f;
            case EMythicNoticeKind::Combat:
                return 1.2f;
            case EMythicNoticeKind::Loot:
            default:
                return 2.5f;
        }
    }

    static bool CanMerge(const FMythicHudNotice &Existing, const FMythicHudNotice &Incoming) {
        return Existing.Kind == Incoming.Kind
               && !Incoming.StackKey.IsNone()
               && Existing.StackKey == Incoming.StackKey;
    }

    static bool GoesToFeed(EMythicNoticeKind Kind) {
        return Kind == EMythicNoticeKind::Loot
               || Kind == EMythicNoticeKind::Combat
               || Kind == EMythicNoticeKind::Warning;
    }

    static bool GoesToBanner(EMythicNoticeKind Kind) {
        return Kind == EMythicNoticeKind::Progression || Kind == EMythicNoticeKind::Celebration;
    }
};
