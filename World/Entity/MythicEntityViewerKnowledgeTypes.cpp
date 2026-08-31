#include "World/Entity/MythicEntityViewerKnowledgeTypes.h"

#include "World/Entity/MythicEntityKnowledgeTags.h"

namespace MythicEntityKnowledgePrivate {

FGameplayTagContainer SanitizeFacet(const FGameplayTagContainer &Source,
                                    const FGameplayTag Root) {
    TArray<FGameplayTag> ValidTags;
    Source.GetGameplayTagArray(ValidTags);
    ValidTags.RemoveAll([Root](const FGameplayTag Tag) {
        return !Tag.IsValid() || Tag.MatchesTagExact(Root)
               || !Tag.MatchesTag(Root);
    });
    ValidTags.Sort([](const FGameplayTag Left, const FGameplayTag Right) {
        return Left.GetTagName().LexicalLess(Right.GetTagName());
    });
    ValidTags.SetNum(
        FMath::Min(ValidTags.Num(),
                   FMythicEntityKnowledgeRules::MaxTagsPerKnowledgeFacet),
        EAllowShrinking::No);

    FGameplayTagContainer Result;
    for (const FGameplayTag Tag : ValidTags) {
        Result.AddTag(Tag);
    }
    return Result;
}

void AppendUniqueTags(FGameplayTagContainer &InOut,
                      const FGameplayTagContainer &Delta) {
    TArray<FGameplayTag> DeltaTags;
    Delta.GetGameplayTagArray(DeltaTags);
    for (const FGameplayTag Tag : DeltaTags) {
        InOut.AddTag(Tag);
    }
}

bool EqualKnowledge(const FMythicEntityKnowledgeView &Left,
                    const FMythicEntityKnowledgeView &Right) {
    return Left.bNameKnown == Right.bNameKnown
           && Left.RecognizedName.EqualTo(Right.RecognizedName)
           && Left.bFactionKnown == Right.bFactionKnown
           && Left.KnownFactionTag == Right.KnownFactionTag
           && Left.bRoleKnown == Right.bRoleKnown
           && Left.KnownRoleTag == Right.KnownRoleTag
           && Left.RelationshipBand == Right.RelationshipBand
           && Left.StandingBand == Right.StandingBand
           && Left.DiscoveredTraits == Right.DiscoveredTraits
           && Left.DiscoveredHistory == Right.DiscoveredHistory
           && Left.KnownLikes == Right.KnownLikes
           && Left.KnownDislikes == Right.KnownDislikes
           && Left.DiscoveredConnections == Right.DiscoveredConnections;
}

} // namespace MythicEntityKnowledgePrivate

FMythicEntityKnowledgeView FMythicEntityKnowledgeRules::Sanitize(
    const FMythicEntityKnowledgeView &Source) {
    using namespace MythicEntityKnowledgePrivate;

    FMythicEntityKnowledgeView Result = Source;
    Result.bRecognitionGranted = false;

    const FString NameString = Result.RecognizedName.ToString();
    Result.bNameKnown = Result.bNameKnown && !Result.RecognizedName.IsEmpty()
                        && NameString.Len() <= MaxRecognizedNameCharacters;
    if (!Result.bNameKnown) {
        Result.RecognizedName = FText::GetEmpty();
    }

    Result.bFactionKnown = Result.bFactionKnown && Result.KnownFactionTag.IsValid();
    if (!Result.bFactionKnown) {
        Result.KnownFactionTag = FGameplayTag();
    }

    Result.bRoleKnown = Result.bRoleKnown && Result.KnownRoleTag.IsValid();
    if (!Result.bRoleKnown) {
        Result.KnownRoleTag = FGameplayTag();
    }

    if (Result.RelationshipBand < EMythicKnownRelationshipBand::Unknown
        || Result.RelationshipBand > EMythicKnownRelationshipBand::Trusted) {
        Result.RelationshipBand = EMythicKnownRelationshipBand::Unknown;
    }
    if (Result.StandingBand < EMythicKnownStandingBand::Unknown
        || Result.StandingBand > EMythicKnownStandingBand::Honored) {
        Result.StandingBand = EMythicKnownStandingBand::Unknown;
    }

    Result.DiscoveredTraits =
        SanitizeFacet(Result.DiscoveredTraits, ENTITY_KNOWLEDGE_TRAIT_ROOT);
    Result.DiscoveredHistory =
        SanitizeFacet(Result.DiscoveredHistory, ENTITY_KNOWLEDGE_HISTORY_ROOT);
    Result.KnownLikes =
        SanitizeFacet(Result.KnownLikes, ENTITY_KNOWLEDGE_LIKES_ROOT);
    Result.KnownDislikes =
        SanitizeFacet(Result.KnownDislikes, ENTITY_KNOWLEDGE_DISLIKES_ROOT);
    Result.DiscoveredConnections = SanitizeFacet(
        Result.DiscoveredConnections, ENTITY_KNOWLEDGE_CONNECTION_ROOT);
    return Result;
}

bool FMythicEntityKnowledgeRules::MergeLearnedDelta(
    FMythicEntityKnowledgeView &InOutKnowledge,
    const FMythicEntityKnowledgeView &LearnedDelta) {
    using namespace MythicEntityKnowledgePrivate;

    const FMythicEntityKnowledgeView Before = Sanitize(InOutKnowledge);
    const FMythicEntityKnowledgeView Delta = Sanitize(LearnedDelta);
    FMythicEntityKnowledgeView Merged = Before;

    if (Delta.bNameKnown) {
        Merged.bNameKnown = true;
        Merged.RecognizedName = Delta.RecognizedName;
    }
    if (Delta.bFactionKnown) {
        Merged.bFactionKnown = true;
        Merged.KnownFactionTag = Delta.KnownFactionTag;
    }
    if (Delta.bRoleKnown) {
        Merged.bRoleKnown = true;
        Merged.KnownRoleTag = Delta.KnownRoleTag;
    }
    if (Delta.RelationshipBand != EMythicKnownRelationshipBand::Unknown) {
        Merged.RelationshipBand = Delta.RelationshipBand;
    }
    if (Delta.StandingBand != EMythicKnownStandingBand::Unknown) {
        Merged.StandingBand = Delta.StandingBand;
    }

    AppendUniqueTags(Merged.DiscoveredTraits, Delta.DiscoveredTraits);
    AppendUniqueTags(Merged.DiscoveredHistory, Delta.DiscoveredHistory);
    AppendUniqueTags(Merged.KnownLikes, Delta.KnownLikes);
    AppendUniqueTags(Merged.KnownDislikes, Delta.KnownDislikes);
    AppendUniqueTags(Merged.DiscoveredConnections,
                     Delta.DiscoveredConnections);
    Merged = Sanitize(Merged);

    const bool bChanged = !EqualKnowledge(Before, Merged);
    InOutKnowledge = MoveTemp(Merged);
    return bChanged;
}

int32 FMythicEntityKnowledgeRules::FindDossierIndex(
    const TConstArrayView<FMythicEntityLearnedDossier> Dossiers,
    const FMythicEntityId &EntityId) {
    if (!EntityId.IsValid()) {
        return INDEX_NONE;
    }
    for (int32 Index = 0; Index < Dossiers.Num(); ++Index) {
        if (Dossiers[Index].EntityId == EntityId) {
            return Index;
        }
    }
    return INDEX_NONE;
}
