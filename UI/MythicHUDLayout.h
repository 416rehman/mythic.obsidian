
#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameplayTagContainer.h"
#include "MythicActivatableWidget.h"
#include "MythicHUDSalience.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "MythicHUDLayout.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHUDRevealChanged, bool);

class UMythicMenuShell;
class UMythicNameplateDirector;
class UMythicNameplateLayer;
class UNamedSlot;

USTRUCT(BlueprintType)
struct FMythicMenuHotkey {
    GENERATED_BODY()

    /** The CommonUI action this key is bound to (UI.Action.Inventory and friends). */
    UPROPERTY(EditDefaultsOnly, Category = "Menu", meta = (Categories = "UI.Action"))
    FGameplayTag ActionTag;

    /** Which page of the shell it opens. Must match a PageId in the shell's Pages array. */
    UPROPERTY(EditDefaultsOnly, Category = "Menu")
    FName PageId;
};

USTRUCT(BlueprintType)
struct FMythicHUDElementRule {
    GENERATED_BODY()

    /** Where it sits when nothing is happening. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
    EMythicHUDSalience Resting = EMythicHUDSalience::Dim;

    /** Where it goes while the player is in a fight (GAS.State.InCombat). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
    EMythicHUDSalience InCombat = EMythicHUDSalience::Lit;

    /**
     * How long PokeElement keeps it Lit before it falls back to the rule. This is what makes a compass flare when a
     * landmark is found and then settle, instead of either shouting forever or never being noticed.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD", meta = (ClampMin = "0.0"))
    float ActivityHoldSeconds = 4.0f;
};

USTRUCT(BlueprintType)
struct FMythicHUDElementBinding {
    GENERATED_BODY()

    /** Name of the widget inside this HUD. Resolved once on init; a name that matches nothing is skipped. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
    FName WidgetName;

    /** Contextual resting, combat, and activity-hold behavior applied to the named HUD widget. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD")
    FMythicHUDElementRule Rule;
};

UCLASS(Abstract, BlueprintType, Blueprintable)
class MYTHIC_API UMythicHUDLayout : public UMythicActivatableWidget {
    GENERATED_BODY()

public:
    UMythicHUDLayout(const FObjectInitializer &ObjectInitializer);


    void NativeOnInitialized() override;
    virtual void NativeDestruct() override;

public:
    // Return true to skip the menu opening
    UFUNCTION(BlueprintImplementableEvent, Category = MythicHUDLayout)
    bool PreEscapeMenuOpen();

    /** Public so Mythic.OpenMenu Escape can reach it; a UI change that cannot be opened cannot be checked. */
    void HandleEscapeAction();

protected:

    UPROPERTY(EditDefaultsOnly)
    TSoftClassPtr<UCommonActivatableWidget> EscapeMenuClass;

    // ---- Tabbed menu shell ----
    /** The one screen behind Inventory, Character, Proficiencies, Map and the rest. */
    UPROPERTY(EditDefaultsOnly, Category = "Menu")
    TSubclassOf<UMythicMenuShell> MenuShellClass;

    /** Which key lands on which page. Adding a hotkey is a row here, not new code. */
    UPROPERTY(EditDefaultsOnly, Category = "Menu")
    TArray<FMythicMenuHotkey> MenuHotkeys;

    /**
     * The bare "open the menu" key (Tab). Lands on the leftmost tab rather than a named page, so Tab is the way in
     * when you do not already know what you are looking for, and the per-system keys are the shortcuts when you do.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Menu", meta = (Categories = "UI.Action"))
    FGameplayTag OpenMenuAction;

public:
    /** Open the shell on a page, or switch pages if it is already up. Public: the pause screen's Settings row
     *  goes through here too, so there is one way into the menu. */
    UFUNCTION(BlueprintCallable, Category = "Menu")
    void OpenMenuOnPage(FName PageId);

    /** Read-only, for the validator that checks every hotkey lands on a page the shell actually has. */
    const TArray<FMythicMenuHotkey> &GetMenuHotkeys() const { return MenuHotkeys; }

protected:
    /**
     * Route the existing Inventory key into the shell. Bound in C++ only when bRouteInventoryToShell is set, so the
     * project can keep its old Blueprint-driven inventory until the shell fully replaces it.
     */
    UPROPERTY(EditDefaultsOnly, Category = "Menu")
    bool bRouteInventoryToShell = false;

    UPROPERTY(EditDefaultsOnly, Category = "Menu")
    FName InventoryPageId = TEXT("Inventory");

    // ---- Lending the inventory to the Character tab ----
    /**
     * The HUD panel the Blueprint parks the inventory widget in. Bound by name, so the existing WBP_PlayerHUD needs no
     * edit.
     *
     * Typed UNamedSlot, not UPanelWidget: binding by name REPLACES the Blueprint's own variable of that name, and the
     * HUD graph already passes it around as a NamedSlot. A looser type here compiles the C++ fine and breaks every
     * pin in the Blueprint.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Menu", meta = (BindWidgetOptional))
    TObjectPtr<UNamedSlot> InventorySlot;

    /**
     * Optional HUD-owned slot for the one local-player nameplate layer. The Blueprint supplies layout/z-order; C++
     * creates at most one configured layer after the HUD exists. Null safely disables nameplates for that HUD.
     */
    UPROPERTY(BlueprintReadOnly, Category = "World Presentation", meta = (BindWidgetOptional))
    TObjectPtr<UNamedSlot> WorldOverlaySlot;

    /** Native/Blueprint layer class created once inside WorldOverlaySlot; null leaves the slot intentionally empty. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Presentation")
    TSubclassOf<UMythicNameplateLayer> NameplateLayerClass;

    /** Returns the live local-player layer, or null when the HUD has no slot/class or creation failed. */
    UFUNCTION(BlueprintPure, Category = "World Presentation")
    UMythicNameplateLayer *GetNameplateLayer() const { return NameplateLayer; }

public:
    /**
     * Hand the live inventory widget over to a caller (the Character tab).
     *
     * The HUD creates that widget once and calls SetInventoryVM on it. A second copy embedded in a menu page never
     * gets that call, which is exactly how the inventory came up empty last time. So the menu borrows the one that
     * works instead of building its own.
     *
     * Returns null when there is nothing to lend.
     */
    UFUNCTION(BlueprintCallable, Category = "Menu")
    UWidget *BorrowInventoryWidget();

    /** Put the borrowed widget back in the HUD, closed. */
    UFUNCTION(BlueprintCallable, Category = "Menu")
    void ReturnInventoryWidget(UWidget *Widget);


    /** Start driving this element's opacity. Idempotent; a second call just updates the entry. */
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void RegisterHUDElement(UWidget *Element, EMythicHUDSalience InitialSalience = EMythicHUDSalience::Hidden);

    /** Stop driving it and leave it as it is. Elements MUST call this on destruct. */
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UnregisterHUDElement(UWidget *Element);

    /** The element's own state changed and it now wants to be seen more, or less. Cheap; call it on any state edge. */
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void SetElementSalience(UWidget *Element, EMythicHUDSalience Salience);

    void SetElementDimTint(UWidget *Element, float DimTint);

    EMythicHUDSalience GetElementSalience(const UWidget *Element) const;

    /** True while the player is holding the whole HUD open on purpose. */
    UFUNCTION(BlueprintPure, Category = "HUD")
    bool IsHUDRevealed() const { return bHUDRevealed; }

    FOnHUDRevealChanged OnHUDRevealChanged;

    /**
     * Something just happened on this element — light it for its rule's hold, then let it settle back.
     *
     * For anything that is interesting at the moment it changes and merely available afterwards: an objective
     * ticking over, a landmark entering compass range, an ability coming off cooldown.
     */
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void PokeElement(UWidget *Element);

    /** Same, by the name used in ContextualElements, so a Blueprint does not have to hold a widget reference. */
    UFUNCTION(BlueprintCallable, Category = "HUD")
    void PokeElementByName(FName WidgetName);

protected:
    void HandleInventoryAction();
    void HandleInspectEntityAction();

    /** Rebuilds the policy-bounded, input-tag-unique LocalPlayer CommonUI bindings from sanitized Focus action rows. */
    UFUNCTION()
    void HandleNameplateProjectionsChanged(int32 LocalRevision);

    /**
     * "Show me everything" — a toggle, not a hold. A contextual HUD hides things the player may still want to check
     * on their own schedule (how much stamina do I actually have?), and with no way to ask, the player is left
     * guessing. Toggle rather than hold because holding a key to read your own HUD is a worse deal for anyone who
     * finds held inputs difficult.
     */
    UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (Categories = "UI.Action"))
    FGameplayTag RevealHUDAction;

    /**
     * The contextual HUD, as a list. Add an element here and it starts obeying the rules; no code, no subclass.
     * Elements that manage their own salience (the vitals) are deliberately absent.
     */
    UPROPERTY(EditDefaultsOnly, Category = "HUD")
    TArray<FMythicHUDElementBinding> ContextualElements;

    /**
     * What Dim actually means: a multiplier on the element's own ink, NOT an opacity. A dim element stays fully
     * opaque and just goes dark, so it keeps its own ground and reads the same over a night field and a dawn sky.
     * Driving this as alpha instead made the whole HUD vanish against a bright horizon.
     */
    UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DimOpacity = 0.45f;

    /** Fast in, slow out: arriving late costs the player information; leaving fast is what reads as twitchy. */
    UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0.1"))
    float SalienceFadeInSpeed = 6.0f;

    UPROPERTY(EditDefaultsOnly, Category = "HUD", meta = (ClampMin = "0.1"))
    float SalienceFadeOutSpeed = 1.7f;

private:
    void ClearBlueprintInventoryOpenFlag();

    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicMenuShell> ActiveMenuShell;

    /** Runtime layer owned by WorldOverlaySlot; never persisted and never shared across split-screen local players. */
    UPROPERTY(Transient)
    TObjectPtr<UMythicNameplateLayer> NameplateLayer;

    struct FContextActionBindingRecord {
        FUIActionBindingHandle Handle;
        FMythicEntityPresentationInstance Subject;
        FGameplayTag ActionTag;
        FGameplayTag InputActionTag;
        uint32 OfferRevision = 0;
        float HoldDurationSeconds = 0.0f;

        bool Matches(const FMythicEntityPresentationInstance &InSubject,
                     const FGameplayTag InActionTag,
                     const uint32 InOfferRevision,
                     const float InHoldDurationSeconds) const {
            return Subject == InSubject && ActionTag == InActionTag
                && OfferRevision == InOfferRevision
                && FMath::IsNearlyEqual(HoldDurationSeconds,
                                        InHoldDurationSeconds,
                                        UE_KINDA_SMALL_NUMBER);
        }
    };

    /** LocalPlayer CommonUI bindings retained by input tag while their focused action contract remains unchanged. */
    TMap<FGameplayTag, FContextActionBindingRecord> ContextActionBindings;

    /** Deliberate learned-dossier hold binding owned by this LocalPlayer HUD. */
    FUIActionBindingHandle InspectActionBinding;

    TWeakObjectPtr<UMythicNameplateDirector> BoundNameplateDirector;

    void RefreshContextActionBindings();
    void ClearContextActionBindings();
    void RemoveContextActionBinding(FGameplayTag InputActionTag);

    struct FMythicHUDElementState {
        TWeakObjectPtr<UWidget> Widget;
        EMythicHUDSalience Want = EMythicHUDSalience::Hidden;

        bool bHasRule = false;
        FMythicHUDElementRule Rule;

        double ActivityUntil = 0.0;

        float DimTintOverride = -1.0f;
    };

    TArray<FMythicHUDElementState> HUDElements;

    FTimerHandle SalienceTimer;

    FTimerHandle CombatBindTimer;

    bool bHUDRevealed = false;
    bool bInCombat = false;
    int32 CombatBindAttempts = 0;

    void SetSalienceTicking(bool bEnabled);

    void HandleAccessibilityChanged();
    FDelegateHandle AccessibilityHandle;
    void TickSalience(float DeltaSeconds);
    void HandleRevealHUD();

    void BindContextualElements();

    void BindCombatState();
    void HandleCombatTagChanged(const FGameplayTag Tag, int32 NewCount);

    bool ApplyRules();

    bool ShouldRevealEverything() const;

    float TargetOpacityFor(EMythicHUDSalience Want) const;

    // The accessibility HUD-opacity cap every salience level multiplies by.
    static float AccessibilityHUDOpacity();

    FMythicHUDElementState *FindElementState(const UWidget *Element);
};
