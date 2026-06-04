// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ConstructorHelpers.h"
#include "SchedulerSubsystem.h"
#include "SchedulerTask.generated.h"

/**
 *
 */


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FExecuteTask, int64, NewCurrentTime, bool, bIsForward, int32, ClipIndex);

UCLASS(BlueprintType, Blueprintable)
class SCHEDULER_API USchedulerTask : public UObject
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TArray<int64> Keyframes;


	// Task执行分发器
	UPROPERTY(BlueprintAssignable, Category = "Scheduler|Task")
	FExecuteTask ExecuteTask;

	// 任务名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task", meta = (ExposeOnSpawn = "true"))
	FString TaskName;
	
	// Task拥有者名称
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Task", meta = (ExposeOnSpawn = "true"))
	FString TaskOwnerName;
	

	// 添加关键帧
	UFUNCTION(BlueprintCallable, Category = "Scheduler|Task")
	void AddKeyframe(int64 NewKeyframe, TArray<int64> InKeyframes, int32& OutIndex, bool& bOutIsInsert);
};
