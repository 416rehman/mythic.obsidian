
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "MythicCodexTypes.h"
#include "MythicCodexComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicOnBestiaryUpdated, FGameplayTag, Key);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicOnTermDiscovered, FGameplayTag, Term);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicCodexComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicCodexComponent();


    // Record a KILL of archetype Key by the owning player: create-or-increment the record; a kill is also an
    // encounter (sets bEncountered). No-op off authority / invalid key / while restoring. Broadcasts OnBestiaryUpdated.
    UFUNCTION(BlueprintCallable, Category = "Knowledge|Bestiary")
    void ServerRegisterBestiaryKill(FGameplayTag Key);

    // Record an ENCOUNTER (seen/fought, not killed) of archetype Key: creates the silhouette record if absent, flags
    // bEncountered. Idempotent (an already-encountered record is untouched — no broadcast). No-op off authority.
    UFUNCTION(BlueprintCallable, Category = "Knowledge|Bestiary")
    void ServerRegisterEncounter(FGameplayTag Key);

    // Discover glossary term Term: idempotent container add; broadcasts OnTermDiscovered ONLY on a genuine first
    // discovery (mirrors UMythicNarrativeStateComponent::ServerSetStoryTag). No-op off authority / while restoring.
    UFUNCTION(BlueprintCallable, Category = "Knowledge|Glossary")
    void ServerDiscoverTerm(FGameplayTag Term);


    // The owning player's intel tier for Key. Thresholds come from the archetype's UMythicBestiaryEntry when the UI
    // has one (pass them through); the 1/10 defaults match unauthored entries. Pure rule: FMythicBestiaryRules.
    UFUNCTION(BlueprintPure, Category = "Knowledge|Bestiary")
    EMythicCodexTier GetBestiaryTier(FGameplayTag Key, int32 KillThresholdBasic = 1, int32 KillThresholdFull = 10) const;

    // Wave P (P5i): has this player mastered Key to the FULL intel tier? The apex-hunt knowledge gate reads this
    // directly; content-facing gates read the mirrored Knowledge.Bestiary.Full.<KeySuffix> ASC tag instead.
    UFUNCTION(BlueprintPure, Category = "Knowledge|Bestiary")
    bool HasFullBestiaryTier(FGameplayTag Key, int32 KillThresholdFull = 10) const {
        return GetBestiaryTier(Key, 1, KillThresholdFull) == EMythicCodexTier::Full;
    }

    UFUNCTION(BlueprintPure, Category = "Knowledge|Bestiary")
    int32 GetKillCount(FGameplayTag Key) const;

    UFUNCTION(BlueprintPure, Category = "Knowledge|Glossary")
    bool HasDiscoveredTerm(FGameplayTag Term) const { return DiscoveredTerms.HasTagExact(Term); }

    const TArray<FMythicBestiaryRecord> &GetAllBestiaryRecords() const { return Bestiary; }
    const FGameplayTagContainer &GetDiscoveredTerms() const { return DiscoveredTerms; }

    void RestoreCodex(const TArray<FMythicBestiaryRecord> &SavedBestiary, const TArray<FGameplayTag> &SavedTerms);

    // ── Events ──
    UPROPERTY(BlueprintAssignable, Category = "Knowledge|Bestiary")
    FMythicOnBestiaryUpdated OnBestiaryUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Knowledge|Glossary")
    FMythicOnTermDiscovered OnTermDiscovered;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    // Per-archetype kill/encounter records. COND_OwnerOnly: private per-player discovery progress on a net-everyone
    // PlayerState (mirrors NarrativeState::StoryTags). Small + rarely mutated → a plain replicated array is fine.
    UPROPERTY(ReplicatedUsing = OnRep_Bestiary, BlueprintReadOnly, Category = "Knowledge|Bestiary")
    TArray<FMythicBestiaryRecord> Bestiary;

    // Discovered glossary terms. Same COND_OwnerOnly rationale.
    UPROPERTY(ReplicatedUsing = OnRep_DiscoveredTerms, BlueprintReadOnly, Category = "Knowledge|Glossary")
    FGameplayTagContainer DiscoveredTerms;

    UFUNCTION()
    void OnRep_Bestiary();

    UFUNCTION()
    void OnRep_DiscoveredTerms();

private:
    FMythicBestiaryRecord &FindOrAddRecord(const FGameplayTag &Key);

    void MirrorFullTierTag(const FGameplayTag &Key) const;

    bool bIsRestoring = false;

    TMap<FGameplayTag, FMythicBestiaryRecord> ClientBestiaryShadow;
    FGameplayTagContainer ClientTermsShadow;
    bool bClientBestiarySeeded = false;
    bool bClientTermsSeeded = false;
};
