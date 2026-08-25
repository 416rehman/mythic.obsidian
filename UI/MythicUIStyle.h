// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Styling/SlateBrush.h"
#include "MythicUIStyle.generated.h"

class UCommonBorderStyle;
class UCommonButtonBase;
class UCommonTextStyle;
class UCommonTextBlock;
class UUserWidget;
class UWidget;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mythic UI Style"))
class MYTHIC_API UMythicUIStyleSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    // ── Type ramp ──
    /** Screen and section titles. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Text")
    TSoftClassPtr<UCommonTextStyle> TitleStyle;

    /** Group headings inside a screen (a stat category, a word column). */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Text")
    TSoftClassPtr<UCommonTextStyle> HeadingStyle;

    /** Ordinary body text — the default for anything not called out. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Text")
    TSoftClassPtr<UCommonTextStyle> BodyStyle;

    /** Quieter text: hints, counts, "not yet spoken". */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Text")
    TSoftClassPtr<UCommonTextStyle> SubtleStyle;

    // ── Frames and buttons ──
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Frames")
    TSoftClassPtr<UCommonBorderStyle> PanelBorderStyle;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    TSoftClassPtr<UCommonButtonBase> TabButtonClass;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    TSoftClassPtr<UCommonButtonBase> ActionButtonClass;

    /**
     * The button every C++-built row uses. Without this, hand-built rows fall back to a raw UButton, which draws the
     * engine's default grey plate and is most of why generated screens look unfinished next to authored ones.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    TSoftClassPtr<UCommonButtonBase> MenuButtonClass;

    /** Name of the text widget inside MenuButtonClass that carries its label. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    FName MenuButtonLabelName = TEXT("Text_ActionName");

    /**
     * Name of the OPTIONAL second text widget inside MenuButtonClass, right-aligned on the same line.
     *
     * A row that says "Kindling" on the left and "+15% Burn on Hit" on the right is scannable; the same two facts
     * joined into one centred sentence is not. It stays collapsed until a caller gives it something to say.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    FName MenuButtonValueName = TEXT("Text_ActionValue");

    /**
     * The component catalogue. Everything the UI is allowed to draw lives here, generated from the
     * material kit, so a screen picks a catalogued component instead of inventing a brush.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Kit")
    TSoftObjectPtr<class UMythicUIKit> Kit;

    // ── Metrics ──
    /**
     * One spacing scale for every generated screen.
     *
     * The steps grow rather than step evenly, because equal gaps read as one undifferentiated list: the eye
     * groups by relative distance, so a heading needs several times the gap of a row, not a little more.
     * Anything laying out a list in C++ takes its gaps from here so screens cannot drift apart.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Metrics")
    float SpaceXS = 4.0f;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Metrics")
    float SpaceS = 8.0f;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Metrics")
    float SpaceM = 14.0f;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Metrics")
    float SpaceL = 24.0f;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Metrics")
    float SpaceXL = 40.0f;

    /** Gap above a section heading: wide, because a heading breaks from what came before. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Metrics")
    float SectionGap = 40.0f;

    /** Gap under a section heading: tight, because the heading belongs to the rows beneath it. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Metrics")
    float SectionHeadingGap = 6.0f;

    /** Every generated list scrolls with the same bar. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Metrics")
    float ScrollBarThickness = 6.0f;

    // ── Semantic colours ──
    // Kept separate from the accent hue on purpose: these mean something, they are not decoration.
    /** An improvement: a bonus, a completed objective, a repair. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Colours")
    FLinearColor Positive = FLinearColor(0.45f, 0.72f, 0.42f, 1.0f);

    /** A cost, a refusal, a break. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Colours")
    FLinearColor Negative = FLinearColor(0.78f, 0.35f, 0.30f, 1.0f);

    /** Caution without refusal: nearly broken, hazard incoming. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Colours")
    FLinearColor Caution = FLinearColor(0.85f, 0.70f, 0.30f, 1.0f);

    /** Deliberately unknown — an unrevealed weave. Reads as withheld, not as broken. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Colours")
    FLinearColor Mystery = FLinearColor(0.58f, 0.54f, 0.66f, 1.0f);

    // ── Slot plate ──
    // The ground every square thing in this game sits on: a hotbar slot, an equipment square, a rune socket. Kept here
    // so those never drift apart, and so it can be re-tuned once instead of in a dozen widgets.
    /**
     * The dark ink a slot is cut out of. Alpha matters: it has to hold text over grass and over sky.
     * These are LINEAR values — Slate gamma-corrects on the way out, so 0.05 linear paints as mid grey, not near
     * black. Anything meant to read as ink belongs down near 0.01.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Slots")
    FLinearColor SlotFill = FLinearColor(0.010f, 0.0085f, 0.0065f, 0.90f);

    /** The hairline round the slot. Aged brass, not gold — gold at full chroma reads as a mobile game. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Slots")
    FLinearColor SlotEdge = FLinearColor(0.52f, 0.42f, 0.24f, 0.90f);

    /** Edge colour for a slot the player has not earned yet. Present, obviously inert. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Slots")
    FLinearColor SlotEdgeLocked = FLinearColor(0.26f, 0.24f, 0.21f, 0.75f);

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Slots", meta = (ClampMin = "0.0"))
    float SlotCornerRadius = 9.0f;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Slots", meta = (ClampMin = "0.0"))
    float SlotEdgeWidth = 1.5f;

    /** The warm parchment ink the type ramp sits in, for anything that has to tint text directly. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Colours")
    FLinearColor Ink = FLinearColor(0.93f, 0.88f, 0.76f, 1.0f);

    /**
     * The rest of the neutral ramp, so a screen never has to invent one.
     *
     * Every widget that hardcoded its own greys drifted from every other: the settings rows alone carried
     * eight literals that duplicated Ink and then disagreed with it. Naming them by ROLE rather than by
     * value is what lets a retune land everywhere at once.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Colours")
    FLinearColor InkLabel = FLinearColor(0.386f, 0.333f, 0.254f, 1.0f);

    /** The one accent. Selection, focus, a filled bar - and nothing else, or it stops meaning "here". */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Colours")
    FLinearColor Accent = FLinearColor(0.584f, 0.361f, 0.020f, 1.0f);

    /** The brighter accent, for the moving part of a control: a switch knob, a slider handle. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Colours")
    FLinearColor AccentBright = FLinearColor(0.807f, 0.558f, 0.102f, 1.0f);

    /** One-pixel rules and control outlines. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Colours")
    FLinearColor Hairline = FLinearColor(0.016f, 0.013f, 0.010f, 1.0f);

    /** The band a row sits on so its label holds contrast against whatever is behind the screen. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Colours")
    FLinearColor Surface = FLinearColor(0.0065f, 0.0056f, 0.0052f, 1.0f);

    /** The empty half of a track or an unlit switch. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Colours")
    FLinearColor Trough = FLinearColor(0.0176f, 0.0137f, 0.0097f, 1.0f);

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Colours")
    FLinearColor InkSubtle = FLinearColor(0.68f, 0.62f, 0.52f, 1.0f);

    virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }
};

UENUM()
enum class EMythicTextRole : uint8 {
    Title,
    Heading,
    Body,
    Subtle,
};

struct MYTHIC_API FMythicUIStyle {
    static const UMythicUIStyleSettings &Get();

    static UCommonTextBlock *MakeText(UWidget *Owner, EMythicTextRole Role);

    static void ApplyTextStyle(UCommonTextBlock *Text, EMythicTextRole Role);

    static FSlateBrush MakeSlotBrush(bool bLocked = false);

    static UObject *WidgetOuterFor(UWidget *Owner);

    static UWidget *MakeButton(UWidget *Owner, EMythicTextRole LabelRole, UCommonTextBlock *&OutLabel,
                               UCommonTextBlock **OutValue = nullptr);

    static void SetOptionalText(UCommonTextBlock *Text, const FText &Value, FLinearColor Colour);

    static void ShowEmptyState(UUserWidget *Page, FName BlockName, bool bEmpty);

    static void WireFocusRing(class UCommonButtonBase *Button);

    static void WireFocusRings(UUserWidget *Root);

    static UWidget *FindFirstFocusable(UUserWidget *Root);

    static void BindButtonClicked(UWidget *Button, UObject *Handler, FName FunctionName);
};
