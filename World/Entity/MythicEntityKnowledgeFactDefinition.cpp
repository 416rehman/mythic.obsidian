#include "World/Entity/MythicEntityKnowledgeFactDefinition.h"

#include "Misc/DataValidation.h"
#include "World/Entity/MythicEntityKnowledgeTags.h"

#define LOCTEXT_NAMESPACE "MythicEntityKnowledgeFactDefinition"

const FPrimaryAssetType UMythicEntityKnowledgeFactDefinition::PrimaryAssetType(
    TEXT("MythicEntityKnowledgeFact"));

FPrimaryAssetId UMythicEntityKnowledgeFactDefinition::GetPrimaryAssetId() const {
    return FactTag.IsValid()
        ? FPrimaryAssetId(PrimaryAssetType, FactTag.GetTagName())
        : FPrimaryAssetId();
}

#if WITH_EDITOR
EDataValidationResult UMythicEntityKnowledgeFactDefinition::IsDataValid(
    FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto AddError = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };

    FGameplayTag RequiredRoot;
    switch (Section) {
    case EMythicEntityKnowledgeFactSection::Trait:
        RequiredRoot = ENTITY_KNOWLEDGE_TRAIT_ROOT;
        break;
    case EMythicEntityKnowledgeFactSection::History:
        RequiredRoot = ENTITY_KNOWLEDGE_HISTORY_ROOT;
        break;
    case EMythicEntityKnowledgeFactSection::Like:
        RequiredRoot = ENTITY_KNOWLEDGE_LIKES_ROOT;
        break;
    case EMythicEntityKnowledgeFactSection::Dislike:
        RequiredRoot = ENTITY_KNOWLEDGE_DISLIKES_ROOT;
        break;
    case EMythicEntityKnowledgeFactSection::Connection:
        RequiredRoot = ENTITY_KNOWLEDGE_CONNECTION_ROOT;
        break;
    }

    if (!FactTag.IsValid() || !RequiredRoot.IsValid()
        || FactTag.MatchesTagExact(RequiredRoot)
        || !FactTag.MatchesTag(RequiredRoot)) {
        AddError(LOCTEXT(
            "InvalidFactTag",
            "Fact Tag must be a non-root Entity.Knowledge.* tag matching the selected Inspect section."));
    }
    if (DisplayName.IsEmpty()) {
        AddError(LOCTEXT(
            "MissingDisplayName",
            "Display Name must contain localized player-facing Inspect text."));
    }
    return Result == EDataValidationResult::Invalid
        ? Result : EDataValidationResult::Valid;
}
#endif

#undef LOCTEXT_NAMESPACE

