#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * Playhead——贯穿 Right 区的垂直游标线
 * 单个 Slate 控件，SOverlay 覆盖整个 SchedulerWidget
 * 宽度/颜色不随缩放变化，仅位置随 CurrentTime + Ruler 滚动刷新
 */
class SCHEDULER_API SSchedulerPlayhead : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSchedulerPlayhead);
		SLATE_ARGUMENT(float, PlayheadWidth)
		SLATE_ARGUMENT(FLinearColor, PlayheadColor)
		SLATE_ARGUMENT(int32, FontSize)
		SLATE_ARGUMENT(FLinearColor, FontColor)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** 更新游标数据——由外部在 OnTimeChanged / RefreshUI 时调用 */
	void UpdateState(int64 InCurrentTime, int64 InViewStartTick,
		float InEffectiveTickPixel, float InLeftSidebarWidth, float InRightWidth);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	float PlayheadWidth = 2.f;
	FLinearColor PlayheadColor = FLinearColor::Red;
	int32 FontSize = 8;
	FLinearColor FontColor = FLinearColor::Red;
	int64 CurrentTime = 0;
	int64 ViewStartTick = 0;
	float EffectiveTickPixel = 10.f;
	float LeftSidebarWidth = 200.f;
};
