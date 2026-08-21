
#include "MythicCodexRegistry.h"

#include "Mythic.h"
#include "Knowledge/MythicBestiaryEntry.h"
#include "Knowledge/MythicGlossaryEntry.h"
#include "Settings/MythicDeveloperSettings.h"

void UMythicCodexRegistry::BuildIndex() {
    bIndexed = true;

    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || Settings->DefaultCodexLibrary.IsNull()) {
        UE_LOG(Myth, Log, TEXT("CodexRegistry: no DefaultCodexLibrary configured — codex content lookups resolve empty."));
        return;
    }

    Library = Settings->DefaultCodexLibrary.Get();
    if (!Library) {
        Library = Settings->DefaultCodexLibrary.LoadSynchronous();
    }
    if (!Library) {
        UE_LOG(Myth, Warning, TEXT("CodexRegistry: DefaultCodexLibrary failed to load (%s)"),
               *Settings->DefaultCodexLibrary.ToString());
        return;
    }

    for (UMythicBestiaryEntry *Entry : Library->BestiaryEntries) {
        if (!Entry) {
            continue;
        }
        if (!Entry->CodexKey.IsValid()) {
            UE_LOG(Myth, Warning, TEXT("CodexRegistry: bestiary entry %s has no CodexKey — skipped"), *GetNameSafe(Entry));
            continue;
        }
        AllBestiary.Add(Entry);
        BestiaryByKey.Add(Entry->CodexKey, Entry);
    }
    for (UMythicGlossaryEntry *Entry : Library->GlossaryEntries) {
        if (!Entry) {
            continue;
        }
        if (!Entry->TermKey.IsValid()) {
            UE_LOG(Myth, Warning, TEXT("CodexRegistry: glossary entry %s has no TermKey — skipped"), *GetNameSafe(Entry));
            continue;
        }
        AllGlossary.Add(Entry);
        GlossaryByKey.Add(Entry->TermKey, Entry);
    }
    if (BestiaryByKey.Num() != AllBestiary.Num() || GlossaryByKey.Num() != AllGlossary.Num()) {
        UE_LOG(Myth, Warning, TEXT("CodexRegistry: duplicate codex keys in %s (bestiary %d/%d unique, glossary %d/%d)"),
               *GetNameSafe(Library.Get()), BestiaryByKey.Num(), AllBestiary.Num(), GlossaryByKey.Num(), AllGlossary.Num());
    }
    UE_LOG(Myth, Log, TEXT("CodexRegistry: indexed %d bestiary entries, %d glossary entries from %s"),
           AllBestiary.Num(), AllGlossary.Num(), *GetNameSafe(Library.Get()));
}

UMythicBestiaryEntry *UMythicCodexRegistry::FindBestiaryEntry(FGameplayTag CodexKey) const {
    EnsureIndexed();
    const TObjectPtr<UMythicBestiaryEntry> *Found = BestiaryByKey.Find(CodexKey);
    return Found ? Found->Get() : nullptr;
}

UMythicGlossaryEntry *UMythicCodexRegistry::FindGlossaryEntry(FGameplayTag TermKey) const {
    EnsureIndexed();
    const TObjectPtr<UMythicGlossaryEntry> *Found = GlossaryByKey.Find(TermKey);
    return Found ? Found->Get() : nullptr;
}

TArray<UMythicBestiaryEntry *> UMythicCodexRegistry::GetAllBestiaryEntries() const {
    EnsureIndexed();
    TArray<UMythicBestiaryEntry *> Out;
    Out.Reserve(AllBestiary.Num());
    for (const TObjectPtr<UMythicBestiaryEntry> &Entry : AllBestiary) {
        Out.Add(Entry.Get());
    }
    return Out;
}

TArray<UMythicGlossaryEntry *> UMythicCodexRegistry::GetAllGlossaryEntries() const {
    EnsureIndexed();
    TArray<UMythicGlossaryEntry *> Out;
    Out.Reserve(AllGlossary.Num());
    for (const TObjectPtr<UMythicGlossaryEntry> &Entry : AllGlossary) {
        Out.Add(Entry.Get());
    }
    return Out;
}
