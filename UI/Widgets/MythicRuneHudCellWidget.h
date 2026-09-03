// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ViewModels/MythicPlayerStatusViewModel.h"
#include "MythicRuneHudCellWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;
class UTextBlock;

/**
 * One pooled rune badge on the HUD: the rune's icon, a radial sweep for a timed window and a stack count. Every
 * call re-texts the authored children; nothing is added or removed after construction.
 */
UCLASS()
class MYTHIC_API UMythicRuneHudCellWidget : public UUserWidget {
    GENERATED_BODY()

public:
    void SetEntry(const FMythicRuneBadgeEntry &Entry);

    /** 0..1 of the radial sweep: what is left of an Active window, what has elapsed of a Cooldown. */
    void SetProgress(float Progress);

    void Clear();

    /** The radial is volatile only while a timer drives it, so Slate stops caching a frame that changes anyway. */
    void SetTicking(bool bTicking);

protected:
    virtual void NativeOnInitialized() override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Icon;

    /** Carries the radial material; its Progress scalar is the only thing driven here. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> Radial;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> Stacks;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD|Colours")
    FLinearColor ReadyTint = FLinearColor::White;

    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD|Colours")
    FLinearColor ActiveTint = FLinearColor::White;

    /** A rune on cooldown recedes rather than vanishes, so the player still knows it is worn. */
    UPROPERTY(EditDefaultsOnly, Category = "Mythic|HUD|Colours")
    FLinearColor CooldownTint = FLinearColor(0.35f, 0.35f, 0.35f, 1.0f);

private:
    const FLinearColor &TintFor(EMythicRuneHudState State) const;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> RadialMaterial;

    FSoftObjectPath ShownIcon;
    EMythicRuneHudState ShownState = EMythicRuneHudState::Hidden;
    int32 ShownStacks = INDEX_NONE;
    bool bTimed = false;
};
