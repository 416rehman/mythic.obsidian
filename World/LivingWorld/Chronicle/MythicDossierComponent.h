
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/Acquaintance/MythicAcquaintanceTypes.h"
#include "MythicDossierComponent.generated.h"

USTRUCT(BlueprintType)
struct FMythicNpcDossier {
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    uint32 NameHash = 0;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Dossier")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Dossier")
    FGameplayTag Faction;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Dossier")
    FGameplayTag RoleTag;

    // How many conversations this player has had with the NPC.
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Dossier")
    int32 TimesMet = 0;

    // Non-Met interactions witnessed/performed involving this NPC (trades, quest help, wrongs...).
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Dossier")
    int32 DeedsWitnessed = 0;

    // The relation's warmth as of the last acquaintance event (display snapshot — the live value is the ledger's).
    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Dossier")
    float LastKnownWarmth = 0.0f;

    UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Dossier")
    bool bDead = false;

    UPROPERTY(SaveGame)
    uint32 KillerNameHash = 0;

    UPROPERTY(SaveGame)
    double LastUpdateTime = 0.0;
};

struct FMythicDossierRules {
    static FMythicNpcDossier *Find(TArray<FMythicNpcDossier> &Dossiers, uint32 NameHash) {
        return Dossiers.FindByPredicate([NameHash](const FMythicNpcDossier &D) { return D.NameHash == NameHash; });
    }
    static const FMythicNpcDossier *Find(const TArray<FMythicNpcDossier> &Dossiers, uint32 NameHash) {
        return Dossiers.FindByPredicate([NameHash](const FMythicNpcDossier &D) { return D.NameHash == NameHash; });
    }

    static FMythicNpcDossier *Upsert(TArray<FMythicNpcDossier> &Dossiers, uint32 NameHash, double Now, int32 Cap) {
        if (NameHash == 0) {
            return nullptr;
        }
        FMythicNpcDossier *Row = Find(Dossiers, NameHash);
        if (!Row) {
            const int32 EffectiveCap = FMath::Max(1, Cap);
            while (Dossiers.Num() >= EffectiveCap) {
                int32 OldestIdx = 0;
                for (int32 i = 1; i < Dossiers.Num(); ++i) {
                    if (Dossiers[i].LastUpdateTime < Dossiers[OldestIdx].LastUpdateTime) {
                        OldestIdx = i;
                    }
                }
                Dossiers.RemoveAt(OldestIdx, 1, EAllowShrinking::No);
            }
            FMythicNpcDossier NewRow;
            NewRow.NameHash = NameHash;
            Row = &Dossiers[Dossiers.Add(NewRow)];
        }
        Row->LastUpdateTime = Now;
        return Row;
    }

    static void ApplyRelationEvent(FMythicNpcDossier &Row, const FMythicNpcRelation &Relation, EMythicNpcInteraction Interaction) {
        if (Interaction == EMythicNpcInteraction::Met) {
            ++Row.TimesMet;
        }
        else {
            ++Row.DeedsWitnessed;
        }
        Row.LastKnownWarmth = Relation.Warmth;
        if (Relation.Faction.IsValid()) {
            Row.Faction = Relation.Faction;
        }
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnDossiersChanged);

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicDossierComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicDossierComponent();


    void ServerObserveNpc(uint32 NameHash, const FText &DisplayName, FGameplayTag Faction, FGameplayTag RoleTag);

    void ServerRecordNpcDeath(uint32 NameHash, uint32 KillerNameHash, const FText &DisplayName, FGameplayTag Faction,
                              FGameplayTag RoleTag, bool bUpsertIfMissing);

    void RestoreDossiers(const TArray<FMythicNpcDossier> &InDossiers);


    /** All dossier rows (the dossier UI's list source). */
    UFUNCTION(BlueprintPure, Category = "Dossier")
    const TArray<FMythicNpcDossier> &GetDossiers() const { return Dossiers; }

    bool GetDossier(uint32 NameHash, FMythicNpcDossier &Out) const;

    UFUNCTION(BlueprintPure, Category = "Dossier")
    int32 GetDossierCount() const { return Dossiers.Num(); }

    UPROPERTY(BlueprintAssignable, Category = "Dossier")
    FMythicOnDossiersChanged OnDossiersChanged;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(ReplicatedUsing = OnRep_Dossiers, SaveGame)
    TArray<FMythicNpcDossier> Dossiers;

    UFUNCTION()
    void OnRep_Dossiers();

    /** Hard cap on dossier rows per player (LRU eviction beyond it — mirrors the acquaintance ledger's bound). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dossier", meta = (ClampMin = "1"))
    int32 MaxDossiers = 64;

private:
    void HandleRelationChanged(const FMythicNpcRelation &Relation, EMythicNpcInteraction Interaction);
    FDelegateHandle RelationChangedHandle;

    double NowSeconds() const;
};
