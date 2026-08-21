
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Containers/ArrayView.h"
#include "World/LivingWorld/Chronicle/MythicWorldChronicleSubsystem.h"
#include "MythicDramatizerRules.generated.h"

USTRUCT(BlueprintType)
struct FMythicDramaTemplate {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dramatizer")
    FString TagPrefix;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dramatizer")
    FText Format;
};

USTRUCT(BlueprintType)
struct FMythicDramatizerConfig {
    GENERATED_BODY()

    /** Authored templates (checked before the code defaults; longest matching TagPrefix wins). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dramatizer")
    TArray<FMythicDramaTemplate> Templates;

    // Category weights for SelectDramaticEntries' score = entry.Significance × weight(tag). Defaults rank
    // war/faction upheaval above deaths above economy above everything else.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dramatizer", meta = (ClampMin = "0.0"))
    float WarWeight = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dramatizer", meta = (ClampMin = "0.0"))
    float DeathWeight = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dramatizer", meta = (ClampMin = "0.0"))
    float EconomyWeight = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dramatizer", meta = (ClampMin = "0.0"))
    float DefaultWeight = 1.0f;
};

struct FMythicDramatizerRules {
    static FString EventLeaf(const FGameplayTag &Tag) {
        if (!Tag.IsValid()) {
            return TEXT("World Event");
        }
        const FString Full = Tag.ToString();
        int32 LastDot = INDEX_NONE;
        if (Full.FindLastChar(TEXT('.'), LastDot) && LastDot + 1 < Full.Len()) {
            return Full.Mid(LastDot + 1);
        }
        return Full;
    }

    static const FMythicDramaTemplate *SelectTemplate(const FGameplayTag &EventTag, const FMythicDramatizerConfig &Config) {
        const FString TagString = EventTag.IsValid() ? EventTag.ToString() : FString();
        const FMythicDramaTemplate *Best = nullptr;
        int32 BestLen = -1;
        for (const FMythicDramaTemplate &Template : Config.Templates) {
            if (Template.TagPrefix.IsEmpty() || !TagString.StartsWith(Template.TagPrefix)) {
                continue;
            }
            if (Template.TagPrefix.Len() > BestLen) {
                Best = &Template;
                BestLen = Template.TagPrefix.Len();
            }
        }
        return Best;
    }

    static FText ComposeBeatText(const FGameplayTag &EventTag, const FText &ActorName, const FText &TargetName,
                                 float Magnitude, const FMythicDramatizerConfig &Config) {
        const FString Leaf = EventLeaf(EventTag);
        const FText SafeActor = ActorName.IsEmpty() ? NSLOCTEXT("Mythic", "DramaUnknownActor", "Unknown powers") : ActorName;
        const FText SafeTarget = TargetName.IsEmpty() ? NSLOCTEXT("Mythic", "DramaUnknownTarget", "the region") : TargetName;

        FFormatNamedArguments Args;
        Args.Add(TEXT("actor"), SafeActor);
        Args.Add(TEXT("target"), SafeTarget);
        Args.Add(TEXT("event"), FText::FromString(Leaf));
        Args.Add(TEXT("magnitude"), FText::AsNumber(FMath::RoundToInt(Magnitude * 100.0f) / 100.0f));

        if (const FMythicDramaTemplate *Template = SelectTemplate(EventTag, Config)) {
            return FText::Format(FTextFormat(Template->Format), Args);
        }
        if (!ActorName.IsEmpty()) {
            return FText::Format(FTextFormat(NSLOCTEXT("Mythic", "DramaDefaultActor", "{actor} — {event}")), Args);
        }
        return FText::Format(FTextFormat(NSLOCTEXT("Mythic", "DramaDefault", "Word spreads of {event}")), Args);
    }

    static float CategoryWeight(const FGameplayTag &EventTag, const FMythicDramatizerConfig &Config) {
        const FString Tag = EventTag.IsValid() ? EventTag.ToString() : FString();
        if (Tag.StartsWith(TEXT("LivingWorld.Event.Territory")) || Tag.StartsWith(TEXT("LivingWorld.Event.Diplomacy"))) {
            return Config.WarWeight;
        }
        if (Tag.StartsWith(TEXT("LivingWorld.Event.Death"))) {
            return Config.DeathWeight;
        }
        if (Tag.StartsWith(TEXT("LivingWorld.Event.Faction.Famine")) || Tag.StartsWith(TEXT("LivingWorld.Event.Economy"))) {
            return Config.EconomyWeight;
        }
        return Config.DefaultWeight;
    }

    static float ScoreEntry(const FMythicChronicleEntry &Entry, const FMythicDramatizerConfig &Config) {
        return Entry.Significance * CategoryWeight(Entry.EventTag, Config);
    }

    static TArray<FMythicChronicleEntry> SelectDramaticEntries(TConstArrayView<FMythicChronicleEntry> Entries,
                                                               int32 MaxCount, const FMythicDramatizerConfig &Config) {
        TArray<FMythicChronicleEntry> Result;
        if (MaxCount <= 0 || Entries.Num() == 0) {
            return Result;
        }
        Result.Append(Entries.GetData(), Entries.Num());
        Result.Sort([&Config](const FMythicChronicleEntry &A, const FMythicChronicleEntry &B) {
            const float ScoreA = ScoreEntry(A, Config);
            const float ScoreB = ScoreEntry(B, Config);
            if (ScoreA != ScoreB) {
                return ScoreA > ScoreB;
            }
            return A.Sequence > B.Sequence;
        });
        if (Result.Num() > MaxCount) {
            Result.SetNum(MaxCount, EAllowShrinking::No);
        }
        return Result;
    }
};
