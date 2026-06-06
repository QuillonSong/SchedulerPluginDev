#include "SSchedulerPlayhead.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"

SSchedulerPlayhead::FArguments::FArguments() = default;

void SSchedulerPlayhead::Construct(const FArguments& InArgs)
{
	PlayheadWidth = InArgs._PlayheadWidth;
	PlayheadColor = InArgs._PlayheadColor;
	FontSize = InArgs._FontSize;
	FontColor = InArgs._FontColor;

	SetVisibility(EVisibility::HitTestInvisible);
}

void SSchedulerPlayhead::UpdateState(int64 InCurrentTime, int64 InViewStartTick,
	float InEffectiveTickPixel, float InLeftSidebarWidth, float InRightWidth)
{
	CurrentTime = InCurrentTime;
	ViewStartTick = InViewStartTick;
	EffectiveTickPixel = InEffectiveTickPixel;
	LeftSidebarWidth = InLeftSidebarWidth;

	const int64 RightTick = ViewStartTick + FMath::CeilToInt64(InRightWidth / EffectiveTickPixel);
	SetVisibility(CurrentTime >= ViewStartTick && CurrentTime <= RightTick
		? EVisibility::HitTestInvisible : EVisibility::Collapsed);

	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 SSchedulerPlayhead::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
	int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	if (PlayheadWidth <= 0.f) return LayerId;

	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	const float X = LeftSidebarWidth + static_cast<float>(CurrentTime - ViewStartTick) * EffectiveTickPixel;
	const float HalfW = PlayheadWidth * 0.5f;
	const float TextH = static_cast<float>(FontSize) + 4.f;

	// Playhead 竖线
	if (LocalSize.Y > TextH)
	{
		FSlateDrawElement::MakeBox(OutDrawElements, LayerId,
			AllottedGeometry.ToPaintGeometry(
				FVector2D(PlayheadWidth, LocalSize.Y - TextH),
				FSlateLayoutTransform(FVector2D(X - HalfW, TextH))),
			FCoreStyle::Get().GetDefaultBrush(),
			ESlateDrawEffect::None, PlayheadColor);
	}

	// 时间标签——OnPaint 直接绘制，与竖线同步对齐
	const FString Label = FString::Printf(TEXT("%lld"), CurrentTime);
	const FSlateFontInfo FontInfo(FCoreStyle::GetDefaultFont(), FontSize);
	const FVector2D LabelSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Label, FontInfo);

	FSlateDrawElement::MakeText(OutDrawElements, LayerId,
		AllottedGeometry.ToPaintGeometry(
			FVector2D(LabelSize.X, LabelSize.Y),
			FSlateLayoutTransform(FVector2D(X - LabelSize.X * 0.5f, 0.f))),
		Label, FontInfo, ESlateDrawEffect::None, FontColor);

	return LayerId;
}
