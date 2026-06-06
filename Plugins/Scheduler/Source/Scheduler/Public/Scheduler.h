// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

//[INFO]用于打印日志。
DECLARE_LOG_CATEGORY_EXTERN(LogScheduler, Log, All);

class FSchedulerModule : public IModuleInterface
{
public:

	
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
