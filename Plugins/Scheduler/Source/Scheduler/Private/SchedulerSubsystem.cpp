#include "SchedulerSubsystem.h"
#include "SSchedulerTrackTitle.h"
#include "SSchedulerTrackBody.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"

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
	if (NewCurrentTime == CurrentTime || (NewCurrentTime < 0))
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
	USchedulerTask* NewTask = NewObject<USchedulerTask>(TaskOwner);
	NewTask->TaskName = MoveTemp(TaskName);
	NewTask->TaskOwnerName = MoveTemp(TaskOwnerName);
	NewTask->TaskOwner = MoveTemp(TaskOwner);
	NewTask->OnTaskInitialized();

	const bool bIsNewOwner = !TrackMap.Contains(NewTask->TaskOwnerName);

	TArray<USchedulerTask*>& OwnerTasks = TaskMap.FindOrAdd(NewTask->TaskOwnerName);
	OwnerTasks.Add(NewTask);

	OnTimeChanged.AddDynamic(NewTask, &USchedulerTask::OnTimeChange);

	if (bIsNewOwner)
	{
		FTrack NewTrack;
		if (TitleRowsBox.IsValid() && BodyRowsBox.IsValid())
		{
			CreateOwnerTrackWidgets(NewTask->TaskOwnerName, NewTrack);
		}
		TrackMap.Add(NewTask->TaskOwnerName, MoveTemp(NewTrack));
	}

	// 增量：始终创建 TaskTrack（无论 Owner 是否新建）
	FTrack* OwnerTrack = TrackMap.Find(NewTask->TaskOwnerName);
	if (OwnerTrack && TitleRowsBox.IsValid() && BodyRowsBox.IsValid())
	{
		CreateTaskTrackWidgets(*OwnerTrack, NewTask);
	}

	return NewTask;
}

bool USchedulerSubsystem::DeleteTask(USchedulerTask* Task)
{
	if (!IsValid(Task))
	{
		return false;
	}

	FTrack* OwnerTrack = TrackMap.Find(Task->TaskOwnerName);
	TArray<USchedulerTask*>* OwnerTasks = TaskMap.Find(Task->TaskOwnerName);
	if (!OwnerTasks)
	{
		return false;
	}

	// 遍历找到对应的 TaskTrack Entry 并销毁
	if (OwnerTrack)
	{
		for (int32 i = OwnerTrack->TaskTracks.Num() - 1; i >= 0; --i)
		{
			if (OwnerTrack->TaskTracks[i].Task == Task)
			{
				DestroyTaskTrackWidgets(*OwnerTrack, OwnerTrack->TaskTracks[i]);
				OwnerTrack->TaskTracks.RemoveAt(i);
				break;
			}
		}
	}

	OwnerTasks->Remove(Task);

	// 分组清空 → 移除 OwnerTrack
	if (OwnerTasks->Num() == 0)
	{
		TaskMap.Remove(Task->TaskOwnerName);
		if (OwnerTrack)
		{
			if (OwnerTrack->IsValid())
			{
				DestroyOwnerTrackWidgets(*OwnerTrack);
			}
			TrackMap.Remove(Task->TaskOwnerName);
		}
	}

	return true;
}

// ── Track 容器初始化 ──

void USchedulerSubsystem::InitializeTrackContainers(
	TSharedPtr<SScrollBox> InTitleScrollBox,
	TSharedPtr<SVerticalBox> InTitleRowsBox,
	TSharedPtr<SScrollBox> InBodyScrollBox,
	TSharedPtr<SVerticalBox> InBodyRowsBox,
	float InTrackHeight,
	FLinearColor InOwnerTrackColor,
	FLinearColor InTaskTrackColor,
	FLinearColor InTrackTextColor,
	FLinearColor InTrackBorderColor,
	FMargin InTrackTitleMargin,
	FMargin InTrackBodyMargin,
	int32 InTrackFontSize)
{
	TitleScrollBox = MoveTemp(InTitleScrollBox);
	TitleRowsBox = MoveTemp(InTitleRowsBox);
	BodyScrollBox = MoveTemp(InBodyScrollBox);
	BodyRowsBox = MoveTemp(InBodyRowsBox);

	TrackHeight = InTrackHeight;
	OwnerTrackColor = InOwnerTrackColor;
	TaskTrackColor = InTaskTrackColor;
	TrackTextColor = InTrackTextColor;
	TrackBorderColor = InTrackBorderColor;
	TrackTitleMargin = InTrackTitleMargin;
	TrackBodyMargin = InTrackBodyMargin;
	TrackFontSize = InTrackFontSize;

	// 补齐已有 TrackMap 中缺失控件的条目
	for (auto& Pair : TrackMap)
	{
		FTrack& Track = Pair.Value;
		if (!Track.IsValid() && TitleRowsBox.IsValid() && BodyRowsBox.IsValid())
		{
			CreateOwnerTrackWidgets(Pair.Key, Track);
		}
		// TODO Phase 2: 补齐已有 Task 的 TaskTrack（需 TaskMap 遍历，暂时跳过）
	}
}

// ── OwnerTrack ──

void USchedulerSubsystem::CreateOwnerTrackWidgets(const FString& OwnerName, FTrack& OutTrack)
{
	const FString DisplayName = FString::Printf(TEXT("▼ %s"), *OwnerName);  // 初始展开

	TSharedRef<SSchedulerTrackTitle> Title = SNew(SSchedulerTrackTitle)
		.DisplayName(DisplayName)
		.RowHeight(TrackHeight)
		.RowColor(OwnerTrackColor)
		.TextColor(TrackTextColor)
		.BorderColor(TrackBorderColor)
		.BorderMargin(TrackTitleMargin)
		.FontSize(TrackFontSize)
		.bIsChild(false)
		.IndentWidth(0.f)
		.OnTrackClicked(FOnClicked::CreateLambda([this, OwnerName]()
		{
			FTrack* Track = TrackMap.Find(OwnerName);
			if (!Track) return FReply::Unhandled();

			Track->bIsCollapsed = !Track->bIsCollapsed;
			const EVisibility Vis = Track->bIsCollapsed ? EVisibility::Collapsed : EVisibility::Visible;
			for (FTaskTrackEntry& Entry : Track->TaskTracks)
			{
				if (Entry.Title.IsValid()) Entry.Title->SetVisibility(Vis);
				if (Entry.Body.IsValid())  Entry.Body->SetVisibility(Vis);
			}
			// 切换箭头
			const FString NewText = FString::Printf(TEXT("%s %s"),
				Track->bIsCollapsed ? TEXT("▶") : TEXT("▼"), *OwnerName);
			if (Track->Title.IsValid()) Track->Title->SetDisplayName(NewText);
			return FReply::Handled();
		}));

	TSharedRef<SSchedulerTrackBody> Body = SNew(SSchedulerTrackBody)
		.RowHeight(TrackHeight)
		.RowColor(OwnerTrackColor)
		.BorderColor(TrackBorderColor)
		.BorderMargin(TrackBodyMargin);

	TitleRowsBox->AddSlot().AutoHeight()[Title];
	BodyRowsBox->AddSlot().AutoHeight()[Body];

	OutTrack.Title = Title;
	OutTrack.Body = Body;
	OutTrack.OwnerRowIndex = TitleRowsBox->NumSlots() - 1;
}

void USchedulerSubsystem::DestroyOwnerTrackWidgets(FTrack& InTrack)
{
	// 先销毁所有子 TaskTrack
	for (FTaskTrackEntry& Entry : InTrack.TaskTracks)
	{
		DestroyTaskTrackWidgets(InTrack, Entry);
	}
	InTrack.TaskTracks.Empty();

	if (TitleRowsBox.IsValid() && InTrack.Title.IsValid())
	{
		TitleRowsBox->RemoveSlot(InTrack.Title.ToSharedRef());
	}
	if (BodyRowsBox.IsValid() && InTrack.Body.IsValid())
	{
		BodyRowsBox->RemoveSlot(InTrack.Body.ToSharedRef());
	}
	InTrack.Title.Reset();
	InTrack.Body.Reset();

	// 后续 OwnerTrack 的索引 -1
	for (auto& Pair : TrackMap)
	{
		FTrack& Track = Pair.Value;
		if (Track.OwnerRowIndex > InTrack.OwnerRowIndex)
		{
			Track.OwnerRowIndex--;
		}
	}
}

// ── TaskTrack ──

void USchedulerSubsystem::CreateTaskTrackWidgets(FTrack& OwnerTrack, USchedulerTask* Task)
{
	TSharedRef<SSchedulerTrackTitle> Title = SNew(SSchedulerTrackTitle)
		.DisplayName(Task->TaskName)
		.RowHeight(TrackHeight)
		.RowColor(TaskTrackColor)
		.TextColor(TrackTextColor)
		.BorderColor(TrackBorderColor)
		.BorderMargin(TrackTitleMargin)
		.FontSize(TrackFontSize)
		.bIsChild(true)
		.IndentWidth(20.f);

	TSharedRef<SSchedulerTrackBody> Body = SNew(SSchedulerTrackBody)
		.RowHeight(TrackHeight)
		.RowColor(TaskTrackColor)
		.BorderColor(TrackBorderColor)
		.BorderMargin(TrackBodyMargin)
		.KeyframesPtr(&Task->Keyframes);

	// 插入到 OwnerTrack 下方、已有 TaskTrack 之后
	const int32 InsertIndex = OwnerTrack.OwnerRowIndex + 1 + OwnerTrack.TaskTracks.Num();
	TitleRowsBox->InsertSlot(InsertIndex).AutoHeight()[Title];
	BodyRowsBox->InsertSlot(InsertIndex).AutoHeight()[Body];

	// 后续 OwnerTrack 的 OwnerRowIndex 需 +1
	for (auto& Pair : TrackMap)
	{
		FTrack& Track = Pair.Value;
		if (Track.OwnerRowIndex > OwnerTrack.OwnerRowIndex)
		{
			Track.OwnerRowIndex++;
		}
	}

	FTaskTrackEntry Entry;
	Entry.Title = Title;
	Entry.Body = Body;
	Entry.Task = Task;

	// 若 Owner 当前已折叠，新 TaskTrack 也需隐藏
	if (OwnerTrack.bIsCollapsed)
	{
		Title->SetVisibility(EVisibility::Collapsed);
		Body->SetVisibility(EVisibility::Collapsed);
	}

	OwnerTrack.TaskTracks.Add(MoveTemp(Entry));
}

void USchedulerSubsystem::DestroyTaskTrackWidgets(FTrack& OwnerTrack, const FTaskTrackEntry& Entry)
{
	if (TitleRowsBox.IsValid() && Entry.Title.IsValid())
	{
		TitleRowsBox->RemoveSlot(Entry.Title.ToSharedRef());
	}
	if (BodyRowsBox.IsValid() && Entry.Body.IsValid())
	{
		BodyRowsBox->RemoveSlot(Entry.Body.ToSharedRef());
	}

	// 后续 OwnerTrack 的 OwnerRowIndex 需 -1
	for (auto& Pair : TrackMap)
	{
		FTrack& Track = Pair.Value;
		if (Track.OwnerRowIndex > OwnerTrack.OwnerRowIndex)
		{
			Track.OwnerRowIndex--;
		}
	}
}
