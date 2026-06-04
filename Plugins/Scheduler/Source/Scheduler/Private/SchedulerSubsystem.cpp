// Fill out your copyright notice in the Description page of Project Settings.


#include "SchedulerSubsystem.h"

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
