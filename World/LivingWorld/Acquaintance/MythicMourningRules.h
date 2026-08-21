
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "MythicMourningRules.generated.h"

USTRUCT(BlueprintType)
struct FMythicAvengerConfig {
    GENERATED_BODY()

    /** Base chance [0,1] a qualifying notable kill spawns an avenger (LOW by default — vengeance is an event, not a tax). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avengers", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BaseChance = 0.10f;

    /** Extra chance per 100 points of the killer's notoriety with the victim's faction (a hated killer draws vengeance). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avengers", meta = (ClampMin = "0.0"))
    float ChancePerNotoriety100 = 0.10f;

    /** Ceiling on the composed chance (vengeance never becomes a certainty). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avengers", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxChance = 0.5f;

    /** Minimum seconds between avenger beats against the SAME player (armed when an avenger actually dispatches). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avengers", meta = (ClampMin = "0.0"))
    float CooldownSeconds = 900.0f;

    /** Seconds between the TELEGRAPH ("kin swear vengeance") and the avenger actually appearing. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avengers", meta = (ClampMin = "0.0"))
    float TelegraphDelaySeconds = 30.0f;

    /** Seconds between subsystem upkeep checks (ONE repeating server timer — never Tick). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avengers", meta = (ClampMin = "5.0"))
    float CheckIntervalSeconds = 15.0f;

    /** Max simultaneously-live avengers per hunted player (an at-cap player is never re-dispatched). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avengers", meta = (ClampMin = "1"))
    int32 MaxSimultaneousAvengers = 2;

    /** NPC-type tag handed to UMythicNPCManager::SpawnRandomNPC for the avenger (CONTENT — unset keeps the whole beat
     *  silent, telegraph included, mirroring the bounty content gate). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avengers")
    FGameplayTag AvengerNPCType;

    /** Avengers appear on a ring near-but-not-on the killer: min ring radius (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avengers", meta = (ClampMin = "500.0"))
    float MinSpawnDistance = 2000.0f;

    /** Max ring radius (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Avengers", meta = (ClampMin = "500.0"))
    float MaxSpawnDistance = 3500.0f;
};


namespace MythicMourning {
inline float AvengerChance(float KillerNotoriety, const FMythicAvengerConfig &Config) {
    const float Notoriety = FMath::Max(0.0f, KillerNotoriety);
    const float Chance = Config.BaseChance + Config.ChancePerNotoriety100 * (Notoriety / 100.0f);
    return FMath::Clamp(Chance, 0.0f, FMath::Clamp(Config.MaxChance, 0.0f, 1.0f));
}

inline bool ShouldSpawnAvenger(bool bNotableDeath, float KillerNotoriety, double TimeSinceLast, int32 LiveAvengers,
                               float Roll01, const FMythicAvengerConfig &Config) {
    if (!bNotableDeath) {
        return false;
    }
    if (TimeSinceLast < Config.CooldownSeconds) {
        return false;
    }
    if (LiveAvengers >= FMath::Max(1, Config.MaxSimultaneousAvengers)) {
        return false;
    }
    return MythicCombat::RollSucceeds(AvengerChance(KillerNotoriety, Config), Roll01);
}
}
