
#include "MythicCodexComponent.h"

#include "Mythic.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

UMythicCodexComponent::UMythicCodexComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicCodexComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicCodexComponent, Bestiary, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicCodexComponent, DiscoveredTerms, COND_OwnerOnly);
}

FMythicBestiaryRecord &UMythicCodexComponent::FindOrAddRecord(const FGameplayTag &Key) {
    for (FMythicBestiaryRecord &Record : Bestiary) {
        if (Record.Key == Key) {
            return Record;
        }
    }
    FMythicBestiaryRecord &NewRecord = Bestiary.AddDefaulted_GetRef();
    NewRecord.Key = Key;
    return NewRecord;
}

void UMythicCodexComponent::ServerRegisterBestiaryKill(FGameplayTag Key) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Key.IsValid()) {
        return;
    }
    FMythicBestiaryRecord &Record = FindOrAddRecord(Key);
    const EMythicCodexTier TierBefore = FMythicBestiaryRules::TierForKills(Record.KillCount, Record.bEncountered, 1, 10);
    Record.KillCount++;
    Record.bEncountered = true;
    const EMythicCodexTier TierAfter = FMythicBestiaryRules::TierForKills(Record.KillCount, Record.bEncountered, 1, 10);
    if (TierAfter == EMythicCodexTier::Full && TierBefore != EMythicCodexTier::Full) {
        MirrorFullTierTag(Key);
    }
    if (!bIsRestoring) {
        UE_LOG(Myth, Verbose, TEXT("Codex: %s bestiary kill %s (now %d)"), *GetNameSafe(Owner), *Key.ToString(), Record.KillCount);
        OnBestiaryUpdated.Broadcast(Key);
    }
}

void UMythicCodexComponent::MirrorFullTierTag(const FGameplayTag &Key) const {
    const FString KeyStr = Key.ToString();
    const FString Prefix(TEXT("Codex.Bestiary."));
    if (!KeyStr.StartsWith(Prefix)) {
        return;
    }
    const FString Suffix = KeyStr.RightChop(Prefix.Len());
    const FGameplayTag MirrorTag = FGameplayTag::RequestGameplayTag(
        FName(*(FString(TEXT("Knowledge.Bestiary.Full.")) + Suffix)), false);
    if (!MirrorTag.IsValid()) {
        return;
    }
    const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(GetOwner());
    UAbilitySystemComponent *ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
    if (ASC) {
        ASC->SetLooseGameplayTagCount(MirrorTag, 1);
        UE_LOG(Myth, Log, TEXT("Codex: %s reached FULL tier on %s → mirrored %s"), *GetNameSafe(GetOwner()), *KeyStr, *MirrorTag.ToString());
    }
}

void UMythicCodexComponent::ServerRegisterEncounter(FGameplayTag Key) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Key.IsValid()) {
        return;
    }
    for (const FMythicBestiaryRecord &Record : Bestiary) {
        if (Record.Key == Key) {
            if (Record.bEncountered) {
                return;
            }
            break;
        }
    }
    FMythicBestiaryRecord &Record = FindOrAddRecord(Key);
    Record.bEncountered = true;
    if (!bIsRestoring) {
        UE_LOG(Myth, Log, TEXT("Codex: %s first encountered %s"), *GetNameSafe(Owner), *Key.ToString());
        OnBestiaryUpdated.Broadcast(Key);
    }
}

void UMythicCodexComponent::ServerDiscoverTerm(FGameplayTag Term) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Term.IsValid()) {
        return;
    }
    if (DiscoveredTerms.HasTagExact(Term)) {
        return;
    }
    DiscoveredTerms.AddTag(Term);
    if (!bIsRestoring) {
        UE_LOG(Myth, Log, TEXT("Codex: %s discovered term %s"), *GetNameSafe(Owner), *Term.ToString());
        OnTermDiscovered.Broadcast(Term);
    }
}

EMythicCodexTier UMythicCodexComponent::GetBestiaryTier(FGameplayTag Key, int32 KillThresholdBasic, int32 KillThresholdFull) const {
    for (const FMythicBestiaryRecord &Record : Bestiary) {
        if (Record.Key == Key) {
            return FMythicBestiaryRules::TierForKills(Record.KillCount, Record.bEncountered, KillThresholdBasic, KillThresholdFull);
        }
    }
    return EMythicCodexTier::Unknown;
}

int32 UMythicCodexComponent::GetKillCount(FGameplayTag Key) const {
    for (const FMythicBestiaryRecord &Record : Bestiary) {
        if (Record.Key == Key) {
            return Record.KillCount;
        }
    }
    return 0;
}

void UMythicCodexComponent::RestoreCodex(const TArray<FMythicBestiaryRecord> &SavedBestiary, const TArray<FGameplayTag> &SavedTerms) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    bIsRestoring = true;
    if (SavedBestiary.Num() > 0) {
        Bestiary = SavedBestiary;
    }
    for (const FGameplayTag &Term : SavedTerms) {
        if (Term.IsValid()) {
            DiscoveredTerms.AddTag(Term);
        }
    }
    for (const FMythicBestiaryRecord &Record : Bestiary) {
        if (FMythicBestiaryRules::TierForKills(Record.KillCount, Record.bEncountered, 1, 10) == EMythicCodexTier::Full) {
            MirrorFullTierTag(Record.Key);
        }
    }
    bIsRestoring = false;
    UE_LOG(MythSaveLoad, Log, TEXT("Codex: %s restored %d bestiary records, %d terms"),
           *GetNameSafe(Owner), SavedBestiary.Num(), SavedTerms.Num());
}

void UMythicCodexComponent::OnRep_Bestiary() {
    const bool bInitialSync = !bClientBestiarySeeded;
    bClientBestiarySeeded = true;
    for (const FMythicBestiaryRecord &Record : Bestiary) {
        const FMythicBestiaryRecord *Shadow = ClientBestiaryShadow.Find(Record.Key);
        if (!bInitialSync && (!Shadow || Shadow->KillCount != Record.KillCount || Shadow->bEncountered != Record.bEncountered)) {
            OnBestiaryUpdated.Broadcast(Record.Key);
        }
        ClientBestiaryShadow.Add(Record.Key, Record);
    }
}

void UMythicCodexComponent::OnRep_DiscoveredTerms() {
    const bool bInitialSync = !bClientTermsSeeded;
    bClientTermsSeeded = true;
    for (const FGameplayTag &Term : DiscoveredTerms) {
        if (!bInitialSync && !ClientTermsShadow.HasTagExact(Term)) {
            OnTermDiscovered.Broadcast(Term);
        }
    }
    ClientTermsShadow = DiscoveredTerms;
}
