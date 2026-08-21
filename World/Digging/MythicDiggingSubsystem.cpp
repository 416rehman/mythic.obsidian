
#include "World/Digging/MythicDiggingSubsystem.h"

#include "World/Digging/MythicDigSiteRules.h"
#include "Engine/World.h"
#include "Mythic.h"

void UMythicDiggingSubsystem::EnsureRegistryLoaded() {
    if (bRegistryResolved) {
        return;
    }
    bRegistryResolved = true;
    if (const UMythicDiggingSettings *Settings = GetDefault<UMythicDiggingSettings>()) {
        if (!Settings->DigSiteRegistry.IsNull()) {
            Registry = Settings->DigSiteRegistry.LoadSynchronous();
        }
    }
    if (!Registry) {
        UE_LOG(Myth, Log, TEXT("Digging: no dig-site registry configured (UMythicDiggingSettings::DigSiteRegistry) — digs yield nothing until content is wired."));
    }
}

const UMythicDigSiteRegistry *UMythicDiggingSubsystem::GetRegistry() {
    EnsureRegistryLoaded();
    return Registry;
}

bool UMythicDiggingSubsystem::ServerResolveAndConsumeAt(const FVector &DigLoc, FMythicDigSiteEntry &OutEntry) {
    const UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client) {
        return false;
    }

    const UMythicDigSiteRegistry *Reg = GetRegistry();
    if (!Reg) {
        return false;
    }

    FMythicDigSiteEntry Entry;
    const bool bSiteExists = Reg->FindSiteAtLocation(DigLoc, Entry);
    const bool bAlreadyConsumed = bSiteExists && ConsumedSiteIds.Contains(Entry.SiteId);

    if (!MythicDigSite::ShouldYieldBuriedFind(bSiteExists, bSiteExists, bAlreadyConsumed)) {
        return false;
    }

    ConsumedSiteIds.Add(Entry.SiteId);
    OutEntry = Entry;
    UE_LOG(Myth, Log, TEXT("Digging: buried find '%s' (site %d) unearthed + consumed."), *Entry.DisplayName.ToString(), Entry.SiteId);
    return true;
}
