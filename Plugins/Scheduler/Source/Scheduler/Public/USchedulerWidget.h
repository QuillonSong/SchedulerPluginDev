#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "Blueprint/UserWidget.h"
#include "SchedulerRulerTypes.h"
#include "USchedulerWidget.generated.h"

class SSchedulerWidget;
class SSchedulerRuler;
class SSchedulerPlayhead;
class SScrollBox;
class SVerticalBox;

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

	// ── Track ──

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Track")
	float TrackHeight = 24.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Track")
	FLinearColor OwnerTrackColor = FLinearColor(0.08f, 0.08f, 0.08f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Track")
	FLinearColor TaskTrackColor = FLinearColor(0.12f, 0.12f, 0.12f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Track")
	FLinearColor TrackTextColor = FLinearColor(0.9f, 0.9f, 0.9f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Track")
	FLinearColor TrackBorderColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Track")
	FMargin TrackTitleMargin = FMargin(1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Track")
	FMargin TrackBodyMargin = FMargin(1.f, 1.f, 0.f, 1.f);  // Body 右=0，其余=1

	//[FIXME]: Keyframe 属性不生效——SyncKeyframeState 同步链路待排查
	//[FIXME]: KeyframeSize/UnCheckedColor/CheckedColor 修改后渲染不变
	//[FIXME]: KeyframeTexture 设置贴图后面片不更新

	// ── Keyframe ──

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Keyframe")
	float KeyframeSize = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Keyframe")
	FLinearColor UnCheckedColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Keyframe")
	FLinearColor CheckedColor = FLinearColor(0.2f, 0.5f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Keyframe")
	TObjectPtr<UTexture2D> KeyframeTexture;

	// ── Playhead ──

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Playhead")
	float PlayheadWidth = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Playhead")
	FLinearColor PlayheadColor = FLinearColor(1.f, 0.f, 0.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Playhead")
	int32 PlayheadFontSize = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Playhead")
	FLinearColor PlayheadFontColor = FLinearColor(1.f, 0.f, 0.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Track")
	int32 TrackFontSize = 10;

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
	TSharedPtr<SSchedulerPlayhead> PlayheadWidget;

	// ── Track 滚动容器 ──

	/** BodyLeft 区垂直滚动框 */
	TSharedPtr<SScrollBox> TitleScrollBox;
	/** BodyLeft 区行容器 */
	TSharedPtr<SVerticalBox> TitleRowsBox;

	/** BodyRight 区垂直滚动框 */
	TSharedPtr<SScrollBox> BodyScrollBox;
	/** BodyRight 区行容器 */
	TSharedPtr<SVerticalBox> BodyRowsBox;

	// 运行时创建的HeadLeft控件实例——由Customize类动态生成，持有引用防GC回收
	UPROPERTY()
	TObjectPtr<UUserWidget> CustomizeInstance;

	// ── Handler ──

	void HandleRulerClicked(int64 Tick);
	void HandleRulerDragged(int64 DeltaTick);
	void HandleRulerScrolled(int32 ScrollDelta);

	// ── 滚动同步 ──

	/** 垂直滚动同步——Title 侧用户滚动时同步 Body 侧 */
	void HandleTitleScrolled(float Offset);
	/** 垂直滚动同步——Body 侧用户滚动时同步 Title 侧 */
	void HandleBodyScrolled(float Offset);

	/** 更新 Playhead 位置——OnTimeChanged / Ruler 滚动时调用 */
	UFUNCTION()
	void UpdatePlayhead(int64 NewCurrentTime, bool bIsForward);

	/** Phase 3：刷新所有 Keyframe 位置——Ruler 拖拽/缩放时自动调用 */
	void RefreshKeyframePositions();

	/** 乒乓防抖门禁 */
	bool bIsScrollSyncing = false;
};
