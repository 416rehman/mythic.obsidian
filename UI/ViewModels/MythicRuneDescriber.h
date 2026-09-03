// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MythicRuneDescriber.generated.h"

class UMythicAchievementSet;
class UMythicRuneComponent;
class UMythicRuneDefinition;
class UMythicUnlockRuleSet;

/**
 * Turns a rune's authored text into what the picker and the socket tip draw, so the numbers a designer tunes on the
 * definition are the numbers the player reads. Nothing here is typed twice.
 */
UCLASS()
class MYTHIC_API UMythicRuneDescriber : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    /**
     * Rune->Description with every "<#Rune.Param.X>" drawn as "<Roll>value</><Context>[min-max]</>": Owner's roll
     * when it has one, the range midpoint when it does not (a rune not yet socketed, or no owner at all). A
     * placeholder the rune does not roll is left as written, so a typo reads on screen instead of vanishing.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Runes")
    static FText DescribeBehaviour(const UMythicRuneDefinition *Rune, const UMythicRuneComponent *Owner);

    /**
     * "Sealed - earn Wanderer: Find ten places nobody sent you to." for socket SlotIndex, resolved from the
     * Unlock.Rule.RuneSlot{N+1} rule's precondition and the achievement it names. A tag with no achievement
     * prints as the tag; a slot with no rule prints the rule id, never an empty string.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|Runes")
    static FText DescribeSealedSocket(int32 SlotIndex, const UMythicUnlockRuleSet *Rules,
                                      const UMythicAchievementSet *Achievements, FText &OutDeedName);

    /** "Unlock.Rule.RuneSlot{N+1}", the naming contract the unlock rules already follow. */
    static FString SocketRuleId(int32 SlotIndex);
};
