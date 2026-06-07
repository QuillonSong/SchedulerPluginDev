
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Task/SchedulerTask.h"
#include "TaskInterface.generated.h"

UINTERFACE(Blueprintable)
class SCHEDULER_API UTaskInterface : public UInterface
{
	GENERATED_BODY()
};

class SCHEDULER_API ITaskInterface
{
	GENERATED_BODY()

public:
	// Task时刻变更回调——TaskOwner实现此接口以接收关键帧区间通知
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scheduler|Task")
	void ExecuteTask(double Alpha, bool bIsForward, FClipIndex ClipIndex);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scheduler|Task")
	void DestroyTask();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Scheduler|Task")
	void RemoveKeyframe(int32 KeyframeIndex);
	
};
 