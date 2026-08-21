#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MythicCampfireComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicOnCampfireLitChanged, bool, bLit);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicCampfireComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicCampfireComponent();

    /** Fuel (burn seconds) a freshly-deployed campfire starts with. 0 = deploys dead (must be fed to light). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campfire", meta = (ClampMin = "0.0"))
    float InitialFuelSeconds = 600.0f;

    /** Max banked burn seconds (AddFuel clamps to this remaining). <= 0 = uncapped. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campfire", meta = (ClampMin = "0.0"))
    float MaxFuelSeconds = 3600.0f;

    /** Burn seconds granted per fuel unit (the ServerAddFuelUnit convenience — one log = this much fire). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Campfire", meta = (ClampMin = "0.0"))
    float FuelSecondsPerUnit = 300.0f;

    /** Fired on lit/unlit flips (server + clients): content binds flames/embers VFX, cook-UI gating, light toggles. */
    UPROPERTY(BlueprintAssignable, Category = "Campfire")
    FMythicOnCampfireLitChanged OnLitChanged;

    // ── Queries ──
    UFUNCTION(BlueprintPure, Category = "Campfire")
    bool IsLit() const { return bLit; }

    /** Seconds of burn left (server-exact; clients see 0 — drive client UI off IsLit/OnLitChanged instead). */
    UFUNCTION(BlueprintPure, Category = "Campfire")
    float GetRemainingBurnSeconds() const;

    // ── Server verbs (content's interactable calls these from its server-side interact handler) ──
    /** Add burn seconds (relights a dead fire). No-op off authority or for <= 0 seconds. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Campfire")
    void ServerAddFuel(float Seconds);

    /** Add one fuel unit (FuelSecondsPerUnit) — the standard "feed the fire" interact verb. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Campfire")
    void ServerAddFuelUnit() { ServerAddFuel(FuelSecondsPerUnit); }

    void SerializeFuelState(TArray<uint8> &OutData) const;
    void RestoreFuelState(const TArray<uint8> &InData);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void OnRep_Lit();

    UPROPERTY(ReplicatedUsing = OnRep_Lit)
    bool bLit = false;

private:
    void ApplyDeadline(double NewDeadline);
    void HandleBurnOut();
    void SetLit(bool bNewLit);
    void SyncWarmthAura();
    double WorldNow() const;

    double BurnDeadline = 0.0;
    FTimerHandle BurnOutTimer;
};
