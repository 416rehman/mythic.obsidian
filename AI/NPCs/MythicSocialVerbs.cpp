// Mythic Living World — Social Interaction Verbs (pure reaction mapping)

#include "MythicSocialVerbs.h"
#include "Mass/Fragments/MythicMassFragments.h" // FMythicPersonalityFragment + EMythicVentChannel

#define LOCTEXT_NAMESPACE "MythicSocial"

// ─────────────────────────────────────────────────────────────
// Tunable thresholds (LOCKED v1). File-scope so the mapping is one source of truth and tweakable without a header
// recompile. Magnitudes are POSITIVE constants; ResolveReaction applies the sign (negative for hostile verbs).
// Anger-threshold ordering is EXPLICIT: Bully (0.45) < Threaten (0.50) < Provoke (0.55) — Bully angers an NPC at a
// LOWER Fight weight (it is the more humiliating act), Provoke requires the most aggressive personality to bite.
// ─────────────────────────────────────────────────────────────
namespace {
    constexpr float ComplimentGain = 5.0f;  // standing gained by a well-received compliment
    constexpr float ProvokeDelta = 10.0f;   // |standing| lost from a provoke
    constexpr float BullyDelta = 20.0f;     // |standing| lost from a bully (worst reputation hit)
    constexpr float ThreatenDelta = 15.0f;  // |standing| lost from a threat

    constexpr float ProvokeAngerThreshold = 0.55f;  // Fight weight at/above which Provoke angers
    constexpr float BullyAngerThreshold = 0.45f;    // Bully angers easier (lower bar)
    constexpr float ThreatenAngerThreshold = 0.50f; // Threaten sits between the two

    constexpr float CowerThreshold = 0.55f; // (Flee or Submit) weight at/above which the NPC is Intimidated
    constexpr float GuardThreshold = 0.50f; // Enforce weight at/above which a hostile verb escalates to CallGuards

    // Safe read of a vent channel weight (guards against any future COUNT drift).
    FORCEINLINE float Vent(const FMythicPersonalityFragment &P, EMythicVentChannel Ch) {
        const int32 Idx = static_cast<int32>(Ch);
        return (Idx >= 0 && Idx < static_cast<int32>(EMythicVentChannel::COUNT)) ? P.VentWeights[Idx] : 0.0f;
    }
} // namespace

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

    // The NPC cowers when it is much more a fleer/submitter than a fighter.
    const bool bCowardly = (FMath::Max(Flee, Submit) >= CowerThreshold) && (Fight < CowerThreshold);
    // The NPC calls for help when it is an enforcer (and not itself a brawler that would rather swing).
    const bool bEnforcer = (Enforce >= GuardThreshold) && (Enforce >= Fight);

    switch (V) {
    case EMythicSocialVerb::Greet:
    case EMythicSocialVerb::Compliment: {
        // Friendly verbs read the standing band. Below Hostile → Cold (rebuffed, no reward). Above Friendly → Warm.
        // In between → Warm for a compliment (it actively flatters), Neutral for a bare greeting.
        if (Standing <= HostileThreshold) {
            Out.Reaction = EMythicSocialReaction::Cold;
            Out.StandingDelta = 0.0f; // a hostile NPC isn't won over by small talk
        }
        else if (Standing >= FriendlyThreshold) {
            Out.Reaction = EMythicSocialReaction::Warm;
            Out.StandingDelta = (V == EMythicSocialVerb::Compliment) ? ComplimentGain : 0.0f;
        }
        else {
            Out.Reaction = (V == EMythicSocialVerb::Compliment) ? EMythicSocialReaction::Warm
                                                                : EMythicSocialReaction::Neutral;
            // A compliment only earns standing when it lands (not Cold) — modeled above; Greet never moves standing.
            Out.StandingDelta = (V == EMythicSocialVerb::Compliment) ? ComplimentGain : 0.0f;
        }
        break;
    }

    case EMythicSocialVerb::Provoke:
    case EMythicSocialVerb::Bully:
    case EMythicSocialVerb::Threaten: {
        // Hostile verbs always cost standing (the player was rude); magnitude is per-verb, sign is negative.
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

        // Reaction precedence: an aggressive NPC bites (Angered); else an enforcer alerts guards (CallGuards); else a
        // coward backs down (Intimidated); else it merely sulks (Cold). Angered/CallGuards both stem from a refusal
        // to be pushed around, but only the brawler turns it into a personal fight.
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
    // HONEST ART BOUNDARY: placeholder English. Real lines come from authored bark/dialogue tables flavored by the
    // NPC's personality + role. Switch on reaction first (it carries the most signal), fall back per verb.
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
