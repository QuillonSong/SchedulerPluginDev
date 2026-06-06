#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Blueprint/UserWidget.h"
#include "SchedulerRulerTypes.h"
#include "USchedulerWidget.generated.h"

class SSchedulerWidget;
class SSchedulerRuler;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRefreshUI);

// SchedulerWidget的UMG包装器——蓝图唯一暴露口，统一管理子控件参数与UI刷新
UCLASS()
class SCHEDULER_API USchedulerWidget : public UWidget
{
	GENERATED_BODY()

public:
	// ── Layout ──

	// Head区固定高度（像素）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Layout")
	float HeadHeight = 100.f;

	// 左区固定宽度（像素），与HeadHeight同为绝对值
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Layout", meta = (ClampMin = "0"))
	float LeftSidebarWidth = 200.f;

	// 是否绘制十字分割线
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Layout")
	bool bIsDrawCross = true;

	// ── Ruler ──

	// 刻度尺绘制样式（颜色、粗细、背景）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	FSchedulerRulerStyle RulerStyle;

	// 大刻度线像素高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	float RulerMajorLength = 15.f;

	// 小刻度线像素高度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	float RulerMinorLength = 5.f;

	// 每 Minor 格子的固定像素宽度——缩放不改此值，改的是每格 Tick 数
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	float RulerMinorPixel = 10.f;

	// 刻度起始偏移（像素），从 Head 区顶部向下计量，上方留白
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	float RulerTickOffset = 0.f;

	// 缩放层级数组——各层级下 Minor/Major 分别对应多少 Tick
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	TArray<FTickLevel> RulerTickLevel;

	// 刻度轴最大 Tick 值——约束可滚动/可显示的 Tick 上界，0 表示不限制
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	int64 RulerMaxTickValue = 0;

	// ── Slot ──

	// 自定义HeadLeft区控件类——用户选取UUserWidget子类，运行时动态创建实例填入左上Slot
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Slot")
	TSubclassOf<UUserWidget> Customize;

	// ── UI 刷新 ──

	// 统一下发 UI 刷新信号——子控件绑定此委托以同步状态
	UPROPERTY(BlueprintAssignable, Category = "Scheduler|Event")
	FOnRefreshUI RefreshUI;

#if WITH_EDITOR
	virtual const FText GetPaletteCategory() override;
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TSharedPtr<SSchedulerWidget> MySlateWidget;
	TSharedPtr<SSchedulerRuler> MyRulerSlate;

	// 运行时创建的HeadLeft控件实例——由Customize类动态生成，持有引用防GC回收
	UPROPERTY()
	TObjectPtr<UUserWidget> CustomizeInstance;

	void HandleRulerClicked(int64 Tick);
	void HandleRulerDragged(int64 DeltaTick);
	void HandleRulerScrolled(int32 ScrollDelta);
};
