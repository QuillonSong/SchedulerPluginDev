#include "USchedulerWidget.h"
#include "SSchedulerWidget.h"
#include "SSchedulerRuler.h"
#include "SchedulerSubsystem.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "UMG"

const FText USchedulerWidget::GetPaletteCategory()
{
	return LOCTEXT("Scheduler", "Scheduler");
}

TSharedRef<SWidget> USchedulerWidget::RebuildWidget()
{
	// 构造刻度尺 Slate 控件，属性取自本类 UPROPERTY
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

	// 提取左上角用户自定义控件——根据Customize类动态创建实例，未设置时传空
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

	// ── 构建 BodyLeft 滚动容器 ──

	SAssignNew(TitleRowsBox, SVerticalBox);
	SAssignNew(TitleScrollBox, SScrollBox)
		.Orientation(Orient_Vertical)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.OnUserScrolled(FOnUserScrolled::CreateLambda([this](float Offset) { HandleTitleScrolled(Offset); }));
	TitleScrollBox->AddSlot()[TitleRowsBox.ToSharedRef()];

	// ── 构建 BodyRight 滚动容器 ──

	SAssignNew(BodyRowsBox, SVerticalBox);
	SAssignNew(BodyScrollBox, SScrollBox)
		.Orientation(Orient_Vertical)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.OnUserScrolled(FOnUserScrolled::CreateLambda([this](float Offset) { HandleBodyScrolled(Offset); }));
	BodyScrollBox->AddSlot()[BodyRowsBox.ToSharedRef()];

	// ── 注入 Subsystem → 补齐已有 TrackMap → 后续 CreateTask/DeleteTask 增量 ──

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

	// ── 组装总布局 ──

	MySlateWidget = SNew(SSchedulerWidget)
		.HeadHeight(HeadHeight)
		.LeftSidebarWidth(LeftSidebarWidth)
		.bIsDrawCross(bIsDrawCross)
		.HeadLeft(HeadLeftContent)
		.HeadRight(MyRulerSlate)
		.BodyLeft(TitleScrollBox)
		.BodyRight(BodyScrollBox);

	return MySlateWidget.ToSharedRef();
}

void USchedulerWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MySlateWidget.Reset();
	MyRulerSlate.Reset();
	TitleScrollBox.Reset();
	TitleRowsBox.Reset();
	BodyScrollBox.Reset();
	BodyRowsBox.Reset();
	if (bReleaseChildren)
	{
		CustomizeInstance = nullptr;
	}
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
	RefreshUI.Broadcast();
}

void USchedulerWidget::HandleRulerScrolled(int32 ScrollDelta)
{
	if (MyRulerSlate.IsValid())
	{
		MyRulerSlate->ZoomTickLevel(ScrollDelta);
	}
	RefreshUI.Broadcast();
}

// ── 滚动同步 ──

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

#undef LOCTEXT_NAMESPACE
