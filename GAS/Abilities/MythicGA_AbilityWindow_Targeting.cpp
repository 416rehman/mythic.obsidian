#include "MythicGA_AbilityWindow.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

bool UMythicGA_AbilityWindow::ResolveHostileIntent(float Range) {
    AActor* Avatar=WindowAvatar.Get();
    if (!Avatar || !CurrentActorInfo || !CurrentActorInfo->IsNetAuthority()) return false;
    FVector Eye; FRotator Rotation; Avatar->GetActorEyesViewPoint(Eye,Rotation);
    const FVector Direction=GetAimDirection().GetSafeNormal();
    if (Eye.ContainsNaN() || Direction.ContainsNaN() || Direction.IsNearlyZero()) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(AbilityHostileIntent),false,Avatar);
    FHitResult RayHit;
    const bool bHit=GetWorld()->LineTraceSingleByChannel(RayHit,Eye,Eye+Direction*Range,ECC_Visibility,Params);
    const auto Eligible=[&](AActor* Target) {
        return IsEligibleAbilityTarget(Target,false,false,false)
            && FVector::DistSquared(Avatar->GetActorLocation(),Target->GetActorLocation())<=FMath::Square(Range);
    };
    if (bHit && Eligible(RayHit.GetActor()) && CheckTargetIncarnation(RayHit.GetActor())) {
        IntentTarget=RayHit.GetActor(); return true;
    }
    if (!FMath::IsFinite(HostileAimAssistDegrees) || HostileAimAssistDegrees<=0) return false;
    const double MaxDepth=bHit ? FVector::DotProduct(RayHit.ImpactPoint-Eye,Direction) : Range;
    const double Cosine=FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(HostileAimAssistDegrees,0.0f,12.0f)));
    TArray<FOverlapResult> Candidates;
    GetWorld()->OverlapMultiByObjectType(Candidates,Avatar->GetActorLocation(),FQuat::Identity,
        FCollisionObjectQueryParams(ECC_Pawn),FCollisionShape::MakeSphere(Range),Params);
    AActor* Best=nullptr; double BestOffset=DBL_MAX,BestDepth=DBL_MAX;
    for (const auto& Candidate:Candidates) {
        auto* Target=Candidate.GetActor();
        if (Target==Best || !Eligible(Target)) continue;
        const FVector Point=Target->GetTargetLocation(Avatar);
        if (Point.ContainsNaN()) continue;
        const FVector Delta=Point-Eye;
        const double Depth=FVector::DotProduct(Delta,Direction);
        if (Depth<=0 || Depth>MaxDepth || FVector::DotProduct(Delta.GetSafeNormal(),Direction)<Cosine) continue;
        const double Offset=(Delta-Direction*Depth).SizeSquared();
        if (Offset>BestOffset || (Offset==BestOffset && (Depth>BestDepth
            || (Depth==BestDepth && Best && !Target->GetFName().LexicalLess(Best->GetFName()))))) continue;
        FHitResult Sight;
        if (GetWorld()->LineTraceSingleByChannel(Sight,Eye,Point,ECC_Visibility,Params) && Sight.GetActor()!=Target) continue;
        Best=Target; BestOffset=Offset; BestDepth=Depth;
    }
    if (!Best || !CheckTargetIncarnation(Best)) return false;
    IntentTarget=Best; return true;
}
