
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "MythicWorldChronicleSubsystem.generated.h"

class UMythicLivingWorldSubsystem;
struct FMythicWorldEvent;

USTRUCT(BlueprintType)
struct FMythicChronicleEntry {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Chronicle")
    FText Text;

    UPROPERTY(BlueprintReadOnly, Category = "Chronicle")
    FGameplayTag EventTag;

    UPROPERTY(BlueprintReadOnly, Category = "Chronicle")
    float WorldTime = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Chronicle")
    float Significance = 0.0f;

    UPROPERTY()
    int32 Sequence = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicOnChronicleEntry, const FMythicChronicleEntry &, Entry);

UCLASS()
class MYTHIC_API UMythicWorldChronicleSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    // Fired (game thread) when a new significant world event is chronicled — for a UI toast / news ticker.
    UPROPERTY(BlueprintAssignable, Category = "World Chronicle")
    FMythicOnChronicleEntry OnChronicleEntry;

    // The most recent chronicle entries, oldest-first (for a chronicle / world-news panel).
    UFUNCTION(BlueprintCallable, Category = "World Chronicle")
    TArray<FMythicChronicleEntry> GetRecentChronicle(int32 MaxCount = 20) const;

    void IngestReplicatedEntry(const FMythicChronicleEntry &Entry);

    // Only events at or above this significance are chronicled (macro events, not every combat tick).
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "World Chronicle")
    float MinSignificance = 0.5f;

    static FString EventTagToReadable(const FGameplayTag &Tag);

protected:
    void HandleWorldSimCommitted();

    FText FormatEvent(const FMythicWorldEvent &Event, UMythicLivingWorldSubsystem *LWS) const;

    void AppendEntry(const FMythicChronicleEntry &Entry);

private:
    TWeakObjectPtr<UMythicLivingWorldSubsystem> LivingWorld;
    FDelegateHandle CommitHandle;

    uint32 LastSeenEventId = 0;

    bool bSeeded = false;

    int32 NextSequence = 1;

    int32 LastIngestedSequence = 0;

    TArray<FMythicChronicleEntry> Entries;
    static constexpr int32 MaxEntries = 256;
};
