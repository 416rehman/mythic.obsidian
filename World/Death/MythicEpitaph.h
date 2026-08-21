
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MythicEpitaph.generated.h"

USTRUCT(BlueprintType)
struct FMythicEpitaphTemplate {
    GENERATED_BODY()

    // Role this template speaks to (e.g. NPC.Role.Noble). Invalid = wildcard (matches any role). A template keyed on a
    // PARENT role (NPC.Role.Merchant) also matches a more specific dead role (NPC.Role.Merchant.Blacksmith).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Epitaph", meta = (Categories = "NPC.Role"))
    FGameplayTag RoleTag;

    // Affiliation/faction this template speaks to (e.g. AI.Affiliation.*). Invalid = wildcard (matches any faction).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Epitaph")
    FGameplayTag Faction;

    // The epitaph body with {name}/{role}/{faction}/{day} tokens (see the struct note). FText so it is localizable.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Epitaph")
    FText BodyFormat;
};

struct FMythicEpitaph {
    static FString TagLeaf(const FGameplayTag &Tag) {
        if (!Tag.IsValid()) {
            return FString();
        }
        const FString Full = Tag.GetTagName().ToString();
        int32 DotIndex = INDEX_NONE;
        if (Full.FindLastChar(TEXT('.'), DotIndex)) {
            return Full.RightChop(DotIndex + 1);
        }
        return Full;
    }

    static FText Compose(const FMythicEpitaphTemplate &Template, FName Name, FGameplayTag Role, FGameplayTag Faction, int32 WorldDay) {
        FString Result = Template.BodyFormat.ToString();
        Result = Result.Replace(TEXT("{name}"), *Name.ToString(), ESearchCase::IgnoreCase);
        Result = Result.Replace(TEXT("{role}"), *TagLeaf(Role), ESearchCase::IgnoreCase);
        Result = Result.Replace(TEXT("{faction}"), *TagLeaf(Faction), ESearchCase::IgnoreCase);
        Result = Result.Replace(TEXT("{day}"), *FString::FromInt(WorldDay), ESearchCase::IgnoreCase);
        return FText::FromString(Result);
    }

    static int32 SelectEpitaphTemplate(FGameplayTag Role, FGameplayTag Faction, TConstArrayView<FMythicEpitaphTemplate> Templates) {
        int32 BestIndex = -1;
        int32 BestScore = -1;
        for (int32 i = 0; i < Templates.Num(); ++i) {
            const FMythicEpitaphTemplate &T = Templates[i];

            if (T.RoleTag.IsValid() && !(Role.IsValid() && Role.MatchesTag(T.RoleTag))) {
                continue;
            }
            if (T.Faction.IsValid() && !(Faction.IsValid() && Faction.MatchesTag(T.Faction))) {
                continue;
            }

            const int32 Score = (T.RoleTag.IsValid() ? 2 : 0) + (T.Faction.IsValid() ? 1 : 0);
            if (Score > BestScore) {
                BestScore = Score;
                BestIndex = i;
            }
        }
        return BestIndex;
    }
};
