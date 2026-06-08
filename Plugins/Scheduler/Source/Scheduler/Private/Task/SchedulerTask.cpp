// Fill out your copyright notice in the Description page of Project Settings.


#include "Task/SchedulerTask.h"
#include "Task/TaskInterface.h"
#include "Scheduler.h"
#include "Core/SchedulerSubsystem.h"


void USchedulerTask::OnTaskInitialized()
{
	if (!Subsystem) return;

	// 按 OwnerName 入池
	TArray<USchedulerTask*>& OwnerTasks = Subsystem->TaskMap.FindOrAdd(TaskOwnerName);
	OwnerTasks.Add(this);

	// 绑定时刻变更回调
	Subsystem->OnTimeChanged.AddDynamic(this, &USchedulerTask::OnTimeChange);

	const bool bIsNewOwner = !Subsystem->TrackMap.Contains(TaskOwnerName);
	if (bIsNewOwner)
	{
		Subsystem->CreateOwnerTrackInternal(TaskOwnerName);
	}

	Subsystem->CreateTaskTrackInternal(*this);
}

void USchedulerTask::OnDestroy()
{
	// 防重入——DestroyOwner 等批量操作可能多次触及同一 Task
	if (bIsOnDestroy) return;
	bIsOnDestroy = true;

	// 解绑时刻变更委托
	if (Subsystem)
	{
		Subsystem->OnTimeChanged.RemoveDynamic(this, &USchedulerTask::OnTimeChange);
	}

	// 通知 TaskOwner——IsValid 防组件随 Actor 销毁后的悬空指针
	if (IsValid(TaskOwner) && TaskOwner->Implements<UTaskInterface>())
	{
		ITaskInterface::Execute_DestroyTask(TaskOwner);
	}

	// 标记 GC
	MarkAsGarbage();
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
		ClipIndex.LastIndex = Left - 1;
		ClipIndex.NextIndex = Left;
	}

	// 计算区间进度 Alpha：0.0 = 在 LastIndex 关键帧上，1.0 = 在 NextIndex 关键帧上
	// 边界（LastIndex == NextIndex，如末帧前进/首帧后退）钳位为 0.0 防除零
	double Alpha = 0.0;
	if (ClipIndex.LastIndex != ClipIndex.NextIndex)
	{
		const int64 Range = Keyframes[ClipIndex.NextIndex] - Keyframes[ClipIndex.LastIndex];
		Alpha = static_cast<double>(InCurrentTime - Keyframes[ClipIndex.LastIndex]) / static_cast<double>(Range);
	}

	// TaskOwner 可能已随 Actor/Component 销毁而失效，IsValid 防悬空
	if (!IsValid(TaskOwner)) return;
	ITaskInterface::Execute_ExecuteTask(TaskOwner, Alpha, bIsForward, ClipIndex);
}

void USchedulerTask::AddKeyframe(int64 NewKeyframe, const TArray<int64>& InKeyframes, int32& OutIndex, bool& bOutIsInsert)
{
	bOutIsInsert = true;

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

	if (Subsystem) Subsystem->RefreshAllKeyframes();
}

void USchedulerTask::RemoveKeyframe(int64 InKeyframe, int32& OriginalIndex)
{
	int32 Index = Keyframes.Find(InKeyframe);
	Keyframes.RemoveAt(Index);
	OriginalIndex = Index;

	if (IsValid(TaskOwner) && TaskOwner->Implements<UTaskInterface>())
	{
		ITaskInterface::Execute_RemoveKeyframe(TaskOwner, Index);
	}

	if (Subsystem) Subsystem->RefreshAllKeyframes();
}

void USchedulerTask::SyncKeyframes(const TArray<int64>& InKeyframes)
{
	Keyframes = InKeyframes;

	// 二分查找依赖有序，外部数据可能未排序或含重复
	Keyframes.Sort();
	for (int32 i = Keyframes.Num() - 1; i > 0; --i)
	{
		if (Keyframes[i] == Keyframes[i - 1])
		{
			Keyframes.RemoveAt(i);
		}
	}

	// 通知 UI 重绘 Keyframe 面片
	if (Subsystem) Subsystem->RefreshAllKeyframes();
}
