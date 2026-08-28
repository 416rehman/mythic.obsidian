// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Itemization/MythicDataAsset.h"
#include "Stats/MythicStatTypes.h"
#include "MythicStatCategoryDefinition.generated.h"

/** Canonical Primary Data Asset defining one stat-sheet category's identity, order, and presentation style. */
UCLASS(BlueprintType)
class MYTHIC_API UMythicStatCategoryDefinition : public UMythicDataAsset {
    GENERATED_BODY()

public:
    /** Stable, non-localized name used by designers, validation logs, and content tooling. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FName DeveloperName;

    /** Internal explanation of the category's gameplay and presentation purpose. Never shown to players. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = true))
    FString DesignerPurpose;

    /** Version of gameplay-relevant category semantics, incremented when those semantics change. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Identity")
    int32 Revision = 1;

    /** Version of player-facing category presentation, incremented when display data changes. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Identity")
    int32 PresentationRevision = 1;

    /** Canonical semantic tag used to group stats into this category at runtime. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stat", meta = (Categories = "Stat.Category"))
    FGameplayTag CategoryTag;

    /** Localized heading shown for this category on the player stat sheet. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
    FText DisplayName;

    /** Ascending order used to arrange categories on the stat sheet. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
    int32 SheetOrder = 0;

    /** Data-authored visual and interaction treatment applied to every row in this category. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Presentation")
    FMythicStatCategoryStyle Style;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

    /** Runtime-safe local validation used by both editor validation and the compiled registry. */
    bool AppendValidationErrors(TArray<FText>& OutErrors) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
