#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "MythicRichTextIconLibrary.generated.h"

/** Stable semantic icons shared by fixed HUD slots and Rich Text inline-image rows. */
UENUM(BlueprintType)
enum class EMythicSemanticIcon : uint8 {
    /** No icon; produces an empty row name and empty markup. */
    None,

    AttentionFocus,
    AttentionInteraction,
    AttentionSoftTarget,
    AttentionHardTarget,
    AttentionLockedTarget,
    CueTalk,
    CueQuestOffer,
    CueQuestTurnIn,
    CueService,
    CueFaction,
    CueRole,
    StateSurrender,
    StateFleeing,
    StateAttacking,
    StateDying,
    StateDowned,
    StateDead,
    RankSuperior,
    RankElite,
    RankChampion,
    RankBoss,
    RankWorldBoss,
    ThreatRisky,
    ThreatDeadly,
    ThreatOverwhelming,
    RelationHostile,
};

/** Typed access to the global Rich Text semantic-icon vocabulary. */
UCLASS()
class MYTHIC_API UMythicRichTextIconLibrary final
    : public UBlueprintFunctionLibrary {
    GENERATED_BODY()

public:
    /** Returns the validated DT_ImageRow row name used by the supplied semantic icon, or None for no icon. */
    UFUNCTION(BlueprintPure, Category = "Mythic|UI|Rich Text")
    static FName GetSemanticIconRowName(EMythicSemanticIcon Icon);

    /** Builds Rich Text inline-image markup for the supplied typed semantic icon; None returns empty text. */
    UFUNCTION(BlueprintPure, Category = "Mythic|UI|Rich Text")
    static FText MakeSemanticIconMarkup(EMythicSemanticIcon Icon);
};
