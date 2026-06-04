// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SchedulerTaskPool.generated.h"

UCLASS()
class SCHEDULER_API ASchedulerTaskPool : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ASchedulerTaskPool();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
	
};
