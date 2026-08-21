
#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityHandle.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Morality/MoralSignature.h"


struct FMythicCrimeRecord {
    int32 PerpEntityIndex = INDEX_NONE;

    int32 VictimEntityIndex = INDEX_NONE;

    FMythicFactionId PerpFaction;

    FString PerpPlayerKey;

    FMythicFactionId ViolatedFaction;

    EMythicMoralSeverity Severity = EMythicMoralSeverity::Condemn;

    FMythicMoralAction ActionMoralVector;

    FMythicCellCoord Cell;

    double WorldTime = 0.0;

    uint16 DirectWitnessCount = 0;

    uint8 PropagationHops = 0;

    float Confidence = 1.0f;

    bool bPropagated = false;
};


struct FMythicCrimeReportQueue {
    TArray<FMythicCrimeRecord> PendingReports;

    static constexpr int32 MaxQueuedReports = 64;

    void Enqueue(const FMythicCrimeRecord& Record) {
        if (PendingReports.Num() >= MaxQueuedReports) {
            PendingReports.RemoveAt(0, 1, EAllowShrinking::No);
        }
        PendingReports.Add(Record);
    }

    int32 Num() const { return PendingReports.Num(); }
    bool IsEmpty() const { return PendingReports.Num() == 0; }

    void FlushPropagated() {
        PendingReports.RemoveAll([](const FMythicCrimeRecord& R) { return R.bPropagated; });
    }
};
