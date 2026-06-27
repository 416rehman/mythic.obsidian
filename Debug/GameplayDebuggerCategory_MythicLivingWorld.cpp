#include "GameplayDebuggerCategory_MythicLivingWorld.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "Engine/HitResult.h"
#include "CollisionQueryParams.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h" // TActorIterator
#include "Components/SplineComponent.h"

#include "MassEntitySubsystem.h"
#include "MassExecutionContext.h"

#include "Mass/EntityHandle.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Mass/Tags/MythicMassTags.h"
#include "Mass/Processors/TerritoryPatrolSpawnerProcessor.h" // static garrison-target helpers (read-only re-derivation)

#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/SettlementRegistry.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Territory/MythicBiome.h"
#include "World/LivingWorld/Creatures/CreatureSpeciesTypes.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/Simulation/SchemeEngine.h"
#include "World/LivingWorld/Simulation/SchemeTypes.h"
#include "World/LivingWorld/Encounters/EncounterDirector.h"
#include "World/LivingWorld/Encounters/EncounterTemplate.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"

#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "World/EnvironmentController/MythicEnvironmentController.h"
#include "World/EnvironmentController/EnvironmentTypes.h"

#include "AI/Party/PartySubsystem.h"
#include "AI/Party/MythicPartyTypes.h"
#include "AI/NPCs/MythicAIController.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/NPCs/MythicNPCManager.h"
#include "AI/NPCs/MythicSocialVerbs.h"                       // EMythicSocialVerb / EMythicSocialReaction (per-NPC last-reaction surface)
#include "World/LivingWorld/Activities/ActivityTypes.h"     // GetCurrentActivityTag display (per-NPC activity surface)
#include "World/LivingWorld/Appearance/AppearanceTypes.h"   // FMythicAppearance descriptor summary
#include "World/LivingWorld/Factions/FactionColor.h"        // MythicFactionColor::GetFactionColor (appearance/faction swatch)
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "AI/Cognition/CognitiveTypes.h"
#include "Player/MythicPlayerRegistrySubsystem.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicFactionStandingComponent.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Objectives/ObjectiveTracker.h"
#include "World/EnvironmentController/MythicEnvironmentHazardComponent.h"
#include "World/GameDirector/MythicGameDirectorSubsystem.h"
#include "World/LivingWorld/Chronicle/MythicWorldChronicleSubsystem.h"
#include "World/Interactables/MythicToggleable.h"
#include "Itemization/Conversion/MythicConversionStation.h"
#include "Itemization/Conversion/ConversionStationComponent.h"
#include "Itemization/Storage/MythicStorageContainer.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "GameModes/Attributes/WorldAttributes.h"
#include "Resources/MythicResourceManagerComponent.h"
#include "UI/MythicDamageNumberSubsystem.h"
#include "Engine/LocalPlayer.h"

// Pane 8 (Cognition) / Pane 9 (Crime/History) / Pane 10 (World State) — all verified-present systems.
#include "World/LivingWorld/Events/ActionEventSubsystem.h"
#include "World/LivingWorld/Events/ActionEventTypes.h"
#include "World/LivingWorld/Crime/CrimeTypes.h"
#include "World/LivingWorld/LivingWorldReplication.h"
#include "GameModes/GameState/MythicGameState.h"
#include "GameModes/MythicGameMode.h"
#include "Subsystem/SaveSystem/MythicSaveGameSubsystem.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"

// Pane 10 (World State) — Living World FINAL cluster: designer spawner (server) + war-map (client).
#include "World/LivingWorld/Spawn/MythicDesignerSpawner.h"       // AMythicDesignerSpawner (placed, server logic)
#include "World/LivingWorld/Spawn/DesignerSpawnerRegistry.h"     // UMythicDesignerSpawnerRegistry (authoritative state)
#include "World/LivingWorld/Spawn/DesignerSpawnerTypes.h"        // FMythicDesignerSpawnerState
#include "UI/WarMap/MythicWarMapSubsystem.h"                     // UMythicWarMapSubsystem (client texture/legend view)
#include "UI/WarMap/MythicWarMapTypes.h"                         // FMythicWarMapLegendEntry

// ─────────────────────────────────────────────────────────────
// File-local helpers
// ─────────────────────────────────────────────────────────────

namespace {

const int32 GMaxShapes = 600; // global world-shape budget shared across all layers

// Deterministic faction color from the uint8 index (no engine-provided faction color API exists).
FColor FactionColor(const FMythicFactionId Id) {
    if (!Id.IsValid()) {
        return FColor(60, 60, 60); // unclaimed / invalid = dark grey
    }
    static const FColor Palette[] = {
        FColor::Red, FColor::Blue, FColor::Green, FColor::Orange,
        FColor::Purple, FColor::Cyan, FColor::Yellow, FColor(120, 180, 255)};
    return Palette[Id.Index % UE_ARRAY_COUNT(Palette)];
}

const TCHAR *DayTimeToString(EDayTime D) {
    switch (D) {
    case Morning:
        return TEXT("Morning");
    case Afternoon:
        return TEXT("Afternoon");
    case Evening:
        return TEXT("Evening");
    default:
        return TEXT("Night");
    }
}

const TCHAR *SeasonToString(ESeason S) {
    switch (S) {
    case Spring:
        return TEXT("Spring");
    case Summer:
        return TEXT("Summer");
    case Autumn:
        return TEXT("Autumn");
    default:
        return TEXT("Winter");
    }
}

const TCHAR *PhaseToString(EMythicSchedulePhase P) {
    switch (P) {
    case EMythicSchedulePhase::Work:
        return TEXT("Work");
    case EMythicSchedulePhase::Rest:
        return TEXT("Rest");
    case EMythicSchedulePhase::Social:
        return TEXT("Social");
    case EMythicSchedulePhase::Travel:
        return TEXT("Travel");
    default:
        return TEXT("Idle");
    }
}

// Display name for a biome (parallel to EMythicBiome, COUNT excluded). Mirrors SeasonToString/PhaseToString.
const TCHAR *BiomeToString(EMythicBiome B) {
    switch (B) {
    case EMythicBiome::Plains:
        return TEXT("Plains");
    case EMythicBiome::Forest:
        return TEXT("Forest");
    case EMythicBiome::Mountain:
        return TEXT("Mountain");
    case EMythicBiome::Wetland:
        return TEXT("Wetland");
    case EMythicBiome::Wasteland:
        return TEXT("Wasteland");
    case EMythicBiome::Desert:
        return TEXT("Desert");
    default:
        return TEXT("?");
    }
}

// Resolve a creature SpeciesId to its display name via the code-default species set (the spawner's unauthored
// fallback; an authored DataTable reuses the same ids). Returns "spN" when the id is not a known default.
FString SpeciesIdToName(uint8 SpeciesId) {
    if (SpeciesId != 0) {
        for (const FMythicCreatureSpeciesRow &Row : MythicCreatureDefaults::GetCodeDefaultSpecies()) {
            if (Row.SpeciesId == SpeciesId && !Row.DisplayName.IsNone()) {
                return Row.DisplayName.ToString();
            }
        }
    }
    return FString::Printf(TEXT("sp%d"), SpeciesId);
}

const TCHAR *PressureChannelName(int32 Idx) {
    static const TCHAR *Names[] = {TEXT("Threat"), TEXT("Injustice"), TEXT("Grief"), TEXT("Shame"), TEXT("Desire"), TEXT("Wrath")};
    return (Idx >= 0 && Idx < PressureChannelCount) ? Names[Idx] : TEXT("?");
}

// Display name for a vent channel (parallel to EMythicVentChannel, COUNT excluded). Mirrors PressureChannelName.
const TCHAR *VentChannelName(int32 Idx) {
    static const TCHAR *Names[] = {TEXT("Fight"), TEXT("Flee"), TEXT("Enforce"), TEXT("Report"),
                                   TEXT("Exploit"), TEXT("Tend"), TEXT("Rally"), TEXT("Submit")};
    return (Idx >= 0 && Idx < VentChannelCount) ? Names[Idx] : TEXT("?");
}

// Display name for a BDI desire type (parallel to the EMythicDesireType enumerators, COUNT excluded).
const TCHAR *DesireTypeName(EMythicDesireType T) {
    static const TCHAR *Names[] = {
        TEXT("Survive"), TEXT("Defend"), TEXT("Avenge"), TEXT("Patrol"), TEXT("Trade"), TEXT("Socialize"),
        TEXT("JoinPlayer"), TEXT("Flee"), TEXT("Rest"), TEXT("Exploit"), TEXT("Rally"), TEXT("Report"), TEXT("FollowSchedule")};
    const int32 i = static_cast<int32>(T);
    return (i >= 0 && i < DesireTypeCount) ? Names[i] : TEXT("?");
}

// Is a desire an ACUTE (red-tinted) survival/threat response vs a routine one?
bool IsAcuteDesire(EMythicDesireType T) {
    return T == EMythicDesireType::Survive || T == EMythicDesireType::Flee ||
           T == EMythicDesireType::Defend || T == EMythicDesireType::Avenge;
}

// Marker color for a witnessed-crime severity.
FColor MoralSeverityColor(EMythicMoralSeverity S) {
    switch (S) {
    case EMythicMoralSeverity::Hostile:
        return FColor::Red;
    case EMythicMoralSeverity::Condemn:
        return FColor::Orange;
    case EMythicMoralSeverity::Disapprove:
        return FColor::Yellow;
    default:
        return FColor(150, 150, 150); // Ignore — grey
    }
}

// Color tint string for a witnessed-crime severity (text pane).
const TCHAR *MoralSeverityTint(EMythicMoralSeverity S) {
    switch (S) {
    case EMythicMoralSeverity::Hostile:
        return TEXT("{red}");
    case EMythicMoralSeverity::Condemn:
        return TEXT("{orange}");
    case EMythicMoralSeverity::Disapprove:
        return TEXT("{yellow}");
    default:
        return TEXT("{grey}");
    }
}

const TCHAR *MoralSeverityName(EMythicMoralSeverity S) {
    switch (S) {
    case EMythicMoralSeverity::Hostile:
        return TEXT("Hostile");
    case EMythicMoralSeverity::Condemn:
        return TEXT("Condemn");
    case EMythicMoralSeverity::Disapprove:
        return TEXT("Disapprove");
    default:
        return TEXT("Ignore");
    }
}

// Display name for a player→NPC social verb (parallel to EMythicSocialVerb, COUNT excluded). Mirrors PhaseToString.
const TCHAR *SocialVerbName(EMythicSocialVerb V) {
    switch (V) {
    case EMythicSocialVerb::Greet:
        return TEXT("Greet");
    case EMythicSocialVerb::Compliment:
        return TEXT("Compliment");
    case EMythicSocialVerb::Provoke:
        return TEXT("Provoke");
    case EMythicSocialVerb::Bully:
        return TEXT("Bully");
    case EMythicSocialVerb::Threaten:
        return TEXT("Threaten");
    default:
        return TEXT("?");
    }
}

// Display name for an NPC's social reaction band (parallel to EMythicSocialReaction, COUNT excluded).
const TCHAR *SocialReactionName(EMythicSocialReaction R) {
    switch (R) {
    case EMythicSocialReaction::Warm:
        return TEXT("Warm");
    case EMythicSocialReaction::Neutral:
        return TEXT("Neutral");
    case EMythicSocialReaction::Cold:
        return TEXT("Cold");
    case EMythicSocialReaction::Intimidated:
        return TEXT("Intimidated");
    case EMythicSocialReaction::Angered:
        return TEXT("Angered");
    case EMythicSocialReaction::CallGuards:
        return TEXT("CallGuards");
    default:
        return TEXT("?");
    }
}

// Color tint string for a social reaction (text pane): warm=green, cold/intimidated=grey/yellow, angered/guards=red.
const TCHAR *SocialReactionTint(EMythicSocialReaction R) {
    switch (R) {
    case EMythicSocialReaction::Warm:
        return TEXT("{green}");
    case EMythicSocialReaction::Cold:
        return TEXT("{grey}");
    case EMythicSocialReaction::Intimidated:
        return TEXT("{yellow}");
    case EMythicSocialReaction::Angered:
    case EMythicSocialReaction::CallGuards:
        return TEXT("{red}");
    default:
        return TEXT("{white}");
    }
}

// Leaf-only name of an FName tag path (strip everything up to + including the last '.'), keeping debug lines tight.
// Shared by the role/group activity tallies (which key TMaps on FGameplayTag::GetTagName()).
FString TagNameLeaf(const FName &TagName) {
    FString Full = TagName.ToString();
    int32 DotIdx = INDEX_NONE;
    if (Full.FindLastChar(TEXT('.'), DotIdx)) {
        return Full.RightChop(DotIdx + 1);
    }
    return Full;
}

} // namespace

// ─────────────────────────────────────────────────────────────
// Construction / registration plumbing
// ─────────────────────────────────────────────────────────────

FGameplayDebuggerCategory_MythicLivingWorld::FGameplayDebuggerCategory_MythicLivingWorld() {
    SetDataPackReplication<FRepData>(&DataPack);

    // All toggles MUST be Replicated: they flip authority-side members that CollectData reads. A Replicated handler
    // pressed on a non-authority client RPCs the server, which re-dispatches on the authority category instance.
    const EGameplayDebuggerInputMode Rep = EGameplayDebuggerInputMode::Replicated;
    BindKeyPress(TEXT("J"), this, &FGameplayDebuggerCategory_MythicLivingWorld::OnToggleEntities, Rep);
    BindKeyPress(TEXT("K"), this, &FGameplayDebuggerCategory_MythicLivingWorld::OnToggleTerritory, Rep);
    BindKeyPress(TEXT("L"), this, &FGameplayDebuggerCategory_MythicLivingWorld::OnToggleSettlements, Rep);
    BindKeyPress(TEXT("B"), this, &FGameplayDebuggerCategory_MythicLivingWorld::OnToggleEvents, Rep);
    BindKeyPress(TEXT("C"), this, &FGameplayDebuggerCategory_MythicLivingWorld::OnToggleCrime, Rep);
    BindKeyPress(TEXT("N"), this, &FGameplayDebuggerCategory_MythicLivingWorld::OnCycleDetail, Rep);
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_MythicLivingWorld::MakeInstance() {
    return MakeShareable(new FGameplayDebuggerCategory_MythicLivingWorld());
}

void FGameplayDebuggerCategory_MythicLivingWorld::FRepData::Serialize(FArchive &Ar) {
    Ar << Summary;
    Ar << ActiveDetail;
    Ar << LayerFlags;
    Ar << ShapesDrawn;
}

// ─────────────────────────────────────────────────────────────
// CollectData — server-side, game thread
// ─────────────────────────────────────────────────────────────

void FGameplayDebuggerCategory_MythicLivingWorld::CollectData(APlayerController *OwnerPC, AActor *DebugActor) {
    UWorld *World = OwnerPC ? OwnerPC->GetWorld() : nullptr;
    if (!World) {
        return;
    }

    UMassEntitySubsystem *MassSub = World->GetSubsystem<UMassEntitySubsystem>();
    UMythicLivingWorldSubsystem *LW =
        OwnerPC->GetGameInstance() ? OwnerPC->GetGameInstance()->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    UMythicTerritoryGrid *Grid = LW ? LW->GetTerritoryGrid() : nullptr;
    if (!MassSub || !LW || !Grid) {
        DataPack.Summary = TEXT("{red}Living World / MASS not available (this category only works in a running game/PIE world).");
        DataPack.ActiveDetail = ActiveDetail;
        DataPack.LayerFlags = 0;
        DataPack.ShapesDrawn = 0;
        return;
    }

    const UMythicFactionDatabase *FDB = LW->GetFactionDatabase();
    const UMythicLivingWorldSettings *Settings = LW->GetSettings();

    const float PromoteT0 = Settings ? Settings->PromotionThreshold : 0.7f;
    const float PromoteT2 = Settings ? Settings->Tier2PromotionThreshold : 0.9f;
    const float ProxW = Settings ? Settings->ProximityWeight : 0.4f;
    const float HystM = Settings ? Settings->SignificanceHysteresisMargin : 0.1f;

    const double WorldTime = World->GetTimeSeconds();

    FMythicCellCoord PlayerCell;
    bool bHasPlayerCell = false;
    if (const APawn *Pawn = OwnerPC->GetPawn()) {
        PlayerCell = Grid->WorldToCell(Pawn->GetActorLocation());
        bHasPlayerCell = true;
    }

    // ─── Ground-conform debug shapes (drape on the terrain like the navmesh overlay, not the flat Z=0 sky plane) ───
    // Grid->CellToWorld returns a flat Z=0, so raw cell markers float far above/below the island's actual terrain (the
    // "look up at the sky" problem). GroundCell line-traces each cell's XY down to the world surface — anchored around the
    // viewer's height so it finds ground near the player regardless of where the island sits relative to the world origin
    // — and caches the result per cell for this CollectData pass (so a cell hit by multiple layers traces once). The
    // existing per-shape +Z offsets then lift each marker a readable amount ABOVE the ground instead of above Z=0.
    const float GroundTraceAnchorZ = (OwnerPC && OwnerPC->GetPawn()) ? OwnerPC->GetPawn()->GetActorLocation().Z : 0.0f;
    TMap<FMythicCellCoord, float> GroundZCache;
    auto GroundCell = [World, Grid, GroundTraceAnchorZ, &GroundZCache](const FMythicCellCoord &Cell) -> FVector {
        FVector P = Grid->CellToWorld(Cell);
        if (const float *Cached = GroundZCache.Find(Cell)) {
            P.Z = *Cached;
            return P;
        }
        float GroundZ = GroundTraceAnchorZ; // fallback: viewer height (keeps markers near the player, never the Z=0 sky)
        if (World) {
            FHitResult Hit;
            const FVector Start(P.X, P.Y, GroundTraceAnchorZ + 50000.0f);
            const FVector End(P.X, P.Y, GroundTraceAnchorZ - 50000.0f);
            static const FName GroundTraceTag(TEXT("MythicLWDebugGround"));
            FCollisionQueryParams Q(GroundTraceTag, /*bTraceComplex*/ false);
            if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Q)) {
                GroundZ = Hit.ImpactPoint.Z;
            }
        }
        GroundZCache.Add(Cell, GroundZ);
        P.Z = GroundZ;
        return P;
    };

    // Single non-owning aliasing pointer + execution-context entity manager, reused by every MASS pass.
    FMassEntityManager &EM = MassSub->GetMutableEntityManager();
    TSharedPtr<FMassEntityManager> EMPtr = TSharedPtr<FMassEntityManager>(&EM, [](FMassEntityManager *) {});

    int32 ShapesDrawn = 0; // global budget shared across L1/L2/L3/L4

    FString Header;
    FString Detail;

    // ═══════════════════════ HEADER (always on) ═══════════════════════
    Header += TEXT("{white}=== LIVING WORLD (server sim) ===\n");
    Header += FString::Printf(TEXT("{white}System: %s   "), LW->IsSystemActive() ? TEXT("{green}ACTIVE{white}") : TEXT("{red}inactive{white}"));
    if (const UMythicCausalFabric *Fabric = LW->GetCausalFabric()) {
        Header += FString::Printf(TEXT("Fabric cap: {yellow}%d{white}   "), Fabric->GetCapacity());
    }
    if (FDB) {
        Header += FString::Printf(TEXT("Factions: {yellow}%d{white}/{grey}%d{white}"), FDB->GetActiveFactionCount(), FDB->GetMaxFactions());
    }
    Header += TEXT("\n");

    // Environment one-liner (game-thread timer-driven; no lock).
    if (UMythicEnvironmentSubsystem *Env = OwnerPC->GetGameInstance() ? OwnerPC->GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr) {
        if (AMythicEnvironmentController *Ctrl = Env->GetEnvironmentController()) {
            const FTimespan Ts = Ctrl->GetTimespan();
            const FGameplayTag WeatherTag = Env->GetWeather();
            Header += FString::Printf(TEXT("{white}Clock: {yellow}Y%d M%d (%s) D%d %02d:%02d{white}  %s  Weather: {yellow}%s{white}\n"),
                                      GetYear(Ts), GetMonthOfYear(Ts), SeasonToString(Env->GetSeason()), GetDayOfMonth(Ts),
                                      Ts.GetHours(), Ts.GetMinutes(), DayTimeToString(Env->GetDayTime()),
                                      WeatherTag.IsValid() ? *WeatherTag.ToString() : TEXT("(none)"));
        }
    }

    // ── MASS Pass A: NPC counts + tier/phase tally + L1 tier markers ──
    int32 Tier[4] = {};
    int32 Phase[5] = {};
    int32 Dirty = 0;
    float MaxAmbient = 0.0f;
    int32 SpyCount = 0;       // Identity.IsSpy() — undercover NPCs (Faction != TrueFaction)
    int32 VisGroupCount = 0;  // Identity.VisibilityGroup != 0 — entities in a non-default LOS group
    uint32 SumRelevantEvents = 0; // Σ Significance.RelevantEventCount — causal-event density feeding rescore
    // Role-from-context tally across all ambient/embodied NPCs (Identity.RoleTag stamped by the population /
    // territory-patrol / traveler spawners). Folded into the SAME Pass A loop — no extra query/iteration.
    TMap<FName, int32> RoleCounts;
    // Built ONCE here, reused by L4 social-edge resolution (handle -> cell).
    TMap<FMassEntityHandle, FMythicCellCoord> EntityCellMap;
    // Nearest hydrated NPCs for the Social detail pane (resolved in Pass B).
    {
        FMassEntityQuery Q(EMPtr);
        Q.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
        Q.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadOnly);
        Q.AddRequirement<FMythicScheduleFragment>(EMassFragmentAccess::ReadOnly);
        Q.AddTagRequirement<FMythicNPCTag>(EMassFragmentPresence::All);
        FMassExecutionContext Ctx(EM);
        Q.ForEachEntityChunk(Ctx, [&](FMassExecutionContext &C) {
            const int32 N = C.GetNumEntities();
            const TConstArrayView<FMythicIdentityFragment> IdView = C.GetFragmentView<FMythicIdentityFragment>();
            const TConstArrayView<FMythicSignificanceFragment> SigView = C.GetFragmentView<FMythicSignificanceFragment>();
            const TConstArrayView<FMythicScheduleFragment> SchView = C.GetFragmentView<FMythicScheduleFragment>();
            for (int32 i = 0; i < N; ++i) {
                const int32 Ti = FMath::Clamp(static_cast<int32>(SigView[i].Tier), 0, 3);
                ++Tier[Ti];
                ++Phase[FMath::Clamp(static_cast<int32>(SchView[i].Phase), 0, 4)];
                if (SigView[i].bDirty) {
                    ++Dirty;
                }
                if (Ti == 0) {
                    MaxAmbient = FMath::Max(MaxAmbient, SigView[i].Score);
                }
                EntityCellMap.Add(C.GetEntity(i), IdView[i].Cell);

                if (IdView[i].IsSpy()) {
                    ++SpyCount;
                }
                if (IdView[i].VisibilityGroup != 0) {
                    ++VisGroupCount;
                }
                if (IdView[i].RoleTag.IsValid()) {
                    ++RoleCounts.FindOrAdd(IdView[i].RoleTag.GetTagName());
                }
                SumRelevantEvents += SigView[i].RelevantEventCount;

                if (bShowEntities && ShapesDrawn < GMaxShapes) {
                    const FColor Col = (Ti >= 2) ? FColor::Green : (Ti == 1) ? FColor::Yellow : FColor(140, 140, 140);
                    // Embodied (Tier2) NPCs MOVE — anchor the marker to the LIVE actor position so it FOLLOWS the walking
                    // NPC. Identity.Cell only updates at cell-boundary crossings and GroundCell snaps to the cell centre,
                    // so a cell-based marker sits frozen at the spawn cell while the actor walks off. Ambient/reactive
                    // (Tier0/1) entities have no actor and fall back to the grounded cell.
                    const AMythicNPCCharacter *EmbodiedActor = (Ti >= 2) ? LW->FindEmbodiedActor(C.GetEntity(i)) : nullptr;
                    const FVector WorldLoc = EmbodiedActor
                        ? EmbodiedActor->GetActorLocation() + FVector(0.0f, 0.0f, 120.0f)
                        : GroundCell(IdView[i].Cell) + FVector(0.0f, 0.0f, 120.0f);
                    AddShape(FGameplayDebuggerShape::MakePoint(
                        WorldLoc, 25.0f, Col,
                        FString::Printf(TEXT("T%d %.2f %s %s"), Ti, SigView[i].Score, *IdView[i].RoleTag.GetTagName().ToString(), PhaseToString(SchView[i].Phase))));
                    ++ShapesDrawn;
                }
            }
        });
    }

    // ── MASS counts: creatures (+species buckets) + hydrated ──
    // CreatureCount + per-species tally come from ONE chunk pass over the creature archetype (the count that was
    // structurally always 0 before the spawner existed). uint8 SpeciesId -> [0,255], so a fixed bucket is bounds-safe.
    int32 CreatureCount = 0;
    int32 HydratedCount = 0;
    int32 SpeciesCounts[256] = {};
    {
        FMassEntityQuery Q(EMPtr);
        Q.AddRequirement<FMythicCreatureFragment>(EMassFragmentAccess::ReadOnly);
        Q.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);
        FMassExecutionContext Ctx(EM);
        Q.ForEachEntityChunk(Ctx, [&](FMassExecutionContext &C) {
            const int32 N = C.GetNumEntities();
            const TConstArrayView<FMythicCreatureFragment> CrView = C.GetFragmentView<FMythicCreatureFragment>();
            for (int32 i = 0; i < N; ++i) {
                ++SpeciesCounts[CrView[i].SpeciesId];
                ++CreatureCount;
            }
        });
    }
    {
        FMassEntityQuery Q(EMPtr);
        Q.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
        Q.AddTagRequirement<FMythicHydratedTag>(EMassFragmentPresence::All);
        HydratedCount = Q.GetNumMatchingEntities();
    }

    // ── KIND tallies: territory soldiers + inter-settlement travelers (the two new sim-derived population kinds).
    //    SoldierCount + a per-faction soldier breakdown come from one chunk pass (Identity.Faction); TravelerCount is
    //    a cheap match count. Both tags ALSO carry FMythicNPCTag, so these are already folded into TotalNPC above —
    //    these counts split that total by KIND. ──
    int32 SoldierCount = 0;
    int32 TravelerCount = 0;
    TMap<uint8, int32> SoldiersByFaction; // Faction.Index -> soldier count (controlled-territory garrisons)
    {
        FMassEntityQuery Q(EMPtr);
        Q.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
        Q.AddTagRequirement<FMythicSoldierTag>(EMassFragmentPresence::All);
        FMassExecutionContext Ctx(EM);
        Q.ForEachEntityChunk(Ctx, [&](FMassExecutionContext &C) {
            const int32 N = C.GetNumEntities();
            const TConstArrayView<FMythicIdentityFragment> IdView = C.GetFragmentView<FMythicIdentityFragment>();
            for (int32 i = 0; i < N; ++i) {
                ++SoldierCount;
                if (IdView[i].Faction.IsValid()) {
                    ++SoldiersByFaction.FindOrAdd(IdView[i].Faction.Index);
                }
            }
        });
    }
    {
        FMassEntityQuery Q(EMPtr);
        Q.AddTagRequirement<FMythicTravelerTag>(EMassFragmentPresence::All);
        TravelerCount = Q.GetNumMatchingEntities();
    }

    // ── GROUP tally: clustered-spawn members (a noble's retinue, a merchant barter party, a friend trio). Group members
    //    ALSO carry FMythicNPCTag (so they're already folded into TotalNPC) + the data-bearing FMythicGroupFragment. One
    //    chunk pass over the group archetype gives the member total, leader count, the set of distinct active GroupIds
    //    (vs the MaxActiveGroups cap), and a per-activity-tag breakdown. ──
    int32 GroupMemberCount = 0;
    int32 GroupLeaderCount = 0;
    TSet<uint32> ActiveGroupIds;            // distinct GroupId == active group count (the despawner culls a group's members together)
    TMap<FName, int32> GroupMembersByActivity; // ActivityTag leaf -> member count
    {
        FMassEntityQuery Q(EMPtr);
        Q.AddRequirement<FMythicGroupFragment>(EMassFragmentAccess::ReadOnly);
        Q.AddTagRequirement<FMythicGroupMemberTag>(EMassFragmentPresence::All);
        FMassExecutionContext Ctx(EM);
        Q.ForEachEntityChunk(Ctx, [&](FMassExecutionContext &C) {
            const int32 N = C.GetNumEntities();
            const TConstArrayView<FMythicGroupFragment> GrView = C.GetFragmentView<FMythicGroupFragment>();
            for (int32 i = 0; i < N; ++i) {
                ++GroupMemberCount;
                if (GrView[i].bIsLeader) {
                    ++GroupLeaderCount;
                }
                if (GrView[i].GroupId != 0) {
                    ActiveGroupIds.Add(GrView[i].GroupId);
                }
                if (GrView[i].ActivityTag.IsValid()) {
                    ++GroupMembersByActivity.FindOrAdd(GrView[i].ActivityTag.GetTagName());
                }
            }
        });
    }
    const int32 ActiveGroupCount = ActiveGroupIds.Num();
    const int32 MaxActiveGroups = Settings ? Settings->MaxActiveGroups : 0;

    // ── MASS tag tallies (cheap GetNumMatchingEntities — same pattern as above). A standing nonzero spawn/despawn
    //    backlog signals ActorSpawnProcessor starvation; Cognitive cross-checks the AIController iterator in pane 5. ──
    int32 CognitiveTagCount = 0;
    int32 EncOwnedTagCount = 0;
    int32 SpawnReqTagCount = 0;
    int32 DespawnReqTagCount = 0;
    {
        FMassEntityQuery Q(EMPtr);
        Q.AddTagRequirement<FMythicCognitiveTag>(EMassFragmentPresence::All);
        CognitiveTagCount = Q.GetNumMatchingEntities();
    }
    {
        FMassEntityQuery Q(EMPtr);
        Q.AddTagRequirement<FMythicEncounterEntityTag>(EMassFragmentPresence::All);
        EncOwnedTagCount = Q.GetNumMatchingEntities();
    }
    {
        FMassEntityQuery Q(EMPtr);
        Q.AddTagRequirement<FMythicActorSpawnRequestTag>(EMassFragmentPresence::All);
        SpawnReqTagCount = Q.GetNumMatchingEntities();
    }
    {
        FMassEntityQuery Q(EMPtr);
        Q.AddTagRequirement<FMythicActorDespawnRequestTag>(EMassFragmentPresence::All);
        DespawnReqTagCount = Q.GetNumMatchingEntities();
    }

    // ── MASS Pass B: hydrated pressure bloom shapes + nearest list (for Social pane) ──
    struct FNearbyHydrated {
        FMassEntityHandle Handle;
        FMythicCellCoord Cell;
        EMythicSignificanceTier Tier = EMythicSignificanceTier::Tier1_Reactive;
        float Score = 0.0f;
        FString Role;
        int32 DomChannel = 0;
        float DomValue = 0.0f;
        bool bDespaired = false;
        // #8 personality vent routing + #36 in-fragment social (captured from the hydrated archetype).
        int32 DomVent = 0;
        float DomVentValue = 0.0f;
        int32 FragEdges = 0;
        bool bHasMetPlayer = false;
        int32 FightTarget = INDEX_NONE;
        double LastEventTime = 0.0;
    };
    TArray<FNearbyHydrated> NearbyHydrated;
    int32 DespairedCount = 0;       // Psychodynamic.bDespaired across all hydrated entities
    int32 MobOnPlayerCount = 0;     // hydrated entities whose FightTargetEntity points at a valid handle
    {
        FMassEntityQuery Q(EMPtr);
        Q.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
        Q.AddRequirement<FMythicSignificanceFragment>(EMassFragmentAccess::ReadOnly);
        Q.AddRequirement<FMythicPsychodynamicFragment>(EMassFragmentAccess::ReadOnly);
        Q.AddRequirement<FMythicPersonalityFragment>(EMassFragmentAccess::ReadOnly); // #8 VentWeights / #36 social handled in pane 3
        Q.AddRequirement<FMythicSocialFragment>(EMassFragmentAccess::ReadOnly);
        Q.AddTagRequirement<FMythicHydratedTag>(EMassFragmentPresence::All);
        FMassExecutionContext Ctx(EM);
        Q.ForEachEntityChunk(Ctx, [&](FMassExecutionContext &C) {
            const int32 N = C.GetNumEntities();
            const TConstArrayView<FMythicIdentityFragment> IdView = C.GetFragmentView<FMythicIdentityFragment>();
            const TConstArrayView<FMythicSignificanceFragment> SigView = C.GetFragmentView<FMythicSignificanceFragment>();
            const TConstArrayView<FMythicPsychodynamicFragment> PsyView = C.GetFragmentView<FMythicPsychodynamicFragment>();
            const TConstArrayView<FMythicPersonalityFragment> PerView = C.GetFragmentView<FMythicPersonalityFragment>();
            const TConstArrayView<FMythicSocialFragment> SocView = C.GetFragmentView<FMythicSocialFragment>();
            for (int32 i = 0; i < N; ++i) {
                float Sum = 0.0f;
                int32 Dom = 0;
                if (PsyView[i].bDespaired) {
                    ++DespairedCount;
                }
                for (int32 c = 0; c < PressureChannelCount; ++c) {
                    Sum += PsyView[i].Pressure[c];
                    if (PsyView[i].Pressure[c] > PsyView[i].Pressure[Dom]) {
                        Dom = c;
                    }
                }
                if (PsyView[i].FightTargetEntity != INDEX_NONE) {
                    ++MobOnPlayerCount;
                }

                if (bShowEntities && ShapesDrawn < GMaxShapes) {
                    const FVector WorldLoc = GroundCell(IdView[i].Cell) + FVector(0.0f, 0.0f, 160.0f);
                    AddShape(FGameplayDebuggerShape::MakePoint(
                        WorldLoc, 30.0f + 20.0f * FMath::Clamp(Sum / 3.0f, 0.0f, 1.0f),
                        PsyView[i].bDespaired ? FColor::Red : FColor::Orange,
                        FString::Printf(TEXT("%s %.2f"), PressureChannelName(Dom), PsyView[i].Pressure[Dom])));
                    ++ShapesDrawn;
                }

                const int32 D = FMath::Abs(IdView[i].Cell.X - PlayerCell.X) + FMath::Abs(IdView[i].Cell.Y - PlayerCell.Y);
                if (D <= 3 && NearbyHydrated.Num() < 8) {
                    FNearbyHydrated NH;
                    NH.Handle = C.GetEntity(i);
                    NH.Cell = IdView[i].Cell;
                    NH.Tier = SigView[i].Tier;
                    NH.Score = SigView[i].Score;
                    NH.Role = IdView[i].RoleTag.GetTagName().ToString();
                    NH.DomChannel = Dom;
                    NH.DomValue = PsyView[i].Pressure[Dom];
                    NH.bDespaired = PsyView[i].bDespaired;
                    // #8 dominant vent channel from personality VentWeights.
                    int32 DomV = 0;
                    for (int32 v = 1; v < VentChannelCount; ++v) {
                        if (PerView[i].VentWeights[v] > PerView[i].VentWeights[DomV]) {
                            DomV = v;
                        }
                    }
                    NH.DomVent = DomV;
                    NH.DomVentValue = PerView[i].VentWeights[DomV];
                    // #36 in-fragment social.
                    NH.FragEdges = SocView[i].EdgeCount;
                    NH.bHasMetPlayer = SocView[i].bHasMetPlayer;
                    // #34 mob fight target + agitation recency.
                    NH.FightTarget = PsyView[i].FightTargetEntity;
                    NH.LastEventTime = PsyView[i].LastEventTime;
                    NearbyHydrated.Add(NH);
                }
            }
        });
    }

    const int32 TotalNPC = Tier[0] + Tier[1] + Tier[2] + Tier[3];
    const bool bGateBroken = (ProxW < PromoteT0);

    // Cheap header counts from the self-locking getters.
    int32 SocialEntities = 0;
    int32 SocialEdges = 0;
    if (const UMythicSocialGraph *G = LW->GetSocialGraph()) {
        SocialEntities = G->GetEntityCount();
        SocialEdges = G->GetTotalEdgeCount();
    }
    // Scheme snapshot (copy under SchemeLock) — fetched once, reused by header + Schemes/Events panes.
    TArray<FMythicScheme> Schemes;
    if (UMythicSchemeEngine *SE = LW->GetSchemeEngine()) {
        Schemes = SE->GetActiveSchemes();
    }
    // SimulationLock-guarded count: the raw registry getter walks the Settlements TMap the sim thread rehashes.
    const int32 SettlementCount = LW->GetSettlementCountSafe();
    uint32 TotalEvents = 0;
    if (const UMythicCausalFabric *Fabric = LW->GetCausalFabric()) {
        TotalEvents = Fabric->GetTotalEventCount();
    }
    // Encounter Director (a WorldSubsystem — game-thread; mirrors MythicCheatManager's access). Active count + cap.
    int32 EncounterCount = 0;
    int32 EncounterCap = 0;
    UMythicEncounterDirector *Director = World->GetSubsystem<UMythicEncounterDirector>();
    if (Director) {
        EncounterCount = Director->GetActiveEncounterCount();
        EncounterCap = Director->GetMaxActiveEncounters();
    }
    // Persistent permadeath ledger (Tier 2-3) — owned by the LWS, game-thread only.
    int32 DeathCount = 0;
    if (const UMythicPersistentNPCRegistry *Deaths = LW->GetPersistentNPCRegistry()) {
        DeathCount = Deaths->GetDeathCount();
    }

    Header += FString::Printf(
        TEXT("{white}MASS: NPC {yellow}%d{white} (Creatures {grey}%d{white})  T0 {grey}%d{white}|T1 {yellow}%d{white}|T2 {green}%d{white}|T3 {green}%d{white}  Hydrated {yellow}%d{white} Dirty {grey}%d{white}\n")
        TEXT("{white}Phase: Work %d Rest %d Social %d Travel %d Idle %d\n")
        TEXT("{white}Gate T0->T1 >= {yellow}%.2f{white}(+%.2f) | T1->T2 >= {yellow}%.2f{white} | proxW {yellow}%.2f{white} bestAmbient {yellow}%.2f{white}\n")
        TEXT("%s")
        TEXT("{white}Social ent {yellow}%d{white} edges {yellow}%d{white}  Schemes {yellow}%d{white}  Settlements {yellow}%d{white}  Events ever {yellow}%u{white}\n")
        TEXT("{white}Encounters {yellow}%d{white}/{grey}%d{white}  Permadeaths (T2-3) {yellow}%d{white}\n"),
        TotalNPC, CreatureCount, Tier[0], Tier[1], Tier[2], Tier[3], HydratedCount, Dirty,
        Phase[0], Phase[1], Phase[2], Phase[3], Phase[4],
        PromoteT0, HystM, PromoteT2, ProxW, MaxAmbient,
        bGateBroken ? TEXT("{red}GATE: fresh ambient ceiling = proximity < threshold; proximity alone cannot self-hydrate.\n")
                    : TEXT("{green}Proximity alone can cross the promotion gate.\n"),
        SocialEntities, SocialEdges, Schemes.Num(), SettlementCount, TotalEvents,
        EncounterCount, EncounterCap, DeathCount);

    Header += FString::Printf(
        TEXT("{white}Tags: Cognitive {yellow}%d{white} EncOwned {yellow}%d{white} spawnReq {grey}%d{white} despawnReq {grey}%d{white}\n")
        TEXT("{white}Spies {yellow}%d{white} VisGroups {yellow}%d{white} Despaired {red}%d{white} mobbing {yellow}%d{white}  sumRelevantEvents {grey}%u{white}\n"),
        CognitiveTagCount, EncOwnedTagCount, SpawnReqTagCount, DespawnReqTagCount,
        SpyCount, VisGroupCount, DespairedCount, MobOnPlayerCount, SumRelevantEvents);

    // ── DYNAMIC-POPULATION COVERAGE (biome / kinds / roles / species) ──
    // (A) Biome at the player's cell — pure, lock-free Grid read (Plains fallback out-of-bounds / pre-Initialize).
    if (bHasPlayerCell) {
        Header += FString::Printf(TEXT("{white}PlayerCell ({yellow}%d,%d{white}) Biome: {green}%s{white}\n"),
                                  PlayerCell.X, PlayerCell.Y, BiomeToString(Grid->GetBiomeAtCell(PlayerCell)));
    }
    // (B) Population KINDS split out of TotalNPC (soldiers/travelers also carry NPCTag), + per-faction soldier garrison.
    {
        FString SoldierByFacStr;
        if (SoldiersByFaction.Num() > 0) {
            SoldierByFacStr = TEXT(" {grey}[");
            for (const TPair<uint8, int32> &Pair : SoldiersByFaction) {
                SoldierByFacStr += FString::Printf(TEXT(" F%d:%d"), Pair.Key, Pair.Value);
            }
            SoldierByFacStr += TEXT(" ]{white}");
        }
        Header += FString::Printf(TEXT("{white}Kinds: Soldiers {yellow}%d{white}%s  Travelers {yellow}%d{white}  Creatures {grey}%d{white}\n"),
                                  SoldierCount, *SoldierByFacStr, TravelerCount, CreatureCount);
    }
    // (B2) GROUP population (clustered spawns) — active groups vs cap, member/leader tally, per-activity breakdown.
    if (GroupMemberCount > 0 || ActiveGroupCount > 0) {
        FString GroupByActStr;
        if (GroupMembersByActivity.Num() > 0) {
            GroupByActStr = TEXT(" {grey}[");
            int32 ShownAct = 0;
            for (const TPair<FName, int32> &Pair : GroupMembersByActivity) {
                if (ShownAct >= 6) {
                    GroupByActStr += TEXT(" ...");
                    break;
                }
                ++ShownAct;
                GroupByActStr += FString::Printf(TEXT(" %s:%d"), *TagNameLeaf(Pair.Key), Pair.Value);
            }
            GroupByActStr += TEXT(" ]{white}");
        }
        const TCHAR *GroupCapTint = (MaxActiveGroups > 0 && ActiveGroupCount >= MaxActiveGroups) ? TEXT("{red}") : TEXT("{yellow}");
        Header += FString::Printf(TEXT("{white}Groups: active %s%d{white}/{grey}%d{white} members {yellow}%d{white} leaders {yellow}%d{white}%s\n"),
                                  GroupCapTint, ActiveGroupCount, MaxActiveGroups, GroupMemberCount, GroupLeaderCount, *GroupByActStr);
    }
    // (C) Role-from-context tally (Identity.RoleTag) — capped at 10 distinct roles to keep the line bounded.
    if (RoleCounts.Num() > 0) {
        FString RoleStr;
        int32 ShownRoles = 0;
        for (const TPair<FName, int32> &Pair : RoleCounts) {
            if (ShownRoles >= 10) {
                RoleStr += TEXT(" ...");
                break;
            }
            ++ShownRoles;
            // Leaf-only role name (strip the "NPC.Role." prefix) to keep the line tight.
            FString Leaf = Pair.Key.ToString();
            int32 DotIdx = INDEX_NONE;
            if (Leaf.FindLastChar(TEXT('.'), DotIdx)) {
                Leaf = Leaf.RightChop(DotIdx + 1);
            }
            RoleStr += FString::Printf(TEXT(" {yellow}%s{white}:%d"), *Leaf, Pair.Value);
        }
        Header += FString::Printf(TEXT("{white}Roles:%s\n"), *RoleStr);
    } else if (TotalNPC > 0) {
        Header += TEXT("{white}Roles: {grey}(none stamped - RoleTag empty on all NPCs){white}\n");
    }
    // (D) Creature species tally (non-zero buckets only) — name-resolved via the code-default species set.
    if (CreatureCount > 0) {
        FString SpeciesStr;
        int32 ShownSpecies = 0;
        for (int32 s = 0; s < 256; ++s) {
            if (SpeciesCounts[s] <= 0) {
                continue;
            }
            if (ShownSpecies >= 12) {
                SpeciesStr += TEXT(" ...");
                break;
            }
            ++ShownSpecies;
            SpeciesStr += FString::Printf(TEXT(" {yellow}%s{white}:%d"), *SpeciesIdToName(static_cast<uint8>(s)), SpeciesCounts[s]);
        }
        Header += FString::Printf(TEXT("{white}Species:%s\n"), *SpeciesStr);
    }

    // #9 NPC MANAGER ROSTER (game-instance subsystem; active/cached/family/pool sizes — server authoritative).
    if (const UMythicNPCManager *NpcMgr = OwnerPC->GetGameInstance() ? OwnerPC->GetGameInstance()->GetSubsystem<UMythicNPCManager>() : nullptr) {
        Header += FString::Printf(
            TEXT("{white}NPCs(mgr): active {yellow}%d{white} cached {grey}%d{white} families {grey}%d{white}/{grey}%d{white} pooled {grey}%d{white}\n"),
            NpcMgr->GetActiveNPCCount(), NpcMgr->GetCachedNPCCount(),
            NpcMgr->GetActiveFamilyCount(), NpcMgr->GetCachedFamilyCount(), NpcMgr->GetPooledNPCCount());
    }
    // #10 GAME DIRECTOR STREAMING (a LocalPlayerSubsystem — viewing client only; null-guarded).
    if (const ULocalPlayer *LP = OwnerPC->GetLocalPlayer()) {
        if (const UMythicGameDirectorSubsystem *GD = LP->GetSubsystem<UMythicGameDirectorSubsystem>()) {
            Header += FString::Printf(TEXT("{white}World streaming: spawned {yellow}%d{white} levels cached {grey}%d{white}\n"),
                                      GD->SpawnedActors.Num(), GD->CachedLevelActorData.Num());
        }
    }
    // #15 DAMAGE NUMBERS (world subsystem; active pool size).
    if (const UMythicDamageNumberSubsystem *DN = World->GetSubsystem<UMythicDamageNumberSubsystem>()) {
        Header += FString::Printf(TEXT("{white}DmgNumbers active: {yellow}%d{white}\n"), DN->GetActiveDamageNumberCount());
    }

    // ═══════════════════════ LAYER SHAPES (independent of detail pane) ═══════════════════════

    // ── L2: Territory cells (NxN window quads tinted by faction, brightness-scaled by influence) ──
    if (bShowTerritory) {
        const int32 Window = 11;
        const int32 Half = Window / 2;
        const float CellSize = Grid->GetCellSize();
        const float HalfCell = 0.45f * CellSize;
        for (int32 dy = -Half; dy <= Half; ++dy) {
            for (int32 dx = -Half; dx <= Half; ++dx) {
                if (ShapesDrawn >= GMaxShapes) {
                    break;
                }
                const FMythicCellCoord Coord(PlayerCell.X + dx, PlayerCell.Y + dy);
                if (!Grid->IsValidCoord(Coord)) {
                    continue;
                }
                const FMythicTerritoryCell Cell = Grid->GetCell(Coord); // SnapshotLock copy-out
                const FColor Base = FactionColor(Cell.DominantFaction);
                const float Bright = FMath::Clamp(0.25f + 0.75f * Cell.Influence, 0.0f, 1.0f);
                const FColor Shown = (FLinearColor(Base) * Bright).ToFColor(false);
                const FVector Ctr = GroundCell(Coord) + FVector(0.0f, 0.0f, 80.0f);
                TArray<FVector> Quad;
                Quad.Reserve(5);
                Quad.Add(Ctr + FVector(-HalfCell, -HalfCell, 0.0f));
                Quad.Add(Ctr + FVector(HalfCell, -HalfCell, 0.0f));
                Quad.Add(Ctr + FVector(HalfCell, HalfCell, 0.0f));
                Quad.Add(Ctr + FVector(-HalfCell, HalfCell, 0.0f));
                Quad.Add(Ctr + FVector(-HalfCell, -HalfCell, 0.0f));
                AddShape(FGameplayDebuggerShape::MakeSegmentList(
                    Quad, 3.0f, Shown, FString::Printf(TEXT("f%d i%.2f"), Cell.DominantFaction.Index, Cell.Influence)));
                ++ShapesDrawn;
                if (Cell.bPlayerOwned && ShapesDrawn < GMaxShapes) {
                    AddShape(FGameplayDebuggerShape::MakePoint(Ctr, 0.30f * CellSize, FColor::Cyan, TEXT("OWNED")));
                    ++ShapesDrawn;
                }
            }
        }
    }

    // ── L3: Settlement boundary outlines (spline-sampled, colored by governing faction) ──
    if (bShowSettlements) {
        {
            // Enumerate IDs + resolve the spline actor through the SimulationLock-guarded subsystem helpers — the raw
            // registry getters (GetAllSettlementIds / GetSettlementActor) walk TMaps the sim thread rehashes.
            TArray<int32> Ids;
            LW->CopyAllSettlementIds(Ids);
            for (const int32 Id : Ids) {
                if (ShapesDrawn >= GMaxShapes) {
                    break;
                }
                FMythicSettlementData Data;
                if (!LW->CopySettlementById(Id, Data)) {
                    continue; // SimulationLock copy-out
                }
                const FColor FacCol = FactionColor(Data.GoverningFaction);
                // Resolve the actor under the lock; a World-Partition-streamed / pending-kill settlement resolves to a
                // stale weak ptr (null) or an invalid actor — skip it.
                AMythicSettlement *Actor = LW->GetSettlementActorSafe(Id);
                if (!IsValid(Actor)) {
                    continue;
                }
                const USplineComponent *Spline = Actor->GetBoundarySpline();
                if (!IsValid(Spline)) {
                    continue;
                }
                const int32 NumPts = Spline->GetNumberOfSplinePoints();
                if (NumPts < 3) {
                    continue;
                }
                // Sample the boundary by CONTROL POINT (raw point positions) — simple + bounds the vert count to NumPts.
                // (This is NOT what caused the [L] crash; the crash was the self-referential Verts.Add(Verts[0]) below.)
                TArray<FVector> Verts;
                Verts.Reserve(NumPts + 1);
                for (int32 p = 0; p < NumPts; ++p) {
                    FVector P = Spline->GetLocationAtSplinePoint(p, ESplineCoordinateSpace::World);
                    P.Z += 50.0f;
                    Verts.Add(P);
                }
                const FVector First = Verts[0]; // copy out first — Verts[0] aliases Verts' storage, and
                                                // TArray::Add(const T&) calls CheckAddress(&Item), which check()s on a
                                                // self-referential element (Array.h:2196) regardless of Reserve/slack.
                Verts.Add(First);               // close the loop; First is on the stack, so CheckAddress passes.
                AddShape(FGameplayDebuggerShape::MakeSegmentList(Verts, 4.0f, FacCol, Data.DisplayName.ToString()));
                ++ShapesDrawn;
            }
        }
    }

    // ── L4: Causal-event points + scheme links + social edges ──
    if (bShowEvents) {
        // Causal events.
        if (UMythicCausalFabric *Fabric = LW->GetCausalFabric()) {
            const TArray<FMythicWorldEvent> Recent = Fabric->GetRecentEvents(20); // copy under FabricLock
            for (const FMythicWorldEvent &Ev : Recent) {
                if (ShapesDrawn >= GMaxShapes) {
                    break;
                }
                const bool bHostile = (Ev.CategoryFlags & (EMythicEventCategory::Combat | EMythicEventCategory::Death | EMythicEventCategory::Crime)) != 0;
                const bool bDiplo = (Ev.CategoryFlags & (EMythicEventCategory::Diplomacy | EMythicEventCategory::Trade)) != 0;
                const bool bMagic = (Ev.CategoryFlags & (EMythicEventCategory::Magic | EMythicEventCategory::Scheme)) != 0;
                const FColor EvCol = bHostile ? FColor::Red : bDiplo ? FColor::Green : bMagic ? FColor::Magenta : FColor(160, 160, 160);
                const FVector EvPos = GroundCell(Ev.Cell) + FVector(0.0f, 0.0f, 200.0f);
                AddShape(FGameplayDebuggerShape::MakePoint(
                    EvPos, FMath::Clamp(20.0f + Ev.Significance * 60.0f, 20.0f, 80.0f), EvCol,
                    FString::Printf(TEXT("%s %.2f"), *Ev.EventTag.ToString(), Ev.Significance)));
                ++ShapesDrawn;
            }
        }

        // Scheme territorial links (TargetCell valid -> draw target marker + origin->target line).
        static const FColor SchemeShapeColor[] = {
            FColor::Red, FColor::Cyan, FColor::Yellow, FColor(160, 80, 200), FColor(160, 80, 200), FColor::Orange, FColor::Blue};
        for (const FMythicScheme &S : Schemes) {
            if (ShapesDrawn >= GMaxShapes) {
                break;
            }
            if (S.TargetCell.X == 0 && S.TargetCell.Y == 0) {
                continue; // non-territorial / default cell — text-only
            }
            const int32 Ti = FMath::Clamp(static_cast<int32>(S.Type), 0, SchemeTypeCount - 1);
            const FVector TargetW = GroundCell(S.TargetCell) + FVector(0.0f, 0.0f, 150.0f);
            AddShape(FGameplayDebuggerShape::MakePoint(TargetW, 40.0f, SchemeShapeColor[Ti], FString::Printf(TEXT("#%u"), S.SchemeId)));
            ++ShapesDrawn;
            TArray<FMythicCellCoord> OriginCells;
            Grid->GetFactionCells(S.OriginFaction, 1, OriginCells);
            if (OriginCells.Num() > 0 && ShapesDrawn < GMaxShapes) {
                const FVector OriginW = GroundCell(OriginCells[0]) + FVector(0.0f, 0.0f, 150.0f);
                AddShape(FGameplayDebuggerShape::MakeSegment(OriginW, TargetW, 3.0f, SchemeShapeColor[Ti], TEXT("")));
                ++ShapesDrawn;
            }
        }

        // Social edges for nearby hydrated NPCs (segment to target only when the target's cell is known this frame).
        if (const UMythicSocialGraph *G = LW->GetSocialGraph()) {
            static const FColor RelCol[] = {FColor::Green, FColor::Cyan, FColor::Red, FColor(255, 140, 0), FColor::Yellow, FColor::White};
            for (const FNearbyHydrated &Src : NearbyHydrated) {
                if (ShapesDrawn >= GMaxShapes) {
                    break;
                }
                TArray<FMythicSocialEdge> Edges;
                G->GetEdges(Src.Handle, WorldTime, Edges); // read-lock copy-out
                const FVector SrcPos = GroundCell(Src.Cell) + FVector(0.0f, 0.0f, 140.0f);
                for (const FMythicSocialEdge &E : Edges) {
                    if (ShapesDrawn >= GMaxShapes) {
                        break;
                    }
                    if (const FMythicCellCoord *TCell = EntityCellMap.Find(E.TargetEntity)) {
                        const int32 R = FMath::Clamp(static_cast<int32>(E.Relation), 0, static_cast<int32>(EMythicSocialRelation::COUNT) - 1);
                        AddShape(FGameplayDebuggerShape::MakeSegment(SrcPos, GroundCell(*TCell) + FVector(0.0f, 0.0f, 140.0f), 2.0f, RelCol[R], TEXT("")));
                        ++ShapesDrawn;
                    }
                }
            }
        }

        // Active encounters — a point at each encounter's cell, colored by lifecycle state (game-thread WorldSubsystem).
        if (Director) {
            static const FColor EncStateColor[] = {
                FColor(160, 160, 160), // Pending  — grey
                FColor::Yellow,        // Spawning — yellow
                FColor::Red,           // Active   — red
                FColor::Orange,        // Completing
                FColor(80, 80, 80)};   // Completed — dark grey
            for (const FMythicActiveEncounter &Enc : Director->GetActiveEncounters()) {
                if (ShapesDrawn >= GMaxShapes) {
                    break;
                }
                const int32 Si = FMath::Clamp(static_cast<int32>(Enc.State), 0, 4);
                const FVector EncPos = GroundCell(Enc.Cell) + FVector(0.0f, 0.0f, 220.0f);
                AddShape(FGameplayDebuggerShape::MakePoint(
                    EncPos, 55.0f, EncStateColor[Si],
                    FString::Printf(TEXT("ENC #%u %s x%d"), Enc.EncounterId, *Enc.TemplateTag.ToString(), Enc.EntityCount)));
                ++ShapesDrawn;
            }
        }

        // Recent permadeaths (Tier 2-3) — grey skull markers at each DeathCell (newest few records).
        if (const UMythicPersistentNPCRegistry *Deaths = LW->GetPersistentNPCRegistry()) {
            const TArray<FMythicPersistentDeathRecord> &Records = Deaths->GetDeathRecords();
            const int32 Start = FMath::Max(0, Records.Num() - 12);
            for (int32 i = Records.Num() - 1; i >= Start; --i) {
                if (ShapesDrawn >= GMaxShapes) {
                    break;
                }
                const FMythicPersistentDeathRecord &Rec = Records[i];
                const FVector DeathPos = GroundCell(Rec.DeathCell) + FVector(0.0f, 0.0f, 180.0f);
                AddShape(FGameplayDebuggerShape::MakePoint(
                    DeathPos, 35.0f, FColor(110, 110, 110),
                    FString::Printf(TEXT("DIED %s F%d"), *Rec.RoleTag.GetTagName().ToString(), Rec.Faction.Index)));
                ++ShapesDrawn;
            }
        }

        // Creature den markers + aggression tint (#35). One ForEachEntityChunk over the creature archetype.
        {
            FMassEntityQuery Q(EMPtr);
            Q.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
            Q.AddRequirement<FMythicCreatureFragment>(EMassFragmentAccess::ReadOnly);
            Q.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);
            FMassExecutionContext Ctx(EM);
            Q.ForEachEntityChunk(Ctx, [&](FMassExecutionContext &C) {
                const int32 N = C.GetNumEntities();
                const TConstArrayView<FMythicIdentityFragment> IdView = C.GetFragmentView<FMythicIdentityFragment>();
                const TConstArrayView<FMythicCreatureFragment> CrView = C.GetFragmentView<FMythicCreatureFragment>();
                for (int32 i = 0; i < N; ++i) {
                    if (ShapesDrawn >= GMaxShapes) {
                        break;
                    }
                    // Only draw creatures near the player's cell (the den ring is cell-scaled, so bound the spam).
                    const int32 Dist = FMath::Abs(IdView[i].Cell.X - PlayerCell.X) + FMath::Abs(IdView[i].Cell.Y - PlayerCell.Y);
                    if (Dist > 8) {
                        continue;
                    }
                    const float Aggr = FMath::Clamp(CrView[i].CurrentAggression, 0.0f, 1.0f);
                    const FColor AggrCol = FLinearColor(Aggr, 1.0f - Aggr, 0.0f).ToFColor(false); // green->red by aggression
                    const FVector Pos = GroundCell(IdView[i].Cell) + FVector(0.0f, 0.0f, 130.0f);
                    AddShape(FGameplayDebuggerShape::MakePoint(
                        Pos, 22.0f, AggrCol,
                        FString::Printf(TEXT("%s pk%d aggr %.2f"), *SpeciesIdToName(CrView[i].SpeciesId), CrView[i].PackId, Aggr)));
                    ++ShapesDrawn;
                    // Den marker + territorial-radius ring (drawn as a square outline scaled by the radius in cells).
                    if (ShapesDrawn < GMaxShapes) {
                        const float CellSize = Grid->GetCellSize();
                        const float R = FMath::Max(1, static_cast<int32>(CrView[i].TerritorialRadius)) * CellSize;
                        const FVector DenCtr = GroundCell(CrView[i].DenCell) + FVector(0.0f, 0.0f, 90.0f);
                        TArray<FVector> Ring;
                        Ring.Reserve(5);
                        Ring.Add(DenCtr + FVector(-R, -R, 0.0f));
                        Ring.Add(DenCtr + FVector(R, -R, 0.0f));
                        Ring.Add(DenCtr + FVector(R, R, 0.0f));
                        Ring.Add(DenCtr + FVector(-R, R, 0.0f));
                        Ring.Add(DenCtr + FVector(-R, -R, 0.0f));
                        AddShape(FGameplayDebuggerShape::MakeSegmentList(Ring, 2.0f, FColor(120, 90, 60), FString::Printf(TEXT("den pk%d"), CrView[i].PackId)));
                        ++ShapesDrawn;
                    }
                }
            });
        }
    }

    // ── L5: Crime markers ([C] toggle) — server-only ActionEventSubsystem crime report queue ──
    if (bShowCrime) {
        if (const UMythicActionEventSubsystem *AE = World->GetSubsystem<UMythicActionEventSubsystem>()) {
            // GetCrimeReportQueue returns a non-const ref (for processors); we read it ONLY — never mutate/drain.
            const FMythicCrimeReportQueue &Queue = const_cast<UMythicActionEventSubsystem *>(AE)->GetCrimeReportQueue();
            for (const FMythicCrimeRecord &Cr : Queue.PendingReports) {
                if (ShapesDrawn >= GMaxShapes) {
                    break;
                }
                const FVector Pos = GroundCell(Cr.Cell) + FVector(0.0f, 0.0f, 210.0f);
                AddShape(FGameplayDebuggerShape::MakePoint(
                    Pos, 45.0f, MoralSeverityColor(Cr.Severity),
                    FString::Printf(TEXT("%s F%d>F%d wit%d"), MoralSeverityName(Cr.Severity), Cr.PerpFaction.Index, Cr.ViolatedFaction.Index, Cr.DirectWitnessCount)));
                ++ShapesDrawn;
            }
        }
    }

    // ═══════════════════════ DETAIL PANE (exactly one) ═══════════════════════
    switch (ActiveDetail) {
    case 0: { // Factions + Economy
        Detail += TEXT("{white}=== DETAIL: FACTIONS + ECONOMY ===\n");
        if (!FDB) {
            Detail += TEXT("{red}Faction DB not available.\n");
            break;
        }
        FDB->ForEachAliveFaction([&Detail](FMythicFactionId Id, const FMythicFactionData &D) {
            FString Flags;
            if (D.bControlsTerritory) {
                Flags += TEXT("T");
            }
            if (D.bHasEconomy) {
                Flags += TEXT("E");
            }
            if (D.bHasCivilianPopulation) {
                Flags += TEXT("C");
            }
            if (D.bParticipatesInTrade) {
                Flags += TEXT("$");
            }
            if (D.bCanNegotiate) {
                Flags += TEXT("D");
            }
            const bool bDeficit = (D.Reserves.Food < 0.0f || D.Reserves.Materials < 0.0f || D.Reserves.Arms < 0.0f || D.Reserves.Wealth < 0.0f);
            Detail += FString::Printf(TEXT("{white}[%d] {green}%s {grey}(%s) {white}[%s]\n"),
                                      Id.Index, *D.DisplayName.ToString(), *D.FactionTag.ToString(), *Flags);
            Detail += FString::Printf(TEXT("  {white}Pop {yellow}%d{white} Cells {yellow}%d{white} Mil {yellow}%.2f{white} Leader#{grey}%d (sig %.2f)\n"),
                                      D.Population, D.ControlledCellCount, D.MilitaryStrength, D.LeaderEntityId, D.LeaderSignificanceScore);
            Detail += FString::Printf(TEXT("  Reserves %sF %.1f M %.1f A %.1f W %.1f{white}  Prices F %.2f M %.2f A %.2f W %.2f\n"),
                                      bDeficit ? TEXT("{red}") : TEXT("{green}"),
                                      D.Reserves.Food, D.Reserves.Materials, D.Reserves.Arms, D.Reserves.Wealth,
                                      D.Prices.Food, D.Prices.Materials, D.Prices.Arms, D.Prices.Wealth);
        });
        break;
    }
    case 1: { // Diplomacy matrix
        Detail += TEXT("{white}=== DETAIL: DIPLOMACY ===\n");
        if (!FDB) {
            Detail += TEXT("{red}Faction DB not available.\n");
            break;
        }
        Detail += TEXT("{white}Legend: {green}A=Allied/F=Friendly {white}N=Neutral {yellow}u=Unfriendly {red}H=Hostile\n");
        // Pass 1: collect ids+names WITHOUT nesting a second DB read (SnapshotLock held across this callback).
        struct FFac {
            FMythicFactionId Id;
            FString Name;
        };
        TArray<FFac> Facs;
        FDB->ForEachAliveFaction([&Facs](FMythicFactionId Id, const FMythicFactionData &D) { Facs.Add({Id, D.DisplayName.ToString()}); });
        static const TCHAR *RelGlyph[] = {TEXT("A"), TEXT("F"), TEXT("N"), TEXT("u"), TEXT("H")};
        static const TCHAR *RelColor[] = {TEXT("{green}"), TEXT("{green}"), TEXT("{white}"), TEXT("{yellow}"), TEXT("{red}")};
        // Pass 2: build the matrix (each GetRelationship locks independently — no nesting).
        for (int32 r = 0; r < Facs.Num(); ++r) {
            FString Row = FString::Printf(TEXT("{white}[%d] %s : "), Facs[r].Id.Index, *Facs[r].Name);
            int32 Allies = 0;
            int32 Wars = 0;
            for (int32 c = 0; c < Facs.Num(); ++c) {
                if (c == r) {
                    Row += TEXT("{white}-");
                    continue;
                }
                const EMythicFactionRelation Rel = FDB->GetRelationship(Facs[r].Id, Facs[c].Id);
                const int32 Ri = FMath::Clamp(static_cast<int32>(Rel), 0, 4);
                Row += FString::Printf(TEXT("%s%s"), RelColor[Ri], RelGlyph[Ri]);
                if (Rel == EMythicFactionRelation::Allied) {
                    ++Allies;
                }
                if (Rel == EMythicFactionRelation::Hostile) {
                    ++Wars;
                }
            }
            Detail += Row + FString::Printf(TEXT("{white}  allies {green}%d{white} wars {red}%d{white}\n"), Allies, Wars);
        }
        break;
    }
    case 2: { // Schemes
        Detail += FString::Printf(TEXT("{white}=== DETAIL: SCHEMES ({yellow}%d{white}) ===\n"), Schemes.Num());
        static const TCHAR *TypeName[] = {TEXT("Assassination"), TEXT("TradeDisruption"), TEXT("TerritoryReclaim"),
                                          TEXT("SpyInfiltration"), TEXT("CompanionRecruitment"), TEXT("MilitaryRaid"), TEXT("DiplomaticPressure")};
        static const TCHAR *StateName[] = {TEXT("Planning"), TEXT("InProgress"), TEXT("Succeeded"), TEXT("Failed"), TEXT("Discovered")};
        auto FacName = [FDB](FMythicFactionId Id) -> FString {
            FMythicFactionData FD;
            if (FDB && FDB->GetFaction(Id, FD)) {
                return FD.DisplayName.ToString();
            }
            return FString::Printf(TEXT("F%d"), Id.Index);
        };
        int32 Shown = 0;
        for (const FMythicScheme &S : Schemes) {
            if (Shown >= 50) {
                break;
            }
            ++Shown;
            const int32 Ti = FMath::Clamp(static_cast<int32>(S.Type), 0, SchemeTypeCount - 1);
            const int32 Si = FMath::Clamp(static_cast<int32>(S.State), 0, 4);
            const TCHAR *StateColor = S.IsDiscovered() ? TEXT("{red}")
                                      : (S.State == EMythicSchemeState::Succeeded) ? TEXT("{green}")
                                      : (S.State == EMythicSchemeState::Failed)    ? TEXT("{grey}")
                                                                                   : TEXT("{green}");
            FString CellStr = (S.TargetCell.X != 0 || S.TargetCell.Y != 0) ? FString::Printf(TEXT(" cell (%d,%d)"), S.TargetCell.X, S.TargetCell.Y) : TEXT("");
            Detail += FString::Printf(TEXT("{white}[#%u] {yellow}%s{white} (%s%s{white}): %s{grey} -> {white}%s  prog {yellow}%.0f%%{white} risk {red}%.2f{white}%s\n"),
                                      S.SchemeId, TypeName[Ti], StateColor, StateName[Si],
                                      *FacName(S.OriginFaction), *FacName(S.TargetFaction), S.Progress * 100.0f, S.DetectionRisk, *CellStr);
        }
        if (Schemes.Num() == 0) {
            Detail += TEXT("{grey}(no active schemes)\n");
        }
        break;
    }
    case 3: { // Social graph + nearby relationships
        Detail += FString::Printf(TEXT("{white}=== DETAIL: SOCIAL (ent {yellow}%d{white} edges {yellow}%d{white}) ===\n"), SocialEntities, SocialEdges);
        if (const UMythicSocialGraph *G = LW->GetSocialGraph()) {
            static const TCHAR *RelName[] = {TEXT("Friend"), TEXT("Family"), TEXT("Rival"), TEXT("Debt"), TEXT("Associate"), TEXT("Subordinate")};
            int32 ShownNpcs = 0;
            for (const FNearbyHydrated &Src : NearbyHydrated) {
                if (ShownNpcs >= 8) {
                    break;
                }
                ++ShownNpcs;
                TArray<FMythicSocialEdge> Edges;
                const int32 NumE = G->GetEdges(Src.Handle, WorldTime, Edges);
                // NPC header line — now also carries personality vent routing (#8) + in-fragment social (#36) + mob/agitation (#34).
                const double Agit = WorldTime - Src.LastEventTime;
                Detail += FString::Printf(TEXT("{white}NPC i:%d (%d,%d) T%d %.2f %s: {green}%d gEdges{white} {grey}frag %d met:%s{white} vent {yellow}%s %.2f{white}%s\n"),
                                          Src.Handle.Index, Src.Cell.X, Src.Cell.Y, static_cast<int32>(Src.Tier), Src.Score, *Src.Role, NumE,
                                          Src.FragEdges, Src.bHasMetPlayer ? TEXT("Y") : TEXT("N"), VentChannelName(Src.DomVent), Src.DomVentValue,
                                          Src.FightTarget != INDEX_NONE ? *FString::Printf(TEXT(" {red}mob->i:%d{white} agit %.0fs"), Src.FightTarget, Agit) : TEXT(""));
                for (const FMythicSocialEdge &E : Edges) {
                    const int32 R = FMath::Clamp(static_cast<int32>(E.Relation), 0, static_cast<int32>(EMythicSocialRelation::COUNT) - 1);
                    const double Age = WorldTime - E.LastInteractionTime;
                    Detail += FString::Printf(TEXT("  {white}%s->i:%d F%d str {yellow}%.2f{white} age {grey}%.0fs{white}\n"),
                                              RelName[R], E.TargetEntity.Index, E.TargetFaction.Index, E.Strength, Age);
                }
            }
            if (ShownNpcs == 0) {
                Detail += TEXT("{grey}(no nearby hydrated NPCs)\n");
            }
        } else {
            Detail += TEXT("{grey}Social graph not initialized.\n");
        }
        break;
    }
    case 4: { // Settlements list
        Detail += TEXT("{white}=== DETAIL: SETTLEMENTS ===\n");
        if (LW->GetSettlementRegistry()) {
            // Enumerate + count through the SimulationLock-guarded subsystem helpers (the raw registry getters walk the
            // Settlements TMap the sim thread rehashes); per-settlement DATA is still snapshotted via CopySettlementById.
            TArray<int32> Ids;
            LW->CopyAllSettlementIds(Ids);
            Detail += FString::Printf(TEXT("{white}Settlements: {yellow}%d{white}\n"), LW->GetSettlementCountSafe());
            int32 Shown = 0;
            for (const int32 Id : Ids) {
                if (Shown >= 40) {
                    break;
                }
                FMythicSettlementData Data;
                if (!LW->CopySettlementById(Id, Data)) {
                    continue;
                }
                ++Shown;
                FString FactionName = TEXT("<unresolved>");
                if (FDB && Data.GoverningFaction.IsValid()) {
                    FMythicFactionData FD;
                    if (FDB->GetFaction(Data.GoverningFaction, FD)) {
                        FactionName = FD.DisplayName.ToString();
                    }
                }
                Detail += FString::Printf(TEXT("  {white}[ID=%d] %s %s faction %s%s{white} (#%d) cells {yellow}%d{white} maxDens {yellow}%d{white} pop {yellow}%d{white}\n"),
                                          Id, *Data.DisplayName.ToString(),
                                          Data.bIsCapital ? TEXT("{yellow}(CAPITAL){white}") : TEXT(""),
                                          Data.GoverningFaction.IsValid() ? TEXT("{green}") : TEXT("{red}"),
                                          *FactionName, Data.GoverningFaction.Index,
                                          Data.RasterizedCells.Num(), Data.MaxPopulationDensity, Data.CurrentEntityCount);
            }
        } else {
            Detail += TEXT("{red}Settlement registry not available.\n");
        }
        break;
    }
    case 5: { // Party / Companions / Players
        Detail += TEXT("{white}=== DETAIL: COMPANIONS & PLAYERS ===\n");
        UMythicPartySubsystem *PartySys = World->GetSubsystem<UMythicPartySubsystem>();
        UMythicPlayerRegistrySubsystem *Reg = World->GetSubsystem<UMythicPlayerRegistrySubsystem>();
        AGameStateBase *GS = World->GetGameState();
        if (!PartySys || !GS) {
            Detail += TEXT("{red}Party subsystem / GameState not available (server-only, running world).\n");
            break;
        }
        Detail += FString::Printf(TEXT("{white}Registered players: {yellow}%d{white}  Registry: %s\n"),
                                  Reg ? Reg->GetRegisteredCount() : GS->PlayerArray.Num(), Reg ? TEXT("{green}OK{white}") : TEXT("{red}missing{white}"));
        for (APlayerState *PSBase : GS->PlayerArray) {
            AMythicPlayerState *PS = Cast<AMythicPlayerState>(PSBase);
            if (!PS) {
                continue;
            }
            const FString Key = PS->GetCanonicalPlayerKey();
            APawn *Leader = Reg ? Reg->GetPawnForKey(Key) : nullptr;
            if (!Leader && PS->GetPlayerController()) {
                Leader = PS->GetPlayerController()->GetPawn();
            }
            TArray<FMythicPartyMember> Members; // by-value snapshot — safe, no lock
            const int32 N = PartySys->GetPartyMembers(Key, Members);
            // #4 persistent character id appended to the player row ("<unsaved>" until a character is loaded).
            const FString PersistId = PS->GetPersistentCharacterId();
            Detail += FString::Printf(TEXT("{white}Player {yellow}%s{white} | Party: {yellow}%d{white}/4 | id {grey}%s{white}\n"),
                                      *Key, N, PersistId.IsEmpty() ? TEXT("<unsaved>") : *PersistId);

            // #1 FACTION STANDING — per-player standing toward each faction (non-zero only), tinted by tier.
            if (const UMythicFactionStandingComponent *Standing = PS->GetFactionStanding()) {
                const TArray<FMythicFactionStandingEntry> &Entries = Standing->GetStandings();
                FString StandLine;
                int32 ShownStand = 0;
                for (const FMythicFactionStandingEntry &E : Entries) {
                    if (FMath::IsNearlyZero(E.Value) || ShownStand >= 12) {
                        continue;
                    }
                    ++ShownStand;
                    const EMythicStandingTier StandTier = Standing->TierForStanding(E.Value);
                    const TCHAR *Tint = (StandTier == EMythicStandingTier::Hostile) ? TEXT("{red}")
                                        : (StandTier == EMythicStandingTier::Friendly) ? TEXT("{green}")
                                                                                       : TEXT("{white}");
                    const TCHAR *TierName = (StandTier == EMythicStandingTier::Hostile) ? TEXT("Hostile")
                                            : (StandTier == EMythicStandingTier::Friendly) ? TEXT("Friendly")
                                                                                           : TEXT("Neutral");
                    FString FacName = FString::Printf(TEXT("F%d"), E.Faction.Index);
                    if (FDB && E.Faction.IsValid()) {
                        FMythicFactionData FD;
                        if (FDB->GetFaction(E.Faction, FD)) {
                            FacName = FD.DisplayName.ToString();
                        }
                    }
                    StandLine += FString::Printf(TEXT(" %s%s %.0f [%s]{white}"), Tint, *FacName, E.Value, TierName);
                }
                if (ShownStand > 0) {
                    Detail += FString::Printf(TEXT("    {white}Standing:%s (host<=%.0f friend>=%.0f)\n"),
                                              *StandLine, Standing->GetHostileThreshold(), Standing->GetFriendlyThreshold());
                }
            }

            // #2/#3 PROFICIENCY/LEVEL + OBJECTIVES — via the resolved AMythicPlayerController.
            if (AMythicPlayerController *MPC = Cast<AMythicPlayerController>(PS->GetPlayerController())) {
                Detail += FString::Printf(TEXT("    {white}Lvl {yellow}%d{white} ({yellow}%.0f%%{white})\n"),
                                          MPC->GetPlayerLevel(), MPC->GetPlayerLevelProgress() * 100.0f);
                const TArray<FProficiencySummary> Profs = MPC->GetProficiencySummaries();
                int32 ShownProf = 0;
                for (const FProficiencySummary &P : Profs) {
                    if (ShownProf >= 8) {
                        break;
                    }
                    ++ShownProf;
                    Detail += FString::Printf(TEXT("      %s L{yellow}%d{white} {grey}%.0f/%.0f{white} ({yellow}%.0f%%{white})\n"),
                                              *P.Name.ToString(), P.Level, P.CurrentXP, P.LevelXPEnd, P.ProgressFraction * 100.0f);
                }
                // #3 OBJECTIVES.
                if (const UObjectiveTracker *Obj = MPC->GetObjectiveTracker()) {
                    Detail += FString::Printf(TEXT("    {white}Objectives: {yellow}%d{white} active / {green}%d{white} done\n"),
                                              Obj->GetActiveCount(), Obj->GetCompletedCount());
                    const TArray<FObjectiveSummary> ObjSums = Obj->GetActiveObjectiveSummaries();
                    int32 ShownObj = 0;
                    for (const FObjectiveSummary &O : ObjSums) {
                        if (ShownObj >= 6) {
                            break;
                        }
                        ++ShownObj;
                        Detail += FString::Printf(TEXT("      %s %d/%d ({yellow}%.0f%%{white})%s\n"),
                                                  *O.DisplayText.ToString(), O.CurrentCount, O.RequiredCount,
                                                  O.ProgressFraction * 100.0f, O.bCompleted ? TEXT(" {green}DONE{white}") : TEXT(""));
                    }
                }
            }

            // #12 player life/downed on the resolved leader pawn (co-op down state, currently invisible).
            if (Leader) {
                if (UMythicLifeComponent *Life = UMythicLifeComponent::FindHealthComponent(Leader)) {
                    Detail += FString::Printf(TEXT("    {white}HP {yellow}%.0f{white}/{yellow}%.0f{white} %s\n"),
                                              Life->GetHealth(), Life->GetMaxHealth(),
                                              Life->IsDowned() ? TEXT("{red}DOWNED{white}") : TEXT(""));
                }
            }
            for (int32 i = 0; i < Members.Num(); ++i) {
                const FMythicPartyMember &M = Members[i];
                AMythicNPCCharacter *NPC = M.NPCActor.Get();
                const TCHAR *LoyalCol = (M.LoyaltyScore >= 0.6f) ? TEXT("{green}") : (M.LoyaltyScore >= 0.15f ? TEXT("{yellow}") : TEXT("{red}"));
                const TCHAR *BetrayCol = (M.BetrayalPressure >= 5.0f) ? TEXT("{red}") : (M.BetrayalPressure >= 3.0f ? TEXT("{yellow}") : TEXT("{grey}"));
                Detail += FString::Printf(TEXT("  [%d] %s{white} | Loyalty %s%.2f{white} | Betrayal %s%.2f{white}/5.0 | Beliefs {yellow}%d{white} | OrigFac {grey}%d{white} | %s\n"),
                                          i, NPC ? *M.CachedDisplayName.ToString() : TEXT("{grey}<rebuilding>"),
                                          LoyalCol, M.LoyaltyScore, BetrayCol, M.BetrayalPressure,
                                          M.SharedBeliefs.Num(), M.OriginalFaction.Index, *UEnum::GetValueAsString(M.RebuildState));

                if (NPC) {
                    const FVector Loc = NPC->GetActorLocation() + FVector(0.0f, 0.0f, 110.0f);
                    const FColor C = (M.BetrayalPressure >= 5.0f || M.LoyaltyScore < 0.15f) ? FColor::Red : (M.LoyaltyScore >= 0.6f ? FColor::Green : FColor::Yellow);
                    if (ShapesDrawn < GMaxShapes) {
                        AddShape(FGameplayDebuggerShape::MakePoint(Loc, 30.0f, C, FString::Printf(TEXT("L%.2f B%.2f"), M.LoyaltyScore, M.BetrayalPressure)));
                        ++ShapesDrawn;
                    }
                    if (Leader && ShapesDrawn < GMaxShapes) {
                        AddShape(FGameplayDebuggerShape::MakeSegment(Loc, Leader->GetActorLocation() + FVector(0.0f, 0.0f, 90.0f), 2.0f, FColor::Cyan, TEXT("follows")));
                        ++ShapesDrawn;
                    }
                    if (AMythicAIController *AICon = Cast<AMythicAIController>(NPC->GetController())) {
                        if (AActor *Tgt = AICon->GetCurrentHostileTarget()) {
                            TArray<TPair<TWeakObjectPtr<AActor>, float>> Threats;
                            AICon->CopyThreatTable(Threats);
                            float TgtThreat = 0.0f;
                            for (const TPair<TWeakObjectPtr<AActor>, float> &Pair : Threats) {
                                if (Pair.Key.Get() == Tgt) {
                                    TgtThreat = Pair.Value;
                                }
                            }
                            Detail += FString::Printf(TEXT("      Target: {red}%s{white} (threat %.1f, %d attackers)\n"), *Tgt->GetName(), TgtThreat, Threats.Num());
                            if (ShapesDrawn < GMaxShapes) {
                                AddShape(FGameplayDebuggerShape::MakeSegment(Loc, Tgt->GetActorLocation(), 2.0f, FColor::Red, TEXT("aggro")));
                                ++ShapesDrawn;
                            }
                            // #37 leash ring around the engage anchor when leashed (LeashRange > 0).
                            FMythicAIDebugState Dbg;
                            AICon->CopyAIDebugState(Dbg);
                            if (Dbg.LeashRange > 0.0f && ShapesDrawn < GMaxShapes) {
                                const float R = Dbg.LeashRange;
                                TArray<FVector> Ring;
                                Ring.Reserve(5);
                                Ring.Add(Dbg.EngageAnchorLocation + FVector(-R, -R, 0.0f));
                                Ring.Add(Dbg.EngageAnchorLocation + FVector(R, -R, 0.0f));
                                Ring.Add(Dbg.EngageAnchorLocation + FVector(R, R, 0.0f));
                                Ring.Add(Dbg.EngageAnchorLocation + FVector(-R, R, 0.0f));
                                Ring.Add(Dbg.EngageAnchorLocation + FVector(-R, -R, 0.0f));
                                AddShape(FGameplayDebuggerShape::MakeSegmentList(Ring, 2.0f, FColor(255, 140, 0), TEXT("leash")));
                                ++ShapesDrawn;
                            }
                        } else {
                            // #37 non-combat runtime: idle/patrol/flee/companion-follow.
                            FMythicAIDebugState Dbg;
                            AICon->CopyAIDebugState(Dbg);
                            const TCHAR *Mode = Dbg.bCompanionFollowActive ? TEXT("{cyan}follow") : (Dbg.bFleeingMove ? TEXT("{red}flee") : TEXT("{grey}idle"));
                            Detail += FString::Printf(TEXT("      %s{white} patrolLeg %d%s\n"),
                                                      Mode, Dbg.PatrolLegIndex,
                                                      Dbg.bCompanionFollowActive && !Dbg.CompanionLeaderKey.IsEmpty() ? *FString::Printf(TEXT(" -> %s"), *Dbg.CompanionLeaderKey) : TEXT(""));
                        }
                    }
                }
            }
            if (N == 0) {
                Detail += TEXT("  {grey}(no companions)\n");
            }
        }
        // Nearby non-party embodied NPCs.
        int32 NearbyEmbodied = 0;
        int32 NearbyWithTarget = 0;
        for (TActorIterator<AMythicAIController> It(World); It; ++It) {
            AMythicAIController *AICon = *It;
            if (!AICon || !AICon->GetPawn()) {
                continue;
            }
            ++NearbyEmbodied;
            if (AICon->GetCurrentHostileTarget()) {
                ++NearbyWithTarget;
            }
        }
        Detail += FString::Printf(TEXT("{white}Embodied AI controllers: {yellow}%d{white} | with a hostile target: {red}%d{white}\n"), NearbyEmbodied, NearbyWithTarget);
        break;
    }
    case 6: { // Causal events (recent log)
        Detail += TEXT("{white}=== DETAIL: CAUSAL EVENTS ===\n");
        if (UMythicCausalFabric *Fabric = LW->GetCausalFabric()) {
            const int32 MaxRecent = 14;
            const TArray<FMythicWorldEvent> Recent = Fabric->GetRecentEvents(MaxRecent); // copy under FabricLock, oldest-first
            Detail += FString::Printf(TEXT("{white}Total ever: {yellow}%u{white}  capacity: {yellow}%d{white}  showing last {yellow}%d{white}\n"),
                                      Fabric->GetTotalEventCount(), Fabric->GetCapacity(), Recent.Num());
            for (int32 i = Recent.Num() - 1; i >= 0; --i) { // print newest first
                const FMythicWorldEvent &Ev = Recent[i];
                const bool bHostile = (Ev.CategoryFlags & (EMythicEventCategory::Combat | EMythicEventCategory::Death | EMythicEventCategory::Crime)) != 0;
                const bool bDiplo = (Ev.CategoryFlags & (EMythicEventCategory::Diplomacy | EMythicEventCategory::Trade)) != 0;
                const bool bMagic = (Ev.CategoryFlags & (EMythicEventCategory::Magic | EMythicEventCategory::Scheme)) != 0;
                const TCHAR *Tint = bHostile ? TEXT("{red}") : bDiplo ? TEXT("{green}") : bMagic ? TEXT("{magenta}") : TEXT("{grey}");
                Detail += FString::Printf(TEXT("  #%u %s%s{white} @ (%d,%d) F%d->F%d sig {yellow}%.2f{white} t={grey}%.1fs{white}\n"),
                                          Ev.EventId, Tint, *Ev.EventTag.ToString(), Ev.Cell.X, Ev.Cell.Y,
                                          Ev.PrimaryFaction.Index, Ev.SecondaryFaction.Index, Ev.Significance, Ev.WorldTime);
            }
            if (Recent.Num() == 0) {
                Detail += TEXT("{grey}(causal fabric empty)\n");
            }
        } else {
            Detail += TEXT("{red}Causal fabric not available.\n");
        }

        // ── Active encounters sub-section (EncounterDirector — game-thread WorldSubsystem) ──
        Detail += FString::Printf(TEXT("{white}--- ENCOUNTERS ({yellow}%d{white}/{grey}%d{white}) ---\n"), EncounterCount, EncounterCap);
        if (Director) {
            static const TCHAR *EncStateName[] = {TEXT("Pending"), TEXT("Spawning"), TEXT("Active"), TEXT("Completing"), TEXT("Completed")};
            const TArray<FMythicActiveEncounter> &Encs = Director->GetActiveEncounters();
            for (const FMythicActiveEncounter &Enc : Encs) {
                const int32 Si = FMath::Clamp(static_cast<int32>(Enc.State), 0, 4);
                const TCHAR *Tint = (Enc.State == EMythicEncounterState::Active) ? TEXT("{red}") : (Enc.State == EMythicEncounterState::Pending) ? TEXT("{grey}") : TEXT("{yellow}");
                Detail += FString::Printf(TEXT("  #%u %s%s{white} (%s) @ (%d,%d) F%d ent {yellow}%d{white}\n"),
                                          Enc.EncounterId, Tint, *Enc.TemplateTag.ToString(), EncStateName[Si],
                                          Enc.Cell.X, Enc.Cell.Y, Enc.OriginFaction.Index, Enc.EntityCount);
            }
            if (Encs.Num() == 0) {
                Detail += TEXT("{grey}(no active encounters)\n");
            }
        } else {
            Detail += TEXT("{grey}(encounter director not available)\n");
        }

        // ── Recent permadeaths sub-section (Tier 2-3 ledger from the PersistentNPCRegistry) ──
        if (const UMythicPersistentNPCRegistry *Deaths = LW->GetPersistentNPCRegistry()) {
            const TArray<FMythicPersistentDeathRecord> &Records = Deaths->GetDeathRecords();
            Detail += FString::Printf(TEXT("{white}--- PERMADEATHS (T2-3, total {yellow}%d{white}) ---\n"), Deaths->GetDeathCount());
            const int32 Start = FMath::Max(0, Records.Num() - 10);
            for (int32 i = Records.Num() - 1; i >= Start; --i) { // newest first
                const FMythicPersistentDeathRecord &Rec = Records[i];
                const double Age = WorldTime - Rec.DeathTime;
                Detail += FString::Printf(TEXT("  {grey}[X]{white} %s F%d @ (%d,%d) {grey}%.0fs ago{white}\n"),
                                          *Rec.RoleTag.GetTagName().ToString(), Rec.Faction.Index, Rec.DeathCell.X, Rec.DeathCell.Y, Age);
            }
            if (Records.Num() == 0) {
                Detail += TEXT("{grey}(no permadeaths recorded)\n");
            }
        }
        break;
    }
    case 7: { // Environment (full)
        Detail += TEXT("{white}=== DETAIL: ENVIRONMENT ===\n");
        UMythicEnvironmentSubsystem *Env = OwnerPC->GetGameInstance() ? OwnerPC->GetGameInstance()->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr;
        AMythicEnvironmentController *Ctrl = Env ? Env->GetEnvironmentController() : nullptr;
        if (!Env || !Ctrl) {
            Detail += TEXT("{red}Environment controller not found (no clock/weather in this world).\n");
            break;
        }
        const FTimespan Ts = Ctrl->GetTimespan();
        const FGameplayTag WeatherTag = Env->GetWeather();
        const bool bPaused = Env->IsWeatherPaused();
        const float SunYaw = Ctrl->GetSunPositionForCurrentTime();
        Detail += FString::Printf(TEXT("{white}Clock: {yellow}Y%d M%d (%s) D%d %02d:%02d{white}  phase: {yellow}%s{white}\n"),
                                  GetYear(Ts), GetMonthOfYear(Ts), SeasonToString(Env->GetSeason()), GetDayOfMonth(Ts),
                                  Ts.GetHours(), Ts.GetMinutes(), DayTimeToString(Env->GetDayTime()));
        Detail += FString::Printf(TEXT("{white}Weather: {yellow}%s{white}  cycle: %s\n"),
                                  WeatherTag.IsValid() ? *WeatherTag.ToString() : TEXT("(none)"), bPaused ? TEXT("{red}PAUSED{white}") : TEXT("{green}running{white}"));
        Detail += FString::Printf(TEXT("{white}Sun yaw: {yellow}%.1f deg{grey} (0=18h 90=00h 180=06h 270=12h){white}\n"), SunYaw);
        // Current weather type + the active "guaranteed" transition goal (the next transitions chain toward it).
        const UWeatherType *CurWeather = Ctrl->GetCurrentWeather();
        const UWeatherType *GoalWeather = Ctrl->GetGuaranteedTargetWeather();
        Detail += FString::Printf(TEXT("{white}Current type: {yellow}%s{white}  guaranteed goal: %s\n"),
                                  CurWeather && CurWeather->Tag.IsValid() ? *CurWeather->Tag.ToString() : TEXT("(none)"),
                                  GoalWeather && GoalWeather->Tag.IsValid() ? *FString::Printf(TEXT("{yellow}%s{white}"), *GoalWeather->Tag.ToString()) : TEXT("{grey}(stable - no pending goal){white}"));
        // The action-event subsystem owns the weather/time perception multiplier (server-only — null on clients).
        FString PercStr = TEXT("{grey}n/a (client){white}");
        if (const UMythicActionEventSubsystem *AE = World->GetSubsystem<UMythicActionEventSubsystem>()) {
            PercStr = FString::Printf(TEXT("{yellow}x%.2f{white}"), AE->GetPerceptionMultiplier());
        }
        Detail += FString::Printf(TEXT("{white}Weather types registered: {yellow}%d{white}  perception mult: %s\n"),
                                  Ctrl->GetWeatherTypes().Num(), *PercStr);
        // #5 WEATHER TRANSITION DETAIL — replaces the goal-only line with live transition progress.
        {
            bool bActive = false;
            FGameplayTag TgtTag;
            float Prog = 0.0f;
            double ElapsedMin = 0.0;
            float LenMin = 0.0f;
            Ctrl->GetWeatherTransitionInfo(bActive, TgtTag, Prog, ElapsedMin, LenMin);
            if (bActive) {
                Detail += FString::Printf(TEXT("{white}Transition: {yellow}-> %s{white} {yellow}%.0f%%{white} (%.1f/%.1f min)\n"),
                                          TgtTag.IsValid() ? *TgtTag.ToString() : TEXT("(loading)"), Prog * 100.0f, ElapsedMin, LenMin);
            } else {
                Detail += TEXT("{white}Transition: {grey}(stable - no transition in progress){white}\n");
            }
        }
        // #6 ENVIRONMENT HAZARDS ON THE PLAYER — the per-player hazard component on the OWNING PC.
        if (UMythicEnvironmentHazardComponent *Haz = OwnerPC->FindComponentByClass<UMythicEnvironmentHazardComponent>()) {
            TArray<int32> HazIdx;
            TArray<FString> HazNames;
            Haz->GetActiveHazards(HazIdx, HazNames);
            if (HazNames.Num() > 0) {
                Detail += FString::Printf(TEXT("{white}Active hazards: {red}%s{white}\n"), *FString::Join(HazNames, TEXT(", ")));
            } else {
                Detail += TEXT("{white}Active hazards: {grey}(none){white}\n");
            }
        } else {
            Detail += TEXT("{white}Active hazards: {grey}(no hazard component on this PC){white}\n");
        }
        break;
    }
    case 8: { // Cognition / BDI
        Detail += TEXT("{white}=== DETAIL: COGNITION / BDI ===\n");
        int32 Shown = 0;
        for (TActorIterator<AMythicNPCCharacter> It(World); It && Shown < 12; ++It) {
            AMythicNPCCharacter *NPC = *It;
            UMythicCognitiveBrainComponent *B = NPC ? NPC->CognitiveBrain : nullptr;
            if (!B) {
                continue;
            }
            ++Shown;
            const FMythicIntention &Intent = B->GetCurrentIntention(); // game-thread written (OnAsyncThinkCompleted)
            const FMythicCellCoord Home = B->GetHomeCell();
            const FMythicCellCoord Work = B->GetCachedWorkCell();
            Detail += FString::Printf(TEXT("{white}%s F%d%s role %s phase %s home(%d,%d) work(%d,%d)\n"),
                                      *B->GetDisplayName().ToString(), B->GetFaction().Index,
                                      B->IsSpyBrain() ? *FString::Printf(TEXT(" {red}[SPY trueF%d]{white}"), B->GetTrueFaction().Index) : TEXT(""),
                                      *B->GetRole().GetTagName().ToString(), PhaseToString(B->GetCachedSchedulePhase()),
                                      Home.X, Home.Y, Work.X, Work.Y);
            if (Intent.bValid) {
                const TCHAR *DTint = IsAcuteDesire(Intent.Desire.Type) ? TEXT("{red}") : TEXT("{yellow}");
                Detail += FString::Printf(TEXT("  Intention: %s%s{white} u=%.2f age %.0fs%s\n"),
                                          DTint, DesireTypeName(Intent.Desire.Type), Intent.Desire.Utility,
                                          WorldTime - Intent.CommitTime, Intent.bStarted ? TEXT(" {green}started{white}") : TEXT(""));
            } else {
                Detail += TEXT("  Intention: {grey}(none committed)\n");
            }
            // #5 top beliefs by confidence (LOCKED copy).
            TArray<FMythicBelief> Beliefs = B->GetBeliefsCopy();
            Beliefs.Sort([](const FMythicBelief &A, const FMythicBelief &C) { return A.Confidence > C.Confidence; });
            const int32 NB = FMath::Min(Beliefs.Num(), 4);
            for (int32 b = 0; b < NB; ++b) {
                const FMythicBelief &Bel = Beliefs[b];
                Detail += FString::Printf(TEXT("    bel %s @(%d,%d) conf {yellow}%.2f{white} hops %d\n"),
                                          *Bel.EventTag.ToString(), Bel.Cell.X, Bel.Cell.Y, Bel.Confidence, Bel.PropagationHops);
            }
            // #6 top desires by utility (LOCKED copy).
            TArray<FMythicDesire> Desires = B->GetLastDesiresCopy();
            Desires.Sort([](const FMythicDesire &A, const FMythicDesire &C) { return A.Utility > C.Utility; });
            const int32 ND = FMath::Min(Desires.Num(), 3);
            if (ND > 0) {
                FString DLine = TEXT("    D:");
                for (int32 d = 0; d < ND; ++d) {
                    DLine += FString::Printf(TEXT(" %s %.2f"), DesireTypeName(Desires[d].Type), Desires[d].Utility);
                }
                Detail += DLine + TEXT("\n");
            }

            // ── Step 5 per-NPC SOCIAL/ACTIVITY/APPEARANCE surface (all server-side, safe-read getters) ──
            // (a) Current ambient activity tag (context-driven activity catalog, Step 3 — server-side authoritative).
            const FGameplayTag ActivityTag = NPC->GetCurrentActivityTag();
            // (b) Last player→NPC social verb + reaction (Step 2 — server-side cache; only after a verb was applied).
            EMythicSocialVerb LastVerb = EMythicSocialVerb::Greet;
            EMythicSocialReaction LastReact = EMythicSocialReaction::Neutral;
            double LastReactTime = 0.0;
            const bool bHasReaction = NPC->GetLastSocialReaction(LastVerb, LastReact, LastReactTime);
            {
                FString ActStr = ActivityTag.IsValid()
                                     ? FString::Printf(TEXT("{yellow}%s{white}"), *TagNameLeaf(ActivityTag.GetTagName()))
                                     : TEXT("{grey}(none){white}");
                FString ReactStr = bHasReaction
                                       ? FString::Printf(TEXT("%s%s -> %s%s {grey}%.0fs ago{white}"), SocialVerbName(LastVerb),
                                                         TEXT("{white}"), SocialReactionTint(LastReact), SocialReactionName(LastReact),
                                                         WorldTime - LastReactTime)
                                       : TEXT("{grey}(none yet){white}");
                Detail += FString::Printf(TEXT("    activity %s  lastReact %s\n"), *ActStr, *ReactStr);
            }
            // (c) Appearance descriptor summary (Step 4 — replicated, server reads its own assigned value). Faction color
            //     resolved via the SAME override-aware accessor the resolver/war-map use (snapshot copy-out, null-tolerant).
            {
                const FMythicAppearance &App = NPC->Appearance;
                const FMythicFactionId Fac = B->GetFaction();
                FColor FacCol = MythicFactionColor::DeterministicColorForId(Fac.IsValid() ? Fac.Index : 0);
                if (FDB && Fac.IsValid()) {
                    FMythicFactionData FData;
                    if (FDB->GetFaction(Fac, FData)) {
                        FacCol = MythicFactionColor::GetFactionColor(FData, Fac.Index);
                    }
                }
                // NOTE: the gameplay-debugger canvas color parser only understands NAMED {color} tokens (not hex), so the
                // faction/primary colors are surfaced as RGB hex DIGITS (not a tinted swatch) to stay render-safe.
                Detail += FString::Printf(
                    TEXT("    look outfit {yellow}%d{white} body %d age %d %s skin %d hair %d parts[H%d/T%d/L%d/F%d] facCol #%02X%02X%02X prim #%02X%02X%02X\n"),
                    App.OutfitSetId, App.BodyType, App.AgeBracket, App.bIsFemale ? TEXT("F") : TEXT("M"),
                    App.SkinTone, App.HairTone, App.HeadPart, App.TorsoPart, App.LegsPart, App.FeetPart,
                    FacCol.R, FacCol.G, FacCol.B, App.PrimaryColor.R, App.PrimaryColor.G, App.PrimaryColor.B);
            }
        }
        if (Shown == 0) {
            Detail += TEXT("{grey}(no embodied NPCs with a cognitive brain)\n");
        }
        break;
    }
    case 9: { // World History — witnessed-crime / witness pipeline (server-only)
        Detail += TEXT("{white}=== DETAIL: WORLD HISTORY (crime / witness) ===\n");
        if (UMythicActionEventSubsystem *AE = World->GetSubsystem<UMythicActionEventSubsystem>()) {
            FMythicCrimeReportQueue &Queue = AE->GetCrimeReportQueue(); // read-only use — never mutate/drain
            Detail += FString::Printf(TEXT("{white}Pipeline: pending {yellow}%d{white} witnessResults {yellow}%d{white} crimeReports {yellow}%d{white}/%d perc {yellow}x%.2f{white}\n"),
                                      AE->GetPendingEvents().Num(), AE->GetPendingWitnessResults().Num(),
                                      Queue.PendingReports.Num(), FMythicCrimeReportQueue::MaxQueuedReports, AE->GetPerceptionMultiplier());
            const int32 Start = FMath::Max(0, Queue.PendingReports.Num() - 16);
            for (int32 i = Queue.PendingReports.Num() - 1; i >= Start; --i) { // newest first
                const FMythicCrimeRecord &Cr = Queue.PendingReports[i];
                const double Age = WorldTime - Cr.WorldTime;
                Detail += FString::Printf(TEXT("  %s%s{white} F%d>F%d @(%d,%d) wit %d hops %d conf {yellow}%.2f{white} %s {grey}%.0fs ago{white}\n"),
                                          MoralSeverityTint(Cr.Severity), MoralSeverityName(Cr.Severity),
                                          Cr.PerpFaction.Index, Cr.ViolatedFaction.Index, Cr.Cell.X, Cr.Cell.Y,
                                          Cr.DirectWitnessCount, Cr.PropagationHops, Cr.Confidence,
                                          Cr.bPropagated ? TEXT("{green}propagated{white}") : TEXT("{grey}pending{white}"), Age);
            }
            if (Queue.PendingReports.Num() == 0) {
                Detail += TEXT("{grey}(no crimes in the report queue)\n");
            }
        } else {
            Detail += TEXT("{grey}(crime pipeline server-only / not present in this world)\n");
        }
        break;
    }
    case 10: { // World State / Save
        Detail += TEXT("{white}=== DETAIL: WORLD STATE / SAVE ===\n");
        // World tier (publicly readable on the game state).
        if (const AMythicGameState *MGS = World->GetGameState<AMythicGameState>()) {
            Detail += FString::Printf(TEXT("{white}World Tier {yellow}%d{white}/{grey}%d{white}\n"), MGS->WorldTier, MGS->MaxWorldTier);
            // #7 WORLD TIER MULTIPLIERS — the per-tier reward/difficulty scalars (ATTRIBUTE_ACCESSORS getters).
            if (const UWorldTierAttributes *WTA = MGS->WorldTierAttributes) {
                Detail += FString::Printf(
                    TEXT("  {white}Mult: gold {yellow}%.2f{white} xp {yellow}%.2f{white} legend {yellow}%.2f{white} mythic {yellow}%.2f{white} | enemyHP {yellow}%.2f{white} enemyDmg {yellow}%.2f{white}\n"),
                    WTA->GetGoldDropRateMultiplier(), WTA->GetExperienceGainMultiplier(),
                    WTA->GetLegendaryDropRateMultiplier(), WTA->GetMythicDropRateMultiplier(),
                    WTA->GetEnemyHealthMultiplier(), WTA->GetEnemyDamageMultiplier());
            }
            // #8 RESOURCES / HARVEST — partially-mined (tracked) + depleted nodes + soonest respawn.
            const TArray<FTrackedDestructibleData> Tracked = MGS->GetTrackedDestructibles();
            int32 Depleted = 0;
            double NextRespawn = TNumericLimits<double>::Max();
            if (const UMythicResourceManagerComponent *RM = MGS->GetComponentByClass<UMythicResourceManagerComponent>()) {
                const TArray<FTrackedDestructibleData> &Gone = RM->GetDestroyedItems();
                Depleted = Gone.Num();
                for (const FTrackedDestructibleData &D : Gone) {
                    if (D.RespawnTime > 0.0) {
                        NextRespawn = FMath::Min(NextRespawn, D.RespawnTime);
                    }
                }
            }
            if (NextRespawn == TNumericLimits<double>::Max()) {
                Detail += FString::Printf(TEXT("{white}Resources: tracked {yellow}%d{white} depleted {yellow}%d{white} next respawn {grey}n/a{white}\n"),
                                          Tracked.Num(), Depleted);
            } else {
                const double InS = NextRespawn - WorldTime;
                Detail += FString::Printf(TEXT("{white}Resources: tracked {yellow}%d{white} depleted {yellow}%d{white} next respawn in {yellow}%.0fs{white}\n"),
                                          Tracked.Num(), Depleted, FMath::Max(0.0, InS));
            }
        } else {
            Detail += TEXT("{grey}(no AMythicGameState - wrong game mode?)\n");
        }
        // #12 TOGGLEABLES tally (iterate-and-count: doors / gates / levers, with on-count).
        {
            int32 ToggN = 0;
            int32 ToggOn = 0;
            for (TActorIterator<AMythicToggleable> It(World); It; ++It) {
                if (AMythicToggleable *T = *It) {
                    ++ToggN;
                    if (T->IsOn()) {
                        ++ToggOn;
                    }
                }
            }
            Detail += FString::Printf(TEXT("{white}Toggleables: {yellow}%d{white} (on {green}%d{white})\n"), ToggN, ToggOn);
        }
        // #14 STORAGE CONTAINERS tally (count only).
        {
            int32 ContN = 0;
            for (TActorIterator<AMythicStorageContainer> It(World); It; ++It) {
                if (*It) {
                    ++ContN;
                }
            }
            Detail += FString::Printf(TEXT("{white}Containers: {yellow}%d{white}\n"), ContN);
        }
        // #13 CONVERSION STATIONS — nearest few with job/fuel state.
        {
            int32 StationN = 0;
            int32 ShownStation = 0;
            for (TActorIterator<AMythicConversionStation> It(World); It; ++It) {
                AMythicConversionStation *St = *It;
                if (!St) {
                    continue;
                }
                ++StationN;
                if (ShownStation >= 4) {
                    continue;
                }
                if (const UConversionStationComponent *Comp = St->GetConversionComponent()) {
                    const FConversionFuelState &Fuel = Comp->GetFuelState();
                    const FString Tags = Comp->GetStationTags().ToStringSimple();
                    if (ShownStation == 0) {
                        Detail += TEXT("{white}--- CONVERSION STATIONS ---\n");
                    }
                    ++ShownStation;
                    Detail += FString::Printf(TEXT("  {white}%s jobs {yellow}%d{white} fuel %s{white} buf {yellow}%.1fs{white} {grey}[%s]{white}\n"),
                                              Tags.IsEmpty() ? TEXT("(station)") : *Tags, Comp->GetJobs().Num(),
                                              Fuel.bBurning ? TEXT("{green}burning") : TEXT("{grey}idle"),
                                              Fuel.BufferedBurnSeconds, *St->GetName());
                }
            }
            if (StationN > 0) {
                Detail += FString::Printf(TEXT("{white}Conversion stations: {yellow}%d{white}\n"), StationN);
            }
        }
        // #18 background sim thread diagnostics (SimulationLock-guarded copy-out).
        uint64 SimTick = 0;
        float SimInterval = 0.0f;
        bool bSimRunning = false;
        if (LW->CopySimDiagnostics(SimTick, SimInterval, bSimRunning)) {
            Detail += FString::Printf(TEXT("{white}Sim thread: running %s tick {yellow}%llu{white} interval {yellow}%.2fs{white}\n"),
                                      bSimRunning ? TEXT("{green}Y{white}") : TEXT("{red}N{white}"),
                                      static_cast<unsigned long long>(SimTick), SimInterval);
        } else {
            Detail += TEXT("{grey}(no background sim thread)\n");
        }
        // #16 save ops (game-instance subsystem; present on server + clients).
        if (const UMythicSaveGameSubsystem *SG = OwnerPC->GetGameInstance() ? OwnerPC->GetGameInstance()->GetSubsystem<UMythicSaveGameSubsystem>() : nullptr) {
            Detail += FString::Printf(TEXT("{white}Save: in-flight writes {yellow}%d{white}  pending loads {yellow}%d{white}\n"),
                                      SG->GetInFlightSaveCount(), SG->GetPendingLoadCount());
        }
        // #17 autosave countdown + pending respawns (server-only).
        if (const AMythicGameMode *GM = World->GetAuthGameMode<AMythicGameMode>()) {
            const float Remain = GM->GetAutosaveTimeRemaining();
            if (Remain >= 0.0f) {
                Detail += FString::Printf(TEXT("{white}Autosave in {yellow}%.0fs{white}  pending respawns {yellow}%d{white}\n"), Remain, GM->GetPendingRespawnCount());
            } else {
                Detail += FString::Printf(TEXT("{white}Autosave {grey}(timer inactive){white}  pending respawns {yellow}%d{white}\n"), GM->GetPendingRespawnCount());
            }
        } else {
            Detail += TEXT("{grey}(autosave/respawn - client, N/A)\n");
        }
        // #19 replicated proxies (the sole faction/encounter source on clients; second-order copies on the server).
        Detail += FString::Printf(TEXT("{white}Replicated proxies: factions {yellow}%d{white} encounters {yellow}%d{white}\n"),
                                  LW->GetAllFactionProxies().Num(), LW->GetAllEncounterProxies().Num());
        // #35 creatures sub-section — nearest few with species/pack/aggression/den.
        Detail += TEXT("{white}--- CREATURES (nearest) ---\n");
        {
            int32 ShownCr = 0;
            FMassEntityQuery Q(EMPtr);
            Q.AddRequirement<FMythicIdentityFragment>(EMassFragmentAccess::ReadOnly);
            Q.AddRequirement<FMythicCreatureFragment>(EMassFragmentAccess::ReadOnly);
            Q.AddTagRequirement<FMythicCreatureTag>(EMassFragmentPresence::All);
            FMassExecutionContext Ctx(EM);
            Q.ForEachEntityChunk(Ctx, [&](FMassExecutionContext &C) {
                const int32 NN = C.GetNumEntities();
                const TConstArrayView<FMythicIdentityFragment> IdView = C.GetFragmentView<FMythicIdentityFragment>();
                const TConstArrayView<FMythicCreatureFragment> CrView = C.GetFragmentView<FMythicCreatureFragment>();
                for (int32 i = 0; i < NN && ShownCr < 10; ++i) {
                    const int32 Dist = FMath::Abs(IdView[i].Cell.X - PlayerCell.X) + FMath::Abs(IdView[i].Cell.Y - PlayerCell.Y);
                    if (Dist > 12) {
                        continue;
                    }
                    ++ShownCr;
                    Detail += FString::Printf(TEXT("  {green}%s{white} pk%d aggr {yellow}%.2f{white}/{grey}%.2f{white} den(%d,%d) r%d @(%d,%d)\n"),
                                              *SpeciesIdToName(CrView[i].SpeciesId), CrView[i].PackId, CrView[i].CurrentAggression, CrView[i].BaseAggression,
                                              CrView[i].DenCell.X, CrView[i].DenCell.Y, CrView[i].TerritorialRadius, IdView[i].Cell.X, IdView[i].Cell.Y);
                }
            });
            if (ShownCr == 0) {
                Detail += TEXT("{grey}(no creatures near the player)\n");
            }
        }

        // #FINAL DESIGNER SPAWNERS (server-authoritative placed actors + the authoritative registry).
        //   Per live spawner: DesignerId, live/cooldown state, spawns-remaining (Max - SpawnsEver), perma-dead, conditions-met.
        //   The registry section below also lists DesignerIds with persisted state whose actor isn't currently loaded
        //   (e.g. streamed-out level) so the save state is visible even when the actor is absent.
        {
            Detail += TEXT("{white}--- DESIGNER SPAWNERS (server) ---\n");
            // Track which DesignerIds have a loaded actor so the registry pass can skip them (avoid double-listing).
            TSet<FName> ShownIds;
            int32 SpawnerN = 0;
            int32 ShownSpawner = 0;
            for (TActorIterator<AMythicDesignerSpawner> It(World); It; ++It) {
                AMythicDesignerSpawner *Sp = *It;
                if (!Sp) {
                    continue;
                }
                ++SpawnerN;
                ShownIds.Add(Sp->DesignerId);
                if (ShownSpawner >= 16) { // bound the listing; the count line below still reports the total
                    continue;
                }
                ++ShownSpawner;

                const int32 Live = Sp->GetLiveCount();
                const int32 Ever = Sp->GetSpawnsEver();
                const int32 Remaining = FMath::Max(0, Sp->MaxSpawnsEver - Ever);
                const bool bPerma = Sp->IsPermaDead();
                // AreConditionsMet re-gathers inputs (server-only meaningful). bMet is only suggestive on a client mirror.
                const bool bMet = Sp->AreConditionsMet();
                // Cooldown remaining is derived from the registry's LastDeathTime (authoritative) + the actor's RespawnCooldownSeconds.
                double CooldownLeft = 0.0;
                if (const UMythicDesignerSpawnerRegistry *Reg = LW->GetDesignerSpawnerRegistry()) {
                    if (const FMythicDesignerSpawnerState *St = Reg->Find(Sp->DesignerId)) {
                        CooldownLeft = FMath::Max(0.0, (St->LastDeathTime + Sp->RespawnCooldownSeconds) - WorldTime);
                    }
                }
                const bool bAtCap = Live >= Sp->MaxConcurrent;
                // State word: terminal perma-death > exhausted (no remaining) > on cooldown > at-concurrency-cap > live > idle.
                const TCHAR *StateWord =
                    bPerma                         ? TEXT("{red}PERMADEAD{white}")
                    : (Remaining == 0)             ? TEXT("{grey}EXHAUSTED{white}")
                    : (CooldownLeft > 0.0)         ? TEXT("{yellow}COOLDOWN{white}")
                    : bAtCap                       ? TEXT("{yellow}AT-CAP{white}")
                    : (Live > 0)                   ? TEXT("{green}LIVE{white}")
                    : (bMet ? TEXT("{green}READY{white}") : TEXT("{grey}WAITING{white}"));

                Detail += FString::Printf(
                    TEXT("  {white}%s %s live {yellow}%d{white}/{grey}%d{white} rem {yellow}%d{white}/{grey}%d{white} cd {yellow}%.0fs{white} cond %s\n"),
                    Sp->DesignerId.IsNone() ? TEXT("{red}(NONE-id){white}") : *Sp->DesignerId.ToString(),
                    StateWord, Live, Sp->MaxConcurrent, Remaining, Sp->MaxSpawnsEver, CooldownLeft,
                    bMet ? TEXT("{green}met{white}") : TEXT("{grey}no{white}"));
            }
            if (SpawnerN > ShownSpawner) {
                Detail += FString::Printf(TEXT("  {grey}... %d more loaded spawner(s){white}\n"), SpawnerN - ShownSpawner);
            }

            // Registry-only entries (persisted DesignerId whose actor isn't currently loaded) — proves save state survives.
            if (const UMythicDesignerSpawnerRegistry *Reg = LW->GetDesignerSpawnerRegistry()) {
                TArray<FName> RegIds;
                Reg->GetAllDesignerIds(RegIds);
                int32 ShownReg = 0;
                for (const FName &Id : RegIds) {
                    if (ShownIds.Contains(Id)) {
                        continue; // already shown via its loaded actor
                    }
                    if (ShownReg >= 8) {
                        Detail += TEXT("  {grey}... (more persisted, no actor){white}\n");
                        break;
                    }
                    ++ShownReg;
                    const FMythicDesignerSpawnerState *St = Reg->Find(Id);
                    Detail += FString::Printf(TEXT("  {grey}[persisted] %s spawnsEver {yellow}%d{white} perma %s{white}\n"),
                                              *Id.ToString(), St ? St->SpawnsEver : 0,
                                              (St && St->bPermaDead) ? TEXT("{red}Y") : TEXT("{grey}N"));
                }
                Detail += FString::Printf(TEXT("  {white}Spawners loaded {yellow}%d{white}  registry entries {yellow}%d{white}\n"),
                                          SpawnerN, RegIds.Num());
            } else if (SpawnerN == 0) {
                Detail += TEXT("  {grey}(no designer spawners placed){white}\n");
            }
        }

        // #FINAL WAR-MAP (CLIENT view) — the war-map texture/legend live on the LOCAL player subsystem, not the server.
        //   Grid dims, accumulated claimed-cell count (delta accumulator), last-refresh marker/legend counts, and the
        //   per-faction cell breakdown from the legend ("cells per faction"). Server PIE instances have no local-player
        //   war-map subsystem — this prints (client-only) there.
        {
            Detail += TEXT("{white}--- WAR-MAP (client) ---\n");
            const ULocalPlayer *LP = OwnerPC->GetLocalPlayer();
            const UMythicWarMapSubsystem *WarMap = LP ? LP->GetSubsystem<UMythicWarMapSubsystem>() : nullptr;
            if (WarMap) {
                Detail += FString::Printf(
                    TEXT("  {white}Grid {yellow}%dx%d{white}  claimed cells {yellow}%d{white}  markers settl {yellow}%d{white} enc {yellow}%d{white}  legend {yellow}%d{white}\n"),
                    WarMap->GetGridWidth(), WarMap->GetGridHeight(), WarMap->GetAccumulatedClaimedCellCount(),
                    WarMap->GetLastSettlementMarkerCount(), WarMap->GetLastEncounterMarkerCount(),
                    WarMap->GetLastLegendEntryCount());
                // Cells per faction (from the legend; bounded by the active faction count).
                TArray<FMythicWarMapLegendEntry> Legend;
                WarMap->GetLegendEntries(Legend);
                if (Legend.Num() > 0) {
                    FString CellsStr;
                    int32 ShownLeg = 0;
                    for (const FMythicWarMapLegendEntry &E : Legend) {
                        if (ShownLeg >= 12) {
                            CellsStr += TEXT(" ...");
                            break;
                        }
                        ++ShownLeg;
                        CellsStr += FString::Printf(TEXT(" {yellow}%s{white}:%d"),
                                                    E.DisplayName.IsEmpty() ? *FString::Printf(TEXT("F%d"), E.FactionId.Index)
                                                                            : *E.DisplayName.ToString(),
                                                    E.ControlledCellCount);
                    }
                    Detail += FString::Printf(TEXT("  {white}Cells/faction:%s\n"), *CellsStr);
                } else {
                    Detail += TEXT("  {grey}(no faction controls a cell yet - deltas not arrived){white}\n");
                }
            } else {
                Detail += TEXT("  {grey}(no local-player war-map subsystem - dedicated-server instance?){white}\n");
            }
        }
        break;
    }
    case 11: { // Chronicle / News — the world-news feed (GameInstance subsystem; server + relayed clients)
        Detail += TEXT("{white}=== DETAIL: CHRONICLE / NEWS ===\n");
        if (const UMythicWorldChronicleSubsystem *Chron = OwnerPC->GetGameInstance() ? OwnerPC->GetGameInstance()->GetSubsystem<UMythicWorldChronicleSubsystem>() : nullptr) {
            const TArray<FMythicChronicleEntry> Recent = Chron->GetRecentChronicle(8); // by-value copy, oldest-first
            Detail += FString::Printf(TEXT("{white}Chronicle: {yellow}%d{white} recent entries\n"), Recent.Num());
            for (int32 i = Recent.Num() - 1; i >= 0; --i) { // newest first
                const FMythicChronicleEntry &E = Recent[i];
                const FString Readable = UMythicWorldChronicleSubsystem::EventTagToReadable(E.EventTag);
                const TCHAR *Tint = (E.Significance >= 0.85f) ? TEXT("{red}") : (E.Significance >= 0.65f) ? TEXT("{yellow}") : TEXT("{grey}");
                Detail += FString::Printf(TEXT("  {grey}t%.0f{white} %ssig%.2f{white} {yellow}%s{white}: %s\n"),
                                          E.WorldTime, Tint, E.Significance, *Readable, *E.Text.ToString());
            }
            if (Recent.Num() == 0) {
                Detail += TEXT("{grey}(no chronicle entries yet - nothing significant has happened)\n");
            }
        } else {
            Detail += TEXT("{grey}(world chronicle subsystem not available)\n");
        }
        break;
    }
    case 12: { // Population — sim-driven WHO spawns + group/garrison/spawn-point/kill-feedback coverage
        Detail += TEXT("{white}=== DETAIL: POPULATION (who spawns) ===\n");

        // ── (1) ARCHETYPE / ROLE MIX — full RoleCounts breakdown (the header line is capped at 10; this is the complete
        //    sorted-by-count list of the role tags the population/territory/group spawners stamped onto Identity.RoleTag). ──
        {
            Detail += FString::Printf(TEXT("{white}--- ROLE MIX ({yellow}%d{white} distinct over {yellow}%d{white} NPCs) ---\n"),
                                      RoleCounts.Num(), TotalNPC);
            if (RoleCounts.Num() > 0) {
                RoleCounts.ValueSort([](const int32 &A, const int32 &B) { return A > B; });
                FString RoleLine;
                int32 ShownRole = 0;
                for (const TPair<FName, int32> &Pair : RoleCounts) {
                    if (ShownRole >= 24) {
                        RoleLine += TEXT(" ...");
                        break;
                    }
                    ++ShownRole;
                    RoleLine += FString::Printf(TEXT(" {yellow}%s{white}:%d"), *TagNameLeaf(Pair.Key), Pair.Value);
                }
                Detail += RoleLine + TEXT("\n");
            } else {
                Detail += TEXT("{grey}(no RoleTag stamped on any NPC)\n");
            }
        }

        // ── (2) CONTESTED-BORDER GARRISON TARGETS — re-derive, read-only, the per-cell soldier target the patrol spawner
        //    computes (ownership×influence×biome, then the at-war contested-border boost) over a small window around the
        //    player so a war's frontline thickening is visible. Mirrors TerritoryPatrolSpawnerProcessor's probe EXACTLY:
        //    lock-free GetCell value-copies + GetRelationship (SnapshotLock) + the SAME static helpers it spawns from. We
        //    do NOT spawn — pure surfacing. Bounded to a 7x7 window, contested cells reported first. ──
        if (bHasPlayerCell && FDB) {
            static const FMythicCellCoord NeighborOffsets[4] = {
                FMythicCellCoord(1, 0), FMythicCellCoord(-1, 0), FMythicCellCoord(0, 1), FMythicCellCoord(0, -1)};
            const int32 Half = 3; // 7x7 window
            int32 ContestedCells = 0;
            int32 ShownGarrison = 0;
            FString GarrisonLines;
            for (int32 dy = -Half; dy <= Half; ++dy) {
                for (int32 dx = -Half; dx <= Half; ++dx) {
                    const FMythicCellCoord Coord(PlayerCell.X + dx, PlayerCell.Y + dy);
                    const FMythicTerritoryCell TC = Grid->GetCell(Coord); // lock-free value-copy
                    if (!TC.DominantFaction.IsValid() || TC.bPlayerOwned) {
                        continue; // wilderness / player property: no faction garrison here
                    }
                    // Skip settlement cells (owned by the ambient/group spawners, not the patrol garrison path).
                    FMythicSettlementData SettlementScratch;
                    if (LW->CopySettlementAtCell(Coord, SettlementScratch)) {
                        continue;
                    }
                    FMythicFactionData FacData;
                    if (!FDB->GetFaction(TC.DominantFaction, FacData) || !FacData.bAlive || FacData.Status != EMythicFactionStatus::Active) {
                        continue;
                    }
                    const EMythicBiome Biome = Grid->GetBiomeAtCell(Coord);
                    const float BiomeMod = UMythicTerritoryPatrolSpawnerProcessor::BiomeGarrisonModifier(Biome);
                    const int32 MaxSoldiers = Settings ? Settings->MaxSoldiersPerControlledCell : 4;
                    const int32 MaxPerCell = Settings ? Settings->MaxEntitiesPerCell : 20;
                    const int32 BaseTarget = UMythicTerritoryPatrolSpawnerProcessor::ComputeTerritoryPatrolDensity(
                        FacData.MilitaryStrength, TC.Influence, MaxSoldiers, MaxPerCell);
                    int32 SoldierTarget = FMath::CeilToInt(static_cast<float>(BaseTarget) * BiomeMod);
                    SoldierTarget = FMath::Clamp(SoldierTarget, 0, FMath::Min(MaxSoldiers, MaxPerCell));
                    // 4-neighbor at-war probe (identical adjacency + Hostile test to the spawner).
                    bool bContested = false;
                    for (const FMythicCellCoord &Offset : NeighborOffsets) {
                        const FMythicCellCoord NCell(Coord.X + Offset.X, Coord.Y + Offset.Y);
                        const FMythicTerritoryCell NTC = Grid->GetCell(NCell);
                        if (!NTC.DominantFaction.IsValid() || NTC.DominantFaction == TC.DominantFaction) {
                            continue;
                        }
                        if (FDB->GetRelationship(TC.DominantFaction, NTC.DominantFaction) == EMythicFactionRelation::Hostile) {
                            bContested = true;
                            break;
                        }
                    }
                    const int32 BoostedTarget = UMythicTerritoryPatrolSpawnerProcessor::ApplyContestedBorderBoost(
                        SoldierTarget, bContested, Settings ? Settings->ContestedBorderSoldierMultiplier : 1.0f,
                        MaxSoldiers, MaxPerCell);
                    if (bContested) {
                        ++ContestedCells;
                    }
                    // Report contested cells (the interesting frontline ones) up to a bound; skip quiet interior cells with
                    // no garrison to keep the pane tight.
                    if (bContested && ShownGarrison < 12) {
                        ++ShownGarrison;
                        GarrisonLines += FString::Printf(TEXT("  {red}FRONT{white} (%d,%d) F%d {grey}%s{white} base {yellow}%d{white} -> boosted {red}%d{white}\n"),
                                                         Coord.X, Coord.Y, TC.DominantFaction.Index, BiomeToString(Biome), SoldierTarget, BoostedTarget);
                    }
                }
            }
            Detail += FString::Printf(TEXT("{white}--- GARRISON (7x7, contested {red}%d{white}, mult {yellow}x%.1f{white}) ---\n"),
                                      ContestedCells, Settings ? Settings->ContestedBorderSoldierMultiplier : 1.0f);
            if (ShownGarrison > 0) {
                Detail += GarrisonLines;
            } else {
                Detail += TEXT("{grey}(no contested-border cells near the player)\n");
            }
        }

        // ── (3) SETTLEMENT SPAWN POINTS by purpose + hostile-camp flag (one-time BeginPlay-generated navmesh anchors). ──
        {
            Detail += TEXT("{white}--- SETTLEMENT SPAWN POINTS (by purpose) ---\n");
            TArray<int32> Ids;
            LW->CopyAllSettlementIds(Ids);
            int32 Shown = 0;
            int32 TotalPoints = 0;
            for (const int32 Id : Ids) {
                FMythicSettlementData Data;
                if (!LW->CopySettlementById(Id, Data)) {
                    continue; // SimulationLock copy-out
                }
                // Tally this settlement's points by purpose (COUNT excluded; index-safe).
                int32 ByPurpose[static_cast<int32>(EMythicSpawnPointPurpose::COUNT)] = {};
                for (const FMythicSpawnPoint &SP : Data.SpawnPoints) {
                    const int32 Pi = FMath::Clamp(static_cast<int32>(SP.Purpose), 0, static_cast<int32>(EMythicSpawnPointPurpose::COUNT) - 1);
                    ++ByPurpose[Pi];
                }
                TotalPoints += Data.SpawnPoints.Num();
                if (Shown >= 24) {
                    continue; // keep counting TotalPoints but stop printing
                }
                ++Shown;
                Detail += FString::Printf(TEXT("  {white}[%d] %s %spts {yellow}%d{white} {grey}(Civ %d Guard %d Enemy %d Any %d){white}\n"),
                                          Id, *Data.DisplayName.ToString(),
                                          Data.bIsHostileCamp ? TEXT("{red}[HOSTILE] {white}") : TEXT(""),
                                          Data.SpawnPoints.Num(),
                                          ByPurpose[static_cast<int32>(EMythicSpawnPointPurpose::Civilian)],
                                          ByPurpose[static_cast<int32>(EMythicSpawnPointPurpose::Guard)],
                                          ByPurpose[static_cast<int32>(EMythicSpawnPointPurpose::Enemy)],
                                          ByPurpose[static_cast<int32>(EMythicSpawnPointPurpose::Any)]);
            }
            if (Ids.Num() == 0) {
                Detail += TEXT("{grey}(no settlements)\n");
            } else if (TotalPoints == 0) {
                Detail += TEXT("{grey}(no spawn points generated - navmesh not ready / cell-center fallback in use)\n");
            }
        }

        // ── (4) ACTIVE GROUPS — distinct GroupIds vs the MaxActiveGroups cap + per-activity-tag member breakdown
        //    (tallied lock-free in the group MASS pass above). ──
        {
            const TCHAR *CapTint = (MaxActiveGroups > 0 && ActiveGroupCount >= MaxActiveGroups) ? TEXT("{red}") : TEXT("{yellow}");
            Detail += FString::Printf(TEXT("{white}--- GROUPS (active %s%d{white}/{grey}%d{white}, members {yellow}%d{white}, leaders {yellow}%d{white}) ---\n"),
                                      CapTint, ActiveGroupCount, MaxActiveGroups, GroupMemberCount, GroupLeaderCount);
            if (GroupMembersByActivity.Num() > 0) {
                for (const TPair<FName, int32> &Pair : GroupMembersByActivity) {
                    Detail += FString::Printf(TEXT("  {yellow}%s{white} members {yellow}%d{white}\n"), *TagNameLeaf(Pair.Key), Pair.Value);
                }
            } else {
                Detail += TEXT("{grey}(no active groups)\n");
            }
        }

        // ── (5) KILL-FEEDBACK READOUT — per-faction Population / Reserves.Arms / MilitaryStrength. A player kill durably
        //    lowers Population (delta-applied baseline → fewer future spawns) and, for armed roles, Reserves.Arms (the
        //    durable source MilitaryStrength is recomputed from each tick). These three columns are exactly what
        //    UMythicLivingWorldSubsystem::ReportNpcDeath moves, so this surfaces the world-sim feedback loop directly. ──
        {
            Detail += TEXT("{white}--- KILL FEEDBACK (faction Pop / Arms / Mil) ---\n");
            if (FDB) {
                FDB->ForEachAliveFaction([&Detail](FMythicFactionId Id, const FMythicFactionData &D) {
                    const TCHAR *ArmsTint = (D.Reserves.Arms <= 0.0f) ? TEXT("{red}") : TEXT("{green}");
                    const TCHAR *MilTint = (D.MilitaryStrength < 0.25f) ? TEXT("{red}") : (D.MilitaryStrength < 0.6f ? TEXT("{yellow}") : TEXT("{green}"));
                    Detail += FString::Printf(TEXT("  {white}[%d] {green}%s{white}: Pop {yellow}%d{white}  Arms %s%.1f{white}  Mil %s%.2f{white}\n"),
                                              Id.Index, *D.DisplayName.ToString(), D.Population,
                                              ArmsTint, D.Reserves.Arms, MilTint, D.MilitaryStrength);
                });
            } else {
                Detail += TEXT("{red}Faction DB not available.\n");
            }
        }
        break;
    }
    default:
        break;
    }

    // ═══════════════════════ FOOTER LEGEND ═══════════════════════
    static const TCHAR *DetailNames[] = {TEXT("Factions"), TEXT("Diplomacy"), TEXT("Schemes"), TEXT("Social"),
                                         TEXT("Settlements"), TEXT("Party"), TEXT("Events"), TEXT("Environment"),
                                         TEXT("Cognition"), TEXT("History"), TEXT("WorldState"), TEXT("Chronicle"),
                                         TEXT("Population")};
    const int32 DetIdx = FMath::Clamp(static_cast<int32>(ActiveDetail), 0, NumDetailPanes - 1);
    FString Footer = FString::Printf(
        TEXT("{white}[J]ent:%s [K]terr:%s [L]settl:%s [B]events:%s [C]crime:%s   [N] detail: %s (%d/%d)   shapes %d/%d\n"),
        bShowEntities ? TEXT("{green}ON{white}") : TEXT("{grey}off{white}"),
        bShowTerritory ? TEXT("{green}ON{white}") : TEXT("{grey}off{white}"),
        bShowSettlements ? TEXT("{green}ON{white}") : TEXT("{grey}off{white}"),
        bShowEvents ? TEXT("{green}ON{white}") : TEXT("{grey}off{white}"),
        bShowCrime ? TEXT("{green}ON{white}") : TEXT("{grey}off{white}"),
        DetailNames[DetIdx], DetIdx + 1, static_cast<int32>(NumDetailPanes), ShapesDrawn, GMaxShapes);

    DataPack.Summary = Header + Detail + Footer;
    DataPack.ActiveDetail = ActiveDetail;
    DataPack.LayerFlags = static_cast<uint8>((bShowEntities ? 1 : 0) | (bShowTerritory ? 2 : 0) | (bShowSettlements ? 4 : 0) | (bShowEvents ? 8 : 0) | (bShowCrime ? 16 : 0));
    DataPack.ShapesDrawn = ShapesDrawn;
}

void FGameplayDebuggerCategory_MythicLivingWorld::DrawData(APlayerController *OwnerPC, FGameplayDebuggerCanvasContext &CanvasContext) {
    // The verbatim server-built text overview (header + active detail pane + toggle footer).
    CanvasContext.Printf(TEXT("%s"), *DataPack.Summary);
}

#endif // WITH_GAMEPLAY_DEBUGGER
