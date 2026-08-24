// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Styling/SlateBrush.h"
#include "MythicUIKit.generated.h"

class UMaterialInterface;

/** The interaction states a component can be asked for. Not every component answers all of them. */
UENUM(BlueprintType)
enum class EMythicUIState : uint8 {
    Normal,
    Hovered,
    Pressed,
    Disabled,
    Selected
};

/**
 * One entry in the kit: a thing you are allowed to draw, and what it is for.
 *
 * Purpose is not decoration. A catalogue that lists a hundred materials and says nothing about when to
 * reach for each one gets ignored, and the next person authors a flat colour instead - which is how the
 * settings scroll bar ended up hand-painted while three scroll-thumb materials already existed.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicUIComponent {
    GENERATED_BODY()

    /** Family.Variant, e.g. "Button.Primary", "Plate.Panel", "Scroll.Thumb". Unique across the kit. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Component")
    FName Id;

    /** When to reach for this one, in a sentence. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Component", meta = (MultiLine = true))
    FText Purpose;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "States")
    TSoftObjectPtr<UMaterialInterface> Normal;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "States")
    TSoftObjectPtr<UMaterialInterface> Hovered;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "States")
    TSoftObjectPtr<UMaterialInterface> Pressed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "States")
    TSoftObjectPtr<UMaterialInterface> Disabled;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "States")
    TSoftObjectPtr<UMaterialInterface> Selected;

    const TSoftObjectPtr<UMaterialInterface> &ForState(EMythicUIState State) const;
};

/**
 * The catalogue of everything the UI is allowed to draw.
 *
 * Generated from the material kit rather than typed by hand, so it cannot describe a kit that no longer
 * exists. Validation runs both ways: an entry pointing at missing art fails, and kit art that no entry
 * claims fails too - unclaimed art is how a second, undocumented style grows beside the first.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicUIKit : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kit")
    TArray<FMythicUIComponent> Components;

    /** Folder the catalogue is generated from and validated against. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Kit")
    FString KitFolder = TEXT("/Game/Mythic/UI/Globals/materials/kit");

    /** The kit named by Mythic UI Style settings, or null if none is configured. */
    static const UMythicUIKit *Get();

    const FMythicUIComponent *Find(FName Id) const;

    /**
     * A brush for a catalogued component. Returns a drawing-nothing brush and warns when the id is
     * unknown, so a typo shows up as a missing element plus a log line rather than the engine grey plate.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|UI")
    FSlateBrush MakeBrush(FName Id, EMythicUIState State = EMythicUIState::Normal,
                          FVector2D Size = FVector2D(8.0, 8.0)) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};
