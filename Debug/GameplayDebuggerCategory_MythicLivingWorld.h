#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerCategory.h"

class FGameplayDebuggerCategory_MythicLivingWorld : public FGameplayDebuggerCategory {
public:
    FGameplayDebuggerCategory_MythicLivingWorld();

    virtual void CollectData(APlayerController *OwnerPC, AActor *DebugActor) override;
    virtual void DrawData(APlayerController *OwnerPC, FGameplayDebuggerCanvasContext &CanvasContext) override;

    static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

protected:
    struct FRepData {
        FString Summary;
        uint8 ActiveDetail = 0;
        uint8 LayerFlags = 0;
        int32 ShapesDrawn = 0;
        void Serialize(FArchive &Ar);
    };
    FRepData DataPack;

    static constexpr uint8 NumDetailPanes = 13;

    bool bShowEntities = true;
    bool bShowTerritory = false;
    bool bShowSettlements = false;
    bool bShowEvents = false;
    bool bShowCrime = false;
    uint8 ActiveDetail = 0;

    void OnToggleEntities() { bShowEntities = !bShowEntities; }
    void OnToggleTerritory() { bShowTerritory = !bShowTerritory; }
    void OnToggleSettlements() { bShowSettlements = !bShowSettlements; }
    void OnToggleEvents() { bShowEvents = !bShowEvents; }
    void OnToggleCrime() { bShowCrime = !bShowCrime; }
    void OnCycleDetail() { ActiveDetail = static_cast<uint8>((ActiveDetail + 1) % NumDetailPanes); }
};

#endif
