#include "USchedulerWidget.h"
#include "SSchedulerWidget.h"
#include "SSchedulerRuler.h"
#include "SSchedulerPlayhead.h"
#include "SchedulerSubsystem.h"
#include "SchedulerRulerTypes.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"

#define LOCTEXT_NAMESPACE "UMG"

const FText USchedulerWidget::GetPaletteCategory()
{
	return LOCTEXT("Scheduler", "Scheduler");
}

TSharedRef<SWidget> USchedulerWidget::RebuildWidget()
{
	MyRulerSlate = SNew(SSchedulerRuler)
		.Style(RulerStyle)
		.MajorLength(RulerMajorLength)
		.MinorLength(RulerMinorLength)
		.MinorPixel(RulerMinorPixel)
		.TickOffset(RulerTickOffset)
		.MaxTickValue(RulerMaxTickValue)
		.TickLevel(RulerTickLevel)
		.OnClicked(FOnSSchedulerRulerClicked::CreateUObject(this, &USchedulerWidget::HandleRulerClicked))
		.OnDragged(FOnSSchedulerRulerDragged::CreateUObject(this, &USchedulerWidget::HandleRulerDragged))
		.OnScrolled(FOnSSchedulerRulerScrolled::CreateUObject(this, &USchedulerWidget::HandleRulerScrolled));

	TSharedPtr<SWidget> HeadLeftContent;
	if (Customize)
	{
		if (!CustomizeInstance || CustomizeInstance->GetClass() != Customize)
		{
			CustomizeInstance = CreateWidget<UUserWidget>(this, Customize);
		}
		if (CustomizeInstance)
		{
			HeadLeftContent = CustomizeInstance->TakeWidget();
		}
	}

	SAssignNew(TitleRowsBox, SVerticalBox);
	SAssignNew(TitleScrollBox, SScrollBox)
		.Orientation(Orient_Vertical)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.OnUserScrolled(FOnUserScrolled::CreateLambda([this](float Offset) { HandleTitleScrolled(Offset); }));
	TitleScrollBox->AddSlot()[TitleRowsBox.ToSharedRef()];

	SAssignNew(BodyRowsBox, SVerticalBox);
	SAssignNew(BodyScrollBox, SScrollBox)
		.Orientation(Orient_Vertical)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.OnUserScrolled(FOnUserScrolled::CreateLambda([this](float Offset) { HandleBodyScrolled(Offset); }));
	BodyScrollBox->AddSlot()[BodyRowsBox.ToSharedRef()];

	// 注入 Subsystem
	if (UWorld* World = GetWorld())
	{
		if (USchedulerSubsystem* Sub = World->GetSubsystem<USchedulerSubsystem>())
		{
			Sub->InitializeTrackContainers(
				TitleScrollBox, TitleRowsBox, BodyScrollBox, BodyRowsBox,
				TrackHeight, OwnerTrackColor, TaskTrackColor,
				TrackTextColor, TrackBorderColor,
				TrackTitleMargin, TrackBodyMargin, TrackFontSize);
		}
	}

	MySlateWidget = SNew(SSchedulerWidget)
		.HeadHeight(HeadHeight)
		.LeftSidebarWidth(LeftSidebarWidth)
		.bIsDrawCross(bIsDrawCross)
		.HeadLeft(HeadLeftContent)
		.HeadRight(MyRulerSlate)
		.BodyLeft(TitleScrollBox)
		.BodyRight(BodyScrollBox);

	SAssignNew(PlayheadWidget, SSchedulerPlayhead)
		.PlayheadWidth(PlayheadWidth)
		.PlayheadColor(PlayheadColor)
		.FontSize(PlayheadFontSize)
		.FontColor(PlayheadFontColor);

	// 绑定 OnTimeChanged → Playhead + 初始位置
	if (UWorld* World = GetWorld())
	{
		if (USchedulerSubsystem* Sub = World->GetSubsystem<USchedulerSubsystem>())
		{
			Sub->OnTimeChanged.AddDynamic(this, &USchedulerWidget::UpdatePlayhead);
			UpdatePlayhead(Sub->GetCurrentTime(), true);
		}
	}

	return SNew(SOverlay)
		+ SOverlay::Slot()[MySlateWidget.ToSharedRef()]
		+ SOverlay::Slot()[PlayheadWidget.ToSharedRef()];
}

void USchedulerWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	if (UWorld* World = GetWorld())
	{
		if (USchedulerSubsystem* Sub = World->GetSubsystem<USchedulerSubsystem>())
		{
			Sub->OnTimeChanged.RemoveDynamic(this, &USchedulerWidget::UpdatePlayhead);
		}
	}
	Super::ReleaseSlateResources(bReleaseChildren);
	MySlateWidget.Reset();
	MyRulerSlate.Reset();
	PlayheadWidget.Reset();
	TitleScrollBox.Reset();
	TitleRowsBox.Reset();
	BodyScrollBox.Reset();
	BodyRowsBox.Reset();
	if (bReleaseChildren) CustomizeInstance = nullptr;
}

void USchedulerWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	if (MySlateWidget.IsValid())
	{
		MySlateWidget->SetHeadHeight(HeadHeight);
		MySlateWidget->SetLeftSidebarWidth(LeftSidebarWidth);
		MySlateWidget->SetDrawCross(bIsDrawCross);
	}
	if (MyRulerSlate.IsValid())
	{
		MyRulerSlate->SetStyle(RulerStyle);
		MyRulerSlate->SetMajorLength(RulerMajorLength);
		MyRulerSlate->SetMinorLength(RulerMinorLength);
		MyRulerSlate->SetMinorPixel(RulerMinorPixel);
		MyRulerSlate->SetTickOffset(RulerTickOffset);
		MyRulerSlate->SetMaxTickValue(RulerMaxTickValue);
		MyRulerSlate->SetTickLevel(RulerTickLevel);
	}
}

void USchedulerWidget::HandleRulerClicked(int64 Tick)
{
	if (UWorld* World = GetWorld())
	{
		if (USchedulerSubsystem* Sub = World->GetSubsystem<USchedulerSubsystem>())
		{
			Sub->SetCurrentTime(Tick);
		}
	}
}

void USchedulerWidget::HandleRulerDragged(int64 DeltaTick)
{
	if (UWorld* World = GetWorld())
	{
		if (USchedulerSubsystem* Sub = World->GetSubsystem<USchedulerSubsystem>())
		{
			UpdatePlayhead(Sub->GetCurrentTime(), true);
		}
	}
	RefreshKeyframePositions();
	RefreshUI.Broadcast();
}

void USchedulerWidget::HandleRulerScrolled(int32 ScrollDelta)
{
	// Ruler 已在 OnMouseWheel 中自更新 ZoomTickLevel，此处不重复调用
	if (UWorld* World = GetWorld())
	{
		if (USchedulerSubsystem* Sub = World->GetSubsystem<USchedulerSubsystem>())
		{
			UpdatePlayhead(Sub->GetCurrentTime(), true);
		}
	}
	RefreshKeyframePositions();
	RefreshUI.Broadcast();
}

void USchedulerWidget::HandleTitleScrolled(float Offset)
{
	if (bIsScrollSyncing) return;
	bIsScrollSyncing = true;
	if (BodyScrollBox.IsValid()) BodyScrollBox->SetScrollOffset(Offset);
	bIsScrollSyncing = false;
}

void USchedulerWidget::HandleBodyScrolled(float Offset)
{
	if (bIsScrollSyncing) return;
	bIsScrollSyncing = true;
	if (TitleScrollBox.IsValid()) TitleScrollBox->SetScrollOffset(Offset);
	bIsScrollSyncing = false;
}

void USchedulerWidget::UpdatePlayhead(int64 NewCurrentTime, bool bIsForward)
{
	if (!PlayheadWidget.IsValid() || !MyRulerSlate.IsValid()) return;

	const TArray<FTickLevel>* Levels = MyRulerSlate->GetTickLevelArray();
	const int32 Lvl = FMath::Clamp(MyRulerSlate->GetActiveLevelIndex(), 0, Levels->Num() - 1);
	const int64 MinorTicks = Levels->Num() > 0 ? (*Levels)[Lvl].Minor : 1;
	const float EffectiveTickPixel = MyRulerSlate->GetMinorPixel() / static_cast<float>(MinorTicks);

	const float CachedW = MySlateWidget.IsValid() ? MySlateWidget->GetCachedGeometry().GetLocalSize().X : 0.f;
	const float RightW = CachedW > 0.f ? CachedW - LeftSidebarWidth : 10000.f;

	PlayheadWidget->UpdateState(
		NewCurrentTime,
		MyRulerSlate->GetViewStartTick(),
		EffectiveTickPixel,
		LeftSidebarWidth,
		RightW);
}

void USchedulerWidget::RefreshKeyframePositions()
{
	if (!MyRulerSlate.IsValid()) return;
	UWorld* World = GetWorld();
	if (!World) return;
	USchedulerSubsystem* Sub = World->GetSubsystem<USchedulerSubsystem>();
	if (!Sub) return;

	Sub->SyncKeyframeState(
		MyRulerSlate->GetViewStartTick(),
		MyRulerSlate->GetActiveLevelIndex(),
		*MyRulerSlate->GetTickLevelArray(),
		MyRulerSlate->GetMinorPixel(),
		KeyframeSize,
		UnCheckedColor,
		CheckedColor,
		KeyframeTexture.Get());
	Sub->RefreshAllKeyframes();
}

#undef LOCTEXT_NAMESPACE
