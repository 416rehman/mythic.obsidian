#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "World/Harvesting/MythicHarvestTypes.h"

#include "MythicHarvestFocusComponent.generated.h"

class AMythicPlayerController;
class UDurabilityFragment;
class UEnhancedInputComponent;
class UEnhancedInputLocalPlayerSubsystem;
class UHarvestToolFragment;
class UInputAction;
class UInputComponent;
class UInputMappingContext;
class UMythicHarvestableDefinition;
class UMythicHarvestToolTypeDefinition;
class UMythicItemInstance;
class UMythicResourceISM;

/** Local presentation result of matching the focused definition against exact live inventory fragments. */
UENUM(BlueprintType)
enum class EMythicHarvestFocusAvailability : uint8 {
    None,
    Ready,
    EquipRequired,
    RequiresTool,
    ToolTierTooLow,
    ToolBroken,
    InvalidSource,
};

/**
 * Immutable local focus presentation built from cooked resource data and owner-replicated inventory state. Blueprint
 * may render this snapshot but cannot use it to authorize work, select a server target, or mutate an ISM instance.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicHarvestFocusPresentation {
    GENERATED_BODY()

    /**
     * Local focus component owns this presence flag for the owning client; Blueprint may read it without side effects,
     * false means every other field is presentation-defaulted, and the unitless value has no authority meaning.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Focus")
    bool bHasFocus = false;

    /**
     * Local focus component derives this tool-readiness state from owner-visible inventory fragments; Blueprint may
     * choose presentation only, invalid or duplicate sources fail closed, and the enum has no numeric units.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Focus")
    EMythicHarvestFocusAvailability Availability = EMythicHarvestFocusAvailability::None;

    /**
     * Local focus component formats this localized action line from direct definitions; Blueprint may display it with
     * no gameplay side effects, invalid focus produces empty text, and the value carries no authorization or units.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Focus")
    FText PromptText;

    /**
     * Settings owns this typed contextual action and the local focus component resolves it; Blueprint may pass it to
     * the platform glyph renderer only, null means contextual input is unavailable, and invoking it grants no server
     * authority or target identity.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Focus")
    TObjectPtr<UInputAction> ContextAction = nullptr;

    /**
     * Cooked resource component owns this exact definition reference; Blueprint may inspect presentation fields only,
     * null invalidates local focus without mutation, and no tag, path, name, or string fallback is attempted.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Focus")
    TObjectPtr<UMythicHarvestableDefinition> HarvestableDefinition = nullptr;

    /**
     * Focused definition owns this exact required family reference; Blueprint may render its name/icon only, null is
     * invalid and disables contextual activation, and no tag/name/string can substitute for exact asset identity.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Focus")
    TObjectPtr<UMythicHarvestToolTypeDefinition> RequiredToolType = nullptr;

    /**
     * Local exact-instance sweep supplies this stable presentation anchor in world centimeters; Blueprint may position
     * a widget without side effects, loss of the instance clears focus, and authority independently validates impact.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Focus")
    FVector AnchorLocation = FVector::ZeroVector;

    /**
     * Focused definition owns this minimum integer tool tier; Blueprint may display it without mutation, invalid
     * definition data fails local activation closed, and units are discrete authored tool tiers.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Focus")
    int32 RequiredToolTier = 0;

    /**
     * Deterministically selected exact candidate supplies this owner-visible tier; Blueprint may display it only,
     * -1 means no matching candidate, and units are discrete tool tiers.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Focus")
    int32 ResolvedToolTier = -1;

    /**
     * Local resolver sets this only when one exact gear-slotted tool answers the required family at sufficient tier;
     * Blueprint may style the prompt but cannot grant authority, false is fail-closed, and the flag is unitless.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Harvest|Focus")
    bool bCanHarvest = false;
};

/** Local-only notification emitted after the immutable focus presentation materially changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FMythicHarvestFocusChanged,
    const FMythicHarvestFocusPresentation &, Focus);

/** Native, ephemeral eligibility row used to test exact direct-reference tool selection without inventory mutation. */
struct MYTHIC_API FMythicHarvestToolEligibilityProbe {
    const UMythicHarvestToolTypeDefinition *ToolType = nullptr;
    FGuid ItemGuid;
    int32 ToolTier = 0;
    bool bInGearSlot = false;
    bool bHasDurabilityFragment = true;
    bool bBroken = false;
};

/** Pure local decision rules shared by runtime focus resolution and native automation tests. */
struct MYTHIC_API FMythicHarvestFocusRules {
    /**
     * OutSlottedToolIndex names only a tool the server would actually wear, so a carried tool never appears there.
     * OutEquipCandidateIndex names the carried tool the prompt may offer to move into its slot.
     */
    static EMythicHarvestFocusAvailability EvaluateToolSelection(
        const UMythicHarvestToolTypeDefinition *RequiredToolType,
        int32 RequiredToolTier,
        TConstArrayView<FMythicHarvestToolEligibilityProbe> Candidates,
        int32 &OutSlottedToolIndex,
        int32 &OutEquipCandidateIndex);
};

/**
 * Nonreplicated, owning-client focus and contextual-control component. It predicts one exact ISM instance, installs a
 * typed Enhanced Input context locally, and may only ask the server to move a carried tool into its own gear slot.
 */
UCLASS(NotBlueprintable, ClassGroup = (Mythic))
class MYTHIC_API UMythicHarvestFocusComponent final : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicHarvestFocusComponent();

    /**
     * Local focus component owns this read-only snapshot for its player controller; Blueprint may render it without
     * side effects, invalid/ambiguous data clears activation, and positions/tier units are documented on its fields.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Harvest|Focus")
    FMythicHarvestFocusPresentation CurrentFocus;

    /**
     * Local focus component broadcasts this on the owning client after a material snapshot change; Blueprint may
     * present the immutable DTO but cannot veto/alter gameplay, failures produce a cleared snapshot, and no RPC fires.
     */
    UPROPERTY(BlueprintAssignable, Category = "Harvest|Focus")
    FMythicHarvestFocusChanged OnFocusChanged;

    /** Binds the settings-owned typed action to this local component; native-only and creates no RPC or input tag. */
    void InitializeLocalInput(UInputComponent *InputComponent);

    /** Performs one immediate local exact-instance focus scan; native-only and never mutates resource gameplay. */
    void RefreshLocalFocus();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void ResolveInputAssets();
    void SetContextMappingActive(bool bActive);
    void HandleContextInteractStarted();
    void ClearFocus();
    void ApplyFocusedInstance(UMythicResourceISM &Resource,
                              FPrimitiveInstanceId PrimitiveInstanceId,
                              const FMythicHarvestNodeId &NodeId,
                              const FVector &AnchorLocation);
    void RefreshFocusedPresentation();
    bool RequestEquipFocusedTool();
    bool IsFocusedInstanceStillValid(FVector *OutAnchorLocation = nullptr) const;
    void RaiseUnavailableNotice(EMythicHarvestFocusAvailability Availability) const;

    UPROPERTY(Transient)
    TObjectPtr<UInputAction> ResolvedContextInteractAction = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UInputMappingContext> ResolvedContextMappingContext = nullptr;

    TWeakObjectPtr<AMythicPlayerController> OwnerController;
    TWeakObjectPtr<UEnhancedInputComponent> BoundInputComponent;
    TWeakObjectPtr<UEnhancedInputLocalPlayerSubsystem> InputSubsystem;
    TWeakObjectPtr<UMythicResourceISM> FocusedResource;
    FPrimitiveInstanceId FocusedPrimitiveInstanceId;
    FMythicHarvestNodeId FocusedNodeId;
    FVector FocusedAnchorLocation = FVector::ZeroVector;
    FTimerHandle FocusScanTimerHandle;
    uint32 InputBindingHandle = 0;
    double LastFocusedSeenTimeSeconds = -1.0;
    bool bInputBindingInstalled = false;
    bool bMappingContextInstalled = false;
    TWeakObjectPtr<class UMythicInventoryComponent> SelectedToolInventory;
    int32 SelectedToolSlotIndex = INDEX_NONE;
    FGuid SelectedToolItemGuid;
    bool bEquipRequestSent = false;
};
