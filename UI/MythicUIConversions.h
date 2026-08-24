// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Styling/SlateColor.h"
#include "MythicUIConversions.generated.h"

UCLASS()
class MYTHIC_API UMythicUIConversions : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    /** The plain colour inside an FSlateColor. Returns the specified colour; a "use foreground" slate colour has none. */
    UFUNCTION(BlueprintPure, Category = "Mythic|UI", meta = (DisplayName = "Slate Color To Linear Color"))
    static FLinearColor SlateColorToLinear(FSlateColor Color);

    /**
     * A rarity colour dimmed to the strength a slot backing wants.
     *
     * Full-strength rarity would flood the square and fight the item icon on top of it. The rarity is meant to be
     * read from the corner of your eye, not stared at.
     */
    UFUNCTION(BlueprintPure, Category = "Mythic|UI", meta = (DisplayName = "Slot Backing Tint"))
    static FLinearColor SlotBackingTint(FSlateColor RarityColor);

    /** The slot's display brush from its view model's already-resolved icon; no-draw while the slot is bare. */
    UFUNCTION(BlueprintPure, Category = "Mythic|UI", meta = (DisplayName = "Slot Icon Brush"))
    static FSlateBrush SlotIconBrush(class UItemSlotVM *SlotVM);
};
