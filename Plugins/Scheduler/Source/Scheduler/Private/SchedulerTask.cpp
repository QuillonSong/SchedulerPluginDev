// Fill out your copyright notice in the Description page of Project Settings.


#include "SchedulerTask.h"


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
}
