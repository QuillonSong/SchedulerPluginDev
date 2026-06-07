
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UI/Ruler/SchedulerRulerTypes.h"
#include "USchedulerRuler.generated.h"

class SSchedulerRuler;
class USchedulerSubsystem;

// UMG 层委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSchedulerRulerClicked, int64, Tick);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSchedulerRulerDragged, int64, DeltaTick);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSchedulerRulerScrolled, int32, ScrollDelta);

// SchedulerRuler 的 UMG 包装器，对蓝图公开
UCLASS(meta = (Category = "Scheduler"))
class SCHEDULER_API USchedulerRuler : public UWidget
{
	GENERATED_BODY()

public:
	FSchedulerRulerStyle Style;
	float MajorLength = 15.f;
	float MinorLength = 5.f;
	float MinorPixel = 10.f;
	float TickOffset = 0.f;
	TArray<FTickLevel> TickLevel;

	// 刻度轴最大 Tick 值——约束可滚动/可显示的 Tick 上界
	int64 MaxTickValue = 0;

	// 当前激活的缩放层级索引——TickLevel 数组下标
	int32 ActiveLevelIndex = 0;

	// ── 动作入口 ──

	// 鼠标左键点击——传出被点击的刻度值（Tick）
	FOnSchedulerRulerClicked OnClicked;
	
	// 鼠标右键拖拽——传出拖拽增量（单位 Tick，正=右拖，负=左拖）
	FOnSchedulerRulerDragged OnDragged;
	
	// 鼠标滚轮——传出滚动量（正=上滚，负=下滚）
	FOnSchedulerRulerScrolled OnScrolled;

	// 刻度轴滚动刷新——绑定至 USchedulerWidget::RefreshUI
	void ScrollRuler();

	// 缩放——滚轮上下切换 TickLevel 索引
	void ZoomTickLevel(int32 Delta);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TSharedPtr<SSchedulerRuler> MySlateWidget;
	TWeakObjectPtr<USchedulerSubsystem> CachedSubsystem;

	// Slate → UMG 转发
	void HandleSetCurrentTime(int64 Tick);
	void HandleRulerDragged(int64 DeltaTick) { OnDragged.Broadcast(DeltaTick); }
	void HandleRulerScrolled(int32 ScrollDelta);
};
