// Fill out your copyright notice in the Description page of Project Settings.


#include "SchedulerTask.h"
#include "TaskInterface.h"
#include "Scheduler.h"


void USchedulerTask::OnTaskInitialized()
{
	//TODO))调用TaskTrack中的Create
}

void USchedulerTask::OnDestory()
{
	//TODO))调用销毁UI
}

void USchedulerTask::OnTimeChange(int64 InCurrentTime, bool bIsForward)
{	
	if (Keyframes.Num() == 0)
	{
		return;
	}

	if (InCurrentTime < Keyframes[0] || InCurrentTime > Keyframes.Last())
	{
		return;
	}

	// 二分查找：Left = 首个 >= InCurrentTime 的索引
	int32 Left = 0;
	int32 Right = Keyframes.Num();
	while (Left < Right)
	{
		const int32 Mid = Left + (Right - Left) / 2;
		if (Keyframes[Mid] < InCurrentTime)
		{
			Left = Mid + 1;
		}
		else
		{
			Right = Mid;
		}
	}

	FClipIndex ClipIndex;
	if (Left < Keyframes.Num() && Keyframes[Left] == InCurrentTime)
	{
		// 精确命中：方向决定区间
		if (bIsForward)
		{
			ClipIndex.LastIndex = Left;
			ClipIndex.NextIndex = FMath::Min(Left + 1, Keyframes.Num() - 1);
		}
		else
		{
			ClipIndex.LastIndex = FMath::Max(Left - 1, 0);
			ClipIndex.NextIndex = Left;
		}
	}
	else
	{
		// 落在两帧之间，与方向无关
		ClipIndex.LastIndex = Left - 1;
		ClipIndex.NextIndex = Left;
	}

	ITaskInterface::Execute_ExecuteTask(TaskOwner, InCurrentTime, bIsForward, ClipIndex);
}

void USchedulerTask::AddKeyframe(int64 NewKeyframe, const TArray<int64>& InKeyframes, int32& OutIndex, bool& bOutIsInsert)
{
	bOutIsInsert = true;

	// 拷贝组件 Keyframes 到 Task 缓存，就地操作
	Keyframes = InKeyframes;

	int32 Left = 0;
	int32 Right = Keyframes.Num();

	while (Left < Right)
	{
		const int32 Mid = Left + (Right - Left) / 2;

		if (Keyframes[Mid] == NewKeyframe)
		{
			OutIndex = Mid;
			bOutIsInsert = false;
			break;
		}

		if (Keyframes[Mid] < NewKeyframe)
			Left = Mid + 1;
		else
			Right = Mid;
	}

	if (bOutIsInsert)
	{
		Keyframes.Insert(NewKeyframe, Left);
		OutIndex = Left;
	}
	// [INFO]移除TArray<int64>& OutKeyframes，不在函数中更新OutKeyframes，外部数组由外部维护；
	// TODO))调用UI更新函数
}

void USchedulerTask::DeleteKeyframe(int64 InKeyframe, int32& OriginalIndex)
{
	int32 Index = Keyframes.Find(InKeyframe);
	Keyframes.RemoveAt(Index);
	OriginalIndex = Index;
}

