// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 十字分割布局的Slate控件
 * ┌──────────────────────────┐
 * │  HeadLeft  │  HeadRight  │ ← Head区，高度=HeadHeight
 * ├────────────┼─────────────┤
 * │  BodyLeft  │  BodyRight  │ ← Body区，填充剩余
 * └────────────┴─────────────┘
 *        ↑ 左区宽=LeftSidebarWeight（比例） → 右区填充剩余
 */
class SCHEDULER_API SSchedulerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSchedulerWidget);
		SLATE_ARGUMENT(float, HeadHeight)
		SLATE_ARGUMENT(float, LeftSidebarWidth)
		SLATE_ARGUMENT(bool, bIsDrawCross)
		SLATE_ARGUMENT(TSharedPtr<SWidget>, HeadLeft)
		SLATE_ARGUMENT(TSharedPtr<SWidget>, HeadRight)
		SLATE_ARGUMENT(TSharedPtr<SWidget>, BodyLeft)
		SLATE_ARGUMENT(TSharedPtr<SWidget>, BodyRight)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	// 绘制十字分割线
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// 运行时动态调整
	void SetHeadHeight(float InHeadHeight);
	void SetLeftSidebarWidth(float InWidth);
	void SetDrawCross(bool bDraw);

private:
	float HeadHeight = 100.f;
	float LeftSidebarWidth = 200.f;
	bool bIsDrawCross = true;

	// 十字分割线样式
	FLinearColor CrossLineColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.f);
	float CrossLineThickness = 2.f;
};
