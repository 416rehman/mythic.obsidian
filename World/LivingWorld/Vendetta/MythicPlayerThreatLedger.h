
#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Vendetta/MythicVendettaTypes.h"


struct FMythicThreatKey {
    uint8 FactionIndex = FMythicFactionId::InvalidIndex;
    FString PlayerKey;

    FMythicThreatKey() = default;
    FMythicThreatKey(FMythicFactionId Faction, const FString &InPlayerKey)
        : FactionIndex(Faction.Index), PlayerKey(InPlayerKey) {}

    bool operator==(const FMythicThreatKey &Other) const {
        return FactionIndex == Other.FactionIndex && PlayerKey == Other.PlayerKey;
    }

    friend uint32 GetTypeHash(const FMythicThreatKey &Key) {
        return HashCombine(GetTypeHash(Key.FactionIndex), GetTypeHash(Key.PlayerKey));
    }
};

struct FMythicVendettaLedgerEntry {
    float Threat = 0.0f;

    double LastVendettaTime = -1.0e30;

    EMythicVendettaType LastVendettaType = EMythicVendettaType::None;
};


class MYTHIC_API FMythicPlayerThreatLedger {
public:
    void AddThreat(FMythicFactionId Faction, const FString &PlayerKey, float DeedMoralSeverity, float DeedWeight,
                   float SoftCap = MythicVendetta::DefaultThreatSoftCap);

    void TickDecay(float Dt, float RatePerSec);

    bool Query(FMythicFactionId Faction, const FString &PlayerKey, FMythicVendettaLedgerEntry &Out) const;

    void StampVendetta(FMythicFactionId Faction, const FString &PlayerKey, double Now, EMythicVendettaType Type);

    const TMap<FMythicThreatKey, FMythicVendettaLedgerEntry> &GetEntries() const { return Entries; }

    void Reset() { Entries.Reset(); }

private:
    TMap<FMythicThreatKey, FMythicVendettaLedgerEntry> Entries;
};
