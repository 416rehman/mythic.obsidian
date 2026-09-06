#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

class UMythicLifeComponent;
enum class EMythicLootCreditSource : uint8 { Other, EnemyKill, Harvest, Quest, Chest };

struct MYTHIC_API FMythicLootCreditContext {
    FGuid Receipt;
    TWeakObjectPtr<AActor> Victim;
    TWeakObjectPtr<APlayerController> CreditedTo;
    int32 EnemyTier = 0;
    EMythicLootCreditSource Source = EMythicLootCreditSource::Other;
    bool IsGenuineEnemyDeath() const { return bGenuineDeath && Source==EMythicLootCreditSource::EnemyKill && Receipt.IsValid(); }
private:
    friend class UMythicLifeComponent;
    bool bGenuineDeath = false;
    static FMythicLootCreditContext FromEnemyDeath(AActor* Victim,APlayerController* Player,int32 Tier) {
        FMythicLootCreditContext Result;
        Result.Receipt=FGuid::NewGuid(); Result.Victim=Victim; Result.CreditedTo=Player; Result.EnemyTier=Tier;
        Result.Source=EMythicLootCreditSource::EnemyKill; Result.bGenuineDeath=true;
        return Result;
    }
};
