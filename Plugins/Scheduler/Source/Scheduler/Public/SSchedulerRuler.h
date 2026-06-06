#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "SchedulerRulerTypes.h"

// Slate 层委托
DECLARE_DELEGATE_OneParam(FOnSSchedulerRulerClicked, int64 /*Tick*/)
DECLARE_DELEGATE_OneParam(FOnSSchedulerRulerDragged, int64 /*DeltaTick*/)
DECLARE_DELEGATE_OneParam(FOnSSchedulerRulerScrolled, int32 /*ScrollDelta*/)

// 水平刻度尺 Slate 实现——绘制 Tick 刻度线，按 TickLevel 区分大小格
class SCHEDULER_API SSchedulerRuler : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSchedulerRuler);
		SLATE_ARGUMENT(FSchedulerRulerStyle, Style)
		SLATE_ARGUMENT(float, MajorLength)
		SLATE_ARGUMENT(float, MinorLength)
		SLATE_ARGUMENT(float, MinorPixel)
		SLATE_ARGUMENT(float, TickOffset)
		SLATE_ARGUMENT(int64, MaxTickValue)
		SLATE_ARGUMENT(TArray<FTickLevel>, TickLevel)
		SLATE_EVENT(FOnSSchedulerRulerClicked, OnClicked)
		SLATE_EVENT(FOnSSchedulerRulerDragged, OnDragged)
		SLATE_EVENT(FOnSSchedulerRulerScrolled, OnScrolled)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// 运行时动态设置
	void SetStyle(const FSchedulerRulerStyle& InStyle);
	void SetMajorLength(float InLength);
	void SetMinorLength(float InLength);
	void SetMinorPixel(float InPixel);
	void SetTickOffset(float InOffset);
	void SetTickLevel(const TArray<FTickLevel>& InLevels);
	void SetMaxTickValue(int64 InValue);
	void ZoomTickLevel(int32 Delta);

	// ── Keyframe 对齐（Phase 3）──
	int64 GetViewStartTick() const { return ViewStartTick; }
	int32 GetActiveLevelIndex() const { return ActiveLevelIndex; }
	float GetMinorPixel() const { return MinorPixel; }
	const TArray<FTickLevel>* GetTickLevelArray() const { return &TickLevel; }

	// 输入事件
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseWheel(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	FSchedulerRulerStyle Style;
	float MajorLength = 15.f;
	float MinorLength = 5.f;
	float MinorPixel = 10.f;
	float TickOffset = 0.f;
	int64 MaxTickValue = 0;
	int64 ViewStartTick = 0;
	int32 ActiveLevelIndex = 0;
	TArray<FTickLevel> TickLevel;

	// 回调
	FOnSSchedulerRulerClicked OnClicked;
	FOnSSchedulerRulerDragged OnDragged;
	FOnSSchedulerRulerScrolled OnScrolled;

	// 拖拽状态
	bool bIsRightDragging = false;
	bool bIsLeftDragging = false;
	float LastDragX = 0.f;

	// 绘制单条刻度线（从上向下）
	void DrawTickLine(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, float X, float LineLength, const FLinearColor& Color, float Thickness) const;

	// 根据 X 坐标计算对应 Tick 值
	int64 TickAtX(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;
};
