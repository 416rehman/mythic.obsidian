#include "World/Harvesting/MythicHarvestReplicationTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicHarvestReplicationTypes)

EMythicHarvestPresentationStreamOrder
FMythicHarvestPresentationStreamToken::Compare(
    const FMythicHarvestPresentationStreamToken &Left,
    const FMythicHarvestPresentationStreamToken &Right) {
    if (!Left.IsValid() || !Right.IsValid()) {
        return EMythicHarvestPresentationStreamOrder::Invalid;
    }
    if (Left == Right) {
        return EMythicHarvestPresentationStreamOrder::Same;
    }

    // The nonce scopes one authority lifetime. Packets from another lifetime must never be ordered against this one,
    // even when their serial happens to look newer.
    if (Left.Nonce != Right.Nonce) {
        return EMythicHarvestPresentationStreamOrder::Conflict;
    }

    const uint32 ForwardDistance = Left.Serial - Right.Serial;
    if (ForwardDistance == 0x80000000u) {
        return EMythicHarvestPresentationStreamOrder::Conflict;
    }
    return ForwardDistance < 0x80000000u
        ? EMythicHarvestPresentationStreamOrder::Newer
        : EMythicHarvestPresentationStreamOrder::Older;
}

bool FMythicHarvestPresentationStreamToken::TryMakeInitial(
    const FGuid &Nonce,
    FMythicHarvestPresentationStreamToken &OutToken) {
    OutToken = FMythicHarvestPresentationStreamToken();
    if (!Nonce.IsValid()) {
        return false;
    }
    OutToken = FMythicHarvestPresentationStreamToken(Nonce, 1);
    return true;
}

bool FMythicHarvestPresentationStreamToken::TryAdvance(
    const FMythicHarvestPresentationStreamToken &Current,
    FMythicHarvestPresentationStreamToken &OutToken) {
    OutToken = FMythicHarvestPresentationStreamToken();
    if (!Current.IsValid()) {
        return false;
    }

    uint32 NextSerial = Current.Serial + 1;
    if (NextSerial == 0) {
        NextSerial = 1;
    }
    OutToken = FMythicHarvestPresentationStreamToken(Current.Nonce, NextSerial);
    return true;
}
