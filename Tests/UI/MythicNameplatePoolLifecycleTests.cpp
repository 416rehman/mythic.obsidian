#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "UI/Nameplate/MythicNameplateActionRailWidget.h"
#include "UI/Nameplate/MythicNameplateLayer.h"
#include "UI/Nameplate/MythicNameplateViewModel.h"
#include "UI/Nameplate/MythicNameplateWidget.h"
#include "UObject/UnrealType.h"
#include "World/Entity/MythicEntityId.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"

namespace {

class FScopedNameplatePoolWorld final {
public:
    FScopedNameplatePoolWorld() {
        InitializationValues = UWorld::InitializationValues()
                                   .CreatePhysicsScene(false)
                                   .ShouldSimulatePhysics(false)
                                   .EnableTraceCollision(false)
                                   .CreateNavigation(false)
                                   .CreateAISystem(false);
        World = UWorld::CreateWorld(
            EWorldType::Game, false,
            MakeUniqueObjectName(nullptr, UWorld::StaticClass(),
                                 TEXT("NameplatePoolLifecycleTest")),
            nullptr, true, ERHIFeatureLevel::Num, &InitializationValues,
            true);
        if (World) {
            World->InitWorld(InitializationValues);
        }
    }

    ~FScopedNameplatePoolWorld() {
        if (World) {
            World->DestroyWorld(false);
        }
    }

    UWorld *Get() const { return World; }

private:
    UWorld::InitializationValues InitializationValues;
    UWorld *World = nullptr;
};

FMythicEntityId MakeNameplatePoolEntityId(const uint32 Salt) {
    return FMythicEntityId::FromAuthorityGuid(
        EMythicEntityDomain::Runtime,
        FGuid(0x51000000u + Salt, 0x62000000u + Salt,
              0x73000000u + Salt, 0x84000000u + Salt));
}

} // namespace

struct FMythicNameplateLayerTestAccess {
    static void SeedPrewarmedRenderers(UMythicNameplateLayer &Layer) {
        Layer.PlateCanvas = NewObject<UCanvasPanel>(&Layer);

        UMythicNameplateWidget *Widget =
            NewObject<UMythicNameplateWidget>(&Layer);
        UMythicNameplateViewModel *ViewModel =
            NewObject<UMythicNameplateViewModel>(&Layer);
        Widget->SetNameplateViewModel(ViewModel);
        Layer.PooledWidgets.Add(Widget);
        Layer.PooledViewModels.Add(ViewModel);
        Layer.ClaimedInstances.AddDefaulted();

        Layer.ActionRailWidget =
            NewObject<UMythicNameplateActionRailWidget>(&Layer);
    }

    static bool HasActionRailRenderer(
        const UMythicNameplateLayer &Layer) {
        return Layer.ActionRailWidget != nullptr;
    }

    static int32 GetViewModelCount(const UMythicNameplateLayer &Layer) {
        return Layer.PooledViewModels.Num();
    }

    static int32 GetClaimCellCount(const UMythicNameplateLayer &Layer) {
        return Layer.ClaimedInstances.Num();
    }

    static FMythicEntityPresentationInstance GetActionRailInstance(
        const UMythicNameplateLayer &Layer) {
        return Layer.ActionRailInstance;
    }

    static UMythicNameplateWidget *GetPooledWidget(
        const UMythicNameplateLayer &Layer, const int32 Index) {
        return Layer.PooledWidgets.IsValidIndex(Index)
            ? Layer.PooledWidgets[Index] : nullptr;
    }
};

struct FMythicNameplateWidgetTestAccess {
    static void SeedPrewarmedBadges(UMythicNameplateWidget &Widget) {
        for (int32 Index = 0;
             Index < UMythicNameplateWidget::StatusBadgeCapacity;
             ++Index) {
            Widget.StatusBadgeRoots.Add(
                NewObject<UHorizontalBox>(&Widget));
        }
    }

    static int32 GetBadgeCount(const UMythicNameplateWidget &Widget) {
        return Widget.StatusBadgeRoots.Num();
    }

    static int32 GetBadgeCapacity() {
        return UMythicNameplateWidget::StatusBadgeCapacity;
    }

    static UImage *SeedOneRenderableBadge(
        UMythicNameplateWidget &Widget) {
        Widget.StatusBadgeHost = NewObject<UHorizontalBox>(&Widget);
        UHorizontalBox *Root = NewObject<UHorizontalBox>(&Widget);
        USizeBox *IconBox = NewObject<USizeBox>(&Widget);
        UImage *Icon = NewObject<UImage>(&Widget);
        UTextBlock *Label = NewObject<UTextBlock>(&Widget);
        UTextBlock *Stack = NewObject<UTextBlock>(&Widget);
        Widget.StatusBadgeRoots.Add(Root);
        Widget.StatusBadgeIconBoxes.Add(IconBox);
        Widget.StatusBadgeIcons.Add(Icon);
        Widget.StatusBadgeLabels.Add(Label);
        Widget.StatusBadgeStacks.Add(Stack);
        return Icon;
    }

    static void RefreshBadges(
        UMythicNameplateWidget &Widget,
        const FMythicNameplateProjection &Projection) {
        Widget.RefreshStatusBadges(Projection);
    }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateLayerReconstructKeepsPoolTest,
    "Mythic.UI.Nameplate.Pool.LayerReconstructKeepsPool",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateLayerReconstructKeepsPoolTest::RunTest(
    const FString &Parameters) {
    UMythicNameplateLayer *Layer = NewObject<UMythicNameplateLayer>();
    FMythicNameplateLayerTestAccess::SeedPrewarmedRenderers(*Layer);

    Layer->NativeDestruct();
    TestEqual(TEXT("destruct retains the ordinary renderer pool"),
              Layer->GetPrewarmedSlotCount(), 1);
    TestEqual(TEXT("destruct retains allocation-stable view models"),
              FMythicNameplateLayerTestAccess::GetViewModelCount(*Layer), 1);
    TestEqual(TEXT("destruct retains one claim cell per renderer"),
              FMythicNameplateLayerTestAccess::GetClaimCellCount(*Layer), 1);
    TestTrue(TEXT("destruct retains the separate prewarmed action rail"),
             FMythicNameplateLayerTestAccess::HasActionRailRenderer(*Layer));

    Layer->NativeConstruct();
    TestEqual(TEXT("reconstruct still has the original renderer pool"),
              Layer->GetPrewarmedSlotCount(), 1);
    TestTrue(TEXT("reconstruct still has the original action rail"),
             FMythicNameplateLayerTestAccess::HasActionRailRenderer(*Layer));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplatePooledFocusExactReleaseTest,
    "Mythic.UI.Nameplate.Pool.FocusUsesPoolAndExactRelease",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicNameplatePooledFocusExactReleaseTest::RunTest(
    const FString &Parameters) {
    FScopedNameplatePoolWorld ScopedWorld;
    UWorld *World = ScopedWorld.Get();
    if (!TestNotNull(TEXT("authority game world exists"), World)) {
        return false;
    }

    UMythicEntityPresentationRegistry *Registry =
        World->GetSubsystem<UMythicEntityPresentationRegistry>();
    if (!TestNotNull(TEXT("presentation registry exists"), Registry)) {
        return false;
    }

    const FMythicEntityPresentationInstance Current =
        Registry->AllocateAuthorityInstance(
            MakeNameplatePoolEntityId(1));
    if (!TestTrue(TEXT("authority allocated an exact instance"),
                  Current.IsValid())) {
        return false;
    }
    const FMythicEntityPresentationInstance Stale(
        Current.Handle, Current.EmbodimentGeneration + 1u);

    UMythicNameplateLayer *Layer = NewObject<UMythicNameplateLayer>();
    FMythicNameplateLayerTestAccess::SeedPrewarmedRenderers(*Layer);

    FMythicNameplateProjection FocusProjection;
    FocusProjection.Instance = Current;
    FocusProjection.DisclosureTier = EMythicNameplateDisclosureTier::Focus;
    FocusProjection.VisualFamily = EMythicNameplateVisualFamily::Combat;
    FocusProjection.AttentionState =
        EMythicNameplateAttentionState::Focused;
    FocusProjection.ResolvedName = FText::FromString(TEXT("Focused Target"));
    TestTrue(TEXT("Focus claims an ordinary pooled renderer"),
             Layer->ApplyProjection(FocusProjection, FVector2D::ZeroVector));
    TestEqual(TEXT("Focus consumes exactly one ordinary claim"),
              Layer->GetClaimedSlotCount(), 1);

    FMythicNameplateActionRailProjection ActionRail;
    ActionRail.Instance = Current;
    FMythicNameplateActionProjection &Action =
        ActionRail.Actions.AddDefaulted_GetRef();
    Action.ActionTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("Context.Action.NPC.Talk")), false);
    Action.ResolvedLabel = FText::FromString(TEXT("Talk"));
    Action.InputActionTag = FGameplayTag::RequestGameplayTag(
        FName(TEXT("UI.Action.ContextPrimary")), false);
    Action.OfferRevision = 17;
    if (!TestTrue(TEXT("action rail uses a canonical action tag"),
                  Action.ActionTag.IsValid()
                      && Action.InputActionTag.IsValid())) {
        return false;
    }
    TestTrue(TEXT("separate action rail accepts the same exact subject"),
             Layer->ApplyActionRailProjection(
                 ActionRail, FVector2D::ZeroVector));
    TestTrue(TEXT("action rail records the current exact instance"),
             FMythicNameplateLayerTestAccess::GetActionRailInstance(*Layer)
                 == Current);

    Registry->ReleaseAuthorityInstance(Current);
    TestNull(TEXT("authority retirement removes live component resolution"),
             Registry->ResolvePresentationComponent(Current));
    TestTrue(TEXT("a claimed renderer can finish a frozen outgoing fade after registry retirement"),
             Layer->UpdateProjectionPlacement(
                 Current, FVector2D(120.0f, 80.0f), 0.5f, 0.9f));
    TestEqual(TEXT("the retiring fade retains exactly one claim until explicit release"),
              Layer->GetClaimedSlotCount(), 1);

    Layer->ReleaseProjection(Stale);
    TestEqual(TEXT("stale generation cannot evict the pooled Focus claim"),
              Layer->GetClaimedSlotCount(), 1);
    TestTrue(TEXT("stale generation cannot release the action rail"),
             FMythicNameplateLayerTestAccess::GetActionRailInstance(*Layer)
                 == Current);

    Layer->ReleaseProjection(Current);
    TestEqual(TEXT("exact current instance releases the pooled Focus claim"),
              Layer->GetClaimedSlotCount(), 0);
    TestFalse(TEXT("exact current instance also releases its action rail"),
              FMythicNameplateLayerTestAccess::GetActionRailInstance(*Layer)
                  .IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateWidgetReconstructKeepsBadgesTest,
    "Mythic.UI.Nameplate.Pool.WidgetReconstructKeepsBadges",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateWidgetReconstructKeepsBadgesTest::RunTest(
    const FString &Parameters) {
    UMythicNameplateWidget *Widget =
        NewObject<UMythicNameplateWidget>();
    FMythicNameplateWidgetTestAccess::SeedPrewarmedBadges(*Widget);

    Widget->NativeDestruct();
    TestEqual(TEXT("destruct retains all four fixed badge roots"),
              FMythicNameplateWidgetTestAccess::GetBadgeCount(*Widget),
              FMythicNameplateWidgetTestAccess::GetBadgeCapacity());

    Widget->NativeConstruct();
    TestEqual(TEXT("reconstruct reuses all four fixed badge roots"),
              FMythicNameplateWidgetTestAccess::GetBadgeCount(*Widget),
              FMythicNameplateWidgetTestAccess::GetBadgeCapacity());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplateResidentOnlyIconContractTest,
    "Mythic.UI.Nameplate.Pool.ResidentOnlyIconContract",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicNameplateResidentOnlyIconContractTest::RunTest(
    const FString &Parameters) {
    TestEqual(TEXT("status renderer has four fixed slots"),
              FMythicNameplateWidgetTestAccess::GetBadgeCapacity(), 4);
    TestNull(
        TEXT("pooled widgets carry no per-slot pending soft-icon requests"),
        FindFProperty<FProperty>(UMythicNameplateWidget::StaticClass(),
                                 TEXT("StatusBadgeRequestedIcons")));
    TestNull(
        TEXT("status identity is not rendered as a separate colored backplate"),
        FindFProperty<FProperty>(UMythicNameplateWidget::StaticClass(),
                                 TEXT("StatusBadgeBackplates")));

    FProperty *ProjectedIcon = FindFProperty<FProperty>(
        FMythicNameplateStatusCandidate::StaticStruct(), TEXT("Icon"));
    TestNotNull(TEXT("status projections expose a resident icon property"),
                ProjectedIcon);
    TestNotNull(TEXT("status projections retain a strong object reference"),
                CastField<FObjectPropertyBase>(ProjectedIcon));
    TestNull(TEXT("status projections never carry a soft asset path"),
             CastField<FSoftObjectProperty>(ProjectedIcon));

    UMythicNameplateWidget *Widget =
        NewObject<UMythicNameplateWidget>();
    UImage *Icon =
        FMythicNameplateWidgetTestAccess::SeedOneRenderableBadge(*Widget);
    UTexture2D *Texture = NewObject<UTexture2D>(Widget);
    FMythicNameplateProjection Projection;
    FMythicNameplateStatusCandidate &Status =
        Projection.Statuses.AddDefaulted_GetRef();
    Status.Icon = Texture;
    Status.DisplayColor = FLinearColor(0.12f, 0.64f, 0.31f, 0.42f);

    FMythicNameplateWidgetTestAccess::RefreshBadges(*Widget, Projection);
    TestTrue(TEXT("the authored status identity color tints the icon glyph"),
             Icon->GetColorAndOpacity().Equals(
                 FLinearColor(0.12f, 0.64f, 0.31f, 1.0f)));
    Widget->ResetForPool();
    TestTrue(TEXT("pool release clears the previous status identity tint"),
             Icon->GetColorAndOpacity().Equals(FLinearColor::White));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNameplatePlacementVisibilityBandTest,
    "Mythic.UI.Nameplate.Pool.PlacementStaysOutOfHitTestAndPrepass",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicNameplatePlacementVisibilityBandTest::RunTest(
    const FString &Parameters) {
    FScopedNameplatePoolWorld ScopedWorld;
    UWorld *World = ScopedWorld.Get();
    if (!TestNotNull(TEXT("authority game world exists"), World)) {
        return false;
    }
    UMythicEntityPresentationRegistry *Registry =
        World->GetSubsystem<UMythicEntityPresentationRegistry>();
    if (!TestNotNull(TEXT("presentation registry exists"), Registry)) {
        return false;
    }
    const FMythicEntityPresentationInstance Instance =
        Registry->AllocateAuthorityInstance(MakeNameplatePoolEntityId(42));
    if (!TestTrue(TEXT("authority allocated an exact instance"),
                  Instance.IsValid())) {
        return false;
    }

    UMythicNameplateLayer *Layer = NewObject<UMythicNameplateLayer>();
    FMythicNameplateLayerTestAccess::SeedPrewarmedRenderers(*Layer);

    FMythicNameplateProjection Projection;
    Projection.Instance = Instance;
    Projection.DisclosureTier = EMythicNameplateDisclosureTier::Focus;
    Projection.VisualFamily = EMythicNameplateVisualFamily::Combat;
    Projection.AttentionState = EMythicNameplateAttentionState::Focused;
    Projection.ResolvedName = FText::FromString(TEXT("Band Target"));

    if (!TestTrue(TEXT("the projection claims a pooled renderer"),
                  Layer->ApplyProjection(Projection, FVector2D::ZeroVector,
                                         1.0f, 1.0f))) {
        return false;
    }
    UMythicNameplateWidget *Widget =
        FMythicNameplateLayerTestAccess::GetPooledWidget(*Layer, 0);
    if (!Widget) {
        AddError(TEXT("pooled renderer missing after a successful claim"));
        return false;
    }

    // A nameplate carries no interactive child, so it must take itself and its whole subtree out
    // of the hit-test grid. SelfHitTestInvisible would still insert every child, every frame.
    TestEqual(TEXT("a shown nameplate keeps its subtree out of the hit-test grid"),
              static_cast<int32>(Widget->GetVisibility()),
              static_cast<int32>(ESlateVisibility::HitTestInvisible));

    Layer->UpdateProjectionPlacement(Instance, FVector2D::ZeroVector, 0.0f, 1.0f);
    // Hidden still walks to every leaf during Slate pre-pass to compute geometry; Collapsed does not.
    TestEqual(TEXT("a fully faded nameplate leaves the pre-pass entirely"),
              static_cast<int32>(Widget->GetVisibility()),
              static_cast<int32>(ESlateVisibility::Collapsed));

    Layer->UpdateProjectionPlacement(Instance, FVector2D::ZeroVector, 1.0f, 1.0f);
    TestEqual(TEXT("fading back in restores the hit-test-invisible band"),
              static_cast<int32>(Widget->GetVisibility()),
              static_cast<int32>(ESlateVisibility::HitTestInvisible));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
