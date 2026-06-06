// Fill out your copyright notice in the Description page of Project Settings.


#include "SchedulerSubsystem.h"
#include "Scheduler.h"

void USchedulerSubsystem::initializationSubsystem()
{
	CurrentTime = 0;
	OnTimeChanged.Broadcast(0, true);
}

int64 USchedulerSubsystem::GetCurrentTime() const
{
	return CurrentTime;
}

void USchedulerSubsystem::SetCurrentTime(int64 NewCurrentTime)
{
	
	if (NewCurrentTime == CurrentTime||(NewCurrentTime <0))
	{
		return;
	}
	
	const bool bIsForward = NewCurrentTime > CurrentTime;
	CurrentTime = NewCurrentTime;
	OnTimeChanged.Broadcast(CurrentTime, bIsForward);
}

void USchedulerSubsystem::CurrentTimePlusPlus()
{
	++CurrentTime;
	OnTimeChanged.Broadcast(CurrentTime, true);
}

void USchedulerSubsystem::CurrentTimeMinusMinus()
{
	--CurrentTime;
	if (CurrentTime < 0)
	{
		CurrentTime = 0;
		return;
	}
	OnTimeChanged.Broadcast(CurrentTime, false);
}

USchedulerTask* USchedulerSubsystem::CreateTask(FString TaskName, FString TaskOwnerName, UObject* TaskOwner)
{
	// TaskOwner即Outer，蓝图节点必须连线有效对象，确保GC生命周期正确
	USchedulerTask* NewTask = NewObject<USchedulerTask>(TaskOwner);
	NewTask->TaskName = MoveTemp(TaskName);
	NewTask->TaskOwnerName = MoveTemp(TaskOwnerName);
	NewTask->TaskOwner = MoveTemp(TaskOwner);
	NewTask->OnTaskInitialized();

	// 初始化完成后按OwnerName入池
	TArray<USchedulerTask*>& OwnerTasks = TaskMap.FindOrAdd(NewTask->TaskOwnerName);
	OwnerTasks.Add(NewTask);

	// 创建完成后绑定时刻变更回调
	OnTimeChanged.AddDynamic(NewTask, &USchedulerTask::OnTimeChange);

	return NewTask;
}

bool USchedulerSubsystem::DeleteTask(USchedulerTask* Task)
{
    //TODO Task解绑委托->Task从池中移除->调用Task的OnDestory函数
    return true;
}


