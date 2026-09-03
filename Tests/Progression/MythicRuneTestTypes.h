#pragma once

#include "CoreMinimal.h"
#include "Progression/Runes/MythicRuneComponent.h"
#include "MythicRuneTestTypes.generated.h"

/** Counts the rune component's dynamic broadcasts the way a widget would: through UFUNCTION handlers. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicRuneTestListener final : public UObject {
    GENERATED_BODY()

public:
    int32 ChangedCount = 0;
    int32 RefusedCount = 0;
    int32 LastRefusedSlot = INDEX_NONE;
    EMythicRuneRefusal LastReason = EMythicRuneRefusal::None;

    void Bind(UMythicRuneComponent *Runes) {
        Runes->OnRunesChanged.AddUniqueDynamic(this, &UMythicRuneTestListener::HandleRunesChanged);
        Runes->OnRuneRefused.AddUniqueDynamic(this, &UMythicRuneTestListener::HandleRuneRefused);
    }

    void Reset() {
        ChangedCount = 0;
        RefusedCount = 0;
        LastRefusedSlot = INDEX_NONE;
        LastReason = EMythicRuneRefusal::None;
    }

    UFUNCTION()
    void HandleRunesChanged() { ChangedCount++; }

    UFUNCTION()
    void HandleRuneRefused(int32 SlotIndex, EMythicRuneRefusal Reason) {
        RefusedCount++;
        LastRefusedSlot = SlotIndex;
        LastReason = Reason;
    }
};
