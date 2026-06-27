#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerCategory.h"

/**
 * Unified "debug everything" Gameplay Debugger category for the Living World.
 *
 * ALWAYS-ON: an overview header (system active, faction count, env clock+weather, MASS tier tallies + promotion gate,
 * social/scheme/settlement/event counts) plus per-NPC entity markers (tier point + hydrated pressure bloom).
 *
 * TOGGLEABLE WORLD-SHAPE LAYERS (letter keys, all Replicated so they reach the authority that runs CollectData):
 *   [J] entities markers (default ON), [K] territory cell quads (OFF), [L] settlement outlines (OFF),
 *   [B] events: causal points + scheme links + social edges + creature dens (OFF),
 *   [C] crime: witnessed-crime markers tinted by moral severity (OFF).
 *
 * CYCLING DETAIL PANE ([N] cycles one of THIRTEEN): Factions+Economy / Diplomacy / Schemes / Social / Settlements /
 * Party-Companions-Players / Causal events / Environment / Cognition (BDI + per-NPC activity / last social reaction /
 * appearance descriptor) / World History (chronicle-less crime feed) /
 * World State (sim thread, save ops, world tier + tier multipliers, resources, replicated proxies, creatures) /
 * Chronicle (world-news feed) / Population (sim-driven WHO spawns: archetype/role mix, contested-border garrison targets,
 * settlement spawn-point counts by purpose, active groups, and the per-faction Pop/Arms/MilitaryStrength kill-feedback
 * readout). Only one pane prints at a time so the canvas text stays bounded.
 *
 * Server-authoritative collection (the sim + MASS entities live on the server); data replicates to the viewing client.
 */
class FGameplayDebuggerCategory_MythicLivingWorld : public FGameplayDebuggerCategory {
public:
    FGameplayDebuggerCategory_MythicLivingWorld();

    virtual void CollectData(APlayerController *OwnerPC, AActor *DebugActor) override;
    virtual void DrawData(APlayerController *OwnerPC, FGameplayDebuggerCanvasContext &CanvasContext) override;

    static TSharedRef<FGameplayDebuggerCategory> MakeInstance();

protected:
    // ─── Replicated data pack ─────────────────────────────
    // All section text is concatenated into one FString on the authority and rendered verbatim by DrawData.
    // The small scalars let DrawData render a consistent toggle footer even though the text was built server-side.
    struct FRepData {
        FString Summary;
        uint8 ActiveDetail = 0; // which D-pane the authority rendered (0..12)
        uint8 LayerFlags = 0;   // bit0 entities, bit1 territory, bit2 settlements, bit3 events, bit4 crime
        int32 ShapesDrawn = 0;  // for the "drew N/Max" footer
        void Serialize(FArchive &Ar);
    };
    FRepData DataPack;

    // Number of cycling detail panes; keep in sync with the switch cases + DetailNames[] footer in the .cpp.
    static constexpr uint8 NumDetailPanes = 13;

    // ─── Input toggle state (authority-side; flipped by Replicated key handlers) ───
    bool bShowEntities = true;
    bool bShowTerritory = false;
    bool bShowSettlements = false;
    bool bShowEvents = false;
    bool bShowCrime = false;
    uint8 ActiveDetail = 0; // 0..(NumDetailPanes-1) selects the active detail pane

    // Toggle handlers — pure member flips (no deref / no allocation, cannot crash in any state).
    void OnToggleEntities() { bShowEntities = !bShowEntities; }
    void OnToggleTerritory() { bShowTerritory = !bShowTerritory; }
    void OnToggleSettlements() { bShowSettlements = !bShowSettlements; }
    void OnToggleEvents() { bShowEvents = !bShowEvents; }
    void OnToggleCrime() { bShowCrime = !bShowCrime; }
    void OnCycleDetail() { ActiveDetail = static_cast<uint8>((ActiveDetail + 1) % NumDetailPanes); }
};

#endif // WITH_GAMEPLAY_DEBUGGER
