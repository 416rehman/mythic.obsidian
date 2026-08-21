
#include "MythicSocialVerbs.h"
#include "Mass/Fragments/MythicMassFragments.h"

#define LOCTEXT_NAMESPACE "MythicSocial"

namespace {
    constexpr float ComplimentGain = 5.0f;
    constexpr float ProvokeDelta = 10.0f;
    constexpr float BullyDelta = 20.0f;
    constexpr float ThreatenDelta = 15.0f;

    constexpr float ProvokeAngerThreshold = 0.55f;
    constexpr float BullyAngerThreshold = 0.45f;
    constexpr float ThreatenAngerThreshold = 0.50f;

    constexpr float CowerThreshold = 0.55f;
    constexpr float GuardThreshold = 0.50f;

    FORCEINLINE float Vent(const FMythicPersonalityFragment &P, EMythicVentChannel Ch) {
        const int32 Idx = static_cast<int32>(Ch);
        return (Idx >= 0 && Idx < static_cast<int32>(EMythicVentChannel::COUNT)) ? P.VentWeights[Idx] : 0.0f;
    }
}

FMythicSocialReactionResult UMythicSocialVerbLibrary::ResolveReaction(
    EMythicSocialVerb V,
    const FMythicPersonalityFragment &P,
    float Standing,
    float HostileThreshold,
    float FriendlyThreshold) {
    FMythicSocialReactionResult Out;

    const float Fight = Vent(P, EMythicVentChannel::Fight);
    const float Flee = Vent(P, EMythicVentChannel::Flee);
    const float Submit = Vent(P, EMythicVentChannel::Submit);
    const float Enforce = Vent(P, EMythicVentChannel::Enforce);

    const bool bCowardly = (FMath::Max(Flee, Submit) >= CowerThreshold) && (Fight < CowerThreshold);
    const bool bEnforcer = (Enforce >= GuardThreshold) && (Enforce >= Fight);

    switch (V) {
    case EMythicSocialVerb::Greet:
    case EMythicSocialVerb::Compliment: {
        if (Standing <= HostileThreshold) {
            Out.Reaction = EMythicSocialReaction::Cold;
            Out.StandingDelta = 0.0f;
        }
        else if (Standing >= FriendlyThreshold) {
            Out.Reaction = EMythicSocialReaction::Warm;
            Out.StandingDelta = (V == EMythicSocialVerb::Compliment) ? ComplimentGain : 0.0f;
        }
        else {
            Out.Reaction = (V == EMythicSocialVerb::Compliment) ? EMythicSocialReaction::Warm
                                                                : EMythicSocialReaction::Neutral;
            Out.StandingDelta = (V == EMythicSocialVerb::Compliment) ? ComplimentGain : 0.0f;
        }
        break;
    }

    case EMythicSocialVerb::Provoke:
    case EMythicSocialVerb::Bully:
    case EMythicSocialVerb::Threaten: {
        float Magnitude = ProvokeDelta;
        float AngerThreshold = ProvokeAngerThreshold;
        if (V == EMythicSocialVerb::Bully) {
            Magnitude = BullyDelta;
            AngerThreshold = BullyAngerThreshold;
        }
        else if (V == EMythicSocialVerb::Threaten) {
            Magnitude = ThreatenDelta;
            AngerThreshold = ThreatenAngerThreshold;
        }
        Out.StandingDelta = -Magnitude;

        if (Fight >= AngerThreshold) {
            Out.Reaction = EMythicSocialReaction::Angered;
            Out.bSetHostile = true;
        }
        else if (bEnforcer) {
            Out.Reaction = EMythicSocialReaction::CallGuards;
            Out.bAlertGuards = true;
        }
        else if (bCowardly) {
            Out.Reaction = EMythicSocialReaction::Intimidated;
        }
        else {
            Out.Reaction = EMythicSocialReaction::Cold;
        }
        break;
    }

    default:
        Out.Reaction = EMythicSocialReaction::Neutral;
        break;
    }

    return Out;
}

FText UMythicSocialVerbLibrary::DefaultBarkFor(EMythicSocialVerb V, EMythicSocialReaction Reaction) {
    switch (Reaction) {
    case EMythicSocialReaction::Warm:
        return LOCTEXT("Bark_Warm", "Well met, friend.");
    case EMythicSocialReaction::Cold:
        return LOCTEXT("Bark_Cold", "I've nothing to say to you.");
    case EMythicSocialReaction::Intimidated:
        return LOCTEXT("Bark_Intimidated", "P-please, I don't want any trouble!");
    case EMythicSocialReaction::Angered:
        return LOCTEXT("Bark_Angered", "You'll regret that!");
    case EMythicSocialReaction::CallGuards:
        return LOCTEXT("Bark_CallGuards", "Guards! Guards, over here!");
    case EMythicSocialReaction::Neutral:
    default:
        switch (V) {
        case EMythicSocialVerb::Greet:
            return LOCTEXT("Bark_Greet", "Hello there.");
        case EMythicSocialVerb::Compliment:
            return LOCTEXT("Bark_Compliment", "That's kind of you to say.");
        default:
            return LOCTEXT("Bark_NeutralHostile", "Watch yourself.");
        }
    }
}

#undef LOCTEXT_NAMESPACE
