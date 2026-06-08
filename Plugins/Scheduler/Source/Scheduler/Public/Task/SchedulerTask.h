
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SchedulerTask.generated.h"

class USchedulerSubsystem;

// 关键帧区间——标识当前时刻落在哪两个关键帧之间
USTRUCT(BlueprintType)
struct SCHEDULER_API FClipIndex
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Task")
	int32 LastIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Task")
	int32 NextIndex = -1;
};

UCLASS(BlueprintType, Blueprintable)
class SCHEDULER_API USchedulerTask : public UObject
{
	GENERATED_BODY()

public:
	
	TArray<int64> Keyframes;// 关键帧组
	FString TaskName;       // 任务名称
	FString TaskOwnerName;	// Task拥有者名称
	UObject* TaskOwner;	    // Task拥有者
	USchedulerSubsystem* Subsystem;//  获取SchedulerSubsystem
	
	
	//TODO))创建UI变量
	//初始化Task
	void OnTaskInitialized();
	
	//销毁Task
	void OnDestroy();
	
	//TimeChange变更通知
	UFUNCTION()
	void OnTimeChange(int64 InCurrentTime, bool bIsForward);
	
	// 添加关键帧
	UFUNCTION(BlueprintCallable, Category = "Scheduler|Task")
	void AddKeyframe(int64 NewKeyframe, const TArray<int64>& InKeyframes, int32& OutIndex, bool& bOutIsInsert);

	//删除关键帧，不对蓝图公开，由未实现的KeyframeUI调用
	void RemoveKeyframe(int64 InKeyframe, int32& OriginalIndex);
	
	// 同步外部关键帧数据——Save/Load后恢复入口，Task仅接管渲染不持有数据源
	UFUNCTION(BlueprintCallable, Category = "Scheduler|Task")
	void SyncKeyframes(const TArray<int64>& InKeyframes);
	
	//TODO))销毁UI
	
private:
	// 防重复初始化
	bool bIsInitialized = false;
	// 无效Task，待销毁
	bool bIsOnDestroy = false;
};
