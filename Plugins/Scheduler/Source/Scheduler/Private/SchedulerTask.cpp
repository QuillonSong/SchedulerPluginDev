// Fill out your copyright notice in the Description page of Project Settings.


#include "SchedulerTask.h"
#include "TaskInterface.h"


void USchedulerTask::OnTaskInitialized()
{
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

void USchedulerTask::AddKeyframe(int64 NewKeyframe, TArray<int64> InKeyframes, int32& OutIndex, bool& bOutIsInsert)
{
	int32 Left = 0;
	int32 Right = InKeyframes.Num();

	while (Left < Right)
	{
		const int32 Mid = Left + (Right - Left) / 2;

		if (InKeyframes[Mid] == NewKeyframe)
		{
			InKeyframes[Mid] = NewKeyframe;  // 原地更新
			OutIndex = Mid; 
			bOutIsInsert = false;
			Keyframes = MoveTemp(InKeyframes);
			return;
		}

		if (InKeyframes[Mid] < NewKeyframe)
			Left = Mid + 1;
		else
			Right = Mid;
	}
	
	// 未找到 → 插入到正确位置
	InKeyframes.Insert(NewKeyframe, Left);
	OutIndex = Left;
	bOutIsInsert = true;
	Keyframes = MoveTemp(InKeyframes);
}
