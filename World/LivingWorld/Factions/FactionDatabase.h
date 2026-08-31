
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GenericTeamAgentInterface.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/Entity/MythicEntityId.h"
#include <atomic>

#include "Engine/DataAsset.h"
#include "FactionDatabase.generated.h"


UENUM(BlueprintType)
enum class EMythicFactionRelation : uint8 {
    Allied UMETA(DisplayName = "Allied"),
    Friendly UMETA(DisplayName = "Friendly"),
    Neutral UMETA(DisplayName = "Neutral"),
    Unfriendly UMETA(DisplayName = "Unfriendly"),
    Hostile UMETA(DisplayName = "Hostile")
};


UENUM(BlueprintType)
enum class EMythicFactionStatus : uint8 {
    Active UMETA(DisplayName = "Active"),

    Annihilated UMETA(DisplayName = "Annihilated"),

    Resistance UMETA(DisplayName = "Resistance"),

    Dormant UMETA(DisplayName = "Dormant")
};


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicIdeologyProfile {
    GENERATED_BODY()

    /**
     * +1.0 = glorifies combat (won't punish murder, NPCs cheer killing).
     * -1.0 = total pacifism (any kill near territory triggers hostility, guards attack killers).
     *  0.0 = pragmatic (self-defense OK, unprovoked murder still condemned).
     * Sim effect: when a kill event occurs near this faction's territory, the severity
     * is (event.Violence × this.Violence). Negative result = faction disapproves.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Violence = 0.0f;

    /**
     * +1.0 = theft is honorable (faction members steal openly, won't report theft).
     * -1.0 = theft is unforgivable (pickpocket → criminal charge, looting → hostile response).
     *  0.0 = indifferent.
     * Sim effect: factions with Theft > RaidIdeologyThreshold (default 0.3) earn raid income
     * by stealing from nearby trade routes. Higher Theft → more profitable raiding.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Theft = 0.0f;

    /**
     * +1.0 = cunning and manipulation are virtues (fraud goes unpunished).
     * -1.0 = absolute intolerance for lies (forged documents → death sentence).
     *  0.0 = neutral.
     * Sim effect: primarily affects NPC reaction severity to deception-tagged events.
     * Two factions with opposing Deception values will have high ideology distance → diplomatic friction.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Deception = 0.0f;

    /**
     * +1.0 = always spare the fallen (executing prisoners → max severity reaction).
     * -1.0 = no quarter given (showing mercy is weakness, faction kills prisoners on sight).
     *  0.0 = practical (won't punish either choice).
     * Sim effect: affects reaction to death/combat events. A Mercy=-1.0 faction won't penalize
     * you for killing surrendered enemies; a Mercy=+1.0 faction treats it as a grave offense.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Mercy = 0.0f;

    /**
     * +1.0 = oaths are absolute (betrayal = death, desertion = hunt-on-sight).
     * -1.0 = every person for themselves (betrayal is just business).
     *  0.0 = moderate expectations.
     * Sim effect: high Loyalty increases faction cohesion — SchismIdeologyThreshold effectively
     * needs more divergence to trigger a split. Low Loyalty → factions fracture more easily.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Loyalty = 0.0f;

    /**
     * +1.0 = zealously protects sacred sites (defiling a temple → instant war).
     * -1.0 = nothing is sacred (grave robbing, blood rituals all acceptable).
     *  0.0 = secular, no strong opinion.
     * Sim effect: affects reaction severity to sanctity-tagged events (shrine desecration,
     * cannibalism, necromancy). High Sanctity factions will condemn necromancer factions.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Sanctity = 0.0f;

    /**
     * +1.0 = absolute rule, strict hierarchy (rebellion → instant hostile response).
     * -1.0 = anarchist (no laws, anti-government, won't enforce order).
     *  0.0 = moderate governance.
     * Sim effect: factions with Authority > TaxAuthorityThreshold (default -0.5) earn tax
     * income from their population. Also affects reaction to authority-tagged events
     * (assassinating leaders, freeing prisoners, inciting rebellion).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Authority = 0.0f;

    /**
     * +1.0 = embraces all magic (mages welcomed, enchanting encouraged).
     * -1.0 = magic is abomination (mages hunted, enchanted items destroyed on sight).
     *  0.0 = neutral on magic.
     * Sim effect: affects reaction to arcane-tagged events (spellcasting, enchanting, potions).
     * Two factions at opposite Arcane extremes will have large ideology distance → diplomatic hostility.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float Arcane = 0.0f;

    float GetAxis(EMythicMoralAxis Axis) const {
        switch (Axis) {
        case EMythicMoralAxis::Violence:
            return Violence;
        case EMythicMoralAxis::Theft:
            return Theft;
        case EMythicMoralAxis::Deception:
            return Deception;
        case EMythicMoralAxis::Mercy:
            return Mercy;
        case EMythicMoralAxis::Loyalty:
            return Loyalty;
        case EMythicMoralAxis::Sanctity:
            return Sanctity;
        case EMythicMoralAxis::Authority:
            return Authority;
        case EMythicMoralAxis::Arcane:
            return Arcane;
        default:
            return 0.0f;
        }
    }

    float &GetAxisMutable(EMythicMoralAxis Axis) {
        switch (Axis) {
        case EMythicMoralAxis::Violence:
            return Violence;
        case EMythicMoralAxis::Theft:
            return Theft;
        case EMythicMoralAxis::Deception:
            return Deception;
        case EMythicMoralAxis::Mercy:
            return Mercy;
        case EMythicMoralAxis::Loyalty:
            return Loyalty;
        case EMythicMoralAxis::Sanctity:
            return Sanctity;
        case EMythicMoralAxis::Authority:
            return Authority;
        case EMythicMoralAxis::Arcane:
            return Arcane;
        default:
            static float Dummy = 0.0f;
            return Dummy;
        }
    }
};


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicFactionData {
    GENERATED_BODY()

    /** Faction display name */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText DisplayName;

    /** Gameplay tag identifying this faction (e.g. "Faction.Imperials") */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayTag FactionTag;


    /**
     * If set, FactionColor below is used as this faction's display color (war-map, minimap, NPC appearance tint) instead
     * of the deterministic-from-id color. Lets designers pin iconic factions to brand colors while every other faction
     * still gets a stable, hue-separated automatic color. Resolved via MythicFactionColor::GetFactionColor.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display")
    bool bOverrideFactionColor = false;

    /** Authored display color, used only when bOverrideFactionColor is true. Otherwise the deterministic color applies. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Display", meta = (EditCondition = "bOverrideFactionColor"))
    FColor FactionColor = FColor::Transparent;

    /** Is this faction still alive? False = annihilated */
    UPROPERTY(BlueprintReadOnly)
    bool bAlive = true;

    /** Current lifecycle status (Active, Annihilated, Resistance, Dormant) */
    UPROPERTY(BlueprintReadOnly)
    EMythicFactionStatus Status = EMythicFactionStatus::Active;

    /**
     * This faction's moral stance on each axis.
     * Compared against player/NPC moral signatures to determine reaction severity.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ideology")
    FMythicIdeologyProfile Ideology;

    /**
     * How severe must an action be before the faction disapproves?
     * Actions scoring above this threshold trigger mild reputation loss, suspicious NPC dialogue, etc.
     * Lower values = faction is more easily offended. Range: 0.0 - 1.0.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "0.0", ClampMax = "1.0",
        Tooltip = "Severity threshold for mild disapproval (suspicious dialogue, minor rep loss). Lower = more easily offended."))
    float DisapproveThreshold = 0.2f;

    /**
     * Actions scoring above this threshold are considered criminal by this faction.
     * Triggers bounty, guard alerts, faction reputation penalties.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "0.0", ClampMax = "1.0",
        Tooltip = "Severity threshold for criminal condemnation (bounties, guard alerts, major rep loss)."))
    float CondemnThreshold = 0.5f;

    /**
     * Actions scoring above this threshold provoke an immediate hostile response.
     * Faction members will attack on sight, declare war, or escalate to lethal force.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ideology", meta = (ClampMin = "0.0", ClampMax = "1.0",
        Tooltip = "Severity threshold for hostile response (attack on sight, war declaration, lethal force)."))
    float HostileThreshold = 0.8f;


    /**
     * Faction produces resources from territory cells and expands/defends via influence.
     * TRUE: Kingdom, Empire, Religious order. FALSE: Bandit roamers, merchant guilds.
     * Controls: territory-based production scaling, influence propagation, governance evolution.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bControlsTerritory = true;

    /**
     * Faction participates in resource production, consumption, and trade.
     * TRUE: Any faction with an economy. FALSE: Mindless creatures, undead hordes.
     * When false: pop grows only via spawning (SpawnRatePerCell), no resource tracking.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bHasEconomy = true;

    /**
     * Population grows organically via births/deaths/migration (vs spawning or recruitment).
     * TRUE: Kingdoms, settlements, villages. FALSE: Mercenary bands (recruitment), creature packs (spawning).
     * Affects: birth/death/migration formulas, food sufficiency impact.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bHasCivilianPopulation = true;

    /**
     * Faction can buy/sell resources with other factions via natural trade.
     * TRUE: Kingdoms, merchants, mercenaries. FALSE: Bandits, creatures.
     * Trade only occurs with Neutral-or-better relationships.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bParticipatesInTrade = true;

    /**
     * Faction relationships can shift via diplomacy scoring each tick.
     * TRUE: All sentient factions. FALSE: Creatures, undead (locked to Hostile forever).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
    bool bCanNegotiate = true;


    /**
     * Designer-set production profile. Defines this faction's economic identity.
     * Each resource is produced per tick scaled by (ControlledCells / ReferenceCells).
     * Ex: Farming kingdom: Food=1.0, Mat=0.3, Arms=0.0, Wealth=0.1
     * Ex: Mining hold:     Food=0.2, Mat=1.0, Arms=0.0, Wealth=0.2
     * Ex: Bandits:         All 0.0 (they steal, not produce)
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Economy")
    FMythicResourceStock BaseProduction;

    /** Computed supply per tick. Territory production + income sources (tax/raid/contract). Sim-written. */
    UPROPERTY(BlueprintReadOnly, Category = "Economy")
    FMythicResourceStock Supply;

    /** Computed demand per tick. Population food + military upkeep. Sim-written. */
    UPROPERTY(BlueprintReadOnly, Category = "Economy")
    FMythicResourceStock Demand;

    /**
     * Resource stockpile. Accumulated Supply - Demand each tick. Can go negative (deficit).
     * Negative reserves = faction is struggling. Affects food sufficiency, military strength, trade.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Economy")
    FMythicResourceStock Reserves;

    /** Market prices = Demand / max(Supply, 0.01). High demand + low supply = expensive. Sim-written. */
    UPROPERTY(BlueprintReadOnly, Category = "Economy")
    FMythicResourceStock Prices;

    /**
     * Aggregate military strength 0.0-1.0. Derived: (Arms*0.6 + Wealth*0.4) / MaxReserve, clamped.
     * 0.0 = defenseless. 0.5 = moderate. 1.0 = fully armed. Affects contract income, diplo weight.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Economy")
    float MilitaryStrength = 0.5f;


    /**
     * Total population count. Set initial value here in data asset.
     * Ex: Large kingdom=500, Small guild=40, Creature pack=0 (grows via spawning).
     * Sim modifies this each tick via births/deaths/migration/spawning/recruitment.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Population")
    int32 Population = 0;

    UPROPERTY(Transient)
    int32 LastAlivePopulation = 0;

    UPROPERTY(Transient)
    bool bHasBeenPopulated = false;

    UPROPERTY(Transient)
    bool bIdeologyDirty = false;

    UPROPERTY(Transient)
    bool bFamineActive = false;

    UPROPERTY(Transient)
    bool bWeaknessActive = false;

    /** Authority/private canonical identity of the current leader; invalid means no leader. */
    FMythicEntityId LeaderEntityId;

    /** Significance score of the current leader. Used for leadership succession on leader death. */
    UPROPERTY(BlueprintReadOnly, Category = "Population")
    float LeaderSignificanceScore = 0.0f;


    /** Number of territory grid cells this faction currently dominates. Sim-written from territory propagation. */
    UPROPERTY(BlueprintReadOnly, Category = "Territory")
    int32 ControlledCellCount = 0;
};


struct FMythicFactionMoralProfile {
    FMythicIdeologyProfile Ideology;
    float DisapproveThreshold = 0.2f;
    float CondemnThreshold = 0.5f;
    float HostileThreshold = 0.8f;
};


UCLASS(BlueprintType, Const)
class MYTHIC_API UMythicFactionDatabaseSettings : public UDataAsset {
    GENERATED_BODY()

public:
    /** Maximum number of factions the system supports. Drives array sizing. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "2", ClampMax = "64"))
    int32 MaxFactions = 20;

    /** Initial faction definitions. Loaded at startup. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TArray<FMythicFactionData> InitialFactions;

    /**
     * What each diplomatic relation is worth on the -100..+100 standing scale. Per-NPC affiliation
     * deltas add on top of this baseline before banding, so authored personality biases the living
     * diplomacy without ever replacing it.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Relations")
    TMap<EMythicFactionRelation, float> RelationStandingBaselines = {
        {EMythicFactionRelation::Allied, 75.0f},
        {EMythicFactionRelation::Friendly, 60.0f},
        {EMythicFactionRelation::Neutral, 0.0f},
        {EMythicFactionRelation::Unfriendly, -60.0f},
        {EMythicFactionRelation::Hostile, -75.0f},
    };
};


UCLASS()
class MYTHIC_API UMythicFactionDatabase : public UObject {
    GENERATED_BODY()

public:
    void Initialize(const UMythicFactionDatabaseSettings *Settings);

    virtual void BeginDestroy() override;


    FMythicFactionData *GetFactionMutable(FMythicFactionId Id);

    void SetRelationship(FMythicFactionId A, FMythicFactionId B, EMythicFactionRelation Relation);

    FMythicFactionId RegisterFaction(const FMythicFactionData &Data);

    FMythicFactionId CreateFactionFromConquest(FMythicFactionId OriginalFaction, int32 SurvivorCount);

    void AnnihilateFaction(FMythicFactionId Id);

    void RestoreResistanceToFaction(FMythicFactionId Id);

    int32 GetRegisteredCount() const { return RegisteredCount; }

    FMythicFactionData *GetFactionMutableByIndex(int32 Index);

    EMythicFactionRelation GetWriteRelationship(FMythicFactionId A, FMythicFactionId B) const;

    void ForEachAliveFactionMutable(TFunctionRef<void(FMythicFactionId, FMythicFactionData &)> Callback);

    void CommitWrites();


    bool GetFaction(FMythicFactionId Id, FMythicFactionData &OutData) const;

    bool GetFactionMoralProfile(FMythicFactionId Id, FMythicFactionMoralProfile &Out) const;

    bool FindFactionByTag(const FGameplayTag &Tag, FMythicFactionData &OutData, FMythicFactionId *OutId = nullptr) const;

    FMythicFactionId FindFactionId(const FGameplayTag &Tag) const;

    EMythicFactionRelation GetRelationship(FMythicFactionId A, FMythicFactionId B) const;

    /** The authored standing value a diplomatic relation contributes (see RelationStandingBaselines). */
    float GetRelationStandingBaseline(EMythicFactionRelation Relation) const;

    /**
     * Bands a -100..+100 stance into an attitude with the same thresholds the player-standing branch
     * uses. Static and pure so tests can pin the banding without a world.
     */
    static ETeamAttitude::Type BandStanding(float Stance, float HostileThreshold, float FriendlyThreshold);

    int32 GetActiveFactionCount() const;

    int32 GetMaxFactions() const { return MaxFactions; }

    void ForEachAliveFaction(TFunctionRef<void(FMythicFactionId, const FMythicFactionData &)> Callback) const;

    void ReportLeaderCandidate(FMythicFactionId FactionId,
                               const FMythicEntityId &EntityId, float Score);

    /** Returns true when the mutable authority snapshot currently assigns EntityId as any living faction's leader. */
    bool ReferencesEntityIdentity(const FMythicEntityId &EntityId) const;

    /** Clears leadership owned by a permanently dead entity so normal candidate succession can resume. */
    bool HandlePermanentEntityDeath(const FMythicEntityId &EntityId);

    /** Clears an exact current leader assignment without implying why that logical reference became invalid. */
    bool ClearLeaderReference(const FMythicEntityId &EntityId);

    /**
     * Clears loaded leader assignments whose logical person cannot be rehydrated after an in-place world restore.
     * The caller must hold the LivingWorld simulation lock and commit the write snapshot before retiring identities.
     */
    int32 ClearUnrestorableLeaderReferences(
        const TSet<FMythicEntityId> &RestorableEntityIds);


    virtual void Serialize(FArchive &Ar) override;

private:
    int32 MaxFactions = 0;

    UPROPERTY(Transient)
    TArray<FMythicFactionData> WriteFactions;

    UPROPERTY(Transient)
    TArray<FMythicFactionData> ReadFactions;

    UPROPERTY(Transient)
    TArray<EMythicFactionRelation> WriteRelationships;

    UPROPERTY(Transient)
    TArray<EMythicFactionRelation> ReadRelationships;

    std::atomic<int32> RegisteredCount = 0;

    UPROPERTY(Transient)
    TMap<EMythicFactionRelation, float> RelationStandingBaselines;

    mutable FCriticalSection SnapshotLock;

    int32 RelationIndex(FMythicFactionId A, FMythicFactionId B) const {
        const int32 Low = FMath::Min(A.Index, B.Index);
        const int32 High = FMath::Max(A.Index, B.Index);
        return Low * MaxFactions + High;
    }
};
