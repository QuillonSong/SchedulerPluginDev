// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SchedulerSubsystem.generated.h"

// 时刻变更委托，bIsForward=true为前进、false为后退
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTimeChanged, int64, NewCurrentTime, bool, bIsForward);

UCLASS()
class SCHEDULER_API USchedulerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
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
	
private:	
	// 当前时刻
	int64 CurrentTime = 0;

};
