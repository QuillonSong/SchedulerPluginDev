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
	NewTask->Subsystem = this;
	NewTask->OnTaskInitialized();  // 入池 + 绑定委托 + 创建 Track 控件
	return NewTask;
}

bool USchedulerSubsystem::DestroyTask(USchedulerTask* Task)
{
	if (!IsValid(Task)) return false;

	FTrack* OwnerTrack = TrackMap.Find(Task->TaskOwnerName);
	TArray<USchedulerTask*>* OwnerTasks = TaskMap.Find(Task->TaskOwnerName);
	if (!OwnerTasks) return false;

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
	Task->OnDestroy();  // 解绑委托 + 接口通知 + MarkAsGarbage

	if (OwnerTasks->Num() == 0)
	{
		TaskMap.Remove(Task->TaskOwnerName);
		if (OwnerTrack) { DestroyOwnerTrackWidgets(*OwnerTrack); }
		TrackMap.Remove(Task->TaskOwnerName);
	}

	return true;
}

void USchedulerSubsystem::CreateOwnerTrackInternal(const FString& OwnerName)
{
	FTrack NewTrack;
	if (TitleRowsBox.IsValid() && BodyRowsBox.IsValid())
	{
		CreateOwnerTrackWidgets(OwnerName, NewTrack);
	}
	TrackMap.Add(OwnerName, MoveTemp(NewTrack));
}

void USchedulerSubsystem::CreateTaskTrackInternal(USchedulerTask& Task)
{
	FTrack* OwnerTrack = TrackMap.Find(Task.TaskOwnerName);
	if (OwnerTrack && TitleRowsBox.IsValid() && BodyRowsBox.IsValid())
	{
		CreateTaskTrackWidgets(*OwnerTrack, &Task);
	}
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
	TSharedRef<SSchedulerTrackTitle> Title = SNew(SSchedulerTrackTitle)
		.DisplayName(OwnerName)
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
			// 切换箭头旋转
			if (Track->Title.IsValid()) Track->Title->SetExpanded(!Track->bIsCollapsed);
			return FReply::Handled();
		}))
		.OnDeleteClicked(FOnClicked::CreateLambda([this, OwnerName]()
		{
			// 删除该 Owner 下所有 Task → 连带删除 OwnerTrack
			if (TArray<USchedulerTask*>* Tasks = TaskMap.Find(OwnerName))
			{
				TArray<USchedulerTask*> ToDelete = *Tasks;  // 拷贝，避免迭代中修改
				for (USchedulerTask* T : ToDelete)
				{
					DestroyTask(T);
				}
			}
			return FReply::Handled();
		}));

	TSharedRef<SSchedulerTrackBody> Body = SNew(SSchedulerTrackBody)
		.RowHeight(TrackHeight)
		.RowColor(OwnerTrackColor)
		.BorderColor(TrackBorderColor)
		.BorderMargin(TrackBodyMargin);

	TitleRowsBox->AddSlot().AutoHeight()[Title];
	BodyRowsBox->AddSlot().AutoHeight()[Body];

	// 初始展开态——箭头旋转 90°
	Title->SetExpanded(true);

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
		.IndentWidth(20.f)
		.OnDeleteClicked(FOnClicked::CreateLambda([this, Task]()
		{
			DestroyTask(Task);
			return FReply::Handled();
		}));

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

	// 绑定 Keyframe 渲染所需指针
	Body->BindRulerState(&CachedViewStartTick, &CachedActiveLevelIndex, &CachedTickLevel, &CachedMinorPixel);
	Body->SetKeyframeParams(CachedKeyframeSize, CachedUnCheckedColor, CachedCheckedColor, CachedKeyframeTexture.Get());

	// 右键删除 Keyframe → Task::RemoveKeyframe
	Body->OnKeyframeDelete.BindLambda([this, Task](int64 Tick)
	{
		int32 Unused;
		Task->RemoveKeyframe(Tick, Unused);
		RefreshAllKeyframes();
	});

	// 若 Owner 当前已折叠，新 TaskTrack 也需隐藏
	if (OwnerTrack.bIsCollapsed)
	{
		Title->SetVisibility(EVisibility::Collapsed);
		Body->SetVisibility(EVisibility::Collapsed);
	}

	OwnerTrack.TaskTracks.Add(MoveTemp(Entry));
}

void USchedulerSubsystem::RefreshAllKeyframes()
{
	for (auto& Pair : TrackMap)
	{
		for (FTaskTrackEntry& Entry : Pair.Value.TaskTracks)
		{
			if (Entry.Body.IsValid())
			{
				Entry.Body->RefreshKeyframes();
			}
		}
	}
}

void USchedulerSubsystem::SyncKeyframeState(int64 InViewStartTick, int32 InActiveLevelIndex,
	const TArray<FTickLevel>& InTickLevel, float InMinorPixel,
	float InKeyframeSize, FLinearColor InUnCheckedColor, FLinearColor InCheckedColor,
	UTexture2D* InTexture)
{
	CachedViewStartTick = InViewStartTick;
	CachedActiveLevelIndex = InActiveLevelIndex;
	CachedTickLevel = InTickLevel;
	CachedMinorPixel = InMinorPixel;
	CachedKeyframeSize = InKeyframeSize;
	CachedUnCheckedColor = InUnCheckedColor;
	CachedCheckedColor = InCheckedColor;
	CachedKeyframeTexture = InTexture;
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
