#include "USchedulerWidget.h"
#include "SSchedulerWidget.h"
#include "SSchedulerRuler.h"
#include "SchedulerSubsystem.h"

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

	MySlateWidget = SNew(SSchedulerWidget)
		.HeadHeight(HeadHeight)
		.LeftSidebarWidth(LeftSidebarWidth)
		.bIsDrawCross(bIsDrawCross)
		.HeadRight(MyRulerSlate);

	return MySlateWidget.ToSharedRef();
}

void USchedulerWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MySlateWidget.Reset();
	MyRulerSlate.Reset();
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

#undef LOCTEXT_NAMESPACE
