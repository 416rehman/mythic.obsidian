
#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Components/RichTextBlockImageDecorator.h"
#include "Engine/Blueprint.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "GameplayCueNotify_Burst.h"
#include "GameplayCueNotifyTypes.h"
#include "GameplayTagsManager.h"
#include "Internationalization/Regex.h"
#include "Sound/SoundBase.h"
#include "UObject/UnrealType.h"

#include "GAS/Abilities/MythicGA_Rune.h"
#include "GAS/MythicTags_GAS.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Progression/MythicAchievementDefinition.h"
#include "Progression/MythicAchievementSet.h"
#include "Progression/Runes/MythicRuneDefinition.h"
#include "Settings/MythicDeveloperSettings.h"
#include "UI/MythicDamageNumberSubsystem.h"

namespace {

const TCHAR *RuneContent_DefinitionRoot = TEXT("/Game/Mythic/Progression/Runes");
const TCHAR *RuneContent_AbilityRoot = TEXT("/Game/Mythic/Progression/Runes/Abilities");
const TCHAR *RuneContent_CueRoot = TEXT("/Game/Mythic/GAS/Cues");
const TCHAR *RuneContent_ImageTablePath = TEXT("/Game/Mythic/UI/DT_ImageRow.DT_ImageRow");

// The set is fourteen runes with one ability each. A count off by one is a rune authored and never wired, or one
// left behind by the definition it replaced.
constexpr int32 RuneContent_ExpectedRunes = 14;
constexpr int32 RuneContent_ExpectedCueTags = 17;

bool RuneContent_HasImageRow(const UDataTable *ImageTable, const FName RowName) {
    return ImageTable && ImageTable->FindRow<FRichImageRow>(RowName, TEXT("Mythic rune content test"), false) != nullptr;
}

FString RuneContent_TagLeaf(const FGameplayTag &Tag) {
    const FString Full = Tag.ToString();
    int32 Dot = INDEX_NONE;
    return Full.FindLastChar(TEXT('.'), Dot) ? Full.RightChop(Dot + 1) : Full;
}

// BurstEffects and its BurstSounds array are both protected on the engine types; the structs are public, so
// reflection hands them back.
int32 RuneContent_BurstSoundCount(const UClass *CueClass) {
    const FStructProperty *Property = CueClass ? FindFProperty<FStructProperty>(CueClass, TEXT("BurstEffects")) : nullptr;
    if (!Property || Property->Struct != FGameplayCueNotify_BurstEffects::StaticStruct()) {
        return INDEX_NONE;
    }
    const FArrayProperty *SoundsProperty =
        FindFProperty<FArrayProperty>(FGameplayCueNotify_BurstEffects::StaticStruct(), TEXT("BurstSounds"));
    if (!SoundsProperty) {
        return INDEX_NONE;
    }
    void *Effects = Property->ContainerPtrToValuePtr<void>(CueClass->GetDefaultObject());
    FScriptArrayHelper Sounds(SoundsProperty, SoundsProperty->ContainerPtrToValuePtr<void>(Effects));
    int32 Count = 0;
    for (int32 Index = 0; Index < Sounds.Num(); Index++) {
        Count += reinterpret_cast<const FGameplayCueNotify_SoundInfo *>(Sounds.GetRawPtr(Index))->Sound ? 1 : 0;
    }
    return Count;
}

bool RuneContent_CheckUiTexture(FAutomationTestBase &Test, const FString &Name, const TCHAR *Role,
                                const TSoftObjectPtr<UTexture2D> &Soft) {
    if (!Test.TestFalse(*FString::Printf(TEXT("%s has a %s"), *Name, Role), Soft.IsNull())) {
        return false;
    }
    UTexture2D *Texture = Soft.LoadSynchronous();
    if (!Test.TestNotNull(*FString::Printf(TEXT("%s %s resolves to a texture"), *Name, Role), Texture)) {
        return false;
    }
    Test.TestTrue(*FString::Printf(TEXT("%s %s is in the UI texture group"), *Name, Role),
                  Texture->LODGroup == TEXTUREGROUP_UI);
    // A wrapping address on a round icon bleeds the far edge into the near one at the bezel rim.
    Test.TestTrue(*FString::Printf(TEXT("%s %s clamps"), *Name, Role),
                  Texture->AddressX == TA_Clamp && Texture->AddressY == TA_Clamp);
    return true;
}

}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRuneContentTest,
    "Mythic.Content.Runes",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRuneContentTest::RunTest(const FString &Parameters) {
    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry &Registry = Module.Get();
    Registry.SearchAllAssets(true);

    TArray<FAssetData> RuneAssets;
    Registry.GetAssetsByClass(UMythicRuneDefinition::StaticClass()->GetClassPathName(), RuneAssets);
    RuneAssets.Sort([](const FAssetData &A, const FAssetData &B) { return A.AssetName.LexicalLess(B.AssetName); });
    if (!TestTrue(TEXT("the project has runes to check - an empty scan would pass for the wrong reason"),
                  RuneAssets.Num() > 0)) {
        return false;
    }

    // Every Blueprint in the ability folder that derives from the rune base. A definition may only grant one of
    // these, and each of these must be granted by exactly one definition.
    TArray<FAssetData> AbilityAssets;
    Registry.GetAssetsByPath(FName(RuneContent_AbilityRoot), AbilityAssets, false);
    TSet<UClass *> RuneAbilityClasses;
    for (const FAssetData &Asset : AbilityAssets) {
        const UClass *AssetClass = Asset.GetClass();
        if (!AssetClass || !AssetClass->IsChildOf(UBlueprint::StaticClass())) {
            continue;
        }
        const UBlueprint *Blueprint = Cast<UBlueprint>(Asset.GetAsset());
        UClass *Generated = Blueprint ? Blueprint->GeneratedClass.Get() : nullptr;
        if (TestTrue(*FString::Printf(TEXT("%s is a UMythicGA_Rune Blueprint"), *Asset.AssetName.ToString()),
                     Generated && Generated->IsChildOf(UMythicGA_Rune::StaticClass()))) {
            RuneAbilityClasses.Add(Generated);
        }
    }

    // The deeds a player can actually complete. A rune keyed on any other tag is a rune nobody ever earns.
    TSet<FGameplayTag> Deeds;
    const UMythicAchievementSet *DeedSet = GetDefault<UMythicDeveloperSettings>()->DefaultAchievementSet.LoadSynchronous();
    if (TestNotNull(TEXT("UMythicDeveloperSettings::DefaultAchievementSet is set"), DeedSet)) {
        for (const UMythicAchievementDefinition *Deed : DeedSet->Achievements) {
            if (Deed && Deed->AchievementTag.IsValid()) {
                Deeds.Add(Deed->AchievementTag);
            }
        }
    }
    if (!TestTrue(TEXT("the achievement set has deeds to earn runes with"), Deeds.Num() > 0)) {
        return false;
    }

    const UGameplayTagsManager &Tags = UGameplayTagsManager::Get();
    const FGameplayTag ParamRoot = FGameplayTag::RequestGameplayTag(TEXT("Rune.Param"), false);
    TestTrue(TEXT("Rune.Param is registered"), ParamRoot.IsValid());

    // A description carries behaviour alone. The picker already colour-codes the category, so a category word
    // leading the line is a label the player reads twice.
    TArray<FString> CategoryWords;
    for (const FGameplayTag &Category :
         Tags.RequestGameplayTagChildren(FGameplayTag::RequestGameplayTag(TEXT("Rune.Category"), false))) {
        CategoryWords.Add(RuneContent_TagLeaf(Category));
    }
    TestTrue(TEXT("Rune.Category has child tags to screen descriptions against"), CategoryWords.Num() > 0);

    const UDataTable *ImageTable = LoadObject<UDataTable>(nullptr, RuneContent_ImageTablePath);
    TestNotNull(TEXT("DT_ImageRow loads"), ImageTable);
    if (ImageTable) {
        TestTrue(TEXT("DT_ImageRow rows are FRichImageRow"), ImageTable->GetRowStruct() == FRichImageRow::StaticStruct());
    }

    const FRegexPattern InlineImage(TEXT("<img id=\"([^\"]+)\""));
    const FRegexPattern Placeholder(TEXT("<#([^>]+)>"));

    int32 InFolder = 0;
    int32 ParameterCount = 0;
    int32 PlaceholderCount = 0;
    int32 ImageCount = 0;
    TMap<UClass *, FString> ClaimedAbilities;

    for (const FAssetData &Asset : RuneAssets) {
        const FString Name = Asset.AssetName.ToString();
        const UMythicRuneDefinition *Rune = Cast<UMythicRuneDefinition>(Asset.GetAsset());
        if (!TestNotNull(*FString::Printf(TEXT("%s loads"), *Name), Rune)) {
            continue;
        }

        const bool bInFolder = Asset.PackagePath.ToString().StartsWith(RuneContent_DefinitionRoot);
        TestTrue(*FString::Printf(TEXT("%s lives under %s"), *Name, RuneContent_DefinitionRoot), bInFolder);
        InFolder += bInFolder ? 1 : 0;

        TestFalse(*FString::Printf(TEXT("%s is named"), *Name), Rune->Name.IsEmpty());
        TestFalse(*FString::Printf(TEXT("%s says what it does"), *Name), Rune->Description.IsEmpty());
        TestFalse(*FString::Printf(TEXT("%s tells the player how to earn it"), *Name), Rune->Hint.IsEmpty());
        TestTrue(*FString::Printf(TEXT("%s carries a category"), *Name), Rune->CategoryTags.Num() > 0);

        TestTrue(*FString::Printf(TEXT("%s names a deed to earn it"), *Name), Rune->RequiredTag.IsValid());
        TestTrue(*FString::Printf(TEXT("%s deed %s is one the achievement set awards"), *Name,
                                  *Rune->RequiredTag.ToString()),
                 Deeds.Contains(Rune->RequiredTag));

        RuneContent_CheckUiTexture(*this, Name, TEXT("icon"), Rune->Icon);
        if (!Rune->HudIcon.IsNull() && Rune->HudIcon.ToSoftObjectPath() != Rune->Icon.ToSoftObjectPath()) {
            RuneContent_CheckUiTexture(*this, Name, TEXT("HUD icon"), Rune->HudIcon);
        }

        UClass *Ability = Rune->Ability;
        const bool bRuneAbility = Ability && Ability->IsChildOf(UMythicGA_Rune::StaticClass());
        TestTrue(*FString::Printf(TEXT("%s ability %s is a UMythicGA_Rune subclass"), *Name,
                                  Ability ? *Ability->GetName() : TEXT("(none)")),
                 bRuneAbility);
        TestTrue(*FString::Printf(TEXT("%s grants an ability"), *Name), Rune->HasPayload());
        if (bRuneAbility) {
            TestTrue(*FString::Printf(TEXT("%s ability %s is authored under %s"), *Name, *Ability->GetName(),
                                      RuneContent_AbilityRoot),
                     RuneAbilityClasses.Contains(Ability));
            if (const FString *Other = ClaimedAbilities.Find(Ability)) {
                AddError(FString::Printf(TEXT("%s and %s grant the same ability %s"), *Name, **Other,
                                         *Ability->GetName()));
            }
            else {
                ClaimedAbilities.Add(Ability, Name);
            }
        }

        const FString Description = Rune->Description.ToString();
        TestFalse(*FString::Printf(TEXT("%s description carries a Rune.Category label"), *Name),
                  Description.Contains(TEXT("Rune.Category")));
        for (const FString &Word : CategoryWords) {
            const bool bStrapline = Description.StartsWith(Word + TEXT(" -"), ESearchCase::IgnoreCase)
                || Description.StartsWith(Word + TEXT(":"), ESearchCase::IgnoreCase);
            TestFalse(*FString::Printf(TEXT("%s description opens with the category strapline '%s'"), *Name, *Word),
                      bStrapline);
        }
        // No rune gates on weather, time or season, so a description naming one promises a rule the ability
        // never reads.
        TestFalse(*FString::Printf(TEXT("%s description references an Environment tag"), *Name),
                  Description.Contains(TEXT("Environment.")));

        // Every inline icon the description names has to be a row the decorator can draw, or it renders as text.
        FRegexMatcher Images(InlineImage, Description);
        while (Images.FindNext()) {
            ++ImageCount;
            const FName RowName(*Images.GetCaptureGroup(1));
            TestTrue(*FString::Printf(TEXT("%s description icon '%s' is a DT_ImageRow row"), *Name, *RowName.ToString()),
                     RuneContent_HasImageRow(ImageTable, RowName));
        }

        for (const TPair<FGameplayTag, FRollDefinition> &Pair : Rune->Parameters) {
            ++ParameterCount;
            const FString Param = Pair.Key.ToString();
            TestTrue(*FString::Printf(TEXT("%s parameter %s is a Rune.Param tag"), *Name, *Param),
                     Pair.Key.IsValid() && Pair.Key.MatchesTag(ParamRoot));
            TestTrue(*FString::Printf(TEXT("%s parameter %s has min <= max (%g > %g)"), *Name, *Param, Pair.Value.Min,
                                      Pair.Value.Max),
                     Pair.Value.Min <= Pair.Value.Max);
        }

        // A placeholder with no parameter behind it reaches the player as its raw tag string.
        FRegexMatcher Placeholders(Placeholder, Description);
        while (Placeholders.FindNext()) {
            ++PlaceholderCount;
            const FString Capture = Placeholders.GetCaptureGroup(1);
            const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Capture), false);
            TestTrue(*FString::Printf(TEXT("%s placeholder <#%s> is a registered tag"), *Name, *Capture), Tag.IsValid());
            TestTrue(*FString::Printf(TEXT("%s placeholder <#%s> is one of its Parameters"), *Name, *Capture),
                     Tag.IsValid() && Rune->Parameters.Contains(Tag));
        }
    }

    for (UClass *Ability : RuneAbilityClasses) {
        if (!ClaimedAbilities.Contains(Ability)) {
            AddError(FString::Printf(TEXT("%s is a rune ability no definition grants"), *Ability->GetName()));
        }
    }

    // Every rune cue tag fires a burst with a sound; a silent cue is a moment the player only sees if they happen to
    // be looking.
    const FGameplayTagContainer CueTags =
        Tags.RequestGameplayTagChildren(FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Rune"), false));
    TestEqual(TEXT("GameplayCue.Rune carries the rune cue tags"), CueTags.Num(), RuneContent_ExpectedCueTags);
    int32 CueAssets = 0;
    int32 CueSounds = 0;
    for (const FGameplayTag &CueTag : CueTags) {
        const FString Leaf = RuneContent_TagLeaf(CueTag);
        const FString AssetPath = FString::Printf(TEXT("%s/GC_Rune_%s.GC_Rune_%s_C"), RuneContent_CueRoot, *Leaf, *Leaf);
        UClass *CueClass = LoadClass<UGameplayCueNotify_Burst>(nullptr, *AssetPath);
        if (!TestNotNull(*FString::Printf(TEXT("%s has a burst cue at %s"), *CueTag.ToString(), *AssetPath), CueClass)) {
            continue;
        }
        ++CueAssets;
        const UGameplayCueNotify_Burst *Cue = CueClass->GetDefaultObject<UGameplayCueNotify_Burst>();
        TestTrue(*FString::Printf(TEXT("GC_Rune_%s answers %s"), *Leaf, *CueTag.ToString()),
                 Cue && Cue->GameplayCueTag == CueTag);
        const int32 Sounds = RuneContent_BurstSoundCount(CueClass);
        TestTrue(*FString::Printf(TEXT("GC_Rune_%s has a sound in BurstSounds"), *Leaf), Sounds > 0);
        CueSounds += Sounds > 0 ? 1 : 0;
    }

    // A rune hit with no row of its own draws in the plain number colour and merges with ordinary hits.
    const UMythicDamageNumberConfig *Numbers = GetDefault<UMythicDeveloperSettings>()->DamageNumberConfig.LoadSynchronous();
    TestNotNull(TEXT("UMythicDeveloperSettings::DamageNumberConfig is set"), Numbers);
    const FGameplayTagContainer HitTags = Tags.RequestGameplayTagChildren(GAS_HIT_RUNE.GetTag());
    TestTrue(TEXT("GAS.Hit.Rune has children to style"), HitTags.Num() > 0);
    int32 Styled = 0;
    for (const FGameplayTag &HitTag : HitTags) {
        const bool bStyled = Numbers && Numbers->TaggedStyles.Contains(HitTag);
        Styled += bStyled ? 1 : 0;
        TestTrue(*FString::Printf(TEXT("%s has its own TaggedStyles row in %s"), *HitTag.ToString(),
                                  Numbers ? *Numbers->GetName() : TEXT("DamageNumberConfig")),
                 bStyled);
    }

    AddInfo(FString::Printf(TEXT("runes: %d scanned, %d under %s / abilities: %d authored, %d claimed / deeds: %d / "
                                 "parameters: %d, placeholders: %d, inline images: %d / cue tags: %d, cue assets: %d, "
                                 "with sounds: %d / hit tags: %d, styled: %d"),
                            RuneAssets.Num(), InFolder, RuneContent_DefinitionRoot, RuneAbilityClasses.Num(),
                            ClaimedAbilities.Num(), Deeds.Num(), ParameterCount, PlaceholderCount, ImageCount,
                            CueTags.Num(), CueAssets, CueSounds, HitTags.Num(), Styled));

    TestEqual(TEXT("the rune folder holds exactly the v2 set"), InFolder, RuneContent_ExpectedRunes);
    TestEqual(TEXT("the ability folder holds one rune ability per rune"), RuneAbilityClasses.Num(),
              RuneContent_ExpectedRunes);
    TestEqual(TEXT("every rune cue tag has a burst asset"), CueAssets, CueTags.Num());
    TestEqual(TEXT("every rune cue asset carries a sound"), CueSounds, CueTags.Num());
    TestEqual(TEXT("every rune hit tag is styled"), Styled, HitTags.Num());
    return true;
}
