
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Task/SchedulerTask.h"
#include "UI/Ruler/SchedulerRulerTypes.h"
#include "SchedulerSubsystem.generated.h"

// 前向声明
class SSchedulerTrackTitle;
class SSchedulerTrackBody;
class SScrollBox;
class SVerticalBox;
class UTexture2D;

// 时刻变更委托，bIsForward=true为前进、false为后退
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeChanged, int64, NewCurrentTime, bool, bIsForward);

/** TaskTrack 行——OwnerTrack 下的子项 */
struct SCHEDULER_API FTaskTrackEntry
{
	TSharedPtr<SSchedulerTrackTitle> Title;
	TSharedPtr<SSchedulerTrackBody>  Body;
	USchedulerTask* Task = nullptr;  // 用于 DeleteTask 时精确匹配
};

/** OwnerTrack 行——持有自身 Title/Body + 子 TaskTrack 列表 */
struct SCHEDULER_API FTrack
{
	TSharedPtr<SSchedulerTrackTitle> Title;
	TSharedPtr<SSchedulerTrackBody>  Body;
	TArray<FTaskTrackEntry> TaskTracks;
	int32 OwnerRowIndex = INDEX_NONE;
	bool bIsCollapsed = false;

	bool IsValid() const { return Title.IsValid() && Body.IsValid(); }
};

UCLASS()
class SCHEDULER_API USchedulerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:

	// Task管理池——按 OwnerName 分组
	TMap<FString, TArray<USchedulerTask*>> TaskMap;

	// Track管理池——与 TaskMap 镜像，按 OwnerName 索引
	TMap<FString, FTrack> TrackMap;

	// 时刻变更事件分发器，任意类型对象均可绑定
	UPROPERTY(BlueprintAssignable, Category = "Scheduler|TimeChange")
	FOnTimeChanged OnTimeChanged;

	// 初始化子系统
	UFUNCTION(BlueprintCallable, Category = "Scheduler")
	void initializationSubsystem();

	// 获取当前时刻
	UFUNCTION(BlueprintPure, Category = "Scheduler|TimeChange")
	int64 GetCurrentTime() const ;

	// 设置当前时刻
	UFUNCTION(BlueprintCallable, Category = "Scheduler|TimeChange")
	void SetCurrentTime(int64 NewCurrentTime);

	// 前进一时刻
	UFUNCTION(BlueprintCallable, Category = "Scheduler|TimeChange")
	void CurrentTimePlusPlus();

	// 后退一时刻
	UFUNCTION(BlueprintCallable, Category = "Scheduler|TimeChange")
	void CurrentTimeMinusMinus();

	//创建Task
	UFUNCTION(BlueprintCallable, Category = "Scheduler|Task")
	USchedulerTask* CreateTask(FString TaskName, FString TaskOwnerName, UObject* TaskOwner);

	//删除Task
	UFUNCTION(BlueprintCallable, Category = "Scheduler|Task")
	bool DestroyTask(USchedulerTask* Task);

	/**
	 * Widget 初始化时调用——注入 Track 容器引用 + 显示属性
	 * 同时遍历已有 TrackMap，为缺失控件的条目补齐 Title/Body
	 */
	void InitializeTrackContainers(
		TSharedPtr<SScrollBox> InTitleScrollBox,
		TSharedPtr<SVerticalBox> InTitleRowsBox,
		TSharedPtr<SScrollBox> InBodyScrollBox,
		TSharedPtr<SVerticalBox> InBodyRowsBox,
		float InTrackHeight,
		FLinearColor InOwnerTrackColor,
		FLinearColor InTaskTrackColor,
		FLinearColor InTrackTextColor,
		FLinearColor InTrackBorderColor,
		FMargin InTrackTitleMargin,
		FMargin InTrackBodyMargin,
		int32 InTrackFontSize);

	/** Phase 3：刷新所有 TaskTrack 的 Keyframe 渲染——无参，读内部存储的 Ruler 指针 */
	void RefreshAllKeyframes();

	/** 同步 Ruler 状态 + Keyframe 参数到缓存 */
	// ── 供 USchedulerTask 调用的内部方法 ──
	void CreateOwnerTrackInternal(const FString& OwnerName);
	void CreateTaskTrackInternal(USchedulerTask& Task);

	void SyncKeyframeState(int64 InViewStartTick, int32 InActiveLevelIndex,
		const TArray<FTickLevel>& InTickLevel, float InMinorPixel,
		float InKeyframeSize, FLinearColor InUnCheckedColor, FLinearColor InCheckedColor,
		UTexture2D* InTexture);

private:
	// 当前时刻
	int64 CurrentTime = 0;

	// ── Track 容器（由 Widget 注入）──
	TSharedPtr<SScrollBox> TitleScrollBox;
	TSharedPtr<SVerticalBox> TitleRowsBox;
	TSharedPtr<SScrollBox> BodyScrollBox;
	TSharedPtr<SVerticalBox> BodyRowsBox;

	// ── Track 显示属性（由 Widget 注入）──
	float TrackHeight = 24.f;
	FLinearColor OwnerTrackColor;
	FLinearColor TaskTrackColor;
	FLinearColor TrackTextColor;
	FLinearColor TrackBorderColor;
	FMargin TrackTitleMargin = FMargin(1.f);
	FMargin TrackBodyMargin = FMargin(1.f, 1.f, 0.f, 1.f);
	int32 TrackFontSize = 10;

	// ── Ruler 状态缓存（Phase 3 Keyframe 对齐用）──
	int64 CachedViewStartTick = 0;
	int32 CachedActiveLevelIndex = 0;
	TArray<FTickLevel> CachedTickLevel;
	float CachedMinorPixel = 10.f;
	float CachedKeyframeSize = 10.f;
	FLinearColor CachedUnCheckedColor = FLinearColor::White;
	FLinearColor CachedCheckedColor = FLinearColor(0.2f, 0.5f, 1.f);
	TObjectPtr<UTexture2D> CachedKeyframeTexture;

	/** 创建 OwnerTrack 的 Title + Body 控件并添入容器 */
	void CreateOwnerTrackWidgets(const FString& OwnerName, FTrack& OutTrack);

	/** 销毁 OwnerTrack 及其所有子 TaskTrack 的控件 */
	void DestroyOwnerTrackWidgets(FTrack& InTrack);

	/** 创建 TaskTrack 的 Title + Body 控件并插入到所属 OwnerTrack 下 */
	void CreateTaskTrackWidgets(FTrack& OwnerTrack, USchedulerTask* Task);

	/** 销毁单个 TaskTrack 控件并从容器移除 */
	void DestroyTaskTrackWidgets(FTrack& OwnerTrack, const FTaskTrackEntry& Entry);
};