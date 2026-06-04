#pragma once

#include "CoreMinimal.h"
#include "SchedulerRulerTypes.generated.h"

// 刻度层级：定义 1Minor / 1Major 分别等于多少 Tick，供缩放功能使用
USTRUCT(BlueprintType)
struct SCHEDULER_API FTickLevel
{
	GENERATED_BODY()

	// 1 Minor 刻度 = ? Tick
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	int64 Minor = 1;

	// 1 Major 刻度 = ? Tick
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	int64 Major = 10;
};

// 刻度尺绘制样式
USTRUCT(BlueprintType)
struct SCHEDULER_API FSchedulerRulerStyle
{
	GENERATED_BODY()

	// 刻度线颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	FLinearColor TickColor = FLinearColor(0.5f, 0.5f, 0.5f, 1.f);

	// 刻度线粗细（像素）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	float TickThickness = 1.f;

	// 背景色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	FLinearColor BackgroundColor = FLinearColor(0.06f, 0.06f, 0.06f, 1.f);

	// 刻度值文字颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	FLinearColor TextColor = FLinearColor(0.7f, 0.7f, 0.7f, 1.f);

	// 刻度值字号
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scheduler|Ruler")
	int32 FontSize = 8;
};