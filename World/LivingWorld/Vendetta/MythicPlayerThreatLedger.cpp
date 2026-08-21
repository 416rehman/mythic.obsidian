
#include "World/LivingWorld/Vendetta/MythicPlayerThreatLedger.h"

void FMythicPlayerThreatLedger::AddThreat(FMythicFactionId Faction, const FString &PlayerKey, float DeedMoralSeverity,
                                          float DeedWeight, float SoftCap) {
    if (!Faction.IsValid() || PlayerKey.IsEmpty()) {
        return;
    }
    const FMythicThreatKey Key(Faction, PlayerKey);
    FMythicVendettaLedgerEntry &Entry = Entries.FindOrAdd(Key);
    Entry.Threat = MythicVendetta::AccumulatePlayerThreat(Entry.Threat, DeedMoralSeverity, DeedWeight, SoftCap);
}

void FMythicPlayerThreatLedger::TickDecay(float Dt, float RatePerSec) {
    if (Entries.Num() == 0) {
        return;
    }
    for (auto It = Entries.CreateIterator(); It; ++It) {
        FMythicVendettaLedgerEntry &Entry = It.Value();
        Entry.Threat = MythicVendetta::DecayThreat(Entry.Threat, RatePerSec, Dt);
        if (Entry.Threat <= 0.0f && Entry.LastVendettaType == EMythicVendettaType::None) {
            It.RemoveCurrent();
        }
    }
}

bool FMythicPlayerThreatLedger::Query(FMythicFactionId Faction, const FString &PlayerKey,
                                      FMythicVendettaLedgerEntry &Out) const {
    if (const FMythicVendettaLedgerEntry *Found = Entries.Find(FMythicThreatKey(Faction, PlayerKey))) {
        Out = *Found;
        return true;
    }
    return false;
}

void FMythicPlayerThreatLedger::StampVendetta(FMythicFactionId Faction, const FString &PlayerKey, double Now,
                                              EMythicVendettaType Type) {
    if (!Faction.IsValid() || PlayerKey.IsEmpty()) {
        return;
    }
    FMythicVendettaLedgerEntry &Entry = Entries.FindOrAdd(FMythicThreatKey(Faction, PlayerKey));
    Entry.LastVendettaTime = Now;
    Entry.LastVendettaType = Type;
}
