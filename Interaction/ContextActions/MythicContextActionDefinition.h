#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "MythicContextActionDefinition.generated.h"

class UTexture2D;

/** Determines which attention relationship must be current before an action can execute. */
UENUM(BlueprintType)
enum class EMythicContextActionFocusPolicy : uint8 {
    NotRequired UMETA(DisplayName = "Not Required"),
    FocusedSubject UMETA(DisplayName = "Focused Subject"),
    FocusedOrLockedSubject UMETA(DisplayName = "Focused or Locked Subject"),
    LockedSubject UMETA(DisplayName = "Locked Subject")
};

/** Determines where the authoritative distance rule for an action is sourced. */
UENUM(BlueprintType)
enum class EMythicContextActionRangePolicy : uint8 {
    NotRequired UMETA(DisplayName = "Not Required"),
    ProviderDefined UMETA(DisplayName = "Provider Defined"),
    DefinitionRange UMETA(DisplayName = "Definition Range")
};

/** Determines whether and from where the server must prove an unobstructed path to the subject. */
UENUM(BlueprintType)
enum class EMythicContextActionLineOfSightPolicy : uint8 {
    NotRequired UMETA(DisplayName = "Not Required"),
    ViewerPawnToSubject UMETA(DisplayName = "Viewer Pawn to Subject"),
    ViewerInteractionOriginToSubject UMETA(DisplayName = "Viewer Interaction Origin to Subject")
};

/** Player-facing meaning used by contextual presentation without parsing gameplay-tag names. */
UENUM(BlueprintType)
enum class EMythicContextActionPresentationSemantic : uint8 {
    /** Action with no more specific compact cue. */
    Other,

    /** Ordinary conversation capability; normally shown only on deliberate Focus. */
    Talk,

    /** Outward request or bark explicitly directed at this viewer. */
    DirectedTalk,

    /** Viewer-specific quest offer. */
    QuestOffer,

    /** Viewer-specific quest objective ready to turn in. */
    QuestTurnIn,

    /** Trade, repair, training, crafting, travel, or another service. */
    Service,

    /** Assist, rescue, revive, or another immediate help action. */
    Assist,
};

/** Controls whether an owner-only action offer may earn an overhead Context presentation. */
UENUM(BlueprintType)
enum class EMythicContextActionWorldPresentationPolicy : uint8 {
    /** Show the action only on the single deliberate Focus card. */
    FocusOnly,

    /** An available owner-only grant may earn a bounded Context opportunity lease. */
    ContextWhenAvailable,
};

/**
 * Canonical player-facing definition for one contextual entity action.
 *
 * Runtime offers and RPCs carry ActionTag, never this object pointer or localized text. Clients resolve the tag
 * through the Asset Manager, keeping presentation authoring local and network payloads compact.
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicContextActionDefinition : public UPrimaryDataAsset {
    GENERATED_BODY()

public:
    /** Primary Asset type used by Asset Manager scan rules and tag-to-definition registries. */
    static const FPrimaryAssetType PrimaryAssetType;

    /** Stable Context.Action.* identity sent in grants and execution requests; invalid tags make this asset unusable. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Identity",
              meta = (Categories = "Context.Action"))
    FGameplayTag ActionTag;

    /** Localized concise verb shown for an available action; an empty value falls back to the action tag in diagnostics only. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Presentation")
    FText DisplayName;

    /** Optional localized generic explanation used when a safe reason tag has no more specific localized mapping. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Presentation")
    FText UnavailableReasonFallback;

    /**
     * Localized player copy for safe Context.Action.Reason.* categories this action chooses to explain. Missing keys
     * use Unavailable Reason Fallback, so replicated reason tags never become raw player-facing strings.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Presentation")
    TMap<FGameplayTag, FText> UnavailableReasonTextOverrides;

    /** Optional localized progress label shown while holding; an empty value falls back to DisplayName. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Presentation")
    FText HoldProgressLabel;

    /** Soft player-facing icon loaded by the HUD; a null icon produces a text-and-input prompt without a placeholder asset. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Presentation")
    TSoftObjectPtr<UTexture2D> Icon;

    /** CommonUI UI.Action.* input identity used to resolve the current device glyph and local binding. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Input",
              meta = (Categories = "UI.Action"))
    FGameplayTag CommonUIInputActionTag;

    /** Relative authored sort weight; larger values win within the same semantic lane, in the inclusive range -1000..1000. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Presentation",
              meta = (ClampMin = "-1000", ClampMax = "1000", UIMin = "-100", UIMax = "100"))
    int32 PresentationPriority = 0;

    /** Authored compact meaning used for cue selection; consumers never infer semantics from ActionTag spelling. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Presentation")
    EMythicContextActionPresentationSemantic PresentationSemantic =
        EMythicContextActionPresentationSemantic::Other;

    /** Whether an available viewer-specific offer remains Focus-only or may earn a bounded Context opportunity. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Presentation")
    EMythicContextActionWorldPresentationPolicy WorldPresentationPolicy =
        EMythicContextActionWorldPresentationPolicy::FocusOnly;

    /** Required continuous hold duration in seconds; zero means tap, and values must be finite and nonnegative. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Input",
              meta = (ClampMin = "0.0", Units = "s"))
    float HoldDurationSeconds = 0.0f;

    /** Attention-state gate revalidated by the authoritative PlayerController before provider execution. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Validation")
    EMythicContextActionFocusPolicy FocusPolicy = EMythicContextActionFocusPolicy::FocusedSubject;

    /**
     * Maximum angle in degrees between the authoritative view direction and this subject when focus is required.
     * Smaller values demand more deliberate aim; this server-side value should match the action's intended UX.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Validation",
              meta = (EditCondition = "FocusPolicy != EMythicContextActionFocusPolicy::NotRequired",
                      EditConditionHides, ClampMin = "0.1", ClampMax = "90.0", Units = "deg"))
    float MaximumFocusAngleDegrees = 35.0f;

    /** Selects whether distance is unrestricted, provider-owned, or checked against MaximumRangeCentimeters. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Validation")
    EMythicContextActionRangePolicy RangePolicy = EMythicContextActionRangePolicy::DefinitionRange;

    /** Maximum authoritative subject distance in centimeters when RangePolicy is DefinitionRange; zero is invalid for that policy. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Validation",
              meta = (EditCondition = "RangePolicy == EMythicContextActionRangePolicy::DefinitionRange",
                      EditConditionHides, ClampMin = "0.0", Units = "cm"))
    float MaximumRangeCentimeters = 250.0f;

    /** Server line-of-sight origin policy; the controller uses the project's dedicated interaction trace channel. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Validation")
    EMythicContextActionLineOfSightPolicy LineOfSightPolicy =
        EMythicContextActionLineOfSightPolicy::ViewerPawnToSubject;

    /** When true, a safe UnavailableWithReason offer may remain visible; false makes unavailable offers project as hidden. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Context Action|Presentation")
    bool bExplainWhenUnavailable = true;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif
};
