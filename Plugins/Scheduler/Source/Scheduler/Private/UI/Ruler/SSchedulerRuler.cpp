#include "UI/Ruler/SSchedulerRuler.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

SSchedulerRuler::FArguments::FArguments() = default;

void SSchedulerRuler::Construct(const FArguments& InArgs)
{
	Style = InArgs._Style;
	MajorLength = InArgs._MajorLength;
	MinorLength = InArgs._MinorLength;
	MinorPixel = FMath::Max(1.f, InArgs._MinorPixel);
	TickOffset = FMath::Max(0.f, InArgs._TickOffset);
	MaxTickValue = FMath::Max<int64>(0, InArgs._MaxTickValue);
	TickLevel = InArgs._TickLevel;

	OnClicked = InArgs._OnClicked;
	OnDragged = InArgs._OnDragged;
	OnScrolled = InArgs._OnScrolled;
}

int32 SSchedulerRuler::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();

	// 绘制背景
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(),
		FCoreStyle::Get().GetDefaultBrush(),
		ESlateDrawEffect::None, Style.BackgroundColor);

	++LayerId;

	// 取当前激活的刻度层级，未配置时使用默认
	const int32 LevelIdx = FMath::Clamp(ActiveLevelIndex, 0, TickLevel.Num() - 1);
	const int64 MinorTicks = TickLevel.Num() > 0 ? TickLevel[LevelIdx].Minor : 1;
	const int64 MajorTicks = TickLevel.Num() > 0 ? TickLevel[LevelIdx].Major : 10;

	// 刻度标签字体——使用引擎默认字体，指定字号
	const FSlateFontInfo FontInfo(FCoreStyle::GetDefaultFont(), Style.FontSize);

	// 实际像素/Tick = Minor 格子宽度 / 每格含 Tick 数
	const float EffectiveTickPixel = MinorPixel / static_cast<float>(MinorTicks);

	// 以 ViewStartTick 为起点，控件宽度确定可见刻度数量
	const int64 FirstTick = ViewStartTick;
	const int64 VisibleTicks = FMath::CeilToInt64(LocalSize.X / EffectiveTickPixel);
	const int64 LastTick = FMath::Min(ViewStartTick + VisibleTicks, MaxTickValue > 0 ? MaxTickValue : INT64_MAX);

	for (int64 Tick = FirstTick; Tick <= LastTick; ++Tick)
	{
		const float X = static_cast<float>(Tick - ViewStartTick) * EffectiveTickPixel;

		if (Tick % MajorTicks == 0)
		{
			DrawTickLine(AllottedGeometry, OutDrawElements, LayerId,
				X, MajorLength, Style.TickColor, Style.TickThickness);

			// 大刻度下方绘制刻度值，水平居中于刻度线
			const FString Label = FString::Printf(TEXT("%lld"), Tick);
			const FVector2D TextSize = FSlateApplication::Get()
				.GetRenderer()->GetFontMeasureService()->Measure(Label, FontInfo);
			const float TextX = X - TextSize.X * 0.5f;
			const float TextY = TickOffset + MajorLength + 2.f;
			FSlateDrawElement::MakeText(
				OutDrawElements, LayerId,
				AllottedGeometry.ToPaintGeometry(
					FVector2f(1.f, 1.f),
					FSlateLayoutTransform(FVector2f(TextX, TextY))),
				Label, FontInfo,
				ESlateDrawEffect::None, Style.TextColor);
		}
		else if (Tick % MinorTicks == 0)
		{
			DrawTickLine(AllottedGeometry, OutDrawElements, LayerId,
				X, MinorLength, Style.TickColor, Style.TickThickness);
		}
	}

	return LayerId;
}

void SSchedulerRuler::DrawTickLine(const FGeometry& AllottedGeometry,
	FSlateWindowElementList& OutDrawElements, int32 LayerId,
	float X, float LineLength, const FLinearColor& Color, float Thickness) const
{
	// 从上向下绘制——起点在 TickOffset，终点在 TickOffset + LineLength
	TArray<FVector2D> Line;
	Line.Add(FVector2D(X, TickOffset));
	Line.Add(FVector2D(X, TickOffset + LineLength));

	FSlateDrawElement::MakeLines(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(), Line,
		ESlateDrawEffect::None, Color, true, Thickness);
}

void SSchedulerRuler::SetStyle(const FSchedulerRulerStyle& InStyle)
{
	Style = InStyle;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SSchedulerRuler::SetMajorLength(float InLength)
{
	MajorLength = InLength;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SSchedulerRuler::SetMinorLength(float InLength)
{
	MinorLength = InLength;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SSchedulerRuler::SetMinorPixel(float InPixel)
{
	MinorPixel = FMath::Max(1.f, InPixel);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SSchedulerRuler::SetTickOffset(float InOffset)
{
	TickOffset = FMath::Max(0.f, InOffset);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SSchedulerRuler::SetTickLevel(const TArray<FTickLevel>& InLevels)
{
	TickLevel = InLevels;
	ActiveLevelIndex = FMath::Clamp(ActiveLevelIndex, 0, TickLevel.Num() - 1);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SSchedulerRuler::ZoomTickLevel(int32 Delta)
{
	if (TickLevel.Num() == 0) return;

	ActiveLevelIndex = FMath::Clamp(ActiveLevelIndex - Delta, 0, TickLevel.Num() - 1);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SSchedulerRuler::SetMaxTickValue(int64 InValue)
{
	MaxTickValue = FMath::Max<int64>(0, InValue);
	ViewStartTick = FMath::Min(ViewStartTick, MaxTickValue);
	Invalidate(EInvalidateWidgetReason::Paint);
}

int64 SSchedulerRuler::TickAtX(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
{
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const int32 Lvl = FMath::Clamp(ActiveLevelIndex, 0, TickLevel.Num() - 1);
	const int64 MinTicks = TickLevel.Num() > 0 ? TickLevel[Lvl].Minor : 1;
	const float EffectiveTickPixel = MinorPixel / static_cast<float>(MinTicks);
	return FMath::Max<int64>(0, ViewStartTick + FMath::RoundToInt64(LocalPos.X / EffectiveTickPixel));
}

FReply SSchedulerRuler::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		bIsRightDragging = true;
		LastDragX = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsLeftDragging = true;
		LastDragX = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X;
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}

	return FReply::Unhandled();
}

FReply SSchedulerRuler::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && bIsRightDragging)
	{
		bIsRightDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsLeftDragging)
	{
		bIsLeftDragging = false;

		if (OnClicked.IsBound())
		{
			OnClicked.ExecuteIfBound(TickAtX(MyGeometry, MouseEvent));
		}

		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}

FReply SSchedulerRuler::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (bIsRightDragging)
	{
		const int32 Lvl = FMath::Clamp(ActiveLevelIndex, 0, TickLevel.Num() - 1);
		const int64 MinTicks = TickLevel.Num() > 0 ? TickLevel[Lvl].Minor : 1;
		const float EffectiveTickPixel = MinorPixel / static_cast<float>(MinTicks);

		const float CurrentX = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X;
		const int64 CurrentTick = FMath::Max<int64>(0, ViewStartTick + FMath::RoundToInt64(CurrentX / EffectiveTickPixel));
		const int64 PrevTick = FMath::Max<int64>(0, ViewStartTick + FMath::RoundToInt64(LastDragX / EffectiveTickPixel));
		const int64 DeltaTick = CurrentTick - PrevTick;

		if (DeltaTick != 0)
		{
			ViewStartTick = FMath::Clamp(ViewStartTick - DeltaTick, (int64)0, MaxTickValue > 0 ? MaxTickValue : INT64_MAX);
			LastDragX = CurrentX;
			Invalidate(EInvalidateWidgetReason::Paint);

			if (OnDragged.IsBound())
			{
				OnDragged.ExecuteIfBound(DeltaTick);
			}
		}
		return FReply::Handled();
	}

	if (bIsLeftDragging && OnClicked.IsBound())
	{
		const int32 Lvl = FMath::Clamp(ActiveLevelIndex, 0, TickLevel.Num() - 1);
		const int64 MinTicks = TickLevel.Num() > 0 ? TickLevel[Lvl].Minor : 1;
		const float EffectiveTickPixel = MinorPixel / static_cast<float>(MinTicks);

		const float CurrentX = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition()).X;
		const int64 CurrentTick = FMath::Max<int64>(0, ViewStartTick + FMath::RoundToInt64(CurrentX / EffectiveTickPixel));
		const int64 PrevTick = FMath::Max<int64>(0, ViewStartTick + FMath::RoundToInt64(LastDragX / EffectiveTickPixel));

		if (CurrentTick != PrevTick)
		{
			OnClicked.ExecuteIfBound(CurrentTick);
			LastDragX = CurrentX;
		}
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SSchedulerRuler::OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	const int32 ScrollDelta = FMath::RoundToInt32(MouseEvent.GetWheelDelta());
	if (ScrollDelta != 0)
	{
		ZoomTickLevel(ScrollDelta);

		if (OnScrolled.IsBound())
		{
			OnScrolled.ExecuteIfBound(ScrollDelta);
		}
	}
	return FReply::Handled();
}
