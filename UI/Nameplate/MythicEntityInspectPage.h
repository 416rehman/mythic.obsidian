#pragma once

#include "CoreMinimal.h"
#include "UI/MythicActivatableWidget.h"
#include "UI/Nameplate/MythicEntityInspectTypes.h"

#include "MythicEntityInspectPage.generated.h"

class UMythicEntityInspectViewModel;
class UTextBlock;

/** Blueprint-skinnable CommonUI page for a viewer-earned entity dossier. */
UCLASS(Abstract, BlueprintType, Blueprintable)
class MYTHIC_API UMythicEntityInspectPage : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    /** Returns the page-owned viewer-safe Inspect view model. */
    UFUNCTION(BlueprintPure, Category = "Mythic|Entity Inspect")
    UMythicEntityInspectViewModel *GetInspectViewModel() const {
        return ViewModel;
    }

    /**
     * Replaces the current sanitized dossier and notifies Blueprint to rebuild its bounded visual rows. The page must
     * not query actors, PlayerState, LivingWorld, factions, GAS, or social graphs from the callback.
     */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Entity Inspect")
    void ApplyInspectProjection(
        const FMythicEntityInspectProjection &Projection);

    /** Called after the page-owned view model receives a coherent new dossier. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic|Entity Inspect")
    void OnInspectProjectionChanged();

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;

    /** Optional localized identity label populated directly from the sanitized Inspect DTO. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> NameText;

    /** Optional learned/public role or species label; collapsed while unknown. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> RoleText;

    /** Optional learned/presented faction label; collapsed while unknown. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> FactionText;

    /** Optional coarse relationship label; exact LivingWorld relation values never reach this widget. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> RelationshipText;

    /** Optional coarse faction-standing label; collapsed until learned by this local viewer. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> StandingText;

    /** Optional combat-owned, viewer-relative danger label. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ThreatText;

    /** Optional permissioned exact combat level; collapsed when authority withholds it. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LevelText;

    /** Optional bounded learned-traits section; no raw trait or simulation tags are rendered. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TraitsText;

    /** Optional bounded witnessed/learned-history section. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> HistoryText;

    /** Optional bounded learned positive-preferences section. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LikesText;

    /** Optional bounded learned negative-preferences section. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> DislikesText;

    /** Optional bounded learned-connections section; it never queries the raw social graph. */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ConnectionsText;

private:
    /** Applies the current sanitized DTO to optional native text bindings before Blueprint presentation hooks run. */
    void RefreshNativeBindings();

    /** Clears and collapses all optional native bindings when the CommonUI page is released. */
    void ResetNativeBindings();

    /** Page-owned DTO storage; never retains a gameplay actor or subsystem. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Mythic|Entity Inspect",
              meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UMythicEntityInspectViewModel> ViewModel;
};
