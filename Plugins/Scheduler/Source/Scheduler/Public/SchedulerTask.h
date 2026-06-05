// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "SchedulerTask.generated.h"

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

	//初始化Task
	void OnTaskInitialized();
	
	//TimeChange变更通知
	UFUNCTION()
	void OnTimeChange(int64 InCurrentTime, bool bIsForward);
	
	// 添加关键帧
	UFUNCTION(BlueprintCallable, Category = "Scheduler|Task")
	void AddKeyframe(int64 NewKeyframe, TArray<int64> InKeyframes, int32& OutIndex, bool& bOutIsInsert);

private:
	// 防重复初始化
	bool bIsInitialized = false;
};
