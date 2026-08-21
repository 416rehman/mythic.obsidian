#pragma once

#include "CoreMinimal.h"
#include "SavedQuestJournal.generated.h"

USTRUCT(BlueprintType)
struct FSerializedQuestJournalEntry {
    GENERATED_BODY()

    // Reference to the UMythicQuestDefinition data asset.
    UPROPERTY(BlueprintReadWrite)
    FSoftObjectPath QuestPath;

    // EMythicQuestState as a raw uint8 (NotStarted/Active/Completed/Failed). Restored verbatim so the terminal latch
    // survives a reload — a Completed quest comes back Completed and is skipped by the recompute (no reward re-grant).
    UPROPERTY(BlueprintReadWrite)
    uint8 State = 0;
};
