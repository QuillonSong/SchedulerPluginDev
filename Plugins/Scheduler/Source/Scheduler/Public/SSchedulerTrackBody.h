#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SOverlay;
struct FTickLevel;

/**
 * BodyRight 区单行体——纯 Slate 控件组合
 * SBox → SOverlay → SBorder(描边) + SBorder(底色) → SOverlay(Phase3 Keyframe容器)
 */
class SCHEDULER_API SSchedulerTrackBody : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSchedulerTrackBody);
		SLATE_ARGUMENT(float, RowHeight)
		SLATE_ARGUMENT(FLinearColor, RowColor)
		SLATE_ARGUMENT(FLinearColor, BorderColor)
		SLATE_ARGUMENT(FMargin, BorderMargin)
		SLATE_ARGUMENT(const int64*, ViewStartTickPtr)
		SLATE_ARGUMENT(const int32*, ActiveLevelIndexPtr)
		SLATE_ARGUMENT(const TArray<FTickLevel>*, TickLevelArrayPtr)
		SLATE_ARGUMENT(const float*, MinorPixelPtr)
		SLATE_ARGUMENT(const TArray<int64>*, KeyframesPtr)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** Phase 3：Keyframe 容器，用于添加 Keyframe Widget */
	TSharedPtr<SOverlay> GetKeyframeOverlay() const { return KeyframeOverlay; }

private:
	TSharedPtr<SOverlay> KeyframeOverlay;
};
