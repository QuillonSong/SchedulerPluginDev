#include "UI/Ruler/USchedulerRuler.h"
#include "UI/Ruler/SSchedulerRuler.h"
#include "Core/SchedulerSubsystem.h"

TSharedRef<SWidget> USchedulerRuler::RebuildWidget()
{
	if (UWorld* World = GetWorld())
	{
		CachedSubsystem = World->GetSubsystem<USchedulerSubsystem>();
	}

	MySlateWidget = SNew(SSchedulerRuler)
		.Style(Style)
		.MajorLength(MajorLength)
		.MinorLength(MinorLength)
		.MinorPixel(MinorPixel)
		.TickOffset(TickOffset)
		.MaxTickValue(MaxTickValue)
		.TickLevel(TickLevel)
		.OnClicked(FOnSSchedulerRulerClicked::CreateUObject(this, &USchedulerRuler::HandleSetCurrentTime))
		.OnDragged(FOnSSchedulerRulerDragged::CreateUObject(this, &USchedulerRuler::HandleRulerDragged))
		.OnScrolled(FOnSSchedulerRulerScrolled::CreateUObject(this, &USchedulerRuler::HandleRulerScrolled));

	return MySlateWidget.ToSharedRef();
}

void USchedulerRuler::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MySlateWidget.Reset();
}

void USchedulerRuler::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MySlateWidget.IsValid())
	{
		MySlateWidget->SetStyle(Style);
		MySlateWidget->SetMajorLength(MajorLength);
		MySlateWidget->SetMinorLength(MinorLength);
		MySlateWidget->SetMinorPixel(MinorPixel);
		MySlateWidget->SetTickOffset(TickOffset);
		MySlateWidget->SetMaxTickValue(MaxTickValue);
		MySlateWidget->SetTickLevel(TickLevel);
	}
}

void USchedulerRuler::ScrollRuler()
{
	if (MySlateWidget.IsValid())
	{
		MySlateWidget->Invalidate(EInvalidateWidgetReason::Paint);
	}
}

void USchedulerRuler::ZoomTickLevel(int32 Delta)
{
	if (MySlateWidget.IsValid())
	{
		MySlateWidget->ZoomTickLevel(Delta);
	}
}

void USchedulerRuler::HandleRulerScrolled(int32 ScrollDelta)
{
	OnScrolled.Broadcast(ScrollDelta);
	ZoomTickLevel(ScrollDelta);
}

void USchedulerRuler::HandleSetCurrentTime(int64 Tick)
{
	OnClicked.Broadcast(Tick);

	if (CachedSubsystem.IsValid())
	{
		CachedSubsystem->SetCurrentTime(Tick);
	}
}