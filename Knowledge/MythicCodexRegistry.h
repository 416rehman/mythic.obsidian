
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Itemization/MythicDataAsset.h"
#include "MythicCodexRegistry.generated.h"

class UMythicBestiaryEntry;
class UMythicGlossaryEntry;

UCLASS(BlueprintType)
class MYTHIC_API UMythicCodexLibrary : public UMythicDataAsset {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    TArray<TObjectPtr<UMythicBestiaryEntry>> BestiaryEntries;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Knowledge")
    TArray<TObjectPtr<UMythicGlossaryEntry>> GlossaryEntries;
};

UCLASS()
class MYTHIC_API UMythicCodexRegistry : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    // Content lookup by codex key. Null when no entry is authored for the key (callers show a generic/"???" page).
    UFUNCTION(BlueprintPure, Category = "Knowledge")
    UMythicBestiaryEntry *FindBestiaryEntry(FGameplayTag CodexKey) const;

    UFUNCTION(BlueprintPure, Category = "Knowledge")
    UMythicGlossaryEntry *FindGlossaryEntry(FGameplayTag TermKey) const;

    // Full authored lists (codex UI page enumeration — including not-yet-discovered entries shown as "???").
    // By-value copies: UHT function signatures use raw pointers while the cached storage is TObjectPtr; these are
    // rare UI-open-time calls, so the copy is irrelevant.
    UFUNCTION(BlueprintPure, Category = "Knowledge")
    TArray<UMythicBestiaryEntry *> GetAllBestiaryEntries() const;

    UFUNCTION(BlueprintPure, Category = "Knowledge")
    TArray<UMythicGlossaryEntry *> GetAllGlossaryEntries() const;

private:
    void EnsureIndexed() const { if (!bIndexed) { const_cast<UMythicCodexRegistry *>(this)->BuildIndex(); } }
    void BuildIndex();

    bool bIndexed = false;

    UPROPERTY(Transient)
    TObjectPtr<UMythicCodexLibrary> Library;

    UPROPERTY(Transient)
    TMap<FGameplayTag, TObjectPtr<UMythicBestiaryEntry>> BestiaryByKey;

    UPROPERTY(Transient)
    TMap<FGameplayTag, TObjectPtr<UMythicGlossaryEntry>> GlossaryByKey;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMythicBestiaryEntry>> AllBestiary;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UMythicGlossaryEntry>> AllGlossary;
};
