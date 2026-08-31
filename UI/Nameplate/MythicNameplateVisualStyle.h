#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Styling/SlateTypes.h"
#include "UI/MythicRichTextIconLibrary.h"
#include "UI/Nameplate/MythicNameplateTypes.h"

#include "MythicNameplateVisualStyle.generated.h"

class UTexture2D;
class UDataTable;

/** Fixed 1080p geometry for one constrained nameplate density profile. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateProfileGeometry {
    GENERATED_BODY()

    /** Maximum logical-pixel bounds before local accessibility scaling. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Geometry")
    FVector2D MaximumSize = FVector2D(240.0f, 54.0f);

    /** Identity font size in logical pixels. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Geometry",
              meta = (ClampMin = "8", ClampMax = "32"))
    int32 IdentityFontSize = 17;

    /** Width of the tactical health band; zero collapses the band. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Geometry",
              meta = (ClampMin = "0.0", ClampMax = "420.0"))
    float HealthBandWidth = 184.0f;

    /** Height of the tactical health band; zero collapses the band. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Geometry",
              meta = (ClampMin = "0.0", ClampMax = "12.0"))
    float HealthBandHeight = 6.0f;
};

/** One resident, color-and-shape-redundant visual token rendered by a fixed nameplate icon slot. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateIconToken {
    GENERATED_BODY()

    /** Stable typed semantic whose Rich Text row is resolved centrally without an authored string identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icon")
    EMythicSemanticIcon SemanticIcon = EMythicSemanticIcon::None;

    /** Strong local texture reference loaded with the visual style; widgets never stream or resolve an asset path. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icon")
    TObjectPtr<UTexture2D> Texture;

    /** Semantic tint paired with the texture silhouette so color is never the only carrier of meaning. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icon")
    FLinearColor Tint = FLinearColor::White;

    /** Fixed logical-pixel bounds used by the pre-authored icon slot. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icon",
              meta = (ClampMin = "8.0", ClampMax = "32.0"))
    FVector2D LogicalSize = FVector2D(16.0f, 16.0f);

    /** Returns true when the style owns a renderable resident texture and valid bounded geometry. */
    bool IsRenderable() const;
};

/** Typed binding from an already-sanitized contextual cue to its local visual token. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateCueIconBinding {
    GENERATED_BODY()

    /** Cue semantic produced by native disclosure and precedence rules; None is not a valid authored binding. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icon")
    EMythicNameplatePrimaryCue Cue = EMythicNameplatePrimaryCue::None;

    /** Resident texture, semantic tint, and bounded geometry for this cue. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icon")
    FMythicNameplateIconToken Icon;
};

/** Typed binding from an authority-redacted combat rank to its local visual token. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateRankIconBinding {
    GENERATED_BODY()

    /** Presented rank semantic; Unknown and Standard intentionally have no authored icon. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icon")
    EMythicPresentedCombatRank Rank = EMythicPresentedCombatRank::Unknown;

    /** Resident texture, semantic tint, and bounded geometry for this rank. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icon")
    FMythicNameplateIconToken Icon;
};

/** Typed binding from a viewer-relative threat band to its local visual token. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicNameplateThreatIconBinding {
    GENERATED_BODY()

    /** Sanitized danger semantic; Unknown and None intentionally have no authored icon. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icon")
    EMythicThreatBand Threat = EMythicThreatBand::Unknown;

    /** Resident texture, semantic tint, and bounded geometry for this threat band. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Icon")
    FMythicNameplateIconToken Icon;
};

/**
 * Project-wide visual tokens for the contextual entity plate and action rail.
 * Gameplay disclosure, target selection, labels, ranks, statuses, and action truth never live in this asset.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicNameplateVisualStyle : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    UMythicNameplateVisualStyle();

    /** Approved composite font for the dominant one-line identity label. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Typography")
    FSlateFontInfo IdentityFont;

    /** Approved composite font for compact subtitle, level, status, and action text. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Typography")
    FSlateFontInfo SecondaryFont;

    /** Quiet one-line identity profile used by Whisper. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profiles")
    FMythicNameplateProfileGeometry IdentityProfile;

    /** Compact contextual profile used by nonfocused safety and opportunity reads. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profiles")
    FMythicNameplateProfileGeometry ContextProfile;

    /** Focused Standard or Superior combat profile. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profiles")
    FMythicNameplateProfileGeometry FocusCombatProfile;

    /** Focused Elite or Champion combat profile. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profiles")
    FMythicNameplateProfileGeometry EliteCombatProfile;

    /** Authority-presented Boss overhead profile. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profiles")
    FMythicNameplateProfileGeometry BossProfile;

    /** Maximum logical-pixel bounds of the one-line contextual action rail. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Profiles")
    FVector2D ActionRailMaximumSize = FVector2D(280.0f, 28.0f);

    /** Dominant identity and primary token color. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
    FLinearColor PrimaryTextColor = FLinearColor(0.90f, 0.86f, 0.79f, 1.0f);

    /** Subordinate subtitle, level, and overflow color. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
    FLinearColor SecondaryTextColor = FLinearColor(0.61f, 0.58f, 0.52f, 1.0f);

    /** Standard health fill color; relation is never inferred from this token. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
    FLinearColor HealthFillColor = FLinearColor(0.55f, 0.055f, 0.071f, 1.0f);

    /** Protected-ally health fill color used only after ally safety is already entitled. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
    FLinearColor AllyHealthFillColor = FLinearColor(0.30f, 0.57f, 0.37f, 1.0f);

    /** Boss/rank accent color paired with a non-color shape token. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Palette")
    FLinearColor RankAccentColor = FLinearColor(0.86f, 0.61f, 0.19f, 1.0f);

    /** Explicit hard-target chevron; ordinary gaze and Focus never render this token. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Semantic Icons")
    FMythicNameplateIconToken HardTargetIcon;

    /** Explicit combat-lock chevron; ordinary gaze, soft targeting, and interaction never render this token. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Semantic Icons")
    FMythicNameplateIconToken LockedTargetIcon;

    /** Typed local bindings for the single primary contextual cue; gameplay never supplies texture references. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Semantic Icons")
    TArray<FMythicNameplateCueIconBinding> CueIcons;

    /** Typed local bindings for Superior through WorldBoss authority-presented rank emblems. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Semantic Icons")
    TArray<FMythicNameplateRankIconBinding> RankIcons;

    /** Typed local bindings for Risky through Overwhelming viewer-relative danger marks. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Semantic Icons")
    TArray<FMythicNameplateThreatIconBinding> ThreatIcons;

    /** Resolves fixed geometry from already-derived disclosure, family, and presented rank. */
    const FMythicNameplateProfileGeometry &ResolveGeometry(
        EMythicNameplateDisclosureTier Disclosure,
        EMythicNameplateVisualFamily Family,
        EMythicPresentedCombatRank Rank) const;

    /** Resolves the local resident visual token for one viewer-safe primary cue, or null when the cue stays silent. */
    const FMythicNameplateIconToken *ResolveCueIcon(
        EMythicNameplatePrimaryCue Cue) const;

    /** Resolves a target token only for explicit hard or locked combat target states. */
    const FMythicNameplateIconToken *ResolveTargetIcon(
        EMythicNameplateAttentionState AttentionState) const;

    /** Resolves the local resident emblem for an authority-presented rank, or null for Unknown and Standard. */
    const FMythicNameplateIconToken *ResolveRankIcon(
        EMythicPresentedCombatRank Rank) const;

    /** Resolves the local resident danger mark for a viewer-relative threat band, or null when no warning is earned. */
    const FMythicNameplateIconToken *ResolveThreatIcon(
        EMythicThreatBand Threat) const;

private:
    /** Strong reference to the single project Rich Text image set used to validate fixed-slot parity. */
    UPROPERTY()
    TObjectPtr<UDataTable> RichTextImageSet;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(
        FDataValidationContext &Context) const override;
#endif
};
